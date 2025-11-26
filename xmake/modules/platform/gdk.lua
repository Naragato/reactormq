local xbox = import("family.xbox", { anonymous = true })

function configure(cfg, target)
    xbox.configure(cfg, target)
    cfg.platform_gdk = true
end
