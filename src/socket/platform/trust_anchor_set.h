//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include <vector>

#include "util/logging/logging.h"

#if REACTORMQ_WITH_UE5
#if WITH_SSL
UE_PUSH_MACRO("UI")
#define UI UI_ST
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#undef UI
UE_POP_MACRO("UI")
#endif // WITH_SSL
#else // REACTORMQ_WITH_UE5
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#endif // REACTORMQ_WITH_UE5

namespace reactormq::socket
{
    /**
     * @brief In-memory set of system trust anchors (root certificates).
     * Platform-specific constructors populate the set from the OS certificate
     * stores (Windows ROOT store, macOS keychain anchors, POSIX/Android
     * CA bundles). Common lifetime management, deduplication and attachment
     * to an SSL_CTX live inline in this header for cross-platform reuse.
     */
    class TrustAnchorSet
    {
    public:
        TrustAnchorSet();

        ~TrustAnchorSet()
        {
            for (X509* cert : m_anchors)
            {
                X509_free(cert);
            }

            m_anchors.clear();
        }

        TrustAnchorSet(const TrustAnchorSet&) = delete;

        TrustAnchorSet& operator=(const TrustAnchorSet&) = delete;

        TrustAnchorSet(TrustAnchorSet&&) = delete;

        TrustAnchorSet& operator=(TrustAnchorSet&&) = delete;

        void addAnchor(X509* cert)
        {
            if (cert == nullptr)
            {
                return;
            }

            const ASN1_TIME* notBeforeTime = X509_get_notBefore(cert);

            if (const ASN1_TIME* notAfterTime = X509_get_notAfter(cert); notAfterTime != nullptr && X509_cmp_current_time(notAfterTime) < 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "TrustAnchorSet: rejecting expired root certificate");
                X509_free(cert);
                return;
            }

            if (notBeforeTime != nullptr && X509_cmp_current_time(notBeforeTime) > 0)
            {
                REACTORMQ_LOG(logging::LogLevel::Warn, "TrustAnchorSet: rejecting not-yet-valid root certificate");
                X509_free(cert);
                return;
            }

            for (const X509* other : m_anchors)
            {
                if (X509_cmp(other, cert) == 0)
                {
                    REACTORMQ_LOG(logging::LogLevel::Debug, "TrustAnchorSet: ignoring duplicate root certificate");
                    X509_free(cert);
                    return;
                }
            }

            REACTORMQ_LOG(logging::LogLevel::Debug, "TrustAnchorSet: adding new trust anchor");

            m_anchors.push_back(cert);
        }

        [[nodiscard]] bool attachTo(const SSL_CTX* sslContext) const
        {
            if (sslContext == nullptr)
            {
                return false;
            }

            X509_STORE* store = SSL_CTX_get_cert_store(sslContext);
            if (store == nullptr)
            {
                return false;
            }

            for (X509* cert : m_anchors)
            {
                if (cert == nullptr)
                {
                    continue;
                }

                if (X509_STORE_add_cert(store, cert) == 0)
                {
                    if (const unsigned long err = ERR_peek_last_error(); ERR_GET_REASON(err) != X509_R_CERT_ALREADY_IN_HASH_TABLE)
                    {
                        REACTORMQ_LOG(logging::LogLevel::Debug, "TrustAnchorSet: X509_STORE_add_cert failed (0x%lx)", err);
                    }

                    ERR_clear_error();
                }
            }

            return true;
        }

        [[nodiscard]] bool empty() const
        {
            return m_anchors.empty();
        }

    private:
        std::vector<X509*> m_anchors;
    };
} // namespace reactormq::socket