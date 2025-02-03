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

    static void ClearTextures(WindowData& data) {
        Textures::Free(data.texture);
    }

    static bool HandleScroll(WindowData& data, int index) {
        if (SCE_S_ISDIR(data.entries[index].d_stat.st_mode)) {
            return false;
        }
        else {
            data.selected = index;
            std::string path = FS::BuildPath(data.entries[index]);
            bool ret = Textures::LoadImageFile(path, data.texture);
            IM_ASSERT(ret);
            return true;
        }

        return false;
    }

    static bool HandlePrev(WindowData& data) {
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

    static bool HandleNext(WindowData& data) {
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

    void HandleInput(WindowData& data, SDL_Event& event) {
        int button = event.gbutton.button;

        switch (data.state) {
            case WINDOW_STATE_IMAGEVIEWER:
                if (button == SDL_GAMEPAD_BUTTON_NORTH) {
                    properties = !properties;
                }
                
                if (!properties) {
                    if ((event.jaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY) && (event.jaxis.value < -8000)) {
                        data.zoom_factor += 0.5f * ImGui::GetIO().DeltaTime;

                        if (data.zoom_factor > 5.0f) {
                            data.zoom_factor = 5.0f;
                        }
                    }
                    else if ((event.jaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY) && (event.jaxis.value > 8000)) {
                        data.zoom_factor -= 0.5f * ImGui::GetIO().DeltaTime;

                        if (data.zoom_factor < 0.1f) {
                            data.zoom_factor = 0.1f;
                        }
                    }
                    if (button == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
                        Windows::ClearTextures(data);
                        sceKernelDelayThread(100000);

                        if (!Windows::HandlePrev(data)) {
                            data.state = WINDOW_STATE_FILEBROWSER;
                        }
                    }
                    else if (button == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                        Windows::ClearTextures(data);
                        sceKernelDelayThread(100000);

                        if (!Windows::HandleNext(data)) {
                            data.state = WINDOW_STATE_FILEBROWSER;
                        }
                    }
                    
                    if (button == SDL_GAMEPAD_BUTTON_EAST) {
                        Windows::ClearTextures(data);
                        data.zoom_factor = 1.0f;
                        data.state = WINDOW_STATE_FILEBROWSER;
                    }
                }
                else {
                    if (button == SDL_GAMEPAD_BUTTON_EAST) {
                        properties = false;
                    }
                }
                
                break;

            default:
                break;
        }
    }

    void MainWindow(WindowData& data) {
        Windows::SetupWindow();
        if (ImGui::Begin("VITAlbum", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            if (ImGui::BeginTabBar("VITAlbum-tabs")) {
                Tabs::FileBrowser(data);
                Tabs::Settings();
                ImGui::EndTabBar();
            }

            if (data.state == WINDOW_STATE_IMAGEVIEWER) {
                Windows::ImageViewer(data);
            }
        }

        Windows::ExitWindow();

        // if (properties) {
        //     Popups::ImageProperties(properties, data);
        // }
    }
}
