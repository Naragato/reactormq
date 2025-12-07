set_project("reactormq")
set_version("1.0.0")
set_languages("cxx20")

includes("./xmake/toolchains/*.lua", "./xmake/modules/options.lua")

add_rules("plugin.vsxmake.autoupdate")

package("libressl")
    set_homepage("https://www.libressl.org/")
    set_description("LibreSSL TLS/crypto stack")

    add_urls("https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-$(version).tar.gz",
             "http://cloudflare.cdn.openbsd.org/pub/OpenBSD/LibreSSL/libressl-$(version).tar.gz",
                 "https://github.com/libressl/portable.git")
    add_versions("4.2.1", "6d5c2f58583588ea791f4c8645004071d00dfa554a5bf788a006ca1eb5abd70b")
    add_defines("OPENSSL_NO_UI_CONSOLE")
    add_deps("cmake")
    on_install(function (package)
        package:config("static")
        local configs = {
            "-DENABLE_EXTRAS=OFF",
            "-DENABLE_TESTS=OFF",
            "-DLIBRESSL_TESTS=OFF",
            "-DLIBRESSL_APPS=OFF",
            "-DOPENSSL_NO_UI_CONSOLE=ON",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release")
        }
        local shimflag = ""
        if package:config("toolchains") == "prospero" then
            local project_dir = os.projectdir()
            local shim_dir = path.join(project_dir, "shim", "prospero")
            table.insert(configs, "-DUNIX=0")
            table.insert(configs, "-DWIN32=0")
            -- Use a CMake-safe include flag without embedded quotes or backslashes
            shimflag = "-I" .. path.translate(shim_dir)
        end

        if package:is_plat("windows") and not package:config("openssl_use_shared_libs") then
            package:add("syslinks", "bcrypt", "crypt32", { private = true })
        end

        if not package:is_plat("windows") and not package:config("openssl_use_shared_libs") then
            package:add("syslinks", "dl", { private = true })
        end

        if get_config("with_threads") and not package:is_plat("windows") then
            package:add("syslinks", "pthread", { private = true })
        end
        io.replace("CMakeLists.txt",
                   "add_subdirectory(tls)\n",
                   "# add_subdirectory(tls)\n",
                   {plain = true})
        import("package.tools.cmake").install(package, configs, {
            cflags   = { shimflag, "-DOPENSSL_NO_UI_CONSOLE", "-DOPENSSL_NO_SOCK", "-DNO_SYS_UN_H", "-DOPENSSL_NO_DLOPEN", "-DNO_SYSLOG"},
            cxxflags = { shimflag,  "-DOPENSSL_NO_UI_CONSOLE", "-DOPENSSL_NO_SOCK", "-DNO_SYS_UN_H", "-DOPENSSL_NO_DLOPEN", "-DNO_SYSLOG"}
        })
    end)

    on_load(function (package)
        package:add("links", "ssl", "crypto")
        package:add("includedirs", "include")
    end)

package_end()

package("aws-lc")
    set_homepage("https://github.com/aws/aws-lc")
    set_description("AWS-LC (libcrypto/libssl), OpenSSL/BoringSSL-derived TLS/crypto library")

    add_urls("https://github.com/aws/aws-lc.git")
    add_versions("1.64.0", "v1.64.0")

    add_configs("build_shared", {description = "Build shared libs", default = false, type = "boolean"})
    add_deps("cmake")

    on_load(function (package)
        package:add("links", "ssl", "crypto")
        package:add("includedirs", "include")
    end)

    on_install(function (package)
        local shared = get_config("openssl_use_shared_libs") and "ON" or "OFF"

        local configs = {
            "-DBUILD_TESTING=OFF",
            "-DBUILD_TOOL=OFF",
            "-DBUILD_LIBSSL=ON",
            "-DBUILD_SHARED_LIBS=" .. shared,
            "-DDISABLE_PERL=ON",
            "-DDISABLE_GO=ON"
        }

        if package:is_plat("windows") then
            table.insert(configs, "-DCMAKE_C_FLAGS=/wd5262")
            table.insert(configs, "-DCMAKE_CXX_FLAGS=/wd5262")
        end

        import("package.tools.cmake").install(package, configs)
    end)
package_end()

