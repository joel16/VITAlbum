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
        SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
                data,
                texture.width,
                texture.height,
                32,
                texture.width * 4,
                0x000000FF,
                0x0000FF00,
                0x00FF0000,
                0xFF000000
        );
        
        if (surface == nullptr) {
            Log::Error("Failed to create SDL surface: %s\n", SDL_GetError());
            return false;
        }
        
        texture.ptr = SDL_CreateTextureFromSurface(GUI::GetRenderer(), surface);
        if (texture.ptr == nullptr) {
            Log::Error("Failed to create SDL texture: %s\n", SDL_GetError());
            SDL_FreeSurface(surface);
            return false;
        }
        
        SDL_FreeSurface(surface);
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

        SDL_QueryTexture(texture.ptr, nullptr, nullptr, &texture.width, &texture.height);
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
        std::string ext = FS::GetFileExt(path);
        
        // Because TIFF does not load via buffer, but directly from the path.
        if ((ext == ".GIF") || (ext == ".WEBP")) {
            ret = Textures::LoadImageAnim(path, texture);
        }
        else if ((ext == ".BMP") || (ext == ".JPEG") || (ext == ".JPG") || (ext == ".LBM") || (ext == ".PCX") ||
            (ext == ".PNG") || (ext == ".PNM") || (ext == ".PPM") || (ext == ".PGM") || (ext == ".PBM") || 
            (ext == ".QOI") || (ext == ".TGA") || (ext == ".XCF") || (ext == ".XPM") || (ext == ".SVG")) {
            ret = Textures::LoadImage(path, texture);
        }
        else if (ext == ".TIFF") {
            ret = Textures::LoadImageTIFF(path, texture);
        }
        else {
            unsigned char *data = nullptr;
            SceOff size = 0;
            FS::ReadFile(path, &data, size);

            if (ext == ".PSD") {
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
