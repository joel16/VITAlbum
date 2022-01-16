#include <imgui_vita.h>
#include <vitaGL.h>

#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "utils.h"
#include "windows.h"

namespace Renderer {
    static void Start(void) {
        ImGui_ImplVitaGL_NewFrame();
        ImGui::NewFrame();
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
    int RenderLoop(void) {
        bool done = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        WindowData data;
        SceCtrlData pad = { 0 };
        
        int ret = 0;
        const std::string path = cfg.device + cfg.cwd;
        if (R_FAILED(ret = FS::GetDirList(path, data.entries)))
            return ret;

        while (!done) {
            Renderer::Start();
            pad = Utils::ReadControls();
            Windows::MainWindow(data, pad);
            Renderer::End(clear_color);

            if (pressed & SCE_CTRL_START)
                done = true;
        }
        
        data.entries.clear();
        return 0;
    }
}
