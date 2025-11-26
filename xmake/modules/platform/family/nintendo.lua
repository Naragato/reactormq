local posix = import("posix", { anonymous = true })
local console = import("console", { anonymous = true })

function configure(cfg, target)
    posix.configure(cfg, target)
    console.configure(cfg, target)
    cfg.family_nintendo = true
end
