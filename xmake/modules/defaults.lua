---------------------------------------------------------------------
-- Unified configuration object builder
---------------------------------------------------------------------
local platform_detect = import("platform_detect", { anonymous = true })
local memcache = import("core.cache.memcache")
local config = import("core.project.config")

function main()
    print("[reactormq:xmake] defaults.lua: building base cfg")

    ---------------------------------------------------------------------
    -- Detect platform/family
    ---------------------------------------------------------------------
    local detection = platform_detect.detect()
    local cfg = {}
    cfg.family = detection.family
    cfg.platform = detection.platform
    cfg.is_console = detection.is_console

    cfg.socket_with_posix = true
    cfg.socket_with_winsocket = false
    cfg.socket_with_sony = false
    cfg.socket_with_ioctl = true
    cfg.socket_with_poll = false
    cfg.socket_with_select = true
    cfg.socket_with_gethostname = true
    cfg.socket_with_getaddrinfo = true
    cfg.socket_with_getnameinfo = true
    cfg.socket_with_close_on_exec = false
    cfg.socket_with_msg_dontwait = false
    cfg.socket_with_recvmmsg = false
    cfg.socket_with_timestamp = false
    cfg.socket_with_nodelay = true
    cfg.platform_has_bsd_time = false
    cfg.platform_has_bsd_ipv6 = false

    cfg.build_shared = false
    cfg.build_tests = true
    cfg.with_threads = true
    cfg.with_exceptions = false
    cfg.with_rtti = false
    cfg.with_console_sink = true
    cfg.with_file_sink = true
    cfg.enable_ue5 = false
    cfg.enable_o3de = false
    cfg.with_socket_polyfill = false

    cfg.secure_socket_with_tls = false
    cfg.secure_socket_with_bio_tls = false

    ---------------------------------------------------------------------
    -- SSL configuration
    ---------------------------------------------------------------------
    cfg.ssl_available = false
    cfg.with_ue5 = false
    cfg.with_o3de = false

    ---------------------------------------------------------------------
    -- Platform identity flags
    ---------------------------------------------------------------------
    cfg.platform_windows = false
    cfg.platform_linux = false
    cfg.platform_android = false
    cfg.platform_macos = false
    cfg.platform_ios = false
    cfg.platform_prospero = false
    cfg.platform_nnx = false
    cfg.platform_gdk = false

    ---------------------------------------------------------------------
    -- Family identity flags
    ---------------------------------------------------------------------
    cfg.family_windows = false
    cfg.family_darwin = false
    cfg.family_posix = false
    cfg.family_console = false
    cfg.family_nintendo = false
    cfg.family_sony = false

    cfg.ssl_provider = "libressl"  -- system|libressl|awslc|none
    cfg.openssl_use_shared_libs = false

    cfg.enable_asan = false
    cfg.enable_msan = false
    cfg.enable_tsan = false

    memcache.set("defaults", "config", cfg)
end