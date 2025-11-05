#if REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include "socket/platform/trust_anchor_set.h"

#include <wincrypt.h>
#include <windows.h>

namespace reactormq::socket
{
    TrustAnchorSet::TrustAnchorSet()
    {
        const HCERTSTORE storeHandle = CertOpenSystemStoreW(0, L"ROOT");
        if (storeHandle == nullptr)
        {
            const DWORD lastError = GetLastError();
            REACTORMQ_LOG(
                logging::LogLevel::Warn, "TrustAnchorSet: CertOpenSystemStoreW(ROOT) failed (%lu)", static_cast<unsigned long>(lastError));
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

#endif // REACTORMQ_PLATFORM_WINDOWS_FAMILY