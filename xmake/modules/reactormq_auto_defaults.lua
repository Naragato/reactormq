local function is_windows_family(cfg)
    return cfg.family == "WindowsFamily"
end
local function is_posix_family(cfg)
    return cfg.family == "PosixFamily" or cfg.family == "DarwinFamily"
end
local function is_playstation_family(cfg)
    return cfg.family == "PlaystationFamily"
end
local function is_nintendo_family(cfg)
    return cfg.family == "NintendoFamily"
end

function build_tests(cfg)
    if cfg.is_console then
        return false
    end
    return true
end

function socket_with_posix(cfg)
    if is_windows_family(cfg) then
        return false
    end
    if is_playstation_family(cfg) then
        return false
    end
    if is_nintendo_family(cfg) then
        return false
    end
    return true
end

function socket_with_winsocket(cfg)
    return is_windows_family(cfg)
end

function socket_with_sony(cfg)
    return is_playstation_family(cfg)
end

function with_threads(_cfg)
    return true
end

function with_rtti(cfg)
    if cfg.is_console then
        return false
    end
    return false
end

function with_exceptions(cfg)
    if cfg.is_console then
        return false
    end
    return false
end

function with_console_sink(cfg)
    if cfg.is_console then
        return false
    end
    return true
end

function with_file_sink(_cfg)
    return true
end

function with_socket_polyfill(cfg)
    if is_playstation_family(cfg) or is_nintendo_family(cfg) then
        return true
    end
    return false
end
