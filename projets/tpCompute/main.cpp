
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.
#include "MeshObject.h"
#include "DUI.h"

#include <omp.h>
#include <array>
#include "Utility.h"

class TP : public AppCamera
{
public:
     
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppCamera(1900, 1000, 4, 3), param(Param()){}

    void initShaders() {
        Scene::InitShader("projets/tpCompute/bf.glsl", param.lightShader);
        Scene::InitShader("projets/tpCompute/depth.glsl", param.depthShader);
        Scene::InitShader("projets/tpCompute/frustum.glsl", param.frustumShader); 
        Scene::InitShader("projets/tpCompute/occlusion.glsl", param.occlusionShader);
        Scene::InitShader("projets/tpCompute/mipmap_maker.glsl", param.mipmapShader);
        Scene::InitShader("projets/tpCompute/mipmap_maker_init.glsl", param.mipmapInitShader);
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


        // Init Meshes

        //meshes.push_back(new MeshObject("data/robot.obj"));
        // auto scene = new MeshObject("data/export.obj");
        // meshes.push_back(scene);
        light = read_mesh("data/cube.obj");
        //cube->model = Translation(0, -0.5, 0) * Scale(20, 1, 20);

        //meshes.push_back(new MeshObject("data/robot.obj"));
        auto bigguy = new MeshObject("data/bigguy.obj");
        ///bigguy->model = Translation(-0, 8, -0) * Scale(0.5, 0.5, 0.5);
        meshes.push_back(bigguy);

        // auto cube = new MeshObject("data/cube.obj");
        // cube->model = Translation(0, 0, -5);// * Scale(50, 1, 50);
        // meshes.push_back(cube);

        // cube = new MeshObject("data/cube.obj");
        // cube->model = Translation(0, 7.5, 8) * Scale(15, 15, 2);
        // meshes.push_back(cube);
        


        Point vmin, vmax;
        meshes[0]->mesh.bounds(vmin, vmax);

        camera().lookat(Point(0, 0, 0), 50);


        param.shadowTexture = make_depth_texture(0, param.fbWidth, param.fbHeight);
        param.occTexture = make_depth_texture(0, param.occWidth, param.occHeight);
        //param.occTexture = make_depth_texture(0, window_width(), window_height());
        int width = param.occWidth / 2;
        int height = param.occHeight / 2;
        param.occMipmapTexture = Scene::makeMipmapTexture(width, height);

        lvl = 1;
        while(width > 4 && height > 4) {
            width = std::max(width / 2, 4);
            height = std::max(height / 2, 4);
            lvl++;
        }
        Scene::InitFramebuffer(param.occFrameBuffer, param.occTexture);
        Scene::InitFramebuffer(param.shadowFrameBuffer, param.shadowTexture);


        std::cout<<"----INIT DONE---"<<std::endl;

        // enable GL CULL FACE
        glEnable(GL_CULL_FACE);

        return 0;
    }

    void hotReload() {
       
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        glDeleteProgram(param.lightShader);
        glDeleteProgram(param.frustumShader);
        glDeleteProgram(param.occlusionShader);
        glDeleteProgram(param.depthShader);

        
        for(auto & mesh : meshes) {
            delete mesh;
        }
        
        return 0;   // pas d'erreur
    }
    
    void doKeyboard() {
        auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            float speed = 1.5f;
                
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
            param.drawCall += mesh->draw(param, param.depthShader, camera(), viewLight, projectionLight, vpInvLight, Translation(param.lightPos));
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        param.generateShadow = false;
    }

    void generateDepthMap(const Transform & view, const Transform & projection, const Transform & vpInv) {
        glBindFramebuffer(GL_FRAMEBUFFER, param.occFrameBuffer);
        glViewport(0, 0, param.occWidth, param.occHeight);
        glClear(GL_DEPTH_BUFFER_BIT);

        // depth range
        float znear = camera().znear();
        float zfar = camera().zfar();
        float depth = zfar - znear;


        // draw
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        for(auto & mesh : meshes) {
            mesh->draw(param, param.depthShader, camera(), view, projection, vpInv, Identity());
        }

        // reset fb
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void generateMipmaps() {
        int tw = param.occWidth;
        int th = param.occHeight;
        for(int i = 0; i < lvl; i++) {
            tw = std::max(tw / 2, 4);
            th = std::max(th / 2, 4);
            GLuint shader = param.mipmapShader;
            if(i == 0) {
                glUseProgram(param.mipmapInitShader);
                shader = param.mipmapInitShader;
                Scene::setDepthSampler(param.occTexture, 0);
            }
            else {
                glUseProgram(param.mipmapShader);
                shader = param.mipmapShader;
                Scene::setMipmapTexture(param.occMipmapTexture, 0, i - 1, GL_READ_ONLY);
            }
            Scene::setMipmapTexture(param.occMipmapTexture, 1, i, GL_WRITE_ONLY, tw, th);

            program_uniform(shader, "w", tw);
            program_uniform(shader, "h", th);
            glUniform1i(glGetUniformLocation(shader, "input"), 0);
            glUniform1i(glGetUniformLocation(shader, "output"), 1);

            int threads[3];
            glGetProgramiv(shader, GL_COMPUTE_WORK_GROUP_SIZE, threads);

            int nbx = std::ceil((float)tw / threads[0]);
            int nby = std::ceil((float)th / threads[1]);
            glDispatchCompute(nbx, nby, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }

    
    // dessiner une nouvelle image
    int render( )
    {
        param.drawCall = 0;

        // if(param.generateShadow) {
        //     generateDepth();
        // }

        glUseProgram(param.lightShader);
        program_uniform(param.lightShader, "lightDir", param.lightDir);
        program_uniform(param.lightShader, "camera", camera().position());
        program_uniform(param.lightShader, "lightColor", param.lightColor);
        program_uniform(param.lightShader, "lightPower", param.lightIntensity);
        program_use_texture(param.lightShader, "shadowMap", 2, param.shadowTexture);
        program_uniform(param.lightShader, "bias", param.shadowBias);
        program_uniform(param.lightShader, "shadowFactor", param.shadowFactor);

        Transform view = camera().view();
        Transform projection = Perspective(45, 1900.0 / 1000.0, camera().znear(), camera().zfar());
        //camera().projection(window_width(), window_height(), 45);
        Transform vpInv = (projection * view).inverse();

        

        generateDepthMap(view, projection, vpInv);

        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int id = 0;
        
        if(param.wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        for(auto & mesh : meshes) {
            glUseProgram(param.lightShader);
            param.drawCall += mesh->draw(param, param.lightShader, camera(), view, projection, vpInv, Identity());
        }
        
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
    Transform shadowvp = Viewport(1, 1);
    

    DUI UI;
    Transform projectionLight;
    Transform viewLight;
    Transform vpInvLight;
    Mesh light;
    int lvl = 0;

    float tick = 0;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    
    return 0;
}
