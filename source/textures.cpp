#include <cstring>

// BMP
#include "libnsbmp.h"

// GIF
#include "libnsgif.h"

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
        if (((long long)width * (long long)height) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
        
        return calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return (unsigned char *)bitmap;
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

namespace GIF {
    static void *bitmap_create(int width, int height) {
        /* ensure a stupidly large bitmap is not created */
        if (((long long)width * (long long)height) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
        
        return calloc(width * height, BYTES_PER_PIXEL);
    }

    static void bitmap_set_opaque(void *bitmap, bool opaque) {
        (void) opaque;  /* unused */
        (void) bitmap;  /* unused */
        assert(bitmap);
    }
    
    static bool bitmap_test_opaque(void *bitmap) {
        (void) bitmap;  /* unused */
        assert(bitmap);
        return false;
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return (unsigned char *)bitmap;
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        free(bitmap);
    }
    
    static void bitmap_modified(void *bitmap) {
        (void) bitmap;  /* unused */
        assert(bitmap);
        return;
    }
}

namespace ICO {
    static void *bitmap_create(int width, int height, unsigned int state) {
        (void) state;  /* unused */
        return calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return (unsigned char *)bitmap;
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
    bool LoadImage(unsigned char *data, int *width, int *height, Tex *texture, void (*free_func)(void *)) {    
        // Create a OpenGL texture identifier
        glGenTextures(1, &texture->id);
        glBindTexture(GL_TEXTURE_2D, texture->id);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        if (*free_func)
            free_func(data);

        texture->width = *width;
        texture->height = *height;
        return true;
    }

    bool LoadImageFile(const std::string &path, Tex *texture) {
        SceOff size = 0;
        unsigned char *data = nullptr, *image = nullptr;

        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
            
        int width = 0, height = 0;
        image = stbi_load_from_memory(data, size, &width, &height, nullptr, BYTES_PER_PIXEL);
        bool ret = LoadImage(image, &width, &height, texture, stbi_image_free);
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
        if (R_FAILED(FS::ReadFile(path, &data, (SceOff *)&size)))
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

        bool ret = LoadImage((unsigned char *)bmp.bitmap, (int *)&bmp.width, (int *)&bmp.height, texture, nullptr);
        bmp_finalise(&bmp);
        delete[] data;
        return ret;
    }

    bool LoadImageGIF(const std::string &path, Tex *texture, unsigned int *frames) {
        gif_bitmap_callback_vt bitmap_callbacks = {
            GIF::bitmap_create,
            GIF::bitmap_destroy,
            GIF::bitmap_get_buffer,
            GIF::bitmap_set_opaque,
            GIF::bitmap_test_opaque,
            GIF::bitmap_modified
        };

        gif_animation gif;
        SceOff size = 0;
        gif_result code = GIF_OK;
        unsigned char *data = nullptr;

        gif_create(&gif, &bitmap_callbacks);
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;
            
        do {
            code = gif_initialise(&gif, size, data);
            if (code != GIF_OK && code != GIF_WORKING) {
                gif_finalise(&gif);
                delete[] data;
                return false;
            }
        } while (code != GIF_OK);
        
        //bool gif_is_animated = gif.frame_count > 1;
        bool gif_is_animated = false;

        if (gif_is_animated) {
            *frames = gif.frame_count;

            for (unsigned int i = 0; i < gif.frame_count; i++) {
                code = gif_decode_frame(&gif, i);
                if (code != GIF_OK) {
                    delete[] data;
                    return false;
                }

                //LoadImage((unsigned char *)gif.frame_image, (int *)&gif.width, (int *)&gif.height, texture[i]);
            }
        }
        else {
            code = gif_decode_frame(&gif, 0);
            if (code != GIF_OK) {
                delete[] data;
                return false;
            }
            
            LoadImage((unsigned char *)gif.frame_image, (int *)&gif.width, (int *)&gif.height, texture, nullptr);
        }

        gif_finalise(&gif);
        delete[] data;
        
        return true;
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
        if (R_FAILED(FS::ReadFile(path, &data, (SceOff *)&size)))
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

        bool ret = LoadImage((unsigned char *)bmp->bitmap, (int *)&bmp->width, (int *)&bmp->height, texture, nullptr);
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
        int width = 0, height = 0, jpegsubsamp = 0;
        tjDecompressHeader2(jpeg, data, size, &width, &height, &jpegsubsamp);
        buffer = new unsigned char[width * height * 4];
        tjDecompress2(jpeg, data, size, buffer, width, 0, height, TJPF_RGBA, TJFLAG_FASTDCT);
        bool ret = LoadImage(buffer, &width, &height, texture, nullptr);
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
            
        int width = 0, height = 0;
        data = drpcx_load_memory(data, size, DRPCX_FALSE, &width, &height, nullptr, BYTES_PER_PIXEL);
        bool ret = LoadImage(data, &width, &height, texture, nullptr);
        delete[] data;
        return ret;
    }

    bool LoadImagePNG(const std::string &path, Tex *texture) {
        unsigned char *data = nullptr;
        SceOff size = 0;
        
        if (R_FAILED(FS::ReadFile(path, &data, &size)))
            return false;

        png_image image;
        std::memset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;

        if (R_FAILED(png_image_begin_read_from_memory(&image, data, size))) {
            delete[] data;
            return false;
        }

        image.format = PNG_FORMAT_RGBA;
        png_bytep buffer = new png_byte[PNG_IMAGE_SIZE(image)];

        if (buffer != nullptr) {
            if (R_FAILED(png_image_finish_read(&image, nullptr, buffer, 0, nullptr))) {
                png_image_free(&image);
                delete[] buffer;
                delete[] data;
                return false;
            }
        }

        bool ret = LoadImage(buffer, (int *)&image.width, (int *)&image.height, texture, nullptr);
        delete[] buffer;
        delete[] data;
        return ret;
    }

    bool LoadImageTIFF(const std::string &path, Tex *texture) {
        TIFF *tif = TIFFOpen(path.c_str(), "r");
        if (tif) {
            uint32 width = 0, height = 0;
            size_t num_pixels = 0;
            uint32 *raster = nullptr;
            
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            num_pixels = width * height;

            raster = (uint32 *)_TIFFmalloc(num_pixels * sizeof (uint32));
            if (raster != nullptr) {
                if (TIFFReadRGBAImage(tif, width, height, raster, 0))
                    LoadImage((unsigned char *)raster, (int *)&width, (int *)&height, texture, _TIFFfree);
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
            
        int width = 0, height = 0;
        data = WebPDecodeRGBA(data, size, &width, &height);
        bool ret = LoadImage(data, &width, &height, texture, nullptr);
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
