local posix = import("posix", { anonymous = true })

function configure(cfg, target)
    posix.configure(cfg, target)

    cfg.platform_has_bsd_time = true
    cfg.platform_has_bsd_ipv6 = true
    cfg.socket_with_msg_dontwait = true

    cfg.family_darwin = true

    if target then
        target:add("frameworks", "Security", "CoreFoundation")
    end
end
