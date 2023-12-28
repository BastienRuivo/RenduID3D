#pragma once

#include <imgui.h>
#include "src/imgui/imgui_impl_sdl_gl3.h"

#include "Param.h"
#include <SDL2/SDL.h>

#include "glcore.h"
#include "DMesh.h"
#include <string>

class DUI
{
private:
    ImGuiIO io;
public:
    DUI();
    void init(SDL_Window * m_window);

    std::string error;

    void paramUI(SDL_Window * m_window, Param & param);
    void createUIWindow(SDL_Window * m_window, const std::string& name);
    void renderUI();
    void renderMeshUi(DMesh * mesh, int id);
    void hotReload(GLuint & shaderProgram, const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    ~DUI() {
        ImGui_ImplSdlGL3_Shutdown();
        ImGui::DestroyContext();
    }
};