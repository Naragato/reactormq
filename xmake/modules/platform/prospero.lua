local playstation = import("family.playstation", { anonymous = true })

function configure(cfg, target)
    playstation.configure(cfg, target)

    cfg.platform_prospero = true
    if target then
        local project_dir = os.projectdir()
        local shim_dir = path.join(project_dir, "shim", "prospero")
        if os.isdir(shim_dir) then
            target:add("includedirs", shim_dir, { private = true })
            target:add("cxflags", "-isystem " .. shim_dir, { force = true })
            target:add("cflags", "-isystem " .. shim_dir, { force = true })
        end
    end
end

function configure_ssl_targets(cfg)
    local ssl_defines = {
        "OPENSSL_NO_SOCK",
        "NO_SYS_UN_H",
        "OPENSSL_NO_DLOPEN",
        "NO_SYSLOG"
    }

    cfg.prospero_ssl_defines = ssl_defines
end
