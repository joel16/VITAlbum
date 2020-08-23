#include <cstring>

// BMP
#include "libnsbmp.h"

// GIF
#include "gif_lib.h"

// JPEG
#include "turbojpeg.h"

// PCX
#define DR_PCX_IMPLEMENTATION
#define DR_PCX_NO_STDIO
#include "dr_pcx.h"

// PNG
#include <png.h>

// STB
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_BMP
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_JPEG
#define STBI_NO_PIC
#define STBI_NO_PNG
#define STBI_ONLY_PNM
#define STBI_ONLY_PSD
#define STBI_ONLY_TGA
#include "stb_image.h"

// TIFF
#include "tiffio.h"

// WEBP
#include "webp/decode.h"

#include "fs.h"
#include "imgui.h"
#include "textures.h"
#include "utils.h"

#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

Tex folder_texture, file_texture, image_texture;

namespace BMP {
    static void *bitmap_create(int width, int height, unsigned int state) {
        (void) state;  /* unused */
        /* ensure a stupidly large (>50Megs or so) bitmap is not created */
        if ((static_cast<long long>(width) * static_cast<long long>(height)) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
        
        return calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char *>(bitmap);
    }
    
    static size_t bitmap_get_bpp(void *bitmap) {
        (void) bitmap;  /* unused */
        return BYTES_PER_PIXEL;
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        free(bitmap);
    }
}

namespace ICO {
    static void *bitmap_create(int width, int height, unsigned int state) {
        (void) state;  /* unused */
        return calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char *>(bitmap);
    }
    
    static size_t bitmap_get_bpp(void *bitmap) {
        (void) bitmap;  /* unused */
        return BYTES_PER_PIXEL;
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        free(bitmap);
    }
}

namespace Textures {
    static bool LoadImage(unsigned char *data, GLint format, Tex *texture, void (*free_func)(void *)) {    
        // Create a OpenGL texture identifier
        glGenTextures(1, &texture->id);
        glBindTexture(GL_TEXTURE_2D, texture->id);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

        if (*free_func)
            free_func(data);
        
        return true;
    }

    bool LoadImageFile(const std::string &path, Tex *texture) {
        SceOff size = 0;
        unsigned char *data = nullptr, *image = nullptr;

        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
        
        image = stbi_load_from_memory(data, size, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = LoadImage(image, GL_RGBA, texture, stbi_image_free);
        delete[] data;
        return ret;
    }

    bool LoadImageBMP(const std::string &path, Tex *texture) {
        bmp_bitmap_callback_vt bitmap_callbacks = {
            BMP::bitmap_create,
            BMP::bitmap_destroy,
            BMP::bitmap_get_buffer,
            BMP::bitmap_get_bpp
        };
        
        bmp_result code = BMP_OK;
        bmp_image bmp;
        unsigned char *data = nullptr;
        size_t size = 0;

        bmp_create(&bmp, &bitmap_callbacks);
        if (R_FAILED(FS::ReadFile(path, &data, reinterpret_cast<SceOff *>(&size))))
            return false;
            
        code = bmp_analyse(&bmp, size, data);
        if (code != BMP_OK) {
            bmp_finalise(&bmp);
            delete[] data;
            return false;
        }

        code = bmp_decode(&bmp);
        if (code != BMP_OK) {
            if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
                bmp_finalise(&bmp);
                delete[] data;
                return false;
            }
            
            /* skip if the decoded image would be ridiculously large */
            if ((bmp.width * bmp.height) > 200000) {
                bmp_finalise(&bmp);
                delete[] data;
                return false;
            }
        }

        texture->width = bmp.width;
        texture->height = bmp.height;
        bool ret = LoadImage(static_cast<unsigned char *>(bmp.bitmap), GL_RGBA, texture, nullptr);
        bmp_finalise(&bmp);
        delete[] data;
        return ret;
    }

