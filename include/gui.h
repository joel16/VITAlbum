#ifndef _VITALBUM_GUI_H_
#define _VITALBUM_GUI_H_

#include "textures.h"

enum GUI_STATES {
    GUI_STATE_HOME,
    GUI_STATE_IMAGE_PREVIEW,
    GUI_STATE_GIF_PREVIEW,
    GUI_STATE_SETTINGS
};

typedef struct {
    int state = GUI_STATE_HOME;
    SceOff selected = 0;
    std::vector<SceIoDirent> entries;
    Tex texture;
    std::vector<Tex> textures;
    int frame_count = 0;
    bool image_properties = false;
} MenuItem;

namespace GUI {
    int RenderLoop(void);
}

#endif
