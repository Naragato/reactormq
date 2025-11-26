    -- Prospero (PlayStation 5) Toolchain
    toolchain("prospero")
        set_kind("standalone")
        set_homepage("https://www.playstation.com/")
        set_description("Sony PlayStation 5 (Prospero) Toolchain")
        -- Retrieve SDK directory from environment
        local sdk_dir = os.getenv("SCE_PROSPERO_SDK_DIR")
        local host_tools_bin = path.join(sdk_dir, "host_tools", "bin")
        local project_dir = os.projectdir()
        local shim_dir = path.join(project_dir, "shim", "prospero")
        -- compilers / assembler / link front-end
        set_toolset("cc",  path.join(host_tools_bin, "prospero-clang"))
        set_toolset("cxx", path.join(host_tools_bin, "prospero-clang"))
        set_toolset("as",  path.join(host_tools_bin, "prospero-clang"))

        -- linkers
        set_toolset("ld",  path.join(host_tools_bin, "prospero-clang"))
        set_toolset("sh",  path.join(host_tools_bin, "prospero-clang"))

        -- archiving / symbols / binary utils
        set_toolset("ar",      path.join(host_tools_bin, "prospero-llvm-ar"))
        set_toolset("strip",   path.join(host_tools_bin, "prospero-llvm-strip"))
        set_toolset("nm",      path.join(host_tools_bin, "prospero-llvm-nm"))
        set_toolset("objcopy", path.join(host_tools_bin, "prospero-llvm-objcopy"))
        set_toolset("objdump", path.join(host_tools_bin, "prospero-llvm-objdump"))
        set_toolset("readelf", path.join(host_tools_bin, "prospero-llvm-readelf"))
        set_toolset("size",    path.join(host_tools_bin, "prospero-llvm-size"))
        set_toolset("strings", path.join(host_tools_bin, "prospero-llvm-strings"))
        set_toolset("cxxfilt", path.join(host_tools_bin, "prospero-llvm-cxxfilt"))
        set_toolset("dwarfdump",  path.join(host_tools_bin, "prospero-llvm-dwarfdump"))
        set_toolset("symbolizer", path.join(host_tools_bin, "prospero-llvm-symbolizer"))

        set_toolset("format",  path.join(host_tools_bin, "prospero-clang-format"))
        set_toolset("tidy",    path.join(host_tools_bin, "prospero-clang-tidy"))

        set_toolset("cov",      path.join(host_tools_bin, "prospero-llvm-cov"))
        set_toolset("profdata", path.join(host_tools_bin, "prospero-llvm-profdata"))
        set_toolset("profgen",  path.join(host_tools_bin, "prospero-llvm-profgen"))

        add_sysincludedirs(shim_dir)
        on_check(function (toolchain)
            if not sdk_dir or not os.isdir(sdk_dir) then
                print("[xmake:prospero] SCE_PROSPERO_SDK_DIR is not set or points to a missing SDK directory: " .. tostring(sdk_dir))
                return false
            end

            local host_tools_bin = path.join(sdk_dir, "host_tools", "bin")
            if not os.isdir(host_tools_bin) then
                print("[xmake:prospero] Prospero SDK is missing required host tools directory: " .. host_tools_bin)
                return false
            end

            return true
        end)

        on_load(function(toolchain)
            local march = "-m64"
            toolchain:add("cxflags", march)
            toolchain:add("mxflags", march)
            toolchain:add("asflags", march)
            toolchain:add("ldflags", march)
            toolchain:add("shflags", march)

            toolchain:add("cxflags", "-target", "x86_64-scei-ps5")
            toolchain:add("cflags", "-target", "x86_64-scei-ps5")
            toolchain:add("ldflags", "-target", "x86_64-scei-ps5")
            toolchain:add("cxflags", "-isystem", shim_dir)
            toolchain:add("cflags", "-isystem", shim_dir)
            toolchain:add("syslinks", "ScePosix_stub_weak")
        end)
    toolchain_end()