    bool LoadImageGIF(const std::string &path, Tex *texture, unsigned int *frames) {
        bool ret = false;
        int error = 0;
        GifFileType *gif = DGifOpenFileName(path.c_str(), &error);
        if (!gif)
            return false;

        if (DGifSlurp(gif) == GIF_ERROR) {
            DGifCloseFile(gif, &error);
            return false;
        }

        //*frames = gif->ImageCount;
        *frames = 0;
        GraphicsControlBlock gcb;

        if (*frames > 1) {
            for(int i = 0; i < *frames; i++) {
                DGifSavedExtensionToGCB(gif, i, &gcb);
                int pixels = gif->SavedImages[i].ImageDesc.Width * gif->SavedImages[i].ImageDesc.Height;
                unsigned char *buffer = new unsigned char[pixels * 4];
                unsigned char *image = buffer;
                
                for (int j = 0; j < pixels; j++) {
                    GifByteType byte = gif->SavedImages[i].RasterBits[j];
                    GifColorType colour = gif->SColorMap->Colors[byte];
                    buffer[4 * j + 0] = colour.Red;
                    buffer[4 * j + 1] = colour.Green;
                    buffer[4 * j + 2] = colour.Blue;
                    buffer[4 * j + 3] = (byte == gcb.TransparentColor) ? 0 : 255;
                }
                
                texture->width = gif->SWidth;
                texture->height = gif->SHeight;
                //ret = LoadImage(image, GL_RGBA, &texture[i], nullptr);
                ret = LoadImage(image, GL_RGBA, texture, nullptr);
                delete[] image;
            }
        }
        else {
            // Get the first frame
            DGifSavedExtensionToGCB(gif, 0, &gcb);
            int pixels = gif->SavedImages[0].ImageDesc.Width * gif->SavedImages[0].ImageDesc.Height;
            unsigned char *buffer = new unsigned char[pixels * 4];
            unsigned char *image = buffer;
            
            for (int i = 0; i < pixels; i++) {
                GifByteType byte = gif->SavedImages[0].RasterBits[i];
                GifColorType colour = gif->SColorMap->Colors[byte];
                buffer[4 * i + 0] = colour.Red;
                buffer[4 * i + 1] = colour.Green;
                buffer[4 * i + 2] = colour.Blue;
                buffer[4 * i + 3] = (byte == gcb.TransparentColor) ? 0 : 255;
            }
            
            texture->width = gif->SWidth;
            texture->height = gif->SHeight;
            ret = LoadImage(image, GL_RGBA, texture, nullptr);
            delete[] image;
        }

        DGifCloseFile(gif, &error);
        return ret;
    }

    bool LoadImageICO(const std::string &path, Tex *texture) {
        bmp_bitmap_callback_vt bitmap_callbacks = {
            ICO::bitmap_create,
            ICO::bitmap_destroy,
            ICO::bitmap_get_buffer,
            ICO::bitmap_get_bpp
        };
        
        uint16_t width = 0, height = 0;
        ico_collection ico;
        bmp_result code = BMP_OK;
        bmp_image *bmp;
        unsigned char *data = nullptr;
        size_t size = 0;

        ico_collection_create(&ico, &bitmap_callbacks);
        if (R_FAILED(FS::ReadFile(path, &data, reinterpret_cast<SceOff *>(&size))))
            return false;
            
        code = ico_analyse(&ico, size, data);
        if (code != BMP_OK) {
            ico_finalise(&ico);
            delete[] data;
            return false;
        }

        bmp = ico_find(&ico, width, height);
        assert(bmp);

        code = bmp_decode(bmp);
        if (code != BMP_OK) {
            if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
                ico_finalise(&ico);
                delete[] data;
                return false;
            }
            
            /* skip if the decoded image would be ridiculously large */
            if ((bmp->width * bmp->height) > 200000) {
                ico_finalise(&ico);
                delete[] data;
                return false;
            }
        }

