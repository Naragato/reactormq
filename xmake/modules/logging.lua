local function _print(prefix, msg)
    print(string.format("[%s] %s", prefix, msg))
end

function log(msg)
    _print("reactormq:xmake", msg)
end

function warn(msg)
    _print("reactormq:xmake", "Warning: " .. msg)
end

function toolchain_log(toolchain, msg)
    _print("xmake:" .. toolchain, msg)
end
