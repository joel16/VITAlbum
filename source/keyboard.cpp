#include <psp2/ime_dialog.h>

#include "keyboard.h"
#include "utils.h"

namespace Keyboard {
    static uint16_t text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

    int GetText(const std::u16string &title) {
        int ret = 0;
        SceImeDialogParam params;

        sceImeDialogParamInit(&params);

        params.type = SCE_IME_TYPE_DEFAULT;
        params.title = (const SceWChar16 *)title.c_str();

        memset(text, 0, sizeof(text));

        params.initialText = text;
        params.inputTextBuffer = text;
        params.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
        
        if (R_FAILED(ret = sceImeDialogInit(&params)))
            return ret;
        
        return 0;
    }
}
