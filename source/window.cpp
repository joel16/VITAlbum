#include <psp2/kernel/threadmgr.h>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "popups.h"
#include "tabs.h"
#include "utils.h"
#include "windows.h"

namespace Windows {
    static bool properties = false;

    void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(960.0f, 544.0f), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };
    
    void ExitWindow(void) {
        ImGui::End();
        ImGui::PopStyleVar();
    };

    static void ClearTextures(WindowData &data) {
        for (unsigned int i = 0; i < data.textures.size(); i++) {
            Textures::Free(data.textures[i]);
        }
        
        data.textures.clear();
        data.frame_count = 0;
    }

    static bool HandleScroll(WindowData &data, int index) {
        if (SCE_S_ISDIR(data.entries[index].d_stat.st_mode)) {
            return false;
        }
        else {
            data.selected = index;
            std::string path = FS::BuildPath(data.entries[index]);
            bool ret = Textures::LoadImageFile(path, data.textures);
            IM_ASSERT(ret);
            return true;
        }

        return false;
    }

    static bool HandlePrev(WindowData &data) {
        bool ret = false;

        for (int i = data.selected - 1; i > 0; i--) {
            std::string filename = data.entries[i].d_name;
            if (filename.empty()) {
                continue;
            }
                
            if (!(ret = Windows::HandleScroll(data, i))) {
                continue;
            }
            else {
                break;
            }
        }

        return ret;
    }

    static bool HandleNext(WindowData &data) {
        bool ret = false;

        if (data.selected == data.entries.size()) {
            return ret;
        }
        
        for (unsigned int i = data.selected + 1; i < data.entries.size(); i++) {
            if (!(ret = Windows::HandleScroll(data, i))) {
                continue;
            }
            else {
                break;
            }
        }

        return ret;
    }

    void MainWindow(WindowData &data, SceCtrlData &pad) {
        Windows::SetupWindow();
        if (ImGui::Begin("VITAlbum", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("VITAlbum-tabs")) {
                Tabs::FileBrowser(data);
                Tabs::Settings();
                ImGui::EndTabBar();
            }

            if (data.state == WINDOW_STATE_IMAGEVIEWER)
                Windows::ImageViewer(data);

            switch (data.state) {
                case WINDOW_STATE_FILEBROWSER:
                    if (pressed & SCE_CTRL_SELECT) {
                        data.state = WINDOW_STATE_SETTINGS;
                    }
                    break;
                
                case WINDOW_STATE_IMAGEVIEWER:
                    if (pressed & SCE_CTRL_TRIANGLE) {
                        properties = !properties;
                    }

                    if (pad.ly > 170) {
                        data.zoom_factor -= 0.5f * ImGui::GetIO().DeltaTime;
                        
                        if (data.zoom_factor < 0.1f) {
                            data.zoom_factor = 0.1f;
                        }
                    }
                    else if (pad.ly < 70) {
                        data.zoom_factor += 0.5f * ImGui::GetIO().DeltaTime;

                        if (data.zoom_factor > 5.0f) {
                            data.zoom_factor = 5.0f;
                        }
                    }

                    if (!properties) {
                        if (pressed & SCE_CTRL_CANCEL) {
                            Windows::ClearTextures(data);
                            data.zoom_factor = 1.0f;
                            data.state = WINDOW_STATE_FILEBROWSER;
                        }

                        if (pressed & SCE_CTRL_LTRIGGER) {
                            Windows::ClearTextures(data);
                            sceKernelDelayThread(100000);

                            if (!Windows::HandlePrev(data)) {
                                data.state = WINDOW_STATE_FILEBROWSER;
                            }
                        }
                        else if (pressed & SCE_CTRL_RTRIGGER) {
                            Windows::ClearTextures(data);
                            sceKernelDelayThread(100000);

                            if (!Windows::HandleNext(data)) {
                                data.state = WINDOW_STATE_FILEBROWSER;
                            }
                        }
                    }
                    else {
                        if (pressed & SCE_CTRL_CANCEL) {
                            properties = false;
                        }
                    }
                    
                    break;
                
                case WINDOW_STATE_SETTINGS:
                    if (pressed & SCE_CTRL_CANCEL) {
                        Config::Save(cfg);
                        data.entries.clear();
                        const std::string path = cfg.device + cfg.cwd;
                        FS::GetDirList(path, data.entries);
                        data.state = WINDOW_STATE_FILEBROWSER;
                    }

                    break;

                default:
                    break;
            }
        }

        Windows::ExitWindow();
    }
}
