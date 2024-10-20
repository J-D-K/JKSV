# This is mainly for IDE Support (CLION), not for building (use Makefile directly).
cmake_minimum_required(VERSION 3.8)
project(JKSV)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        src/cfg.cpp
        src/curlfuncs.cpp
        src/data.cpp
        src/fs.cpp
        src/gd.cpp
        src/gfx.cpp
        src/main.cpp
        src/rfs.cpp
        src/type.cpp
        src/ui.cpp
        src/util.cpp
        src/webdav.cpp
        src/fs/dir.cpp
        src/fs/remote.cpp
        src/fs/file.cpp
        src/fs/fsfile.c
        src/fs/zip.cpp
        src/gfx/textureMgr.cpp
        src/ui/ext.cpp
        src/ui/fld.cpp
        src/ui/fm.cpp
        src/ui/miscui.cpp
        src/ui/sett.cpp
        src/ui/sldpanel.cpp
        src/ui/thrdProc.cpp
        src/ui/ttl.cpp
        src/ui/ttlview.cpp
        src/ui/uistr.cpp
        src/ui/usr.cpp)

# Specify external includes here
include_directories(./inc)
include_directories(./inc/fs)
include_directories(./inc/gfx)
include_directories(./inc/ui)

include_directories($ENV{DEVKITPRO}/devkitA64/aarch64-none-elf/include)
include_directories($ENV{DEVKITPRO}/devkitA64/lib/gcc/aarch64-none-elf/10.1.0/include)
include_directories($ENV{DEVKITPRO}/libnx/include)
include_directories($ENV{DEVKITPRO}/portlibs/switch/include)
include_directories($ENV{DEVKITPRO}/portlibs/switch/include/freetype2)

add_executable(JKSV ${SOURCE_FILES})