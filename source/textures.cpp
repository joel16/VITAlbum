#include <cstring>
#include <memory>
#include <psp2/kernel/clib.h>

// STB
#define STBI_NEON
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_BMP
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_JPEG
#define STBI_NO_PIC
#define STBI_NO_PNG
#define STBI_NO_PNM
#define STBI_NO_TGA
#define STBI_ONLY_PSD
#include "stb_image.h"

#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "log.h"
#include "textures.h"
#include "utils.h"

std::vector<Tex> icons;
unsigned const FOLDER = 0, IMAGE = 1;

namespace Textures {
    static bool Create(unsigned char *data, Tex &tex) {
        tex.ptr = SDL_CreateTexture(GUI::GetRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, tex.width, tex.height);
        
        if (!tex.ptr) {
            Log::Error("Failed to create SDL texture: %s\n", SDL_GetError());
            return false;
        }
        
        if (SDL_UpdateTexture(tex.ptr, nullptr, data, tex.width * 4) != 0) {
            Log::Error("Failed to update texture: %s\n", SDL_GetError());
            SDL_DestroyTexture(tex.ptr);
            return false;
        }
        
        return true;
    }

    static bool LoadImageOther(unsigned char **data, SceOff &size, Tex &tex) {
        stbi_uc *image = nullptr;
        image = stbi_load_from_memory(*data, size, &tex.width, &tex.height, nullptr, STBI_rgb_alpha);
        bool ret = Textures::Create(image, tex);
        stbi_image_free(image);
        return ret;
    }
    
    static bool LoadImageAnim(const std::string &path, Tex &tex) {
        unsigned char *data = nullptr;
        SceOff size = 0;
        
        if (FS::ReadFile(path, &data, size) < 0 || !data) {
            Log::Error("Failed to read %s into memory\n", path.c_str());
            return false;
        }
        
        SDL_IOStream *io = SDL_IOFromMem(data, size);
        if (!io) {
            delete[] data; 
            return false;
        }
        
        tex.anim = IMG_LoadAnimation_IO(io, true);
        delete[] data;

        if (!tex.anim || tex.anim->count == 0) {
            Log::Error("Couldn't decode %s: %s\n", path.c_str(), SDL_GetError());
            return false;
        }
        
        tex.width = tex.anim->w;
        tex.height = tex.anim->h;
        
        SDL_Surface *frame = tex.anim->frames[0];
        
        tex.ptr = SDL_CreateTexture(GUI::GetRenderer(), frame->format, SDL_TEXTUREACCESS_STREAMING, tex.width, tex.height);
        
        if (!tex.ptr) {
            Log::Error("Failed to create animation streaming texture: %s\n", SDL_GetError());
            IMG_FreeAnimation(tex.anim);
            tex.anim = nullptr;
            return false;
        }
        
        SDL_UpdateTexture(tex.ptr, nullptr, frame->pixels, frame->pitch);
        tex.frame = 0;
        tex.tick = SDL_GetTicks();
        return true;
    }

    static bool LoadImage(const std::string &path, Tex &tex) {
        tex.ptr = IMG_LoadTexture(GUI::GetRenderer(), path.c_str());

        if (!tex.ptr) {
            Log::Error("Couldn't load %s: %s\n", path.c_str(), SDL_GetError());
            return false;
        }

        SDL_PropertiesID properties = SDL_GetTextureProperties(tex.ptr);
        tex.width = SDL_GetNumberProperty(properties, SDL_PROP_TEXTURE_WIDTH_NUMBER, -1);
        tex.height = SDL_GetNumberProperty(properties, SDL_PROP_TEXTURE_HEIGHT_NUMBER, -1);
        return true;
    }

    void Free(Tex &tex) {
        if (tex.anim) {
            IMG_FreeAnimation(tex.anim);
            tex.anim = nullptr;
        }
        
        if (tex.ptr) {
            SDL_DestroyTexture(tex.ptr);
            tex.ptr = nullptr;
        }

        tex.frame = 0;
        tex.tick = 0;
    }

    bool LoadImageFile(const std::string &path, Tex &tex) {
        bool ret = false;
        const char *ext = FS::GetFileExt(path.c_str());
        
        if ((strncasecmp(ext, ".GIF", 4) == 0) || (strncasecmp(ext, ".WEBP", 5) == 0)) {
            ret = Textures::LoadImageAnim(path, tex);
        }
        else if ((strncasecmp(ext, ".BMP", 4) == 0) || (strncasecmp(ext, ".JPG", 4) == 0) || (strncasecmp(ext, ".JPEG", 5) == 0)
            || (strncasecmp(ext, ".LBM", 4) == 0) || (strncasecmp(ext, ".PCX", 4) == 0) || (strncasecmp(ext, ".LBM", 4) == 0)
            || (strncasecmp(ext, ".PNG", 4) == 0) || (strncasecmp(ext, ".PNM", 4) == 0) || (strncasecmp(ext, ".PPM", 4) == 0)
            || (strncasecmp(ext, ".PGM", 4) == 0) || (strncasecmp(ext, ".PBM", 4) == 0) || (strncasecmp(ext, ".QOI", 4) == 0)
            || (strncasecmp(ext, ".TGA", 4) == 0) || (strncasecmp(ext, ".XCF", 4) == 0) || (strncasecmp(ext, ".XPM", 4) == 0)
            || (strncasecmp(ext, ".SVG", 4) == 0) ||(strncasecmp(ext, ".TIFF", 5) == 0)) {
            ret = Textures::LoadImage(path, tex);
        }
        else {
            unsigned char *data = nullptr;
            SceOff size = 0;
            FS::ReadFile(path, &data, size);

            if (strncasecmp(ext, ".PSD", 4) == 0) {
                ret = Textures::LoadImageOther(&data, size, tex);
            }

            if (data) {
                delete[] data;
            }
        }
        
        return ret;
    }
    
    void Init(void) {
        const int num_icons = 2;
        std::string filenames[num_icons] = {
            "app0:res/folder.png",
            "app0:res/image.png"
        };

        for (int i = 0; i < num_icons; i++) {
            Tex tex;
            bool ret = Textures::LoadImage(filenames[i], tex);
            IM_ASSERT(ret);

            icons.push_back(tex);
        }
    }

    void Exit(void) {
        for (unsigned int i = 0; i < icons.size(); i++) {
            Textures::Free(icons[i]);
        }
    }
}
