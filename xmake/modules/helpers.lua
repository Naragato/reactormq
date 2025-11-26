local logging = import("logging", { anonymous = true })

function is_windows_family(cfg)
    return cfg.family == "WindowsFamily"
end

function is_posix_family(cfg)
    return cfg.family == "PosixFamily" or cfg.family == "DarwinFamily"
end

function is_playstation_family(cfg)
    return cfg.family == "PlaystationFamily"
end

function is_nintendo_family(cfg)
    return cfg.family == "NintendoFamily"
end

function is_console(cfg)
    return cfg.is_console
end

function load_msvc(target)
    import("core.tool.toolchain")
    return toolchain.load("msvc", { plat = target:plat(), arch = target:arch() })
end
