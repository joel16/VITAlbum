#include <imgui_vita.h>
#include <vitaGL.h>

#include "fs.h"
#include "keyboard.h"
#include "textures.h"
#include "utils.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace Renderer {
    static void Start(void) {
        vglStartRendering();
        ImGui_ImplVitaGL_NewFrame();
    }
    
    static void End(ImVec4 clear_color) {
        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglStopRendering();
    }
    
    void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(960.0f, 544.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    }
    
    void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

namespace GUI {
    enum GUI_STATES {
        GUI_STATE_HOME,
        GUI_STATE_IMAGE_PREVIEW,
        GUI_STATE_GIF_PREVIEW
    };

    static int frame_count = 0;

    static void ImageWindow(SceIoDirent *entry, Tex *texture) {
        Renderer::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin(entry->d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(texture->width, texture->height)) * 0.5f);
            ImGui::Image(reinterpret_cast<ImTextureID>(texture->id), ImVec2(texture->width, texture->height));
        }
        
        Renderer::ExitWindow();
        ImGui::PopStyleVar();
    }

    static void GifWindow(SceIoDirent *entry, std::vector<Tex> textures) {
        Renderer::SetupWindow();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin(entry->d_name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            frame_count++;
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(textures[frame_count].width, textures[frame_count].height)) * 0.5f);
            sceKernelDelayThread(textures[frame_count].delay * 10000);
            ImGui::Image(reinterpret_cast<ImTextureID>(textures[frame_count].id), ImVec2(textures[frame_count].width, textures[frame_count].height));

            // Reset frame counter
            if (frame_count == textures.size() - 1)
                frame_count = 0;
        }
        
        Renderer::ExitWindow();
        ImGui::PopStyleVar();
    }
    
    static void PropertiesWindow(bool *window, const std::string &cwd, SceIoDirent *entry, Tex *texture) {
        ImGui::OpenPopup("Properties");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
        ImGui::SetNextWindowPos(ImVec2(480.0f, 272.0f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        std::string new_width, new_height;
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
                Utils::GetSizeString(size, entry->d_stat.st_size);
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
            if (ImGui::Button("Edit width")) {
                new_width = Keyboard::GetText("Enter width", std::to_string(texture->width));
            }

            ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing

            std::string height_text = "Height: ";
            height_text.append(std::to_string(texture->height));
            height_text.append("px");
            ImGui::Text(height_text.c_str());
            ImGui::SameLine(0.0f, 10.0f);
            if (ImGui::Button("Edit height")) {
                new_height = Keyboard::GetText("Enter height", std::to_string(texture->height));
            }

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
        std::vector<Tex> textures; // GIFs

        int gui_state = GUI_STATE_HOME;
        
        while (window) {
            Renderer::Start();
            Renderer::SetupWindow();
            
            if (ImGui::Begin("VITAlbum", &window, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), FS::CWD.c_str());
                
                ImGui::BeginChild("##FS::GetDirList");
                for (SceOff i = 0; i < entry_count; i++) {
                    if (SCE_S_ISDIR(entries[i].d_stat.st_mode))
                        ImGui::Image(reinterpret_cast<ImTextureID>(folder_texture.id), ImVec2(folder_texture.width, folder_texture.height));
                    else {
                        std::string filename = entries[i].d_name;
                        std::string ext = FS::GetFileExt(filename);
                        
                        if ((ext == ".BMP") || (ext == ".GIF") || (ext == ".ICO") || (ext == ".JPG") || (ext == ".JPEG") || (ext == ".PCX")
                            || (ext == ".PNG") || (ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA") || (ext == ".TIFF")
                            || (ext == ".WEBP"))
                            ImGui::Image(reinterpret_cast<ImTextureID>(image_texture.id), ImVec2(image_texture.width, image_texture.height));
                        else
                            ImGui::Image(reinterpret_cast<ImTextureID>(file_texture.id), ImVec2(file_texture.width, file_texture.height));
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
                            else if ((ext == ".PGM") || (ext == ".PPM") || (ext == ".PSD") || (ext == ".TGA")) {
                                SceBool image_ret = Textures::LoadImageFile(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".GIF") {
                                SceBool image_ret = Textures::LoadImageGIF(path, textures);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_GIF_PREVIEW;
                            }
                            else if (ext == ".ICO") {
                                SceBool image_ret = Textures::LoadImageICO(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if ((ext == ".JPG") || (ext == ".JPEG")) {
                                SceBool image_ret = Textures::LoadImageJPEG(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".PCX") {
                                SceBool image_ret = Textures::LoadImagePCX(path, &texture);
                                IM_ASSERT(image_ret);
                                gui_state = GUI_STATE_IMAGE_PREVIEW;
                            }
                            else if (ext == ".PNG") {
                                SceBool image_ret = Textures::LoadImagePNG(path, &texture);
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
                    GUI::ImageWindow(&entries[selected], &texture);
                    if (properties_window)
                        GUI::PropertiesWindow(&properties_window, FS::CWD, &entries[selected], &texture);
                    break;

                case GUI_STATE_GIF_PREVIEW:
                    GUI::GifWindow(&entries[selected], textures);
                    if (properties_window)
                        GUI::PropertiesWindow(&properties_window, FS::CWD, &entries[selected], &textures[0]);
                    break;

                default:
                    break;
            }
            
            Renderer::ExitWindow();
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
                     if (pressed & SCE_CTRL_TRIANGLE)
                        properties_window = !properties_window;
                    
                    if (!properties_window) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            for (int i = 0; i < textures.size(); i++)
                                Textures::Free(&textures[i]);
                            
                            textures.clear();
                            frame_count = 0;
                            gui_state = GUI_STATE_HOME;
                        }
                    }
                    else {
                        if (pressed & SCE_CTRL_CANCEL)
                            properties_window = false;
                    }
                    
                    break;
            }
        }
    }
}
