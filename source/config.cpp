#include <jansson.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include "config.h"
#include "fs.h"
#include "utils.h"

#define CONFIG_VERSION 1

config_t config;

namespace Config {
    static const char *config_file = "{\n\t\"config_ver\": %d,\n\t\"sort\": %d,\n\t\"dev_options\": %d,\n\t\"image_filename\": %d,\n\t\"last_dir\": \"%s\"\n}";
    static int config_version_holder = 0;
    
    int Save(config_t config) {
        int ret = 0;
        char *buf = new char[1024];
        SceSize len = std::snprintf(buf, 1024, config_file, CONFIG_VERSION, config.sort, config.dev_options, config.image_filename, config.cwd.c_str());
        
        if (R_FAILED(ret = FS::WriteFile("ux0:data/VITAlbum/config.json", buf, len))) {
            delete[] buf;
            return ret;
        }
        
        delete[] buf;
        return 0;
    }
    
    static void SetDefault(config_t *config) {
        config->sort = 0;
        config->dev_options = false;
        config->image_filename = false;
        config->cwd = "ux0:";
    }

    int Load(void) {
        int ret = 0;

        if (!FS::DirExists("ux0:data"))
            sceIoMkdir("ux0:data", 0777);
        if (!FS::DirExists("ux0:data/VITAlbum"))
            sceIoMkdir("ux0:data/VITAlbum", 0777);
            
        if (!FS::FileExists("ux0:data/VITAlbum/config.json")) {
            Config::SetDefault(&config);
            return Config::Save(config);
        }
            
        SceUID file = 0;
        if (R_FAILED(ret = file = sceIoOpen("ux0:data/VITAlbum/config.json", SCE_O_RDONLY, 0)))
            return ret;
            
        SceOff size = sceIoLseek(file, 0, SEEK_END);
        char *buffer =  new char[size + 1];
        
        if (R_FAILED(ret = sceIoPread(file, buffer, size + 1, SCE_SEEK_SET))) {
            delete[] buffer;
            sceIoClose(file);
            return ret;
        }
        
        if (R_FAILED(ret = sceIoClose(file))) {
            delete[] buffer;
            return ret;
        }
            
        json_t *root;
        json_error_t error;
        root = json_loads(buffer, 0, &error);
        delete[] buffer;
        
        if (!root)
            return -1;
            
        json_t *config_ver = json_object_get(root, "config_ver");
        config_version_holder = json_integer_value(config_ver);
        
        json_t *sort = json_object_get(root, "sort");
        config.sort = json_integer_value(sort);
        
        json_t *dev_options = json_object_get(root, "dev_options");
        config.dev_options = json_integer_value(dev_options);
        
        json_t *image_filename = json_object_get(root, "image_filename");
        config.image_filename = json_integer_value(image_filename);
        
        json_t *last_dir = json_object_get(root, "last_dir");
        config.cwd = json_string_value(last_dir);
        
        if (!FS::DirExists(config.cwd))
            config.cwd = "ux0:";
            
        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder < CONFIG_VERSION) {
            sceIoRemove("ux0:data/VITAlbum/config.json");
            Config::SetDefault(&config);
            return Config::Save(config);
        }
        
        json_decref(root);
        return 0;
    }
}
