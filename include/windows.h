#ifndef _VITALBUM_WINDOWS_H_
#define _VITALBUM_WINDOWS_H_

#include "gui.h"
#include "imgui.h"
#include "textures.h"

namespace Windows {
    inline void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(960.0f, 544.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };
    
    inline void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };
    
    void FileBrowserWindow(MenuItem *item);
    void GifWindow(MenuItem *item);
    void ImageWindow(MenuItem *item);
    void SettingsWindow(void);
}

#endif