        texture->width = bmp->width;
        texture->height = bmp->height;
        bool ret = LoadImage(static_cast<unsigned char *>(bmp->bitmap), GL_RGBA, texture, nullptr);
        ico_finalise(&ico);
        delete[] data;
        return ret;
    }

    bool LoadImageJPEG(const std::string &path, Tex *texture) {
        unsigned char *data = nullptr, *buffer = nullptr;
        SceOff size = 0;
        
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
            
        tjhandle jpeg = tjInitDecompress();
        int jpegsubsamp = 0;
        tjDecompressHeader2(jpeg, data, size, &texture->width, &texture->height, &jpegsubsamp);
        buffer = new unsigned char[texture->width * texture->height * 3];
        tjDecompress2(jpeg, data, size, buffer, texture->width, 0, texture->height, TJPF_RGB, TJFLAG_FASTDCT);
        bool ret = LoadImage(buffer, GL_RGB, texture, nullptr);
        tjDestroy(jpeg);
        delete[] buffer;
        delete[] data;
        return ret;
    }

    bool LoadImagePCX(const std::string &path, Tex *texture) {
        unsigned char *data = nullptr;
        SceOff size = 0;
        
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
        
        data = drpcx_load_memory(data, size, DRPCX_FALSE, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = LoadImage(data, GL_RGBA, texture, nullptr);
        delete[] data;
        return ret;
    }

    bool LoadImagePNG(const std::string &path, Tex *texture) {
        bool ret = false;
        unsigned char *data = nullptr;
        SceOff size = 0;
        
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return ret;
        
        png_image image;
        memset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;
        
        if (png_image_begin_read_from_memory(&image, data, size) != 0) {
            png_bytep buffer;
            image.format = PNG_FORMAT_RGBA;
            buffer = new png_byte[PNG_IMAGE_SIZE(image)];
            
            if (buffer != nullptr && png_image_finish_read(&image, nullptr, buffer, 0, nullptr) != 0) {
                texture->width = image.width;
                texture->height = image.height;
                ret = Textures::LoadImage(buffer, GL_RGBA, texture, nullptr);
                delete[] buffer;
                png_image_free(&image);
            }
            else {
                if (buffer == nullptr)
                    png_image_free(&image);
                else
                    delete[] buffer;
            }
        }

        delete[] data;
        return ret;
    }

    bool LoadImageTIFF(const std::string &path, Tex *texture) {
        TIFF *tif = TIFFOpen(path.c_str(), "r");
        if (tif) {
            size_t num_pixels = 0;
            uint32 *raster = nullptr;
            
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &texture->width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &texture->height);
            num_pixels = texture->width * texture->height;

            raster = static_cast<uint32 *>(_TIFFmalloc(num_pixels * sizeof (uint32)));
            if (raster != nullptr) {
                if (TIFFReadRGBAImage(tif, texture->width, texture->height, raster, 0))
                    LoadImage(reinterpret_cast<unsigned char *>(raster), GL_RGBA, texture, _TIFFfree);
            }

            TIFFClose(tif);
            return true;
        }
        return false;
    }

    bool LoadImageWEBP(const std::string &path, Tex *texture) {
        unsigned char *data = nullptr;
        SceOff size = 0;
        
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
        
        data = WebPDecodeRGBA(data, size, &texture->width, &texture->height);
        bool ret = LoadImage(data, GL_RGBA, texture, nullptr);
        delete[] data;
        return ret;
    }

    void Free(Tex *texture) {
        glDeleteTextures(1, &texture->id);
    }

    void Init(void) {
        bool image_ret = LoadImagePNG("app0:res/file.png", &file_texture);
        IM_ASSERT(image_ret);
        
        image_ret = LoadImagePNG("app0:res/folder.png", &folder_texture);
        IM_ASSERT(image_ret);
        
        image_ret = LoadImagePNG("app0:res/image.png", &image_texture);
        IM_ASSERT(image_ret);
    }

    void Exit(void) {
        Free(&image_texture);
        Free(&folder_texture);
        Free(&file_texture);
    }
}
