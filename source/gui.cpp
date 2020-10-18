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
        bool done = false, properties = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        MenuItem item;
        
        int ret = 0;
        if (R_FAILED(ret = FS::GetDirList(config.cwd, item.entries)))
            return ret;

        while (!done) {
            Renderer::Start();
            Windows::FileBrowserWindow(&item);
            
            switch (item.state) {
                case GUI_STATE_IMAGE_PREVIEW:
                    Windows::ImageWindow(&item);

                    if (properties)
                        Popups::ImageProperties(&properties, &item, &item.textures[0]);
                    
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
                        if (R_SUCCEEDED(FS::ChangeDirPrev(item.entries)))
                            GImGui->NavId = 0;
                    }

                    break;
                
                case GUI_STATE_IMAGE_PREVIEW:
                    if (pressed & SCE_CTRL_TRIANGLE)
                        properties = !properties;
                    
                    if (!properties) {
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
                            properties = false;
                    }
                    
                    break;
                
                case GUI_STATE_SETTINGS:
                    if (pressed & SCE_CTRL_CANCEL) {
                        Config::Save(config);
                        item.entries.clear();
                        FS::GetDirList(config.cwd, item.entries);
                        item.state = GUI_STATE_HOME;
                    }

                    break;

                default:
                    break;
            }

            if (pressed & SCE_CTRL_START)
                done = true;
        }
        
        item.entries.clear();
        return 0;
    }
}
