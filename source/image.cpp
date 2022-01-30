#include <psp2/kernel/threadmgr.h>

#include "config.h"
#include "imgui.h"
#include "windows.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Windows {
    void ImageViewer(WindowData &data) {
        Windows::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags_ filename_flag = !cfg.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None;
        
        if (ImGui::Begin(data.entries[data.selected].d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar | filename_flag)) {
            if (((data.textures[0].width * data.zoom_factor) <= 960.0f) && ((data.textures[0].height * data.zoom_factor) <= 544.0f))
                ImGui::SetCursorPos((ImGui::GetContentRegionAvail() - ImVec2((data.textures[data.frame_count].width * data.zoom_factor),
                    (data.textures[data.frame_count].height * data.zoom_factor))) * 0.5f);

            if (data.textures.size() > 1) {
                sceKernelDelayThread(data.textures[data.frame_count].delay);

                ImGui::Image(reinterpret_cast<ImTextureID>(data.textures[data.frame_count].id), ImVec2(data.textures[data.frame_count].width * data.zoom_factor,
                    data.textures[data.frame_count].height * data.zoom_factor));
                
                data.frame_count++;
                
                // Reset frame counter
                if (data.frame_count == data.textures.size() - 1)
                    data.frame_count = 0;
            }
            else
                ImGui::Image(reinterpret_cast<ImTextureID>(data.textures[0].id), ImVec2(data.textures[0].width * data.zoom_factor, data.textures[0].height * data.zoom_factor));
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
