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

extern Tex folder_texture, image_texture;

namespace Textures {
    bool LoadImageFile(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImageBMP(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImageGIF(unsigned char **data, SceOff *size, std::vector<Tex> &textures);
    bool LoadImageICO(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImageJPEG(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImagePCX(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImagePNG(unsigned char **data, SceOff *size, Tex *texture);
    bool LoadImageSVG(unsigned char **data, Tex *texture);
    bool LoadImageTIFF(const std::string &path, Tex *texture);
    bool LoadImageWEBP(unsigned char **data, SceOff *size, Tex *texture);
    void Free(Tex *texture);
    void Init(void);
    void Exit(void);
}

#endif
