#pragma once

#include <string>
#include <vector>
#include <SDL3_image/SDL_image.h>

struct Tex {
    SDL_Texture *ptr = nullptr;
    IMG_Animation *anim = nullptr;
    int width = 0;
    int height = 0;
    int frame = 0;
    uint64_t tick = 0;
};

extern std::vector<Tex> icons;
extern unsigned const FOLDER, IMAGE;

namespace Textures {
    bool LoadImageFile(const std::string &path, Tex &texture);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}
