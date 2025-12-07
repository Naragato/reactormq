function configure(cfg, target)

    local provider = cfg.ssl_provider
    cfg.ssl_available = false
    if provider == "libressl" then
        if target then
            target:add("packages", "libressl")
        end
        cfg.ssl_available = true
    elseif provider == "aws-lc" then
        if target then
            target:add("packages", "aws-lc")
        end
        cfg.ssl_available = true
    elseif provider == "system" then
        if target then
            target:add("packages", "openssl")
        end
        cfg.ssl_available = true
    elseif provider == "none" then
        cfg.ssl_available = false
    end

    if cfg.ssl_available then
        if cfg.with_socket_polyfill then
            cfg.secure_socket_with_bio_tls = true
            cfg.secure_socket_with_tls = false
        else
            cfg.secure_socket_with_tls = true
            cfg.secure_socket_with_bio_tls = false
        end
    else
        cfg.secure_socket_with_tls = false
        cfg.secure_socket_with_bio_tls = false
    end
    local platform = import("platform." .. cfg.platform, { alias = "platform" })

    if platform.configure_ssl_targets then
        platform.configure_ssl_targets(cfg)
    end

    if cfg.ssl_available and target then
        if is_plat("windows") and not cfg.openssl_use_shared_libs then
            target:add("syslinks", "crypt32", "bcrypt")
        end
        if is_plat("linux", "android") and not cfg.openssl_use_shared_libs then
            target:add("syslinks", "dl")
        end
    end
end
