function apply_warnings()
    return function(target)
        import("core.tool.toolchain")
        import("core.base.semver")
        local msvc = toolchain.load("msvc", { plat = target:plat(), arch = target:arch() })
        if msvc then
            target:add("cxflags", "/W4", "/permissive-", "/Zc:__cplusplus", "/Zc:preprocessor")
        else
            target:add("cxflags", "-Wall", "-Wextra", "-Wpedantic")
        end
    end
end
function apply_features(cfg)
    return function(target)
        import("core.tool.toolchain")
        local msvc = toolchain.load("msvc", { plat = target:plat(), arch = target:arch() })
        if msvc then
            if not cfg.with_rtti then
                target:add("cxflags", "/GR-")
            end
            if not cfg.with_exceptions then
                target:add("cxflags", "-fno-exceptions")
            end
        else
            if not cfg.with_exceptions then
                target:add("cxflags", "-fno-exceptions")
            end
            if not cfg.with_rtti then
                target:add("cxflags", "-fno-rtti")
            end
            target:add("cxflags", "-fvisibility=hidden", "-fvisibility-inlines-hidden")
        end
    end
end
function apply_sanitizers(cfg)
    return function(target)
        if is_plat("windows") then
            if cfg.enable_msan then
                print("Warning: MemorySanitizer is not supported on Windows; ignoring.")
                cfg.enable_msan = false
            end
            if cfg.enable_tsan then
                print("Warning: ThreadSanitizer is not supported on Windows; ignoring.")
                cfg.enable_tsan = false
            end
        end
        local has_sanitizer = false
        import("core.tool.toolchain")
        local msvc = toolchain.load("msvc", { plat = target:plat(), arch = target:arch() })
        if cfg.enable_asan then
            if msvc then
                target:add("cxflags", "/fsanitize=address")
                target:add("ldflags", "/fsanitize=address")
            else
                target:add("cxflags", "-fsanitize=address")
                target:add("ldflags", "-fsanitize=address")
            end
            has_sanitizer = true
        end
        if cfg.enable_msan and not is_plat("windows") then
            target:add("cxflags", "-fsanitize=memory", "-fsanitize-memory-track-origins=2", "-fno-omit-frame-pointer")
            target:add("ldflags", "-fsanitize=memory", "-fsanitize-memory-track-origins=2")
            has_sanitizer = true
        end
        if cfg.enable_tsan and not is_plat("windows") then
            target:add("cxflags", "-fsanitize=thread")
            target:add("ldflags", "-fsanitize=thread")
            has_sanitizer = true
        end
        if has_sanitizer then
            target:add("defines", "REACTORMQ_WITH_SANITIZERS=1")
        end
    end
end
function apply_defines(cfg)
    return function(target)
        local function add_bool_define(field, macro)
            local value = cfg[field] and 1 or 0
            target:add("defines", string.format("%s=%d", macro, value))
        end
        add_bool_define("socket_with_posix", "REACTORMQ_SOCKET_WITH_POSIX_SOCKET")
        add_bool_define("socket_with_winsocket", "REACTORMQ_SOCKET_WITH_WINSOCKET")
        add_bool_define("socket_with_sony", "REACTORMQ_SOCKET_WITH_SONY_SOCKET")
        add_bool_define("socket_with_ioctl", "REACTORMQ_SOCKET_WITH_IOCTL")
        add_bool_define("socket_with_poll", "REACTORMQ_SOCKET_WITH_POLL")
        add_bool_define("socket_with_select", "REACTORMQ_SOCKET_WITH_SELECT")
        add_bool_define("socket_with_gethostname", "REACTORMQ_SOCKET_WITH_GETHOSTNAME")
        add_bool_define("socket_with_getaddrinfo", "REACTORMQ_SOCKET_WITH_GETADDRINFO")
        add_bool_define("socket_with_getnameinfo", "REACTORMQ_SOCKET_WITH_GETNAMEINFO")
        add_bool_define("socket_with_close_on_exec", "REACTORMQ_SOCKET_WITH_CLOSE_ON_EXEC")
        add_bool_define("socket_with_msg_dontwait", "REACTORMQ_SOCKET_WITH_MSG_DONTWAIT")
        add_bool_define("socket_with_recvmmsg", "REACTORMQ_SOCKET_WITH_RECVMMSG")
        add_bool_define("socket_with_timestamp", "REACTORMQ_SOCKET_WITH_TIMESTAMP")
        add_bool_define("socket_with_nodelay", "REACTORMQ_SOCKET_WITH_NODELAY")
        add_bool_define("platform_has_bsd_time", "REACTORMQ_PLATFORM_HAS_BSD_TIME")
        add_bool_define("platform_has_bsd_ipv6", "REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS")
        target:add("defines", string.format("_HAS_EXCEPTIONS=%d", cfg.with_exceptions and 1 or 0))
        add_bool_define("with_console_sink", "REACTORMQ_WITH_CONSOLE_SINK")
        add_bool_define("with_file_sink", "REACTORMQ_WITH_FILE_SINK")
        add_bool_define("with_socket_polyfill", "REACTORMQ_WITH_SOCKET_POLYFILL")
        add_bool_define("secure_socket_with_tls", "REACTORMQ_SECURE_SOCKET_WITH_TLS")
        add_bool_define("secure_socket_with_bio_tls", "REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS")
        add_bool_define("ssl_available", "REACTORMQ_WITH_TLS")
        add_bool_define("platform_windows", "REACTORMQ_PLATFORM_WINDOWS")
        add_bool_define("platform_linux", "REACTORMQ_PLATFORM_LINUX")
        add_bool_define("platform_android", "REACTORMQ_PLATFORM_ANDROID")
        add_bool_define("platform_macos", "REACTORMQ_PLATFORM_MACOS")
        add_bool_define("platform_ios", "REACTORMQ_PLATFORM_IOS")
        add_bool_define("platform_prospero", "REACTORMQ_PLATFORM_PROSPERO")
        add_bool_define("platform_nnx", "REACTORMQ_PLATFORM_NNX")
        add_bool_define("platform_gdk", "REACTORMQ_PLATFORM_GDK")
        add_bool_define("family_windows", "REACTORMQ_PLATFORM_WINDOWS_FAMILY")
        add_bool_define("family_darwin", "REACTORMQ_PLATFORM_DARWIN_FAMILY")
        add_bool_define("family_posix", "REACTORMQ_PLATFORM_POSIX_FAMILY")
        add_bool_define("family_console", "REACTORMQ_PLATFORM_CONSOLE_FAMILY")
        add_bool_define("family_nintendo", "REACTORMQ_PLATFORM_NINTENDO_FAMILY")
        add_bool_define("family_sony", "REACTORMQ_PLATFORM_SONY_FAMILY")
    end
end
