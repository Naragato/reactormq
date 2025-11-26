function detect()
    local family = ""
    local platform = ""
    local is_console = false

    if is_plat("windows") then
        local toolchain = get_config("toolchain")
        if toolchain == "gdk" then
            family = "WindowsFamily"
            platform = "GDK"
            is_console = true
        else
            family = "WindowsFamily"
            platform = "Windows"
            is_console = false
        end

    elseif is_plat("linux") then
        family = "PosixFamily"
        platform = "Linux"
        is_console = false

    elseif is_plat("macosx") then
        family = "DarwinFamily"
        platform = "MacOS"
        is_console = false

    elseif is_plat("iphoneos") then
        family = "DarwinFamily"
        platform = "iOS"
        is_console = false

    elseif is_plat("android") then
        family = "PosixFamily"
        platform = "Android"
        is_console = false

    else
        local toolchain = get_config("toolchain")

        if toolchain == "prospero" then
            family = "PlaystationFamily"
            platform = "Prospero"
            is_console = true

        elseif toolchain == "switch" then
            family = "NintendoFamily"
            platform = "Switch"
            is_console = true

        else
            print("[reactormq:xmake] Warning: unrecognized platform; using PosixFamily/Linux defaults.")
            family = "PosixFamily"
            platform = "Linux"
            is_console = false
        end
    end

    return {
        family = family,
        platform = platform,
        is_console = is_console
    }
end
