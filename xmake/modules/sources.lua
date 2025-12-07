function collect_socket_sources(cfg, sources_dir)
    local files = {}

    if cfg.socket_with_winsocket then
        table.insert(files, path.join(sources_dir, "socket/platform/win_socket.cpp"))
    end

    if cfg.socket_with_posix then
        table.insert(files, path.join(sources_dir, "socket/platform/posix_socket.cpp"))
    end

    if cfg.socket_with_sony then
        table.insert(files, path.join(sources_dir, "socket/platform/sony_socket.cpp"))
    end

    if cfg.secure_socket_with_tls then
        table.insert(files, path.join(sources_dir, "socket/platform/native_secure_socket.cpp"))
    end

    if cfg.secure_socket_with_bio_tls then
        table.insert(files, path.join(sources_dir, "socket/platform/bio_secure_socket.cpp"))
    end

    if cfg.ssl_available then
        if cfg.family_darwin then
            table.insert(files, path.join(sources_dir, "socket/platform/darwin_trust_anchor_set.cpp"))
        elseif cfg.family_windows then
            table.insert(files, path.join(sources_dir, "socket/platform/win_trust_anchor_set.cpp"))
        else
            table.insert(files, path.join(sources_dir, "socket/platform/posix_trust_anchor_set.cpp"))
        end
    end

    return files
end
