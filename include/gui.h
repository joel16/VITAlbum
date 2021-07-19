#ifndef _VITALBUM_GUI_H_
#define _VITALBUM_GUI_H_

#include <psp2/io/dirent.h>

#include "textures.h"

enum GUI_STATES {
    GUI_STATE_HOME,
    GUI_STATE_IMAGE_PREVIEW,
    GUI_STATE_SETTINGS
};

typedef struct {
    GUI_STATES state = GUI_STATE_HOME;
    SceOff selected = 0;
    std::vector<SceIoDirent> entries;
    std::vector<Tex> textures;
    int frame_count = 0;
    float zoom_factor = 1.0f;
} MenuItem;

namespace GUI {
    int RenderLoop(void);
}

#endif
