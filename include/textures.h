#ifndef _VITALBUM_TEXTURES_H_
#define _VITALBUM_TEXTURES_H_

#include <string>
#include <vector>
#include <vitaGL.h>

typedef struct {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    int delay = 0;
} Tex;

extern std::vector<Tex> icons;
extern unsigned const FOLDER, IMAGE;

namespace Textures {
    bool LoadImageFile(const std::string &path, std::vector<Tex> &textures);
    void Free(Tex &texture);
    void Init(void);
    void Exit(void);
}

#endif
