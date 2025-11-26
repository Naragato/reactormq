local posix = import("family.posix", { anonymous = true })

function configure(cfg, target)
    posix.configure(cfg, target)
    cfg.platform_has_bsd_ipv6 = true
    cfg.socket_with_poll = true
    cfg.socket_with_msg_dontwait = true
    cfg.socket_with_recvmmsg = true
    cfg.socket_with_timestamp = true

    cfg.platform_linux = true
end
