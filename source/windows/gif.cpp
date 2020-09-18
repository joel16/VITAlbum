#include "imgui.h"
#include "windows.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Windows {
    void GifWindow(MenuItem *item) {
        Windows::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        
        if (ImGui::Begin(item->entries[item->selected].d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            if ((item->textures[0].width <= 960) && (item->textures[0].height <= 544))
                ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(item->textures[0].width, item->textures[0].height)) * 0.5f);
                
            sceKernelDelayThread(item->textures[item->frame_count].delay * 10000);
            ImGui::Image(reinterpret_cast<ImTextureID>(item->textures[item->frame_count].id), ImVec2(item->textures[item->frame_count].width, item->textures[item->frame_count].height));
            item->frame_count++;
            
            // Reset frame counter
            if (item->frame_count == item->textures.size() - 1)
                item->frame_count = 0;
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
