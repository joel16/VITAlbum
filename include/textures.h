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

extern Tex folder_texture, file_texture, image_texture;

namespace Textures {
    bool LoadImageFile(const std::string &path, Tex *texture);
    bool LoadImageBMP(const std::string &path, Tex *texture);
    bool LoadImageGIF(const std::string &path, std::vector<Tex> &textures);
    bool LoadImageICO(const std::string &path, Tex *texture);
    bool LoadImageJPEG(const std::string &path, Tex *texture);
    bool LoadImagePCX(const std::string &path, Tex *texture);
    bool LoadImagePNG(const std::string &path, Tex *texture);
    bool LoadImageTIFF(const std::string &path, Tex *texture);
    bool LoadImageWEBP(const std::string &path, Tex *texture);
    void Free(Tex *texture);
    void Init(void);
    void Exit(void);
}

#endif
