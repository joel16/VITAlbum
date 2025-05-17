cmake_minimum_required(VERSION 3.5)

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
set(VITA_VERSION "01.41")

add_definitions(-DAPP_VERSION="${VITA_VERSION}")
add_definitions(
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -DIMGUI_DISABLE_DEMO_WINDOWS -DIMGUI_DISABLE_DEBUG_TOOLS
    -DIMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS -DIMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
    -DIMGUI_DISABLE_WIN32_FUNCTIONS -DIMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION -DIMGUI_DISABLE_SSE
    -DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS -DIMGUI_ENABLE_FREETYPE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wall -Wno-psabi -Wno-unused-function -fno-rtti -std=gnu++17")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)

include_directories(
    ${VITASDK}/arm-vita-eabi/include/freetype2/
    libs/imgui
    libs/imgui/backends
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
    libs/imgui/backends/imgui_impl_sdl3.cpp
    libs/imgui/backends/imgui_impl_sdlrenderer3.cpp
    libs/imgui/misc/freetype/imgui_freetype.cpp
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
    tiff
    lzma
    zstd
    SDL3_image::SDL3_image
    SDL3::SDL3
    webpdemux
    webp
    sharpyuv
    pthread
    SceLibJson_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
    FILE res/folder.png res/folder.png
    FILE res/image.png res/image.png
)
