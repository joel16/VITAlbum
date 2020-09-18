#include "config.h"
#include "gui.h"
#include "keyboard.h"
#include "popups.h"
#include "utils.h"

namespace Popups {
    void ImageProperties(MenuItem *item, Tex *texture) {
        ImGui::OpenPopup("Properties");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        std::string new_width, new_height;
        if (ImGui::BeginPopupModal("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string parent_text = "Parent: ";
            parent_text.append(config.cwd);
            ImGui::Text(parent_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string name_text = "Name: ";
            name_text.append(item->entries[item->selected].d_name);
            ImGui::Text(name_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            if (!SCE_S_ISDIR(item->entries[item->selected].d_stat.st_mode)) {
                char size[16];
                Utils::GetSizeString(size, item->entries[item->selected].d_stat.st_size);
                std::string size_text = "Size: ";
                size_text.append(size);
                ImGui::Text(size_text.c_str());
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string width_text = "Width: ";
            width_text.append(std::to_string(texture->width));
            width_text.append("px");
            ImGui::Text(width_text.c_str());
            ImGui::SameLine(0.0f, 10.0f);
            if (ImGui::Button("Edit width"))
                new_width = Keyboard::GetText("Enter width", std::to_string(texture->width));

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string height_text = "Height: ";
            height_text.append(std::to_string(texture->height));
            height_text.append("px");
            ImGui::Text(height_text.c_str());
            ImGui::SameLine(0.0f, 10.0f);
            if (ImGui::Button("Edit height"))
                new_height = Keyboard::GetText("Enter height", std::to_string(texture->height));

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            /*if (ImGui::Button("Apply changes", ImVec2(120, 0))) {
                // int new_width_int = std::stoi(new_width);
                // int new_height_int = std::stoi(new_height);
                if ((!new_height.empty()) || (!new_width.empty())) {
                    if (new_width != std::to_string(texture->width))
                    else if (new_height != std::to_string(texture->height))
                    
                }
            }*/

            ImGui::SameLine();
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                item->image_properties = false;
            }
            
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
    }
}
