#pragma once

#include <SDL3/SDL.h>

namespace GUI {
    SDL_Renderer *GetRenderer(void);
    SDL_Window *GetWindow(void);
    int Init(void);
    void Exit(void);
    int RenderLoop(void);
}
