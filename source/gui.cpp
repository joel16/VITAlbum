#include <imgui_vita.h>
#include <vitaGL.h>

#include "fs.h"
#include "imgui_internal.h"
#include "textures.h"
#include "utils.h"

namespace Renderer {
    static void Start(void) {
        vglStartRendering();
        ImGui_ImplVitaGL_NewFrame();
    }
    
    static void End(ImVec4 clear_color) {
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglStopRendering();
    }
}

namespace GUI {
    enum GUI_STATES {
        GUI_STATE_HOME,
        GUI_STATE_IMAGE_PREVIEW,
        GUI_STATE_GIF_PREVIEW
    };

    static int gui_state = GUI_STATE_HOME;

    static void ImageWindow(SceIoDirent *entry, Tex *texture) {
        ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({960.0f, 544.0f}, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        if (ImGui::Begin(entry->d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            ImGui::Image((void *)(intptr_t)texture->id, ImVec2(texture->width, texture->height));
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    static void GifWindow(SceIoDirent *entry, Tex **texture, unsigned int *frames) {
        ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({960.0f, 544.0f}, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        if (ImGui::Begin(entry->d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            // for (unsigned int i = 0; i < *frames; i++) {
            //     ImGui::Image((void *)(intptr_t)texture[i]->id, ImVec2(texture[i]->width, texture[i]->height));
            // }
            ImGui::Image((void *)(intptr_t)texture[0]->id, ImVec2(texture[0]->width, texture[0]->height));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    static void PropertiesWindow(bool *window, const std::string &cwd, SceIoDirent *entry, Tex *texture) {
        ImGui::OpenPopup("Properties");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        if (ImGui::BeginPopupModal("Properties", window, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::string parent_text = "Parent: ";
            parent_text.append(cwd);
            ImGui::Text(parent_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string name_text = "Name: ";
            name_text.append(entry->d_name);
            ImGui::Text(name_text.c_str());

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            if (!SCE_S_ISDIR(entry->d_stat.st_mode)) {
                char size[16];
                Utils::GetSizeString(size, (double)entry->d_stat.st_size);
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
            ImGui::Button("Edit width");

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string height_text = "Height: ";
            height_text.append(std::to_string(texture->height));
            height_text.append("px");
            ImGui::Text(height_text.c_str());
            ImGui::SameLine(0.0f, 10.0f);
            ImGui::Button("Edit height");

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                *window = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }
    
    void MainMenu(void) {
        bool window = true, properties_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        
        FS::CWD = "ux0:";
        SceIoDirent *entries;
        SceOff entry_count = FS::GetDirList(FS::CWD, &entries);
        SceOff selected = 0;

        Tex texture; // Common image formats
        Tex *textures; // GIFs
        unsigned int frames = 0; // GIFs
        
        while (window) {
            Renderer::Start();
            
            ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Once);
            ImGui::SetNextWindowSize({960.0f, 544.0f}, ImGuiCond_Once);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            
            if (ImGui::Begin("VITAlbum", &window, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), FS::CWD.c_str());
                
                ImGui::BeginChild("##FS::GetDirList");
                for (SceOff i = 0; i < entry_count; i++) {
                    if (SCE_S_ISDIR(entries[i].d_stat.st_mode))
                        ImGui::Image((void *)(intptr_t)folder_texture.id, ImVec2(folder_texture.width, folder_texture.height));
                    else {
                        std::string filename = entries[i].d_name;
                        std::string ext = FS::GetFileExt(filename);
                        
                        if ((ext == ".BMP") || (ext == ".GIF") || (ext == ".ICO") || (ext == ".JPG") || (ext == ".JPEG") || (ext == ".PCX")
                            || (ext == ".PNG") || (ext == ".PGM") || (ext == ".PPM") || (ext == ".TGA") || (ext == ".TIFF") || (ext == ".WEBP"))
                            ImGui::Image((void *)(intptr_t)image_texture.id, ImVec2(image_texture.width, image_texture.height));
                        else
                            ImGui::Image((void *)(intptr_t)file_texture.id, ImVec2(file_texture.width, file_texture.height));
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Selectable(entries[i].d_name)) {
                        if (SCE_S_ISDIR(entries[i].d_stat.st_mode)) {
                            std::string filename = entries[i].d_name;
                            if (filename == "..")
                                entry_count = FS::ChangeDirPrev(&entries);
                            else
                                entry_count = FS::ChangeDirNext(filename, &entries);
                            GImGui->NavId = 0;
                        }
                        else {
                            std::string path = FS::BuildPath(&entries[i]);
                            std::string ext = FS::GetFileExt(path);

                            if (ext == ".BMP") {
                                SceBool image_ret = Textures::LoadImageBMP(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if ((ext == ".JPG") || (ext == ".JPEG") || (ext == ".PNG") || (ext == ".PGM") || (ext == ".PPM")
                                || (ext == ".TGA")) {
                                SceBool image_ret = Textures::LoadImageFile(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".GIF") {
                                //SceBool image_ret = Textures::LoadImageGIF(path, &textures, &frames);
                                //IM_ASSERT(image_ret);
                                //gui_state = GUI_STATE_GIF_PREVIEW;
                                SceBool image_ret = Textures::LoadImageGIF(path, &texture, &frames);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".ICO") {
                                SceBool image_ret = Textures::LoadImageICO(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".PCX") {
                                SceBool image_ret = Textures::LoadImagePCX(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".TIFF") {
                                SceBool image_ret = Textures::LoadImageTIFF(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".WEBP") {
                                SceBool image_ret = Textures::LoadImageWEBP(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                        }
                    }

                    if (ImGui::IsItemHovered())
                        selected = i;
                }
                ImGui::EndChild();
            }

            switch (gui_state) {
                case GUI_STATE_IMAGE_PREVIEW:
                    ImageWindow(&entries[selected], &texture);
                    if (properties_window)
                        PropertiesWindow(&properties_window, FS::CWD, &entries[selected], &texture);
                    break;

                case GUI_STATE_GIF_PREVIEW:
                    GifWindow(&entries[selected], &textures, &frames);
                    break;

                default:
                    break;
            }
            
            ImGui::End();
            ImGui::PopStyleVar();
            Renderer::End(clear_color);

            Utils::ReadControls();

            switch (gui_state) {
                case GUI_STATE_IMAGE_PREVIEW:
                    if (pressed & SCE_CTRL_TRIANGLE)
                        properties_window = !properties_window;
                    
                    if (!properties_window) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            Textures::Free(&texture);
                            gui_state = GUI_STATE_HOME;
                        }
                    }
                    else {
                        if (pressed & SCE_CTRL_CANCEL)
                            properties_window = false;
                    }
                    
                    break;

                case GUI_STATE_GIF_PREVIEW:
                    if (pressed & SCE_CTRL_CANCEL) {
                        for (int i = 0; i < frames; i++) {
                            Textures::Free(&textures[i]);
                        }
                        gui_state = GUI_STATE_HOME;
                    }
                    
                    break;
            }
        }
    }
}
