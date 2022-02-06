#include <cstring>
#include <psp2/kernel/clib.h>
#include <memory>

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

// SVG
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// TIFF
#include "tiffio.h"

// WEBP
#include <webp/decode.h>
#include <webp/demux.h>

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
        return static_cast<unsigned char*>(bitmap);
    }
    
    static size_t bitmap_get_bpp([[maybe_unused]] void *bitmap) {
        return BYTES_PER_PIXEL;
    }
    
    static void bitmap_destroy(void *bitmap) {
        assert(bitmap);
        std::free(bitmap);
    }
}

namespace ICO {
    static void *bitmap_create(int width, int height, [[maybe_unused]] unsigned int state) {
        return std::calloc(width * height, BYTES_PER_PIXEL);
    }
    
    static unsigned char *bitmap_get_buffer(void *bitmap) {
        assert(bitmap);
        return static_cast<unsigned char*>(bitmap);
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
    static bool Create(unsigned char *data, GLint format, Tex &texture) {    
        // Create a OpenGL texture identifier
        glGenTextures(1, &texture.id);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        
        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, data);
        return true;
    }

    static bool LoadImageOther(unsigned char **data, SceOff &size, Tex &texture) {
        stbi_uc *image = nullptr;
        image = stbi_load_from_memory(*data, size, &texture.width, &texture.height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::Create(static_cast<unsigned char*>(image), GL_RGBA, texture);
        stbi_image_free(image);
        return ret;
    }

    static bool LoadImageBMP(unsigned char **data, SceOff &size, Tex &texture) {
        bmp_bitmap_callback_vt bitmap_callbacks = {
            BMP::bitmap_create,
            BMP::bitmap_destroy,
            BMP::bitmap_get_buffer,
            BMP::bitmap_get_bpp
        };
        
        bmp_result code = BMP_OK;
        bmp_image bmp;
        bmp_create(&bmp, &bitmap_callbacks);
            
        code = bmp_analyse(&bmp, size, *data);
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

        texture.width = bmp.width;
        texture.height = bmp.height;
        bool ret = Textures::Create(static_cast<unsigned char*>(bmp.bitmap), GL_RGBA, texture);
        bmp_finalise(&bmp);
        return ret;
    }

    static bool LoadImageGIF(const std::string &path, std::vector<Tex> &textures) {
        bool ret = false;
        int error = 0;
        GifFileType *gif = DGifOpenFileName(path.c_str(), &error);

        if (!gif) {
            Log::Error("DGifOpenFileName failed: %d\n", error);
            return ret;
        }

        if (DGifSlurp(gif) != GIF_OK) {
            Log::Error("DGifSlurp failed: %d\n", gif->Error);
            return ret;
        }

        if (gif->ImageCount <= 0) {
            Log::Error("Gif does not contain any images.\n");
            return ret;
        }
        
        textures.resize(gif->ImageCount);
        
        // seiken's example code from:
        // https://forums.somethingawful.com/showthread.php?threadid=2773485&userid=0&perpage=40&pagenumber=487#post465199820
        int width = gif->SWidth;
        int height = gif->SHeight;
        std::unique_ptr<SceUInt32[]> pixels(new SceUInt32[width * height]);
        
        for (int i = 0; i < width * height; ++i)
            pixels[i] = gif->SBackGroundColor;
            
        for (int i = 0; i < gif->ImageCount; ++i) {
            const SavedImage &frame = gif->SavedImages[i];
            bool transparency = false;
            unsigned char transparency_byte = 0;
            
            // Delay time in hundredths of a second.
            int delay_time = 1;
            for (int j = 0; j < frame.ExtensionBlockCount; ++j) {
                const ExtensionBlock &block = frame.ExtensionBlocks[j];
                
                if (block.Function != GRAPHICS_EXT_FUNC_CODE)
                    continue;
                    
                // Here's the metadata for this frame.
                char dispose = (block.Bytes[0] >> 2) & 7;
                transparency = block.Bytes[0] & 1;
                delay_time = block.Bytes[1] + (block.Bytes[2] << 8);
                transparency_byte = block.Bytes[3];
                
                if (dispose == 2) {
                    // Clear the canvas.
                    for (int k = 0; k < width * height; ++k)
                        pixels[k] = gif->SBackGroundColor;
                }
            }
            
            // Colour map for this frame.
            ColorMapObject *map = frame.ImageDesc.ColorMap ? frame.ImageDesc.ColorMap : gif->SColorMap;
            
            // Region this frame draws to.
            int fw = frame.ImageDesc.Width;
            int fh = frame.ImageDesc.Height;
            int fl = frame.ImageDesc.Left;
            int ft = frame.ImageDesc.Top;
            
            for (int y = 0; y < std::min(height, fh); ++y) {
                for (int x = 0; x < std::min(width, fw); ++x) {
                    unsigned char byte = frame.RasterBits[x + y * fw];

                    // Transparent pixel.
                    if (transparency && byte == transparency_byte)
                        continue;
                        
                    // Draw to canvas.
                    const GifColorType &c = map->Colors[byte];
                    pixels[fl + x + (ft + y) * width] = c.Red | (c.Green << 8) | (c.Blue << 16) | (0xff << 24);
                }
            }
            
            textures[i].width = width;
            textures[i].height = height;
            textures[i].delay = delay_time * 10000;
            
            // Here's the actual frame, pixels.get() is now a pointer to the 32-bit RGBA
            // data for this frame you might expect.
            ret = Textures::Create(reinterpret_cast<unsigned char*>(pixels.get()), GL_RGBA, textures[i]);
        }
        
        if (DGifCloseFile(gif, &error) != GIF_OK) {
            Log::Error("DGifCloseFile failed: %d\n", error);
            return false;
        }

        return true;
    }

    static bool LoadImageICO(unsigned char **data, SceOff &size, Tex &texture) {
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

        code = ico_analyse(&ico, size, *data);
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

        texture.width = bmp->width;
        texture.height = bmp->height;
        bool ret = Textures::Create(static_cast<unsigned char*>(bmp->bitmap), GL_RGBA, texture);
        ico_finalise(&ico);
        return ret;
    }

    static bool LoadImageJPEG(unsigned char **data, SceOff &size, Tex &texture) {
        tjhandle jpeg = tjInitDecompress();
        int jpegsubsamp = 0;

        if (R_FAILED(tjDecompressHeader2(jpeg, *data, size, &texture.width, &texture.height, &jpegsubsamp))) {
            Log::Error("tjDecompressHeader2 failed: %s\n", tjGetErrorStr());
            delete[] data;
            return false;
        }
        
        std::unique_ptr<unsigned char[]> buffer(new unsigned char[texture.width * texture.height * 3]);
        if (R_FAILED(tjDecompress2(jpeg, *data, size, buffer.get(), texture.width, 0, texture.height, TJPF_RGB, TJFLAG_FASTDCT))) {
            Log::Error("tjDecompress2 failed: %s\n", tjGetErrorStr());
            return false;
        }
        
        bool ret = Textures::Create(buffer.get(), GL_RGB, texture);
        tjDestroy(jpeg);
        return ret;
    }

    static bool LoadImagePCX(unsigned char **data, SceOff &size, Tex &texture) {
        *data = drpcx_load_memory(*data, size, DRPCX_FALSE, &texture.width, &texture.height, nullptr, BYTES_PER_PIXEL);
        bool ret = Textures::Create(*data, GL_RGBA, texture);
        return ret;
    }

    static bool LoadImagePNG(unsigned char **data, SceOff &size, Tex &texture) {
        bool ret = false;
        png_image image;
        sceClibMemset(&image, 0, (sizeof image));
        image.version = PNG_IMAGE_VERSION;
        
        if (png_image_begin_read_from_memory(&image, *data, size) != 0) {
            image.format = PNG_FORMAT_RGBA;
            std::unique_ptr<png_byte[]> buffer(new png_byte[PNG_IMAGE_SIZE(image)]);
            
            if (png_image_finish_read(&image, nullptr, buffer.get(), 0, nullptr) != 0) {
                texture.width = image.width;
                texture.height = image.height;
                ret = Textures::Create(buffer.get(), GL_RGBA, texture);
                png_image_free(&image);
            }
            else {
                Log::Error("png_image_finish_read failed: %s\n", image.message);
                png_image_free(&image);
            }
        }
        else
            Log::Error("png_image_begin_read_from_memory failed: %s\n", image.message);
        
        return ret;
    }

    static bool LoadImageSVG(unsigned char **data, Tex &texture) {
        NSVGimage *svg;
        svg = nsvgParse(reinterpret_cast<char *>(*data), "px", 96);
        
        texture.width = svg->width;
        texture.height = svg->height;
        
        NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
        std::unique_ptr<unsigned char[]> buffer(new unsigned char[texture.width * texture.height * BYTES_PER_PIXEL]);
        nsvgRasterize(rasterizer, svg, 0, 0, 1, buffer.get(), texture.width, texture.height, texture.width * BYTES_PER_PIXEL);
        
        bool ret = Textures::Create(buffer.get(), GL_RGBA, texture);

        nsvgDelete(svg);
        nsvgDeleteRasterizer(rasterizer);
        return ret;
    }

    static bool LoadImageTIFF(const std::string &path, Tex &texture) {
        TIFF *tif = TIFFOpen(path.c_str(), "r");
        if (tif) {
            size_t num_pixels = 0;
            uint32 *raster = nullptr;
            
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &texture.width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &texture.height);
            num_pixels = texture.width * texture.height;

            raster = static_cast<uint32 *>(_TIFFmalloc(num_pixels * sizeof(uint32)));
            if (raster != nullptr) {
                if (TIFFReadRGBAImageOriented(tif, texture.width, texture.height, raster, ORIENTATION_TOPLEFT)) {
                    Textures::Create(reinterpret_cast<unsigned char*>(raster), GL_RGBA, texture);
                    _TIFFfree(raster);
                }
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

    static bool LoadImageWEBP(unsigned char **data, SceOff &size, std::vector<Tex> &textures) {
        bool ret = false;
        VP8StatusCode status = VP8_STATUS_OK;
        WebPBitstreamFeatures features = {0};
        
        status = WebPGetFeatures(*data, size, &features);
        if (status != VP8_STATUS_OK)
            return ret;

        if (features.has_animation) {
            int frame_index = 0, prev_timestamp = 0;
            WebPData webp_data = {0};
            WebPAnimDecoder *dec = {0};
            WebPAnimInfo info = {0};

            WebPDataInit(&webp_data);
            webp_data.bytes = *data;
            webp_data.size = size;

            dec = WebPAnimDecoderNew(&webp_data, nullptr);
            if (dec == nullptr)
                return ret;

            if (!WebPAnimDecoderGetInfo(dec, &info)) {
                WebPAnimDecoderDelete(dec);
                return ret;
            }

            textures.resize(info.frame_count);

            while (WebPAnimDecoderHasMoreFrames(dec)) {
                unsigned char *frame_rgba = nullptr;
                int timestamp = 0;

                if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &timestamp)) {
                    WebPAnimDecoderDelete(dec);
                    return ret;
                }
                
                textures[frame_index].width = info.canvas_width;
                textures[frame_index].height = info.canvas_height;
                textures[frame_index].delay = (timestamp - prev_timestamp) * 1000;
                ret = Textures::Create(frame_rgba, GL_RGBA, textures[frame_index]);
                ++frame_index;
                prev_timestamp = timestamp;
            }
        }
        else {
            *data = WebPDecodeRGBA(*data, size, &textures[0].width, &textures[0].height);
            ret = Textures::Create(*data, GL_RGBA, textures[0]);
        }

        return ret;
    }

    void Free(Tex &texture) {
        glDeleteTextures(1, &texture.id);
    }

    bool LoadImageFile(const std::string &path, std::vector<Tex> &textures) {
        bool ret = false;
        std::string ext = FS::GetFileExt(path);
        
        // Resize to 1 initially. If the file is a GIF it will be resized accordingly.
        textures.resize(1);
        
        // Because TIFF does not load via buffer, but directly from the path.
        if (ext == ".TIFF")
            ret = Textures::LoadImageTIFF(path, textures[0]);
        else if (ext == ".GIF")
            ret = Textures::LoadImageGIF(path, textures);
        else {
            unsigned char *data = nullptr;
            SceOff size = 0;
            FS::ReadFile(path, &data, size);
            
            if (ext == ".BMP")
                ret = Textures::LoadImageBMP(&data, size, textures[0]);
            else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA"))
                ret = Textures::LoadImageOther(&data, size, textures[0]);
            else if (ext == ".ICO")
                ret = Textures::LoadImageICO(&data, size, textures[0]);
            else if ((ext == ".JPG") || (ext == ".JPEG"))
                ret = Textures::LoadImageJPEG(&data, size, textures[0]);
            else if (ext == ".PCX")
                ret = Textures::LoadImagePCX(&data, size, textures[0]);
            else if (ext == ".PNG")
                ret = Textures::LoadImagePNG(&data, size, textures[0]);
            else if (ext == ".SVG")
                ret = Textures::LoadImageSVG(&data, textures[0]);
            else if (ext == ".WEBP")
                ret = Textures::LoadImageWEBP(&data, size, textures);
            
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
            if (R_FAILED(FS::ReadFile(filenames[i], &data[i], size[i])))
                break;
        }

        icons.resize(num_icons);
        bool ret = Textures::LoadImagePNG(&data[0], size[0], icons[FOLDER]);
        IM_ASSERT(ret);
        
        ret = Textures::LoadImagePNG(&data[1], size[1], icons[IMAGE]);
        IM_ASSERT(ret);

        for (int i = 0; i < num_icons; i++) {
            if (data[i])
                delete[] data[i];
        }
    }

    void Exit(void) {
        for (unsigned int i = 0; i < icons.size(); i++)
            Textures::Free(icons[i]);
    }
}
