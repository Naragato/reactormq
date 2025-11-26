local windows = import("windows", { anonymous = true })
local console = import("console", { anonymous = true })

function configure(cfg, target)
    windows.configure(cfg, target)
    console.configure(cfg, target)
end
