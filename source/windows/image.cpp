#include "imgui.h"
#include "windows.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Windows {
    void ImageWindow(MenuItem *item) {
        Windows::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin(item->entries[item->selected].d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            if ((item->texture.width <= 960) && (item->texture.height <= 544))
                ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(item->texture.width, item->texture.height)) * 0.5f);
            
            ImGui::Image(reinterpret_cast<ImTextureID>(item->texture.id), ImVec2(item->texture.width, item->texture.height));
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