add_requires("libressl", {configs = {shared = false}})
set_languages("cxx20")
target("reactormq")
    on_load(function (target)
        local helpers = import("xmake.modules.helpers", { anonymous = true })
        helpers.configure(target)
        local sources = import("xmake.modules.sources", { anonymous = true })
        import("core.cache.memcache")
        local cfg = memcache.get("defaults", "config")
        local extra_socket_sources = sources.collect_socket_sources(cfg, "$(projectdir)/src")

        for k, v in pairs(extra_socket_sources) do
            target:add("files", v)
        end

        if #extra_socket_sources > 0 then
            target:add("files", table.unpack(extra_socket_sources))
        end

        if get_config("build_shared") then
            target:set("kind", "shared")
        else
            target:set("kind", "static")
        end

        target:set("languages", "cxx20")
    end)
    set_exceptions("none")
    add_files("$(projectdir)/src/**.cpp|socket/platform/*.cpp")

    add_headerfiles("$(projectdir)/include/reactormq/**.h")
    add_headerfiles("$(projectdir)/include/reactormq/**.hpp")

    add_includedirs("$(projectdir)/include", {public = true})
    add_includedirs("$(projectdir)/src", {public = false})

    add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
target_end()

local build_tests = get_config("build_tests")
if build_tests ~= "off" then
    add_requires("llvm")
    add_requires("gtest")
    target("reactormq_tests")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_packages("gtest")
        add_files("$(projectdir)/tests/**.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
        on_load(function (target)
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
        end)
    target_end()

    target("unit_tests")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_packages("gtest")
        add_files("$(projectdir)/tests/unit/**.cpp", "$(projectdir)/tests/fixtures/**.cpp", "$(projectdir)/tests/test_main.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        add_tests("unit")
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
        on_load(function (target)
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
        end)
    target_end()

    target("integration_tests")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_packages("gtest")
        add_files("$(projectdir)/tests/integration/**.cpp", "$(projectdir)/tests/fixtures/**.cpp", "$(projectdir)/tests/test_main.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
        on_load(function (target)
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
        end)
    target_end()

    target("stress_tests")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_packages("gtest")
        add_files("$(projectdir)/tests/stress/test_concurrent_commands.cpp", "$(projectdir)/tests/fixtures/**.cpp", "$(projectdir)/tests/test_main.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
        on_load(function (target)
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
        end)
    target_end()

    target("fuzz_mqtt_codec")
        set_toolchains("@llvm")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_files("$(projectdir)/tests/fuzz/fuzz_mqtt_codec.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        on_load(function (target)
            if is_plat("windows") then
                return
            end
            if is_plat("macosx") then
                target:add("cxflags", "-stdlib=libc++", {force = true})
                target:add("ldflags", "-stdlib=libc++", {force = true})
                target:add("ldflags", "-L/opt/homebrew/opt/llvm/lib/c++", {force = true})
            end
            target:add("cxflags", "-fsanitize=fuzzer-no-link", {force = true})
            target:add("ldflags", "-fsanitize=fuzzer,address", {force = true})
            target:add("defines", "REACTORMQ_WITH_FUZZING=1")
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
        end)
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
    target_end()

    target("fuzz_fixed_header")
        set_toolchains("@llvm")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_files("$(projectdir)/tests/fuzz/fuzz_fixed_header.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        on_load(function (target)
            if is_plat("windows") then
                return
            end
            target:add("cxflags", "-fsanitize=fuzzer")
            target:add("ldflags", "-fsanitize=fuzzer")
            target:add("defines", "REACTORMQ_WITH_FUZZING=1")
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
        end)
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
    target_end()

    target("fuzz_variable_byte_integer")
        set_toolchains("@llvm")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_files("$(projectdir)/tests/fuzz/fuzz_variable_byte_integer.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        on_load(function (target)
            if is_plat("windows") then
                return
            end
            target:add("cxflags", "-fsanitize=fuzzer")
            target:add("ldflags", "-fsanitize=fuzzer")
            target:add("defines", "REACTORMQ_WITH_FUZZING=1")
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
        end)
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
    target_end()

    target("fuzz_publish_packet")
        set_toolchains("@llvm")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_files("$(projectdir)/tests/fuzz/fuzz_publish_packet.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        on_load(function (target)
            if is_plat("windows") then
                return
            end
            target:add("cxflags", "-fsanitize=fuzzer")
            target:add("ldflags", "-fsanitize=fuzzer")
            target:add("defines", "REACTORMQ_WITH_FUZZING=1")
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
        end)
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
    target_end()

    target("fuzz_connect_packet")
        set_toolchains("@llvm")
        set_kind("binary")
        set_group("tests")
        set_default(false)
        add_deps("reactormq")
        add_files("$(projectdir)/tests/fuzz/fuzz_connect_packet.cpp")
        add_includedirs("$(projectdir)/tests", "$(projectdir)/src", "$(projectdir)/include")
        on_load(function (target)
            if is_plat("windows") then
                return
            end
            target:add("cxflags", "-fsanitize=fuzzer")
            target:add("ldflags", "-fsanitize=fuzzer")
            target:add("defines", "REACTORMQ_WITH_FUZZING=1")
            target:add("defines", "REACTORMQ_WITH_TESTS=1")
            local helpers = import("xmake.modules.helpers", { anonymous = true })
            helpers.configure(target)
        end)
        add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")
    target_end()
end
