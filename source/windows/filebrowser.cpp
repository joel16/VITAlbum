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
                        std::string ext = FS::GetFileExt(path);

                        bool image_ret = false;
                        item->textures.resize(1); // Resize to 1 initially. If the file is a GIF it will be resized accordingly.

                        // Because TIFF does not load via buffer, but directly from the path.
                        if (ext == ".TIFF")
                            image_ret = Textures::LoadImageTIFF(path, &item->textures[0]);
                        else {
                            unsigned char *data = nullptr;
                            SceOff size = 0;
                            FS::ReadFile(path, &data, &size);
                            
                            if (ext == ".BMP")
                                image_ret = Textures::LoadImageBMP(&data, &size, &item->textures[0]);
                            else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA"))
                                image_ret = Textures::LoadImageFile(&data, &size, &item->textures[0]);
                            else if (ext == ".GIF")
                                image_ret = Textures::LoadImageGIF(&data, &size, item->textures);
                            else if (ext == ".ICO")
                                image_ret = Textures::LoadImageICO(&data, &size, &item->textures[0]);
                            else if ((ext == ".JPG") || (ext == ".JPEG"))
                                image_ret = Textures::LoadImageJPEG(&data, &size, &item->textures[0]);
                            else if (ext == ".PCX")
                                image_ret = Textures::LoadImagePCX(&data, &size, &item->textures[0]);
                            else if (ext == ".PNG")
                                image_ret = Textures::LoadImagePNG(&data, &size, &item->textures[0]);
                            else if (ext == ".SVG")
                                image_ret = Textures::LoadImageSVG(&data, &item->textures[0]);
                            else if (ext == ".WEBP")
                                image_ret = Textures::LoadImageWEBP(&data, &size, &item->textures[0]);
                            
                            delete[] data;
                        }

                        IM_ASSERT(image_ret);
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
