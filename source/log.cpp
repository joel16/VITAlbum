#include <psp2/io/fcntl.h>
#include <cstdarg>
#include <cstdio>

#include "fs.h"
#include "config.h"
#include "utils.h"

namespace Log {
    static SceUID log_file = 0;
    static SceOff offset = 0;

    void Init(void) {
        if (!config.dev_options)
            return;
        
        if (!FS::FileExists("ux0:data/VITAlbum/debug.log"))
            FS::CreateFile("ux0:data/VITAlbum/debug.log");
            
        if (R_FAILED(log_file = sceIoOpen("ux0:data/VITAlbum/debug.log", SCE_O_RDWR | SCE_O_APPEND, 0)))
            return;
            
        SceOff size = sceIoLseek(log_file, 0, SEEK_END);
        unsigned char *buffer = new unsigned char[size];
        
        int bytes_read = 0;
        if (R_FAILED(bytes_read = sceIoPread(log_file, buffer, size, SCE_SEEK_SET))) {
            delete[] buffer;
            return;
        }
        
        delete[] buffer;
        offset += bytes_read;
    }

    void Exit(void) {
        if (!config.dev_options)
            return;
            
        if (R_FAILED(sceIoClose(log_file)))
            return;
    }
    
    void Error(const char *data, ...) {
        if (!config.dev_options)
            return;
        
        char buf[512];
        va_list args;
        va_start(args, data);
        std::vsnprintf(buf, sizeof(buf), data, args);
        va_end(args);
        
        std::string error_string = "[ERROR] ";
        error_string.append(buf);
        
        std::printf("%s", error_string.c_str());
        if (R_FAILED(sceIoPwrite(log_file, error_string.data(), error_string.length(), offset)))
            return;
            
        offset += error_string.length();
    }
}
