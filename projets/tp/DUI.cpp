#include "DUI.h"
#include "texture.h"
#include "program.h"

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

void DUI::paramUI(SDL_Window * m_window, Param & param)
{
    ImGui::ColorEdit3("Light Color", &param.lightColor.r);
    ImGui::SliderFloat("Light Intensity", &param.lightIntensity, 0.0, 80.0);
    ImGui::Text("Elapsed Time: %.2f ms", param.elapsedTime);
    ImGui::SameLine();
    ImGui::Text("FPS: %.2f", 1000.0 / param.elapsedTime);
    ImGui::SameLine();
    ImGui::Text("DrawCall: %d", param.drawCall);

    ImGui::TextColored(ImVec4(1, 0, 0, 1), error.c_str());
    

    if(ImGui::Button("Reset")) {
        param.lightColor = Color(1, 1, 1);
        param.lightIntensity = 5.0f;
    }

    ImGui::SameLine();

    ImGui::Checkbox("Normal Mapping", &param.useNormalMapping);
    ImGui::SameLine();
    ImGui::Checkbox("Draw Bbox", &param.drawBbox);
    ImGui::SameLine();
    ImGui::Checkbox("Perform Culling", &param.performCulling);

    if(ImGui::Checkbox("Wireframe", &param.wireframe)) {
        if(param.wireframe)
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        else
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
    if(ImGui::SliderFloat3("Light P", &param.lightPos.x, -100, 100)) {
        param.generateShadow = true;
    }
    if(ImGui::SliderFloat3("Light D", &param.lightDir.x, -1, 1)) {
        param.generateShadow = true;
    }
    if(ImGui::SliderInt("Ortho Size", &param.orthoSize, 1, 50)) {
        param.generateShadow = true;
    }
    ImGui::SliderFloat("Shadow Factor", &param.shadowFactor, 0.0, 1.0);
    ImGui::SliderFloat("Shadow Bias", &param.shadowBias, 0.0, 0.1, "%.5f");
    ImGui::SliderFloat("F0", &param.F0, 0.0, 10.0);
    ImGui::SliderFloat("Alpha", &param.alpha, 0.0, 10.0);


    if(ImGui::Button("Generate Shadow")) {
        param.generateShadow = true;
    }
    if(ImGui::Button("Save Texture")) {
        param.saveTexture = true;
    }
    ImGui::Image((void*)(intptr_t)param.shadowTexture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
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

void DUI::renderMeshUi(DMesh * mesh, int id) {
    std::string ids = std::to_string(id);
    if(ImGui::CollapsingHeader(("Mesh" + ids).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Vertices: %d", mesh->getVertexCount());
        ImGui::SliderFloat3(("Translation" + ids).c_str(), &(mesh->T.x), -10, 10);
        ImGui::SliderFloat3(("Scale" + ids).c_str(), &(mesh->S.x), -2, 2);
        ImGui::SliderAngle(("Rotation X" + ids).c_str(), &(mesh->R.x), -180, 180);
        ImGui::SliderAngle(("Rotation Y" + ids).c_str(), &(mesh->R.y), -180, 180);
        ImGui::SliderAngle(("Rotation Z" + ids).c_str(), &(mesh->R.z), -180, 180);
    }
}

void DUI::hotReload(GLuint & shaderProgram, const std::string& path, const std::string& name) {
    if(ImGui::Button(name.c_str())) {
        glDeleteProgram(shaderProgram);
        shaderProgram = read_program(path.c_str());
        program_format_errors(shaderProgram, this->error);
    }
}