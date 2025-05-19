#include <cmath>
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
        if (data.texture.ptr || data.texture.frames) {
            Textures::Free(data.texture);
        }

        if (data.book.page != nullptr) {
            SDL_DestroyTexture(data.book.page);
            data.book.page = nullptr;
        }
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

    static bool NavigatePage(WindowData &data, int direction) {
        int new_page = data.book.page_number + direction;
        
        if (new_page < 0 || new_page >= data.book.page_count) {
            data.book.page_count = 0;
            data.book.page_number = 0;
            return false;
        }
        
        data.book.page_number = new_page;
        Reader::RenderPage(data.book);
        Reader::ResetPosition(data.book);
        return true;
    }

    static bool NavigateImage(WindowData& data, int direction) {
        int start = data.selected + direction;
        int end = direction > 0 ? data.entries.size() : -1;
        
        for (int i = start; i != end; i += direction) {
            const std::string& filename = data.entries[i].d_name;
            
            if (filename.empty()) {
                continue;
            }
            
            if (Windows::HandleScroll(data, i)) {
                return true;
            }
        }
        
        return false;
    }
    
    void HandleAnalogInput(WindowData& data, bool isBook) {
        if (!data.gamepad) {
            return;
        }
        
        Sint16 axisValue = SDL_GetGamepadAxis(data.gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
        float value = axisValue / 32767.0f;
        const float deadzone = 0.2f;
        
        if (std::fabs(value) > deadzone) {
            float zoomSpeed = 0.5f * ImGui::GetIO().DeltaTime;
            
            if (isBook) {
                float newZoom = data.book.zoom - value * zoomSpeed;
                Reader::SetZoom(data.book, newZoom);
            }
            else {
                data.zoom_factor -= value * zoomSpeed;

                if (data.zoom_factor > 5.0f) {
                    data.zoom_factor = 5.0f;
                }

                if (data.zoom_factor < 0.1f) {
                    data.zoom_factor = 0.1f;
                }
            }
        }
    }

    void HandleInput(WindowData& data, SDL_Event& event) {
        int button = event.gbutton.button;

        switch (data.state) {
            case WINDOW_STATE_BOOKVIEWER:
                if (button == SDL_GAMEPAD_BUTTON_WEST) {
                    data.book.rotate += 90.f;
                    if (data.book.rotate > 360.f) {
                        data.book.rotate = 0.f;
                    }
                    
                    Reader::RenderPage(data.book);
                }
                else if (button == SDL_GAMEPAD_BUTTON_EAST) {
                    Windows::ClearTextures(data);
                    data.book.page_count = 0;
                    data.book.page_number = 0;
                    data.zoom_factor = 1.0f;
                    data.state = WINDOW_STATE_FILEBROWSER;
                }
                else if (button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                    Windows::ClearTextures(data);
                    sceKernelDelayThread(100000);
                    
                    if (!Windows::NavigatePage(data, -1)) {
                        data.state = WINDOW_STATE_FILEBROWSER;
                    }
                }
                else if (button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                    Windows::ClearTextures(data);
                    sceKernelDelayThread(100000);
                    
                    if (!Windows::NavigatePage(data, 1)) {
                        data.state = WINDOW_STATE_FILEBROWSER;
                    }
                }

                break;
            case WINDOW_STATE_IMAGEVIEWER:
                if (button == SDL_GAMEPAD_BUTTON_NORTH) {
                    properties = !properties;
                }
                
                if (!properties) {
                    if (button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
                        Windows::ClearTextures(data);
                        sceKernelDelayThread(100000);

                        if (!Windows::NavigateImage(data, -1)) {
                            data.state = WINDOW_STATE_FILEBROWSER;
                        }
                    }
                    else if (button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
                        Windows::ClearTextures(data);
                        sceKernelDelayThread(100000);

                        if (!Windows::NavigateImage(data, 1)) {
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

            if (data.state == WINDOW_STATE_BOOKVIEWER) {
                Windows::Viewer(data, true);
            }
            else if (data.state == WINDOW_STATE_IMAGEVIEWER) {
                Windows::Viewer(data, false);
            }
        }

        Windows::ExitWindow();

        // if (properties) {
        //     Popups::ImageProperties(properties, data);
        // }
    }
}
