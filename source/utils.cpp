#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/system_param.h>
#include <psp2/kernel/clib.h>
#include <cstring>
#include <cstdio>

#include "log.h"
#include "utils.h"

namespace Utils {

    int InitAppUtil(void) {
        SceAppUtilInitParam init;
        SceAppUtilBootParam boot;
        sceClibMemset(&init, 0, sizeof(SceAppUtilInitParam));
        sceClibMemset(&boot, 0, sizeof(SceAppUtilBootParam));
        
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilInit(&init, &boot))) {
            Log::Error("sceAppUtilInit failed: 0x%lx\n", ret);
            return ret;
        }
        
        SceCommonDialogConfigParam param;
        sceCommonDialogConfigParamInit(&param);

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, reinterpret_cast<int *>(&param.language)))) {
            Log::Error("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG) failed: 0x%lx\n", ret);
            return ret;
        }

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, reinterpret_cast<int *>(&param.enterButtonAssign)))) {
            Log::Error("sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON) failed: 0x%lx\n", ret);
            return ret;
        }

        if (R_FAILED(ret = sceCommonDialogSetConfigParam(&param))) {
            Log::Error("sceCommonDialogSetConfigParam failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    int EndAppUtil(void) {
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilShutdown())) {
            Log::Error("sceAppUtilShutdown failed: 0x%lx\n", ret);
            return ret;
        }
        
        return 0;
    }

    void GetSizeString(char *string, double size) {
        int i = 0;
        const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
        
        while (size >= 1024.0f) {
            size /= 1024.0f;
            i++;
        }
        
        std::sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, size, units[i]);
    }
}
