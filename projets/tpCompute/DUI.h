#pragma once

#include <imgui.h>
#include "src/imgui/imgui_impl_sdl_gl3.h"

#include "Param.h"
#include <SDL2/SDL.h>

#include "texture.h"
#include "program.h"

class DUI
{
private:
    ImGuiIO io;
public:
    DUI();
    void init(SDL_Window * m_window);

    void paramUI(SDL_Window * m_window, Param & param);
    void createUIWindow(SDL_Window * m_window, const std::string& name);
    void renderUI();
    ~DUI() {
        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();
    }
};