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

// SVG
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

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

std::vector<Tex> icons;
unsigned const FOLDER = 0, IMAGE = 1;

namespace BMP {
    static void *bitmap_create(int width, int height, [[maybe_unused]] unsigned int state) {
        /* ensure a stupidly large (>50Megs or so) bitmap is not created */
        if ((static_cast<long long>(width) * static_cast<long long>(height)) > (MAX_IMAGE_BYTES/BYTES_PER_PIXEL))
            return nullptr;
        
        return std::calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char *>(bitmap);
    }
    
    static size_t bitmap_get_bpp([[maybe_unused]] void *bitmap) {
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

    static void bitmap_set_opaque([[maybe_unused]] void *bitmap, [[maybe_unused]] bool opaque) {
        assert(bitmap);
    }
    
    static bool bitmap_test_opaque([[maybe_unused]] void *bitmap) {
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
    
    static void bitmap_modified([[maybe_unused]] void *bitmap) {
        assert(bitmap);
        return;
    }
}

namespace ICO {
    static void *bitmap_create(int width, int height, [[maybe_unused]] unsigned int state) {
        return std::calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char *>(bitmap);
    }
    
    static size_t bitmap_get_bpp([[maybe_unused]] void *bitmap) {
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

    static bool LoadImageOther(unsigned char **data, SceOff *size, Tex *texture) {
        unsigned char *image = nullptr;
        image = stbi_load_from_memory(*data, *size, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::LoadImage(image, GL_RGBA, texture, stbi_image_free);
        return ret;
    }

    static bool LoadImageBMP(unsigned char **data, SceOff *size, Tex *texture) {
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

    static bool LoadImageGIF(unsigned char **data, SceOff *size, std::vector<Tex> &textures) {
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

    static bool LoadImageICO(unsigned char **data, SceOff *size, Tex *texture) {
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

    static bool LoadImageJPEG(unsigned char **data, SceOff *size, Tex *texture) {
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

    static bool LoadImagePCX(unsigned char **data, SceOff *size, Tex *texture) {
        *data = drpcx_load_memory(*data, *size, DRPCX_FALSE, &texture->width, &texture->height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::LoadImage(*data, GL_RGBA, texture, nullptr);
        return ret;
    }

    static bool LoadImagePNG(unsigned char **data, SceOff *size, Tex *texture) {
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

    static bool LoadImageSVG(unsigned char **data, Tex *texture) {
        NSVGimage *svg;
        svg = nsvgParse(reinterpret_cast<char *>(*data), "px", 96);
        
        texture->width = svg->width;
        texture->height = svg->height;
        
        NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
        unsigned char *image = new unsigned char[texture->width * texture->height * 4];
        nsvgRasterize(rasterizer, svg, 0, 0, 1, image, texture->width, texture->height, texture->width * 4);
        
        bool ret = Textures::LoadImage(image, GL_RGBA, texture, nullptr);
        
        delete[] image;
        nsvgDelete(svg);
        nsvgDeleteRasterizer(rasterizer);
        return ret;
    }

    static bool LoadImageTIFF(const std::string &path, Tex *texture) {
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

    static bool LoadImageWEBP(unsigned char **data, SceOff *size, Tex *texture) {
        *data = WebPDecodeRGBA(*data, *size, &texture->width, &texture->height);
        bool ret = Textures::LoadImage(*data, GL_RGBA, texture, nullptr);
        return ret;
    }

    void Free(Tex *texture) {
        glDeleteTextures(1, &texture->id);
    }

    bool LoadImageFile(const std::string &path, std::vector<Tex> &textures) {
        bool ret = false;
        std::string ext = FS::GetFileExt(path);
        
        // Resize to 1 initially. If the file is a GIF it will be resized accordingly.
        textures.resize(1);
        
        // Because TIFF does not load via buffer, but directly from the path.
        if (ext == ".TIFF")
            ret = Textures::LoadImageTIFF(path, &textures[0]);
        else {
            unsigned char *data = nullptr;
            SceOff size = 0;
            FS::ReadFile(path, &data, &size);
            
            if (ext == ".BMP")
                ret = Textures::LoadImageBMP(&data, &size, &textures[0]);
            else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA"))
                ret = Textures::LoadImageOther(&data, &size, &textures[0]);
            else if (ext == ".GIF")
                ret = Textures::LoadImageGIF(&data, &size, textures);
            else if (ext == ".ICO")
                ret = Textures::LoadImageICO(&data, &size, &textures[0]);
            else if ((ext == ".JPG") || (ext == ".JPEG"))
                ret = Textures::LoadImageJPEG(&data, &size, &textures[0]);
            else if (ext == ".PCX")
                ret = Textures::LoadImagePCX(&data, &size, &textures[0]);
            else if (ext == ".PNG")
                ret = Textures::LoadImagePNG(&data, &size, &textures[0]);
            else if (ext == ".SVG")
                ret = Textures::LoadImageSVG(&data, &textures[0]);
            else if (ext == ".WEBP")
                ret = Textures::LoadImageWEBP(&data, &size, &textures[0]);
            
            delete[] data;
        }
        
        return ret;
    }
    
    void Init(void) {
        const int num_icons = 2;
        unsigned char *data[num_icons] = { nullptr, nullptr };
        SceOff size[num_icons] = { 0, 0 };
        std::string filenames[num_icons] = {
            "app0:res/folder.png",
            "app0:res/image.png"
        };

        for (int i = 0; i < num_icons; i++) {
            if (R_FAILED(FS::ReadFile(filenames[i], &data[i], &size[i])))
                break;
        }

        icons.resize(num_icons);
        bool ret = Textures::LoadImagePNG(&data[0], &size[0], &icons[FOLDER]);
        IM_ASSERT(ret);
        
        ret = Textures::LoadImagePNG(&data[1], &size[1], &icons[IMAGE]);
        IM_ASSERT(ret);

        for (int i = 0; i < num_icons; i++) {
            if (data[i])
                delete[] data[i];
        }
    }

    void Exit(void) {
        for (unsigned int i = 0; i < icons.size(); i++)
            Textures::Free(&icons[i]);
    }
}
