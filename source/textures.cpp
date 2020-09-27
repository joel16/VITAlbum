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
#include "log.h"
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
        
        return std::calloc(width * height, BYTES_PER_PIXEL);
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
        std::free(bitmap);
    }
}

namespace GIF {
    static void *bitmap_create(int width, int height) {
        /* ensure a stupidly large bitmap is not created */
        if ((static_cast<long long>(width) * static_cast<long long>(height)) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
        
        return std::calloc(width * height, BYTES_PER_PIXEL);
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
        return static_cast<unsigned char *>(bitmap);
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        std::free(bitmap);
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
        return std::calloc(width * height, BYTES_PER_PIXEL);
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
        std::free(bitmap);
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

    bool LoadImageFile(unsigned char **data, SceOff *size, Tex *texture) {
        unsigned char *image = nullptr;
        image = stbi_load_from_memory(*data, *size, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::LoadImage(image, GL_RGBA, texture, stbi_image_free);
        return ret;
    }

    bool LoadImageBMP(unsigned char **data, SceOff *size, Tex *texture) {
        bmp_bitmap_callback_vt bitmap_callbacks = {
            BMP::bitmap_create,
            BMP::bitmap_destroy,
            BMP::bitmap_get_buffer,
            BMP::bitmap_get_bpp
        };
        
        bmp_result code = BMP_OK;
        bmp_image bmp;
        bmp_create(&bmp, &bitmap_callbacks);
            
        code = bmp_analyse(&bmp, *size, *data);
        if (code != BMP_OK) {
            Log::Error("bmp_analyse failed: %d\n", code);
            bmp_finalise(&bmp);
            return false;
        }

        code = bmp_decode(&bmp);
        if (code != BMP_OK) {
            if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
                Log::Error("bmp_decode failed: %d\n", code);
                bmp_finalise(&bmp);
                return false;
            }
            
            /* skip if the decoded image would be ridiculously large */
            if ((bmp.width * bmp.height) > 200000) {
                Log::Error("bmp_decode failed: width*height is over 200000\n");
                bmp_finalise(&bmp);
                return false;
            }
        }

        texture->width = bmp.width;
        texture->height = bmp.height;
        bool ret = Textures::LoadImage(static_cast<unsigned char *>(bmp.bitmap), GL_RGBA, texture, nullptr);
        bmp_finalise(&bmp);
        return ret;
    }

    bool LoadImageGIF(unsigned char **data, SceOff *size, std::vector<Tex> &textures) {
        gif_bitmap_callback_vt bitmap_callbacks = {
            GIF::bitmap_create,
            GIF::bitmap_destroy,
            GIF::bitmap_get_buffer,
            GIF::bitmap_set_opaque,
            GIF::bitmap_test_opaque,
            GIF::bitmap_modified
        };

        bool ret = false;
        gif_animation gif;
        gif_result code = GIF_OK;
        gif_create(&gif, &bitmap_callbacks);
            
        do {
            code = gif_initialise(&gif, *size, *data);
            if (code != GIF_OK && code != GIF_WORKING) {
                Log::Error("gif_initialise failed: %d\n", code);
                gif_finalise(&gif);
                return ret;
            }
        } while (code != GIF_OK);
        
        bool gif_is_animated = gif.frame_count > 1;

        if (gif_is_animated) {
            textures.resize(gif.frame_count);

            for (unsigned int i = 0; i < gif.frame_count; i++) {
                code = gif_decode_frame(&gif, i);
                if (code != GIF_OK) {
                    Log::Error("gif_decode_frame failed: %d\n", code);
                    return false;
                }

                textures[i].width = gif.width;
                textures[i].height = gif.height;
                textures[i].delay = gif.frames->frame_delay;
                ret = Textures::LoadImage(static_cast<unsigned char *>(gif.frame_image), GL_RGBA, &textures[i], nullptr);
            }
        }
        else {
            code = gif_decode_frame(&gif, 0);
            if (code != GIF_OK) {
                Log::Error("gif_decode_frame failed: %d\n", code);
                return false;
            }
            
            textures[0].width = gif.width;
            textures[0].height = gif.height;
            ret = Textures::LoadImage(static_cast<unsigned char *>(gif.frame_image), GL_RGBA, &textures[0], nullptr);
        }

        gif_finalise(&gif);
        return ret;
    }

    bool LoadImageICO(unsigned char **data, SceOff *size, Tex *texture) {
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

        ico_collection_create(&ico, &bitmap_callbacks);

        code = ico_analyse(&ico, *size, *data);
        if (code != BMP_OK) {
            Log::Error("ico_analyse failed: %d\n", code);
            ico_finalise(&ico);
            return false;
        }

        bmp = ico_find(&ico, width, height);
        assert(bmp);

        code = bmp_decode(bmp);
        if (code != BMP_OK) {
            if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) {
                Log::Error("bmp_decode failed: %d\n", code);
                ico_finalise(&ico);
                return false;
            }
            
            /* skip if the decoded image would be ridiculously large */
            if ((bmp->width * bmp->height) > 200000) {
                Log::Error("bmp_decode failed: width*height is over 200000\n");
                ico_finalise(&ico);
                delete[] *data;
                return false;
            }
        }

        texture->width = bmp->width;
        texture->height = bmp->height;
        bool ret = Textures::LoadImage(static_cast<unsigned char *>(bmp->bitmap), GL_RGBA, texture, nullptr);
        ico_finalise(&ico);
        return ret;
    }

    bool LoadImageJPEG(unsigned char **data, SceOff *size, Tex *texture) {
        unsigned char *buffer = nullptr;
        tjhandle jpeg = tjInitDecompress();
        int jpegsubsamp = 0;

        if (R_FAILED(tjDecompressHeader2(jpeg, *data, *size, &texture->width, &texture->height, &jpegsubsamp))) {
            Log::Error("tjDecompressHeader2 failed: %s\n", tjGetErrorStr());
            delete[] data;
            return false;
        }
        
        buffer = new unsigned char[texture->width * texture->height * 3];

        if (R_FAILED(tjDecompress2(jpeg, *data, *size, buffer, texture->width, 0, texture->height, TJPF_RGB, TJFLAG_FASTDCT))) {
            Log::Error("tjDecompress2 failed: %s\n", tjGetErrorStr());
            delete[] buffer;
            return false;
        }
        
        bool ret = Textures::LoadImage(buffer, GL_RGB, texture, nullptr);
        tjDestroy(jpeg);
        delete[] buffer;
        return ret;
    }

    bool LoadImagePCX(unsigned char **data, SceOff *size, Tex *texture) {
        *data = drpcx_load_memory(*data, *size, DRPCX_FALSE, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::LoadImage(*data, GL_RGBA, texture, nullptr);
        return ret;
    }

    bool LoadImagePNG(unsigned char **data, SceOff *size, Tex *texture) {
        bool ret = false;
        png_image image;
        std::memset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;
        
        if (png_image_begin_read_from_memory(&image, *data, *size) != 0) {
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
                if (buffer == nullptr) {
                    Log::Error("png_byte buffer: returned nullptr\n");
                    png_image_free(&image);
                }
                else {
                    Log::Error("png_image_finish_read failed: %s\n", image.message);
                    delete[] buffer;
                }
            }
        }
        else
            Log::Error("png_image_begin_read_from_memory failed: %s\n", image.message);
        
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
                if (TIFFReadRGBAImage(tif, texture->width, texture->height, raster))
                    LoadImage(reinterpret_cast<unsigned char *>(raster), GL_RGBA, texture, _TIFFfree);
                else
                    Log::Error("TIFFReadRGBAImage failed\n");

            }
            else
                Log::Error("_TIFFmalloc failed\n");

            TIFFClose(tif);
            return true;
        }
        else
            Log::Error("TIFFOpen failed\n");
        
