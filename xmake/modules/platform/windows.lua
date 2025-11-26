local windows = import("family.windows", { anonymous = true })

function configure(cfg, target)
    windows.configure(cfg, target)
    cfg.platform_windows = true
end
