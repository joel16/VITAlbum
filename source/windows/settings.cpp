#include <png.h>
#include <webp/decode.h>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "windows.h"

namespace Windows {
    static void Separator(void) {
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
    }

    void SettingsWindow(MenuItem *item) {
        Windows::SetupWindow();
        
        if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::TreeNode("Sort Settings")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::RadioButton(" By name (ascending)", &config.sort, 0);
                ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
                ImGui::RadioButton(" By name (descending)", &config.sort, 1);
                ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
                ImGui::RadioButton(" By size (largest first)", &config.sort, 2);
                ImGui::Dummy(ImVec2(0.0f, 15.0f)); // Spacing
                ImGui::RadioButton(" By size (smallest first)", &config.sort, 3);
                ImGui::TreePop();
            }
            
            Windows::Separator();
            
            if (ImGui::TreeNode("Image/Gif Viewer")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Checkbox(" Display filename", &config.image_filename);
                ImGui::TreePop();
            }
            
            Windows::Separator();
            
            if (ImGui::TreeNode("Developer Options")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Checkbox(" Enable logs", &config.dev_options);
                ImGui::TreePop();
            }

            Windows::Separator();
            
            if (ImGui::TreeNode("About")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                std::string vitalbum_ver = APP_VERSION;
                vitalbum_ver.erase(0, std::min(vitalbum_ver.find_first_not_of('0'), vitalbum_ver.size() - 1));
                ImGui::Text("VITAlbum version: %s", vitalbum_ver.c_str());
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("Author: Joel16");
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("Dear imGui version: %s", ImGui::GetVersion());
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("libpng version: %s", PNG_LIBPNG_VER_STRING);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("libwebp version: %d.%d.%d", (WebPGetDecoderVersion() >> 16) & 0xFF, (WebPGetDecoderVersion() >> 8) & 0xFF, WebPGetDecoderVersion() & 0xFF);
                ImGui::TreePop();
            }
        }
        
        Windows::ExitWindow();
    }
}