        return false;
    }

    bool LoadImageWEBP(unsigned char **data, SceOff *size, Tex *texture) {
        *data = WebPDecodeRGBA(*data, *size, &texture->width, &texture->height);
        bool ret = Textures::LoadImage(*data, GL_RGBA, texture, nullptr);
        return ret;
    }

    void Free(Tex *texture) {
        glDeleteTextures(1, &texture->id);
    }

    void Init(void) {
        unsigned char *data[3] = { nullptr, nullptr, nullptr };
        SceOff size[3] = { 0, 0, 0 };
        std::string filenames[3] = {
            "app0:res/file.png",
            "app0:res/folder.png",
            "app0:res/image.png"
        };

        for (int i = 0; i < 3; i++) {
            if (R_FAILED(FS::ReadFile(filenames[i], &data[i], &size[i])))
                break;
        }

        bool image_ret = Textures::LoadImagePNG(&data[0], &size[0], &file_texture);
        IM_ASSERT(image_ret);
        
        image_ret = Textures::LoadImagePNG(&data[1], &size[1], &folder_texture);
        IM_ASSERT(image_ret);
        
        image_ret = Textures::LoadImagePNG(&data[2], &size[2], &image_texture);
        IM_ASSERT(image_ret);

        for (int i = 0; i < 3; i++) {
            if (data[i])
                delete[] data[i];
        }
    }

    void Exit(void) {
        Textures::Free(&image_texture);
        Textures::Free(&folder_texture);
        Textures::Free(&file_texture);
    }
}
