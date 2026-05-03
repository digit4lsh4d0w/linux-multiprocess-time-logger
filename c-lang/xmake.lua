set_project("time-logger")
set_version("0.1.0")
set_xmakever("3.0.0")

add_rules("mode.debug", "mode.release")

set_languages("c23")
set_warnings("all", "extra", "pedantic")

add_cflags(
    "-Wconversion",
    "-Wshadow",
    "-Wunreachable-code",
    "-Wvla"
)

add_defines(
    "_POSIX_C_SOURCE=200809L",
    "_DEFAULT_SOURCE"
)

set_toolchains("llvm")

target("time-logger")
set_kind("binary")
set_targetdir("$(builddir)/bin")

add_files("src/*.c")
add_includedirs("include", { public = true })

if is_plat("linux") then
    add_syslinks("rt", "pthread")
end

if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
elseif is_mode("release") then
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end

add_rules("plugin.compile_commands.autoupdate")
