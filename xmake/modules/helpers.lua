function configure(target)
    import("core.cache.memcache")
    local defaults = import("defaults")
    defaults()
    local cfg = memcache.get("defaults", "config")
    if not cfg then
        raise("reactormq: defaults config not found in memcache")
    end

    local platform_module_name = string.lower(cfg.platform)
    local platform = import("platform." .. platform_module_name, { anonymous = true })
    platform.configure(cfg, target)

    local resolve_tristate = import("option_resolver", { anonymous = true })

    for k, v in pairs(cfg) do
        target:add("options", k)
        cfg[k] = resolve_tristate(get_config(k), function()
            return cfg[k]
        end)
    end

    local flags = import("flags", { anonymous = true })
    flags.apply_warnings()(target)
    flags.apply_features(cfg)(target)
    flags.apply_sanitizers(cfg)(target)

    local ssl = import("ssl", { anonymous = true })
    ssl.configure(cfg, target)

    if flags.apply_defines then
        flags.apply_defines(cfg)(target)
    end
end