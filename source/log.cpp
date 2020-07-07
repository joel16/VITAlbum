#include <psp2/io/fcntl.h>
#include <cstdarg>
#include <cstdio>

#include "utils.h"

namespace Log {
    SceUID log_handle = 0;

    int OpenHande(void) {
        int ret = 0;
        
        if (R_FAILED(ret = log_handle = sceIoOpen("ux0:/vpk/debug.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777)))
            return ret;
        
        return 0;
    }

    int CloseHandle(void) {
        int ret = 0;
        
        if (R_FAILED(ret = sceIoClose(log_handle)))
            return ret;
        
        return 0;
    }

    int Debug(const char *format, ...) {
        va_list list;
        char string[1024] = {0};
        
        va_start(list, format);
        int length = std::vsprintf(string, format, list);
        va_end(list);
        
        int ret = 0;
        if (R_FAILED(ret = sceIoWrite(log_handle, string, length)))
            return ret;
        
        return 0;
    }
}
