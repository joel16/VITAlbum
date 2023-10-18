#pragma once

#include <string>
#include <vector>
#include <psp2common/types.h>
#include <SDL.h>

typedef struct {
    SDL_Texture *ptr;
    int width = 0;
    int height = 0;
    SceUInt delay = 0; // microseconds
} Tex;

extern std::vector<Tex> icons;
extern unsigned const FOLDER, IMAGE;

namespace Textures {
    bool LoadImageFile(const std::string &path, std::vector<Tex> &textures);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}
