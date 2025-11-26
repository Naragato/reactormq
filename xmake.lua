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
        import("core.cache.memcache")
        local defaults = import("xmake.modules.defaults")
        defaults()
        local cfg = memcache.get("defaults", "config")
        if not cfg then
            raise("reactormq: defaults config not found in memcache")
        end

        local platform_module_name = string.lower(cfg.platform)
        local platform = import("xmake.modules.platform." .. platform_module_name, { anonymous = true })
        platform.configure(cfg, target)

        local resolve_tristate = import("xmake.modules.option_resolver", { anonymous = true })

        for k, v in pairs(cfg) do
            target:add("options", k)
            cfg[k] = resolve_tristate(get_config(k), function()
                return cfg[k]
            end)
        end

        local flags = import("xmake.modules.flags", { anonymous = true })
        flags.apply_warnings()(target)
        flags.apply_features(cfg)(target)
        flags.apply_sanitizers(cfg)(target)

        local ssl = import("xmake.modules.ssl", { anonymous = true })
        ssl.configure(cfg, target)

        if flags.apply_defines then
            flags.apply_defines(cfg)(target)
        end

        local sources = import("xmake.modules.sources", { anonymous = true })
        local extra_socket_sources = sources.collect_socket_sources(cfg, "$(projectdir)/src")

        for k, v in pairs(extra_socket_sources) do
            target:add("files", v)
        end

        if #extra_socket_sources > 0 then
            target:add("files", table.unpack(extra_socket_sources))
        end

        if get_config("build_shared") then
            target:set("kind", "shared")
            target:add("defines", "REACTORMQ_BUILD")
        else
            target:set("kind", "static")
            target:add("defines", "REACTORMQ_STATIC")
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
