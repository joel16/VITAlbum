cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(VITAlbum)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VITAlbum")
set(VITA_TITLEID "VITALBUM0")
set(VITA_VERSION "01.40")

add_definitions(-DAPP_VERSION="${VITA_VERSION}")
add_definitions(
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS
    -DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
    -DIMGUI_DISABLE_WIN32_FUNCTIONS -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION -DIMGUI_ENABLE_FREETYPE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wall -Wno-psabi -Wno-unused-function -fno-rtti -std=gnu++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

find_package(SDL2 REQUIRED)

include_directories(
    ${VITASDK}/arm-vita-eabi/include/freetype2/
    ${SDL2_INCLUDE_DIR}
    libs/giflib
    libs/imgui
    libs/imgui/backends
    libs/libnsbmp
    libs/libtiff
    libs
    include
)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libs/lib
)

add_executable(${PROJECT_NAME}
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_sdl2.cpp
    libs/imgui/backends/imgui_impl_sdlrenderer2.cpp
    libs/imgui/misc/freetype/imgui_freetype.cpp
    libs/libnsbmp/libnsbmp.c
    source/config.cpp
    source/fs.cpp
    source/gui.cpp
    source/image.cpp
    source/keyboard.cpp
    source/log.cpp
    source/main.cpp
    source/textures.cpp
    source/utils.cpp
    source/window.cpp
    source/popups/properties.cpp
    source/tabs/filebrowser.cpp
    source/tabs/settings.cpp
)

target_link_libraries(${PROJECT_NAME}
    freetype
    bz2
    gif
    tiff
    webpdemux
    webp
    sharpyuv
    turbojpeg
    jpeg
    png
    pthread
    lzma
    z
    zstd
    SDL2::SDL2-static
    SceLibJson_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE res/folder.png res/folder.png
    FILE res/image.png res/image.png
)
