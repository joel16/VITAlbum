#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/system_param.h>
#include <cstring>
#include <cstdio>

#include "utils.h"

int SCE_CTRL_ENTER, SCE_CTRL_CANCEL;
unsigned int pressed;

namespace Utils {
    static SceCtrlData pad, old_pad;

    int InitAppUtil(void) {
        SceAppUtilInitParam init;
        SceAppUtilBootParam boot;
        std::memset(&init, 0, sizeof(SceAppUtilInitParam));
        std::memset(&boot, 0, sizeof(SceAppUtilBootParam));
        
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilInit(&init, &boot)))
            return ret;
        
        SceCommonDialogConfigParam param;
        sceCommonDialogConfigParamInit(&param);

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&param.language)))
            return ret;

        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&param.enterButtonAssign)))
            return ret;

        if (R_FAILED(ret = sceCommonDialogSetConfigParam(&param)))
            return ret;
        
        return 0;
    }

    int EndAppUtil(void) {
        int ret = 0;
        
        if (R_FAILED(ret = sceAppUtilShutdown()))
            return ret;
        
        return 0;
    }

    void ReadControls(void) {
        std::memset(&pad, 0, sizeof(SceCtrlData));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        pressed = pad.buttons & ~old_pad.buttons;
        old_pad = pad;
    }

    int GetEnterButton(void) {
        int button = 0, ret = 0;
        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button)))
            return ret;
        
        if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
            return SCE_CTRL_CIRCLE;
        else
            return SCE_CTRL_CROSS;
        
        return 0;
    }

    int GetCancelButton(void) {
        int button = 0, ret = 0;
        if (R_FAILED(ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &button)))
            return ret;
        
        if (button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
            return SCE_CTRL_CROSS;
        else
            return SCE_CTRL_CIRCLE;
        
        return 0;
    }

    void GetSizeString(char *string, double size) {
        int i = 0;
        const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
        
        while (size >= 1024.0f) {
            size /= 1024.0f;
            i++;
        }
        
        sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, size, units[i]);
    }
}
