#include "DUI.h"

DUI::DUI()
{
}

void DUI::init(SDL_Window * m_window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    io = ImGui::GetIO();(void)io;
    ImGui_ImplSdlGL3_Init(m_window);

    ImGui::StyleColorsDark();
}

void InitShader(const std::string & filename, GLuint & shader) {
    shader = read_program(filename.c_str());
    if(program_print_errors(shader)) {
        exit(0);
    }
}

void DUI::paramUI(SDL_Window * m_window, Param & param)
{
    ImGui::ColorEdit3("Light Color", &param.lightColor.r);
    ImGui::SliderFloat("Light Intensity", &param.lightIntensity, 0.0, 80.0);
    ImGui::InputInt("Drawn Object", &param.drawCall, 1, 100, ImGuiInputTextFlags_ReadOnly);
    // 8 digits precision, no scientific notation
    ImGui::InputFloat("Depth", &param.depth, 0.0, 0.0, "%.8f", ImGuiInputTextFlags_ReadOnly);
    

    if(ImGui::Button("Reset")) {
        param.lightColor = Color(1, 1, 1);
        param.lightIntensity = 5.0f;
    }

    ImGui::SameLine();

    if(ImGui::Checkbox("Wireframe", &param.wireframe)) {
        if(param.wireframe)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }

    ImGui::SameLine();
    if(ImGui::Checkbox("Show Bbox", &param.showBbox)) {
    }

    if(ImGui::SliderFloat3("Light P", &param.lightPos.x, -100, 100)) {
        param.generateShadow = true;
    }
    if(ImGui::SliderFloat3("Light D", &param.lightDir.x, -1, 1)) {
        param.generateShadow = true;
    }
    if(ImGui::SliderInt("Ortho Size", &param.orthoSize, 10, 300)) {
        param.generateShadow = true;
    }
    ImGui::SliderFloat("Shadow Factor", &param.shadowFactor, 0.0, 1.0);
    ImGui::SliderFloat("Shadow Bias", &param.shadowBias, 0.0, 0.1, "%.5f");

    if(ImGui::Button("Recompile shaders")) {
         // delete all shaders
        glDeleteProgram(param.lightShader);
        glDeleteProgram(param.frustumShader);
        glDeleteProgram(param.occlusionShader);
        glDeleteProgram(param.depthShader);
        glDeleteProgram(param.mipmapShader);


        InitShader("projets/tpCompute/bf.glsl", param.lightShader);
        InitShader("projets/tpCompute/depth.glsl", param.depthShader);
        InitShader("projets/tpCompute/frustum.glsl", param.frustumShader); 
        InitShader("projets/tpCompute/occlusion.glsl", param.occlusionShader);
        InitShader("projets/tpCompute/mipmap_maker.glsl", param.mipmapShader);
        InitShader("projets/tpCompute/mipmap_maker_init.glsl", param.mipmapInitShader);
        // InitShader("projets/tpCompute/culling.glsl", param.cullingShader);
    }


    if(ImGui::Button("Generate Shadow")) {
        param.generateShadow = true;
    }
    if(ImGui::Button("Save Texture")) {
        param.saveTexture = true;
    }

    
    ImGui::Image((void*)(intptr_t)param.occTexture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));


}

void DUI::createUIWindow(SDL_Window * m_window, const std::string& name)
{
    ImGui_ImplSdlGL3_NewFrame(m_window);
    ImGui::Begin(name.c_str());
}

void DUI::renderUI() {
    ImGui::End();
    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
}