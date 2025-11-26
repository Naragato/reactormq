function configure(cfg, target)
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
    cfg.family_posix = true
    if target and cfg.with_threads then
        target:add("syslinks", "pthread")
    end
end
