#include <psp2/sysmodule.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "log.h"
#include "reader.h"
#include "textures.h"
#include "utils.h"

int _newlib_heap_size_user = 192 * 1024 * 1024;

namespace Services {
    int Init(void) {
        sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

        if (!FS::DirExists("ux0:data/VITAlbum")) {
            FS::MakeDir("ux0:data/VITAlbum");
        }

        Log::Init();
        Config::Load();
        GUI::Init();
        Textures::Init();
        Utils::InitAppUtil();
        Reader::Init();
        return 0;
    }
    
    void Exit(void) {
        // Clean up
        Reader::Exit();
        Utils::EndAppUtil();
        Textures::Exit();
        Log::Exit();
        
        sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
        GUI::Exit();
    }
}

int main(int argc, char *argv[]) {
    Services::Init();
    GUI::RenderLoop();
    Services::Exit();
    return 0;
}
