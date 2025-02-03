#include <cstring>
#include <memory>
#include <psp2/kernel/clib.h>

// STB
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

// TIFF
#include "tiffio.h"
#include "tiffiop.h"

#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "log.h"
#include "textures.h"
#include "utils.h"

std::vector<Tex> icons;
unsigned const FOLDER = 0, IMAGE = 1;

namespace Textures {
    static bool Create(unsigned char *data, Tex &texture) {
        SDL_Surface *surface = SDL_CreateSurfaceFrom(
            texture.width,
            texture.height,
            SDL_PIXELFORMAT_RGBA8888,
            data,
            texture.width * 4
        );
        
        if (surface == nullptr) {
            Log::Error("Failed to create SDL surface: %s\n", SDL_GetError());
            return false;
        }
        
        texture.ptr = SDL_CreateTextureFromSurface(GUI::GetRenderer(), surface);
        if (texture.ptr == nullptr) {
            Log::Error("Failed to create SDL texture: %s\n", SDL_GetError());
            SDL_DestroySurface(surface);
            return false;
        }
        
        SDL_DestroySurface(surface);
        return true;
    }

    static bool LoadImageOther(unsigned char **data, SceOff &size, Tex &texture) {
        stbi_uc *image = nullptr;
        image = stbi_load_from_memory(*data, size, &texture.width, &texture.height, nullptr, STBI_rgb_alpha);
        bool ret = Textures::Create(image, texture);
        stbi_image_free(image);
        return ret;
    }

    static bool LoadImageAnim(const std::string &path, Tex &texture) {
        texture.anim = IMG_LoadAnimation(path.c_str());
        if (!texture.anim) {
            Log::Error("Couldn't load %s: %s\n", path.c_str(), SDL_GetError());
        }
        
        texture.width = texture.anim->w;
        texture.height = texture.anim->h;
        texture.frames = (SDL_Texture **)SDL_calloc(texture.anim->count, sizeof(*texture.frames));
        
        if (!texture.frames) {
            Log::Error("Couldn't allocate textures\n");
            IMG_FreeAnimation(texture.anim);
            return false;
        }
        
        for (int i = 0; i < texture.anim->count; ++i) {
            texture.frames[i] = SDL_CreateTextureFromSurface(GUI::GetRenderer(), texture.anim->frames[i]);
        }
        
        return true;
    }

    static bool LoadImage(const std::string &path, Tex &texture) {
        texture.ptr = IMG_LoadTexture(GUI::GetRenderer(), path.c_str());

        if (!texture.ptr) {
            Log::Error("Couldn't load %s: %s\n", path.c_str(), SDL_GetError());
            return false;
        }

        SDL_PropertiesID properties = SDL_GetTextureProperties(texture.ptr);
        texture.width = SDL_GetNumberProperty(properties, SDL_PROP_TEXTURE_WIDTH_NUMBER, -1);
        texture.height = SDL_GetNumberProperty(properties, SDL_PROP_TEXTURE_HEIGHT_NUMBER, -1);
        return true;
    }

    void Free(Tex &texture) {
        if (texture.frames) {
            for (int i = 0; i < texture.anim->count; ++i) {
                SDL_DestroyTexture(texture.frames[i]);
            }

            IMG_FreeAnimation(texture.anim);

            // Not sure why we need to specify this? I'd assume Destroy/Free would set it to NULL
            texture.frames = nullptr;
            texture.anim = nullptr;
        }

        if (texture.ptr) {
            SDL_DestroyTexture(texture.ptr);
            texture.ptr = nullptr;
        }
    }

    static bool LoadImageTIFF(const std::string &path, Tex &texture) {
        TIFF *tif = TIFFOpen(path.c_str(), "r");
        if (tif) {
            size_t pixel_count = 0;
            SceUInt32 *raster = nullptr;
            
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &texture.width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &texture.height);
            pixel_count = texture.width * texture.height;
            
            raster = (SceUInt32 *)_TIFFCheckMalloc(tif, pixel_count, sizeof(SceUInt32), "raster buffer");
            if (raster != nullptr) {
                if (TIFFReadRGBAImageOriented(tif, texture.width, texture.height, raster, ORIENTATION_TOPLEFT)) {
                    Textures::Create(reinterpret_cast<unsigned char*>(raster), texture);
                    _TIFFfree(raster);
                }
                else {
                    Log::Error("TIFFReadRGBAImage failed\n");
                }

            }
            else {
                Log::Error("_TIFFmalloc failed\n");
            }

            TIFFClose(tif);
            return true;
        }
        else {
            Log::Error("TIFFOpen failed\n");
        }
        
        return false;
    }

    bool LoadImageFile(const std::string &path, Tex &texture) {
        bool ret = false;
        const char *ext = FS::GetFileExt(path.c_str());
        
        // Because TIFF does not load via buffer, but directly from the path.
        if ((strncasecmp(ext, ".GIF", 4) == 0) || (strncasecmp(ext, ".WEBP", 5) == 0)) {
            ret = Textures::LoadImageAnim(path, texture);
        }
        else if ((strncasecmp(ext, ".BMP", 4) == 0) || (strncasecmp(ext, ".JPG", 4) == 0) || (strncasecmp(ext, ".JPEG", 5) == 0)
            || (strncasecmp(ext, ".LBM", 4) == 0) || (strncasecmp(ext, ".PCX", 4) == 0) || (strncasecmp(ext, ".LBM", 4) == 0)
            || (strncasecmp(ext, ".PNG", 4) == 0) || (strncasecmp(ext, ".PNM", 4) == 0) || (strncasecmp(ext, ".PPM", 4) == 0)
            || (strncasecmp(ext, ".PGM", 4) == 0) || (strncasecmp(ext, ".PBM", 4) == 0) || (strncasecmp(ext, ".QOI", 4) == 0)
            || (strncasecmp(ext, ".TGA", 4) == 0) || (strncasecmp(ext, ".XCF", 4) == 0) || (strncasecmp(ext, ".XPM", 4) == 0)
            || (strncasecmp(ext, ".SVG", 4) == 0)) {
            ret = Textures::LoadImage(path, texture);
        }
        else if (strncasecmp(ext, ".TIFF", 5) == 0) {
            ret = Textures::LoadImageTIFF(path, texture);
        }
        else {
            unsigned char *data = nullptr;
            SceOff size = 0;
            FS::ReadFile(path, &data, size);

            if (strncasecmp(ext, ".PSD", 4) == 0) {
                ret = Textures::LoadImageOther(&data, size, texture);
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
            Tex texture;
            bool ret = Textures::LoadImage(filenames[i], texture);
            IM_ASSERT(ret);

            icons.push_back(texture);
        }
    }

    void Exit(void) {
        for (unsigned int i = 0; i < icons.size(); i++) {
            Textures::Free(icons[i]);
        }
    }
}
