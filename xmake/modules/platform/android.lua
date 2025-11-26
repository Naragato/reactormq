local posix = import("family.posix", { anonymous = true })

function configure(cfg, target)
    posix.configure(cfg, target)
    cfg.socket_with_ioctl = true
    cfg.socket_with_msg_dontwait = true
    cfg.platform_android = true
end
