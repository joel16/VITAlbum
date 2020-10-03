#include <imgui_vita.h>
#include <vitaGL.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "popups.h"
#include "utils.h"
#include "windows.h"

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
}

namespace GUI {
    int RenderLoop(void) {
        bool done = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        MenuItem item;
        item.entry_count = FS::GetDirList(config.cwd, &item.entries);

        while (!done) {
            Renderer::Start();
            Windows::FileBrowserWindow(&item);
            
            switch (item.state) {
                case GUI_STATE_IMAGE_PREVIEW:
                    Windows::ImageWindow(&item);

                    if (item.image_properties)
                        Popups::ImageProperties(&item, &item.texture);
                    
                    break;

                case GUI_STATE_GIF_PREVIEW:
                    Windows::GifWindow(&item);
                    
                    if (item.image_properties)
                        Popups::ImageProperties(&item, &item.textures[0]);
                    
                    break;

                case GUI_STATE_SETTINGS:
                    Windows::SettingsWindow(&item);
                    break;

                default:
                    break;
            }

            Renderer::End(clear_color);
            Utils::ReadControls();

            switch (item.state) {
                case GUI_STATE_HOME:
                    if (pressed & SCE_CTRL_SELECT)
                        item.state = GUI_STATE_SETTINGS;
                    else if (pressed & SCE_CTRL_CANCEL) {
                        SceOff value = FS::ChangeDirPrev(&item.entries);
                        if (value >= 0) {
                            item.entry_count = value;
                            GImGui->NavId = 0;
                        }
                    }

                    break;
                
                case GUI_STATE_IMAGE_PREVIEW:
                    if (pressed & SCE_CTRL_TRIANGLE)
                        item.image_properties = !item.image_properties;
                    
                    if (!item.image_properties) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            Textures::Free(&item.texture);
                            item.state = GUI_STATE_HOME;
                        }
                    }
                    else {
                        if (pressed & SCE_CTRL_CANCEL)
                            item.image_properties = false;
                    }
                    
                    break;

                case GUI_STATE_GIF_PREVIEW:
                     if (pressed & SCE_CTRL_TRIANGLE)
                        item.image_properties = !item.image_properties;
                    
                    if (!item.image_properties) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            for (int i = 0; i < item.textures.size(); i++)
                                Textures::Free(&item.textures[i]);
                            
                            item.textures.clear();
                            item.frame_count = 0;
                            item.state = GUI_STATE_HOME;
                        }
                    }
                    else {
                        if (pressed & SCE_CTRL_CANCEL)
                            item.image_properties = false;
                    }
                    
                    break;

                case GUI_STATE_SETTINGS:
                    if (pressed & SCE_CTRL_CANCEL) {
                        Config::Save(config);
                        FS::FreeDirEntries(&item.entries, item.entry_count);
                        item.entry_count = FS::GetDirList(config.cwd, &item.entries);
                        item.state = GUI_STATE_HOME;
                    }

                    break;

                default:
                    break;
            }

            if (pressed & SCE_CTRL_START)
                done = true;
        }

        FS::FreeDirEntries(&item.entries, item.entry_count);
        return 0;
    }
}
