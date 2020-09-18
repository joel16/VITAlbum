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
        }
        
        Windows::ExitWindow();
    }
}
