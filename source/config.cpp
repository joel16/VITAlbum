#include <psp2/json.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include "config.h"
#include "fs.h"
#include "utils.h"

#define CONFIG_VERSION 2

config_t config;

namespace Config {
    static const char *config_file = "{\n\t\"config_ver\": %d,\n\t\"dev_options\": %s,\n\t\"image_filename\": %s,\n\t\"last_dir\": \"%s\",\n\t\"sort\": %d\n}";
    static int config_version_holder = 0;
    
    class Allocator : public sce::Json::MemAllocator {
        public:
            Allocator() {}
            
            virtual void *allocateMemory(size_t size, void *unk) override {
                return std::malloc(size);
            }
            
            virtual void freeMemory(void *ptr, void *unk) override {
                std::free(ptr);
            }
    };
    
    int Save(config_t config) {
        int ret = 0;
        char *buf = new char[1024];
        SceSize len = std::snprintf(buf, 1024, config_file, CONFIG_VERSION, config.dev_options? "true" : "false", 
            config.image_filename? "true" : "false", config.cwd.c_str(), config.sort);
        
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

        Allocator *alloc = new Allocator();

        sce::Json::InitParameter params;
        params.allocator = alloc;
        params.bufSize = static_cast<SceSize>(size);

        sce::Json::Initializer init = sce::Json::Initializer();
        init.initialize(&params);

        sce::Json::Value value = sce::Json::Value();
        sce::Json::Parser::parse(value, buffer, params.bufSize);

        // We know sceJson API loops through the child values in root alphabetically.
        config_version_holder = value.getValue(0).getInteger();
        config.dev_options = value.getValue(1).getBoolean();
        config.image_filename = value.getValue(2).getBoolean();
        config.cwd = value.getValue(3).getString().c_str();
        config.sort = value.getValue(4).getInteger();

        init.terminate();
        delete alloc;
        
        if (!FS::DirExists(config.cwd))
            config.cwd = "ux0:";
            
        // Delete config file if config file is updated. This will rarely happen.
        if (config_version_holder < CONFIG_VERSION) {
            sceIoRemove("ux0:data/VITAlbum/config.json");
            Config::SetDefault(&config);
            return Config::Save(config);
        }

        return 0;
    }
}
