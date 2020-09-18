#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "windows.h"

namespace Windows {
    void FileBrowserWindow(MenuItem *item) {
        Windows::SetupWindow();
        
        if (ImGui::Begin("VITAlbum", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), config.cwd.c_str());
            
            ImGui::BeginChild("##FS::GetDirList");
            for (SceOff i = 0; i < item->entry_count; i++) {
                if (SCE_S_ISDIR(item->entries[i].d_stat.st_mode))
                    ImGui::Image(reinterpret_cast<ImTextureID>(folder_texture.id), ImVec2(folder_texture.width, folder_texture.height));
                else {
                    std::string filename = item->entries[i].d_name;
                    std::string ext = FS::GetFileExt(filename);
                    
                    if ((ext == ".BMP") || (ext == ".GIF") || (ext == ".ICO") || (ext == ".JPG") || (ext == ".JPEG") || (ext == ".PCX")
                        || (ext == ".PNG") || (ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA") || (ext == ".TIFF")
                        || (ext == ".WEBP"))
                        ImGui::Image(reinterpret_cast<ImTextureID>(image_texture.id), ImVec2(image_texture.width, image_texture.height));
                    else
                        ImGui::Image(reinterpret_cast<ImTextureID>(file_texture.id), ImVec2(file_texture.width, file_texture.height));
                }
                
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
                        
                        if (ext == ".BMP") {
                            bool image_ret = Textures::LoadImageBMP(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA")) {
                            bool image_ret = Textures::LoadImageFile(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if (ext == ".GIF") {
                            bool image_ret = Textures::LoadImageGIF(path, item->textures);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_GIF_PREVIEW;
                        }
                        else if (ext == ".ICO") {
                            bool image_ret = Textures::LoadImageICO(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if ((ext == ".JPG") || (ext == ".JPEG")) {
                            bool image_ret = Textures::LoadImageJPEG(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if (ext == ".PCX") {
                            bool image_ret = Textures::LoadImagePCX(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if (ext == ".PNG") {
                            bool image_ret = Textures::LoadImagePNG(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if (ext == ".TIFF") {
                            bool image_ret = Textures::LoadImageTIFF(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
                        else if (ext == ".WEBP") {
                            bool image_ret = Textures::LoadImageWEBP(path, &item->texture);
                            IM_ASSERT(image_ret);
                            item->state = GUI_STATE_IMAGE_PREVIEW;
                        }
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
