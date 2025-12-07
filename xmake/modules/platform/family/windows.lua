function configure(cfg, target)
    cfg.socket_with_winsocket = true
    cfg.socket_with_posix = false

    cfg.family_windows = true

    if target then
        target:add("defines", "WIN32_LEAN_AND_MEAN", "NOMINMAX", "UNICODE", "_UNICODE")
        target:add("syslinks", "ws2_32")
    end
end
