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
            for (SceOff i = 0; i < item->entry_count; i++) {
                if (SCE_S_ISDIR(item->entries[i].d_stat.st_mode))
                    ImGui::Image(reinterpret_cast<ImTextureID>(folder_texture.id), ImVec2(folder_texture.width, folder_texture.height));
                else if (FS::IsImageType(item->entries[i].d_name))
                    ImGui::Image(reinterpret_cast<ImTextureID>(image_texture.id), ImVec2(image_texture.width, image_texture.height));
                
                ImGui::SameLine();
                
                if (ImGui::Selectable(item->entries[i].d_name)) {
                    if (SCE_S_ISDIR(item->entries[i].d_stat.st_mode)) {
                        std::string filename = item->entries[i].d_name;
                        if (filename == "..")
                            item->entry_count = FS::ChangeDirPrev(&item->entries);
                        else
                            item->entry_count = FS::ChangeDirNext(filename, &item->entries);
                        
                        GImGui->NavId = 0;
                    }
                    else {
                        std::string path = FS::BuildPath(&item->entries[i]);
                        std::string ext = FS::GetFileExt(path);
                        
                        unsigned char *data = nullptr;
                        SceOff size = 0;
                        FS::ReadFile(path, &data, &size);

                        if (ext == ".GIF") {
                            bool image_ret = Textures::LoadImageGIF(&data, &size, item->textures);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_GIF_PREVIEW;
                        }
                        else {
                            bool image_ret = false;
                            
                            if (ext == ".BMP")
                                image_ret = Textures::LoadImageBMP(&data, &size, &item->texture);
                            else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA"))
                                image_ret = Textures::LoadImageFile(&data, &size, &item->texture);
                            else if (ext == ".ICO")
                                image_ret = Textures::LoadImageICO(&data, &size, &item->texture);
                            else if ((ext == ".JPG") || (ext == ".JPEG"))
                                image_ret = Textures::LoadImageJPEG(&data, &size, &item->texture);
                            else if (ext == ".PCX")
                                image_ret = Textures::LoadImagePCX(&data, &size, &item->texture);
                            else if (ext == ".PNG")
                                image_ret = Textures::LoadImagePNG(&data, &size, &item->texture);
                            else if (ext == ".SVG")
                                image_ret = Textures::LoadImageSVG(&data, &item->texture);
                            else if (ext == ".TIFF")
                                image_ret = Textures::LoadImageTIFF(path, &item->texture);
                            else if (ext == ".WEBP")
                                image_ret = Textures::LoadImageWEBP(&data, &size, &item->texture);

                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        
                        delete[] data;
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
