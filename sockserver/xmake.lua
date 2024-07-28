add_rules("mode.debug", "mode.release")

set_languages("c++20")

target("dst_debugger_sockserver")
    add_rules("win.sdk.application")
    add_links("WS2_32") -- WinSock2

    set_kind("binary")
    set_pcxxheader("src/pch.hpp")
    add_files("*.rc", "src/*.cpp")
