#include <psp2/kernel/threadmgr.h>

#include "config.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "windows.h"

namespace Windows {
    static int delay = 0;
    static int current_frame = 0;
    
    void ImageViewer(WindowData &data) {
        Windows::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGuiWindowFlags_ filename_flag = !cfg.image_filename? ImGuiWindowFlags_NoTitleBar : ImGuiWindowFlags_None;
        
        if (ImGui::Begin(data.entries[data.selected].d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar | filename_flag)) {
            if (((data.texture.width * data.zoom_factor) <= 960.0f) && ((data.texture.height * data.zoom_factor) <= 544.0f))
                ImGui::SetCursorPos((ImGui::GetContentRegionAvail() - ImVec2((data.texture.width * data.zoom_factor),
                    (data.texture.height * data.zoom_factor))) * 0.5f);

            if (data.texture.frames) {
                ImGui::Image(reinterpret_cast<ImTextureID>(data.texture.frames[current_frame]), ImVec2(data.texture.width * data.zoom_factor, data.texture.height * data.zoom_factor));
                
                if (data.texture.anim) {
                    if (data.texture.anim->delays[current_frame]) {
                        delay = data.texture.anim->delays[current_frame];
                    }
                    else {
                        delay = 100;
                    }

                    sceKernelDelayThread(delay * 1000);
                }
                
                current_frame = (current_frame + 1) % data.texture.anim->count;
            }
            else {
                ImGui::Image(reinterpret_cast<ImTextureID>(data.texture.ptr), ImVec2(data.texture.width * data.zoom_factor, data.texture.height * data.zoom_factor));
            }
        }
        
        Windows::ExitWindow();
        ImGui::PopStyleVar();
    }
}
