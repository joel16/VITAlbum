#pragma once

#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>

#include "gui.h"
#include "imgui.h"
#include "textures.h"

enum WINDOW_STATES {
    WINDOW_STATE_FILEBROWSER = 0,
    WINDOW_STATE_IMAGEVIEWER,
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
} WindowData;

extern int sort;

namespace Windows {
    void SetupWindow(void);
    void ExitWindow(void);
    void MainWindow(WindowData &data, SceCtrlData &pad);
    void ImageViewer(WindowData &data);
}
