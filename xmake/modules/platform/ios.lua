local darwin = import("family.darwin", { anonymous = true })

function configure(cfg, target)
    darwin.configure(cfg, target)
    cfg.platform_ios = true
end
