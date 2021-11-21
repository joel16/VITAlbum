#include <jconfig.h>
#include <png.h>
#include <tiffvers.h>
#include <webp/decode.h>

#include "config.h"
#include "fs.h"
#include "imgui.h"
#include "windows.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

namespace Tabs {
    static void Separator(void) {
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
    }

    void Settings(void) {
        if (ImGui::BeginTabItem("Settings")) {
            if (ImGui::TreeNode("Image/Gif Viewer")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Checkbox(" Display filename", &cfg.image_filename);
                ImGui::TreePop();
            }
            
            Tabs::Separator();
            
            if (ImGui::TreeNode("Developer Options")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Checkbox(" Enable logs", &cfg.dev_options);
                ImGui::TreePop();
            }

            Tabs::Separator();
            
            if (ImGui::TreeNode("About")) {
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                std::string vitalbum_ver = APP_VERSION;
                vitalbum_ver.erase(0, std::min(vitalbum_ver.find_first_not_of('0'), vitalbum_ver.size() - 1));
                ImGui::Text("VITAlbum version: %s", vitalbum_ver.c_str());
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("Author: Joel16");
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("Dear imGui version: %s", ImGui::GetVersion());
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("libjpeg-turbo version: %s", TO_STRING(LIBJPEG_TURBO_VERSION));
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("libpng version: %s", PNG_LIBPNG_VER_STRING);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("LibTIFF version: %d", TIFFLIB_VERSION);
                ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
                ImGui::Text("libwebp version: %d.%d.%d", (WebPGetDecoderVersion() >> 16) & 0xFF, (WebPGetDecoderVersion() >> 8) & 0xFF, WebPGetDecoderVersion() & 0xFF);
                ImGui::TreePop();
            }

            ImGui::EndTabItem();
        }
    }
}
