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
        ImGui_ImplVitaGL_NewFrame();
    }
    
    static void End(ImVec4 clear_color) {
        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
        vglSwapBuffers(GL_FALSE);
    }
}

namespace GUI {
    static void ClearTextures(MenuItem *item) {
        for (int i = 0; i < item->textures.size(); i++)
            Textures::Free(&item->textures[i]);
        
        item->textures.clear();
        item->frame_count = 0;
    }

    static bool HandleScroll(MenuItem *item, int index) {
        if (SCE_S_ISDIR(item->entries[index].d_stat.st_mode))
            return false;
        else {
            item->selected = index;
            std::string path = FS::BuildPath(&item->entries[index]);
            bool ret = Textures::LoadImageFile(path, item->textures);
            IM_ASSERT(ret);
            return true;
        }

        return false;
    }

    static bool HandlePrev(MenuItem *item) {
        bool ret = false;

        for (int i = item->selected - 1; i > 0; i--) {
            std::string filename = item->entries[i].d_name;
            if (filename.empty())
                continue;
                
            if (!(ret = GUI::HandleScroll(item, i)))
                continue;
            else
                break;
        }

        return ret;
    }

    static bool HandleNext(MenuItem *item) {
        bool ret = false;

        if (item->selected == item->entries.size())
            return ret;
        
        for (int i = item->selected + 1; i < item->entries.size(); i++) {
            if (!(ret = GUI::HandleScroll(item, i)))
                continue;
            else
                break;
        }

        return ret;
    }

    int RenderLoop(void) {
        bool done = false, properties = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        MenuItem item;
        SceCtrlData pad = { 0 };
        
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
            pad = Utils::ReadControls();

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

                    if (pad.ly > 170) {
                        item.zoom_factor -= 0.5f * ImGui::GetIO().DeltaTime;
                        
                        if (item.zoom_factor < 0.1f)
                            item.zoom_factor = 0.1f;
                    }
                    else if (pad.ly < 70) {
                        item.zoom_factor += 0.5f * ImGui::GetIO().DeltaTime;

                        if (item.zoom_factor > 5.0f)
                            item.zoom_factor = 5.0f;
                    }

                    if (!properties) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            GUI::ClearTextures(&item);
                            item.zoom_factor = 1.0f;
                            item.state = GUI_STATE_HOME;
                        }

                        if (pressed & SCE_CTRL_LTRIGGER) {
                            GUI::ClearTextures(&item);
                            sceKernelDelayThread(100000);

                            if (!GUI::HandlePrev(&item))
                                item.state = GUI_STATE_HOME;
                        }
                        else if (pressed & SCE_CTRL_RTRIGGER) {
                            GUI::ClearTextures(&item);
                            sceKernelDelayThread(100000);

                            if (!GUI::HandleNext(&item))
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

            /*if (pressed & SCE_CTRL_START)
                done = true;*/
        }
        
        item.entries.clear();
        return 0;
    }
}
