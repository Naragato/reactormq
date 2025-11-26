local nintendo = import("family.nintendo", { anonymous = true })

function configure(cfg, target)
    nintendo.configure(cfg, target)

    cfg.platform_nnx = true
end
