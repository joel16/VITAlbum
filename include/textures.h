#pragma once

#include <string>
#include <vector>
#include <psp2common/types.h>
#include <string>
#include <vector>
#include <psp2common/types.h>
#include <SDL_image.h>

typedef struct {
    SDL_Texture *ptr = nullptr;
    SDL_Texture **frames = nullptr;
    IMG_Animation *anim = nullptr;
    int width = 0;
    int height = 0;
} Tex;

extern std::vector<Tex> icons;
extern unsigned const FOLDER, IMAGE;

namespace Textures {
    bool LoadImageFile(const std::string &path, Tex &texture);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}
