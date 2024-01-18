
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.
#include "MeshObject.h"
#include "DUI.h"

#include <omp.h>
#include <array>
#include "Utility.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
        //Scene::InitShader("projets/tpCompute/culling.glsl", param.cullingShader);
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
        
        MeshObject * cube;
        // cube = new MeshObject("data/cube.obj");
        // cube->model = Translation(0, 0, -5) * Scale(50, 1, 50);
        // meshes.push_back(cube);

        // cube = new MeshObject("data/cube.obj");
        // cube->model = Translation(0, 7.5, 8) * Scale(15, 15, 2);
        // meshes.push_back(cube);

        // cube = new MeshObject("data/cube.obj");
        // cube->model = Translation(0, 4, -5) * Scale(2, 2, 2);
        // meshes.push_back(cube);

        auto bigguy = new MeshObject("data/export.obj");
        bigguy->model = Scale(0.1, 0.1, 0.1);
        meshes.push_back(bigguy);
        

        Point pmin, pmax;
        meshes[0]->mesh.bounds(pmin, pmax);
        pmin = meshes[0]->model(pmin);
        pmax = meshes[0]->model(pmax);
        camera().lookat(pmin, pmax);


        param.shadowTexture = make_depth_texture(0, param.fbWidth, param.fbHeight);
        //param.occTexture = make_float_texture(0, param.occWidth, param.occHeight);
        param.occTexture = make_depth_texture(0, param.occWidth, param.occHeight);
        int width = param.occWidth / 2;
        int height = param.occHeight / 2;
        param.occMipmapTexture = Scene::makeMipmapTexture(width, height);

        param.lvl = 1;
        while(width > param.minw && height > param.minh) {
            width = std::max(width / 2, param.minw);
            height = std::max(height / 2, param.minh);
            param.lvl++;
        }
        Scene::InitFramebuffer(param.occFrameBuffer, param.occTexture, GL_DEPTH_ATTACHMENT);
        Scene::InitFramebuffer(param.shadowFrameBuffer, param.shadowTexture, GL_DEPTH_ATTACHMENT);

        for(auto & mesh : meshes) {
            auto groups = mesh->mesh.groups();
            for(auto & group : groups) {
                //draw bbox
                Mesh bbox = make_bbox(group.min, group.max);
                bboxs.push_back(bbox);
                bboxsModel.push_back(mesh->model);
            }
        }


        std::cout<<"----INIT DONE---"<<std::endl;

        return 0;
    }

    void hotReload() {
       
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        for(auto & mesh : meshes) {
            delete mesh;
        }
        
        glDeleteProgram(param.lightShader);
        glDeleteProgram(param.frustumShader);
        glDeleteProgram(param.occlusionShader);
        glDeleteProgram(param.depthShader);
        glDeleteProgram(param.mipmapShader);
        glDeleteProgram(param.mipmapInitShader);
        //glDeleteProgram(param.cullingShader);

        glDeleteTextures(1, &param.shadowTexture);
        glDeleteTextures(1, &param.occTexture);
        glDeleteTextures(1, &param.occMipmapTexture);
        glDeleteFramebuffers(1, &param.shadowFrameBuffer);
        glDeleteFramebuffers(1, &param.occFrameBuffer);
        
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
            param.drawCall += mesh->draw(param, param.depthShader, viewLight, projectionLight, vpInvLight, Translation(param.lightPos), true);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        param.generateShadow = false;
    }

    void generateDepthMap(const Transform & view, const Transform & projection, const Transform & vpInv, bool performCulling) {
        glBindFramebuffer(GL_FRAMEBUFFER, param.occFrameBuffer);
        glViewport(0, 0, param.occWidth, param.occHeight);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // draw
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        for(auto & mesh : meshes) {
            mesh->draw(param, param.depthShader, view, projection, vpInv, Identity(), performCulling);
        }

        // reset fb
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void generateMipmaps() {
        int tw = param.occWidth;
        int th = param.occHeight;
        for(int i = 0; i < param.lvl; i++) {
            tw = std::max(tw / 2, param.minw);
            th = std::max(th / 2, param.minh);
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


            // write to image

            // std::vector<float> data(tw * th);
            // glBindTexture(GL_TEXTURE_2D, param.occMipmapTexture);
            // glGetTexImage(GL_TEXTURE_2D, i, GL_RED, GL_FLOAT, data.data());
            // glBindTexture(GL_TEXTURE_2D, 0);
            // Image img(tw, th);

            // for(int j = 0; j < tw * th; j++) {
            //     img(j) = Color(data[j], data[j], data[j]);
            // }

            // write_image(img, ("mipmap" + std::to_string(i) + ".png").c_str());
        }
        //exit(0);
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
        Transform projection = camera().projection(window_width(), window_height(), 45);
        Transform vpInv = (projection * view).inverse();

        
        int id = 0;
        generateDepthMap(view, projection, vpInv, false);
        generateMipmaps();

        glViewport(0, 0, window_width(), window_height());
        glClearDepth(1.0f);
        glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        
        if(param.wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }


        for(auto & mesh : meshes) {
            glUseProgram(param.lightShader);
            param.drawCall += mesh->draw(param, param.lightShader, view, projection, vpInv, Identity(), true);
        }

        


        
        draw(light, Translation(param.lightPos), camera());

        

        
        if(param.showBbox) {
            for(int i = 0; i < bboxs.size(); i++) {
                draw(bboxs[i], bboxsModel[i], camera());
            }
        }

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

    std::vector<Mesh> bboxs;
    std::vector<Transform> bboxsModel;
    

    DUI UI;
    Transform projectionLight;
    Transform viewLight;
    Transform vpInvLight;
    Mesh light;

    float tick = 0;
};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    
    return 0;
}
