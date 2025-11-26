//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_PLATFORM_WINDOWS_FAMILY && (REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)
#include "socket/platform/trust_anchor_set.h"

#if REACTORMQ_WITH_UE5
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#ifdef THIRD_PARTY_INCLUDES_START
THIRD_PARTY_INCLUDES_START
#endif // THIRD_PARTY_INCLUDES_START
#include <wincrypt.h>
#ifdef THIRD_PARTY_INCLUDES_END
THIRD_PARTY_INCLUDES_END
#endif // THIRD_PARTY_INCLUDES_END

namespace reactormq::socket
{
    TrustAnchorSet::TrustAnchorSet()
    {
        const HCERTSTORE storeHandle = CertOpenSystemStoreW(0, L"ROOT");
        if (storeHandle == nullptr)
        {
            const DWORD lastError = GetLastError();
            REACTORMQ_LOG(
                logging::LogLevel::Warn,
                "TrustAnchorSet: CertOpenSystemStoreW(ROOT) failed (%lu)",
                static_cast<unsigned long>(lastError));
            return;
        }

        PCCERT_CONTEXT context = nullptr;

        while ((context = CertEnumCertificatesInStore(storeHandle, context)) != nullptr)
        {
            const BYTE* encoded = context->pbCertEncoded;
            const DWORD encodedSize = context->cbCertEncoded;
            const unsigned char* buffer = encoded;

            X509* cert = d2i_X509(nullptr, &buffer, static_cast<long>(encodedSize));
            if (cert == nullptr)
            {
                ERR_clear_error();
                continue;
            }

            addAnchor(cert);
        }

        CertCloseStore(storeHandle, 0);
    }
} // namespace reactormq::socket

#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY && (REACTORMQ_SECURE_SOCKET_WITH_TLS || REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS)