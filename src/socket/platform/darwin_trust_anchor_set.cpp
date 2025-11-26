//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_PLATFORM_DARWIN_FAMILY && (REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)
#include "socket/platform/trust_anchor_set.h"

#include <Security/Security.h>
#include <TargetConditionals.h>

namespace reactormq::socket
{
    TrustAnchorSet::TrustAnchorSet()
    {
#if REACTORMQ_PLATFORM_IOS
        // TODO:
        // iOS uses a restricted trust store; for now we do not enumerate anchors.
        // TLS verification will continue to rely on the platform defaults.
        return;
#else
        CFArrayRef anchors = nullptr;
        if (const OSStatus status = SecTrustCopyAnchorCertificates(&anchors); status != errSecSuccess || anchors == nullptr)
        {
            return;
        }

        struct AnchorsReleaser
        {
            CFArrayRef ref{ nullptr };

            explicit AnchorsReleaser(CFArrayRef r)
                : ref(r)
            {
            }

            ~AnchorsReleaser()
            {
                if (ref != nullptr)
                {
                    CFRelease(ref);
                }
            }

            AnchorsReleaser(const AnchorsReleaser&) = delete;

            AnchorsReleaser& operator=(const AnchorsReleaser&) = delete;

            AnchorsReleaser(AnchorsReleaser&& other) noexcept
                : ref(other.ref)
            {
                other.ref = nullptr;
            }

            AnchorsReleaser& operator=(AnchorsReleaser&& other) noexcept
            {
                if (this != &other)
                {
                    if (ref != nullptr)
                    {
                        CFRelease(ref);
                    }
                    ref = other.ref;
                    other.ref = nullptr;
                }
                return *this;
            }
        };
        AnchorsReleaser nchorsReleaser{ anchors };
        const CFIndex count = CFArrayGetCount(anchors);
        for (CFIndex index = 0; index < count; ++index)
        {
            const void* value = CFArrayGetValueAtIndex(anchors, index);
            if (value == nullptr)
            {
                continue;
            }

            const auto secCert = static_cast<SecCertificateRef>(const_cast<void*>(value));

            if (CFDataRef derData = SecCertificateCopyData(secCert); derData != nullptr)
            {
                const UInt8* bytes = CFDataGetBytePtr(derData);
                const CFIndex len = CFDataGetLength(derData);
                const unsigned char* buffer = bytes;

                if (X509* cert = d2i_X509(nullptr, &buffer, len); cert != nullptr)
                {
                    addAnchor(cert); // assumes ownership
                }
                else
                {
                    ERR_clear_error();
                }

                CFRelease(derData);
            }
        }
#endif
    }
} // namespace reactormq::socket

#endif // REACTORMQ_PLATFORM_DARWIN_FAMILY && (REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)