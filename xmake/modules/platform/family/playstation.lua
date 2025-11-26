local posix = import("posix", { anonymous = true })
local console = import("console", { anonymous = true })

function configure(cfg, target)
    posix.configure(cfg, target)
    console.configure(cfg, target)
    cfg.socket_with_posix = false
    cfg.socket_with_sony = true

    cfg.socket_with_gethostname = false
    cfg.socket_with_getaddrinfo = false
    cfg.socket_with_getnameinfo = false
    cfg.socket_with_select = false
    cfg.socket_with_ioctl = false

    cfg.socket_with_msg_dontwait = true

    cfg.family_sony = true
end
