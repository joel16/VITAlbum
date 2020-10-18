#ifndef _VITALBUM_POPUPS_H_
#define _VITALBUM_POPUPS_H_

#include "imgui.h"
#include "textures.h"

namespace Popups {
    inline void SetupPopup(const char *id) {
        ImGui::OpenPopup(id);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    };
    
    inline void ExitPopup(void) {
        ImGui::EndPopup();
        ImGui::PopStyleVar();
    };
    
    void ImageProperties(bool *state, MenuItem *item, Tex *texture);
}

#endif
