
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.
#include "Utility.h"
#include "MeshObject.h"
#include "DUI.h"

#include <omp.h>
#include <array>

class TP : public AppCamera
{
public:
     
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppCamera(1900, 1000, 4, 3), param(Param()){}

    void initShaders() {
        Scene::InitShader("projets/tpCompute/bf.glsl", lightShader);
        Scene::InitShader("projets/tpCompute/shadow.glsl", shadowShader);
        Scene::InitShader("projets/tpCompute/frustum.glsl", param.frustumShader); 
        Scene::InitShader("projets/tpCompute/occlusion.glsl", param.occlusionShader);
    }

    // creation des objets de l'application
    int init( )
    {
        // decrire un repere / grille 
        Mesh m_repere = make_grid(10);

        UI.init(m_window);

        Scene::Init(Color(0.4f, 0.6f, 0.8f, 1.f));
        
        // Init Shaders
        initShaders();

        camera().projection(1900, 1000, 45);
        test.projection(1900, 1000, 45);

        // Init Meshes

        //meshes.push_back(new MeshObject("data/robot.obj"));
        // auto scene = new MeshObject("data/export.obj");
        // meshes.push_back(scene);
        light = read_mesh("data/cube.obj");
        //cube->model = Translation(0, -0.5, 0) * Scale(20, 1, 20);

        //meshes.push_back(new MeshObject("data/robot.obj"));
        auto bigguy = new MeshObject("data/bigguy.obj");
        bigguy->model = Translation(-0, 4.75, -0) * Scale(0.5, 0.5, 0.5);
        meshes.push_back(bigguy);

        auto cube = new MeshObject("data/cube.obj");
        cube->model = Translation(0, -0.5, 0) * Scale(50, 1, 50);
        meshes.push_back(cube);

        cube = new MeshObject("data/cube.obj");
        cube->model = Translation(0, 7.5, 8) * Scale(15, 15, 2);
        meshes.push_back(cube);
        

        test.lookat(Point(0, 0, 0), Point(0, 0, 1));

        int cadre = 0;
        float w = 30.f;
        camera().lookat(Point(-w, -w, -w), Point(w, w, w));
        test.lookat(Point(-w, -w, -w), Point(w, w, w));

        param.shadowTexture = make_depth_texture(0, param.fbWidth, param.fbHeight);

        std::cout<< "Depth texture :: width = " << param.fbWidth << " height = " << param.fbHeight << std::endl;

        param.shadowFrameBuffer = 0;
        glGenFramebuffers(1, &param.shadowFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, param.shadowFrameBuffer);

        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, param.shadowTexture, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout<<"ERROR FRAMEBUFFER"<<std::endl;
            return -1;
        }

        param.occTexture = make_depth_texture(0, window_width(), window_height());

        glGenFramebuffers(1, &param.occFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, param.occFrameBuffer);

        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, param.occTexture, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::cout<<"----INIT DONE---"<<std::endl;



        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        return 0;
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        glDeleteProgram(lightShader);
        glDeleteProgram(param.frustumShader);
        glDeleteProgram(param.occlusionShader);
        glDeleteProgram(shadowShader);

        for(auto & mesh : meshes) {
            delete mesh;
        }
        
        return 0;   // pas d'erreur
    }
    
    void doKeyboard() {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            float speed = 1.5f;
            if(key_state('z'))
                test.move(speed);
            if(key_state('s'))
                test.move(-speed);

            if(key_state('a'))
                test.rotation(speed * 0.2, 0);
            if(key_state('e'))
                test.rotation(-speed * 0.2, 0);

            if(key_state('w'))
                test.rotation(0, speed * 0.2);
            
            if(key_state('x'))
                test.rotation(0, -speed * 0.2);
                
            //     m_camera.rotation(mx, my);      // tourne autour de l'objet
            // else if(mb & SDL_BUTTON(3))
            //     m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height()); // deplace le point de rotation
            // else if(mb & SDL_BUTTON(2))
            //     m_camera.move(mx);           // approche / eloigne l'objet
            
            // SDL_MouseWheelEvent wheel= wheel_event();
            // if(wheel.y != 0)
            // {
            //     clear_wheel_event();
            //     m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
            // }
        }
    }
    
    void getZBuffer(GLuint & textureToStore, float w, float h)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, param.occFrameBuffer);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void legacyDraw(GLenum primitive, const GLuint & vao, int first, int n) {
        glBindVertexArray(vao);
        glDrawArrays(primitive, first, n);
    }

    void generateDepth() {
        projectionLight = Ortho(-param.orthoSize, param.orthoSize, -param.orthoSize, param.orthoSize, -param.orthoSize, param.orthoSize);
        // dir = look at 0 0 0 from lightPos
        viewLight = Lookat(Point(param.lightPos), Point(param.lightDir), Vector(0, 1, 0));
        vpInvLight = (projectionLight * viewLight).inverse();
        glBindFramebuffer(GL_FRAMEBUFFER, param.shadowFrameBuffer);

        glViewport(0, 0, param.fbWidth, param.fbHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for(auto & mesh : meshes) {
            param.drawCall += mesh->draw(param, shadowShader, viewLight, projectionLight, vpInvLight, Translation(param.lightPos));

        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        param.generateShadow = false;
    }

    
    // dessiner une nouvelle image
    int render( )
    {
        param.drawCall = 0;

        // if(param.generateShadow) {
        //     generateDepth();
        // }

        glUseProgram(lightShader);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        program_uniform(lightShader, "lightDir", param.lightDir);
        program_uniform(lightShader, "camera", camera().position());
        program_uniform(lightShader, "lightColor", param.lightColor);
        program_uniform(lightShader, "lightPower", param.lightIntensity);
        program_use_texture(lightShader, "shadowMap", 2, param.shadowTexture);
        program_uniform(lightShader, "bias", param.shadowBias);
        program_uniform(lightShader, "shadowFactor", param.shadowFactor);

        Transform view = camera().view();
        
        Transform projection = camera().projection(window_width(), window_height(), 45);
        Transform vpInv = (projection * view).inverse();
        Transform viewTest = test.view();
        Transform projectionTest = test.projection(window_width(), window_height(), 45);
        Transform vpInvTest = (projectionTest * viewTest).inverse();


        

        int id = 0;

        for(auto & mesh : meshes) {
            glUseProgram(lightShader);
            //program_uniform(lightShader, "mvpLightMatrix", shadowvp * projectionLight * viewLight * Translation(param.lightPos));
            param.drawCall += mesh->draw(param, lightShader, view, projection, vpInv, Identity());
        }

        getZBuffer(param.occTexture, window_width(), window_height());

        draw(light, Translation(param.lightPos), camera());

        doKeyboard();

        UI.createUIWindow(m_window, "UI");
        UI.paramUI(m_window, param);
        UI.renderUI();

        return 1;
    }

protected:
    std::vector<MeshObject*> meshes;
    Param param;

    GLuint lightShader;
    GLuint shadowShader;
    Transform shadowvp = Viewport(1, 1);
    

    DUI UI;
    Transform projectionLight;
    Transform viewLight;
    Transform vpInvLight;

    Orbiter test;

    Mesh light;

    bool isWireframe = false;

    float tick = 0;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    
    return 0;
}
