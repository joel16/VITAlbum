#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "windows.h"

namespace Windows {
    static bool focus = false;

    void FileBrowserWindow(MenuItem *item) {
        Windows::SetupWindow();
        
        if (ImGui::Begin("VITAlbum", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), config.cwd.c_str());

            if (!focus) {
                ImGui::SetNextWindowFocus();
                focus = true;
            }
            
            ImGui::BeginChild("##FS::GetDirList");
            for (unsigned int i = 0; i < item->entries.size(); i++) {
                if (SCE_S_ISDIR(item->entries[i].d_stat.st_mode))
                    ImGui::Image(reinterpret_cast<ImTextureID>(icons[FOLDER].id), ImVec2(icons[FOLDER].width, icons[FOLDER].height));
                else if (FS::IsImageType(item->entries[i].d_name))
                    ImGui::Image(reinterpret_cast<ImTextureID>(icons[IMAGE].id), ImVec2(icons[IMAGE].width, icons[IMAGE].height));
                
                ImGui::SameLine();
                
                if (ImGui::Selectable(item->entries[i].d_name)) {
                    if (SCE_S_ISDIR(item->entries[i].d_stat.st_mode)) {
                        std::string filename = item->entries[i].d_name;
                        if (filename == "..")
                            FS::ChangeDirPrev(item->entries);
                        else
                            FS::ChangeDirNext(filename, item->entries);
                        
                        GImGui->NavId = 0;
                    }
                    else {
                        std::string path = FS::BuildPath(&item->entries[i]);
                        bool ret = Textures::LoadImageFile(path, item->textures);
                        IM_ASSERT(ret);
                        item->state = GUI_STATE_IMAGE_PREVIEW;
                    }
                }
                
                if (ImGui::IsItemHovered())
                    item->selected = i;
            }
            
            ImGui::EndChild();
        }
        
        Windows::ExitWindow();
    }
}
