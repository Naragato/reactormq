//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#if REACTORMQ_PLATFORM_POSIX_FAMILY && !REACTORMQ_PLATFORM_WINDOWS_FAMILY
#include "trust_anchor_set.h"

#include <array>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <sys/stat.h>

namespace reactormq::socket
{
    namespace
    {
        void loadFromPemBundleFile(TrustAnchorSet& set, const char* path)
        {
            if (path == nullptr || *path == '\0')
            {
                return;
            }

            FILE* file = std::fopen(path, "r");
            if (file == nullptr)
            {
                return;
            }

            for (X509* cert = nullptr; (cert = PEM_read_X509(file, nullptr, nullptr, nullptr)) != nullptr;)
            {
                set.addAnchor(cert);
            }

            std::fclose(file);
        }

        void loadFromCertificateDirectory(TrustAnchorSet& set, const char* dirPath)
        {
            if (dirPath == nullptr || *dirPath == '\0')
            {
                return;
            }

            DIR* dir = opendir(dirPath);
            if (dir == nullptr)
            {
                return;
            }

            const dirent* entry = nullptr;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (entry->d_name[0] == '.')
                {
                    continue;
                }

                const std::string filePath = std::string(dirPath) + "/" + entry->d_name;
                FILE* file = std::fopen(filePath.c_str(), "r");
                if (file == nullptr)
                {
                    continue;
                }

                if (X509* cert = PEM_read_X509(file, nullptr, nullptr, nullptr); nullptr != cert)
                {
                    set.addAnchor(cert);
                }
                else
                {
                    ERR_clear_error();
                }

                std::fclose(file);
            }

            closedir(dir);
        }
    } // namespace

    TrustAnchorSet::TrustAnchorSet()
    {
        const char* bundlePath = nullptr;

#if defined(HAS_STD_GETENV) && HAS_STD_GETENV
        const char* envVarName = X509_get_default_cert_file_env();
        if (envVarName != nullptr)
        {
            bundlePath = std::getenv(envVarName);
        }
#endif // HAS_STD_GETENV

        if (bundlePath == nullptr || *bundlePath == '\0')
        {
            bundlePath = X509_get_default_cert_file();
        }

        if (bundlePath != nullptr && *bundlePath != '\0')
        {
            loadFromPemBundleFile(*this, bundlePath);
        }

        const std::array<std::string, 3> extraBundlePaths = {
            "/etc/pki/tls/certs/ca-bundle.crt",
            "/etc/ssl/certs/ca-certificates.crt",
            "/etc/ssl/ca-bundle.pem",
        };
        for (const auto& path : extraBundlePaths)
        {
            loadFromPemBundleFile(*this, path.c_str());
        }

        const auto androidDir = "/system/etc/security/cacerts";
        struct stat androidDirStat{};
        if (stat(androidDir, &androidDirStat) == 0 && S_ISDIR(androidDirStat.st_mode))
        {
            loadFromCertificateDirectory(*this, androidDir);
        }
    }
} // namespace reactormq::socket

#endif // REACTORMQ_PLATFORM_POSIX_FAMILY && !REACTORMQ_PLATFORM_WINDOWS_FAMILY