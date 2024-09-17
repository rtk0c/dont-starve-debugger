add_rules("mode.debug", "mode.release")

set_languages("c17")

target("sockbridge")
    --add_links("WS2_32") -- WinSock2

    set_kind("binary")
    add_files("src/*.c")
