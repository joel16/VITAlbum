#pragma once

#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>
#include <SDL3/SDL.h>

#include "gui.h"
#include "imgui.h"
#include "reader.h"
#include "textures.h"

enum WINDOW_STATES {
    WINDOW_STATE_FILEBROWSER = 0,
    WINDOW_STATE_IMAGEVIEWER,
    WINDOW_STATE_BOOKVIEWER,
    WINDOW_STATE_SETTINGS
};

enum FS_SORT_STATE {
    FS_SORT_ALPHA_ASC = 0,
    FS_SORT_ALPHA_DESC,
    FS_SORT_SIZE_ASC,
    FS_SORT_SIZE_DESC
};

typedef struct {
    WINDOW_STATES state = WINDOW_STATE_FILEBROWSER;
    SceOff selected = 0;
    std::vector<SceIoDirent> entries;
    Tex texture;
    unsigned int frame_count = 0;
    float zoom_factor = 1.0f;
    SDL_Gamepad *gamepad;
    Book book;
} WindowData;

extern int sort;

namespace Windows {
    void SetupWindow(void);
    void ExitWindow(void);
    void HandleAnalogInput(WindowData& data, bool isBook);
    void HandleInput(WindowData& data, SDL_Event& event);
    void MainWindow(WindowData& data);
    void Viewer(WindowData &data, bool isBook);
}
