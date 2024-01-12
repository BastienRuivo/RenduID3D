
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.


#include "wavefront.h"
#include "uniforms.h"
#include "texture.h"
#include "DMesh.h"
#include "Param.h"
#include "DTexture.h"
#include "DUI.h"

#include "orbiter.h"
#include "draw.h"        
#include "app_camera.h"        // classe Application a deriver

#include <omp.h>
#include <array>

#include <chrono>


const float speed= 0.075f;        // facteur d'avancement automatique
Mesh make_grid_camera( )
{
    Mesh camera= Mesh(GL_LINES);
    
    // pyramide de vision de la camera
    camera.color(Yellow());
    camera.vertex(0,0,0);
    camera.vertex(-0.5, -0.5, -1);
    camera.vertex(0,0,0);
    camera.vertex(-0.5, 0.5, -1);
    camera.vertex(0,0,0);
    camera.vertex(0.5, 0.5, -1);
    camera.vertex(0,0,0);
    camera.vertex(0.5, -0.5, -1);
    
    camera.vertex(-0.5, -0.5, -1);
    camera.vertex(-0.5, 0.5, -1);
 
    camera.vertex(-0.5, 0.5, -1);
    camera.vertex(0.5, 0.5, -1);
 
    camera.vertex(0.5, 0.5, -1);
    camera.vertex(0.5, -0.5, -1);
    
    camera.vertex(0.5, -0.5, -1);
    camera.vertex(-0.5, -0.5, -1);
    
    // axes XYZ
    
    return camera;
}
std::array<Point, 8> boxify(const Point & min, const Point & max) {
    return {
            Point(min.x, min.y, min.z),
            Point(min.x, min.y, max.z),
            Point(min.x, max.y, min.z),
            Point(min.x, max.y, max.z),
            Point(max.x, min.y, min.z),
            Point(max.x, min.y, max.z),
            Point(max.x, max.y, min.z),
            Point(max.x, max.y, max.z)
        };
}

std::array<vec4, 8> boxify(const Point & min, const Point & max, const Transform & mvp) {
    return {
            mvp(vec4(min.x, min.y, min.z, 1.0)),
            mvp(vec4(min.x, min.y, max.z, 1.0)),
            mvp(vec4(min.x, max.y, min.z, 1.0)),
            mvp(vec4(min.x, max.y, max.z, 1.0)),
            mvp(vec4(max.x, min.y, min.z, 1.0)),
            mvp(vec4(max.x, min.y, max.z, 1.0)),
            mvp(vec4(max.x, max.y, min.z, 1.0)),
            mvp(vec4(max.x, max.y, max.z, 1.0))
        };
}

struct frustum
{
    const std::array<Point, 8> frustum = boxify(Point(-1, -1, -1), Point(1, 1, 1));
    
    bool isInFrustum(const Point & min, const Point & max, const Transform & mvp, const Transform & vpInv, const Transform & model) {

        auto cube = boxify(min, max, mvp);
        
        // proective space
        int out[6] = {0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < 8; i++)
        {
            out[0] = out[0] + (cube[i].x < -cube[i].w);
            out[1] = out[1] + (cube[i].x > cube[i].w);
            out[2] = out[2] + (cube[i].y < -cube[i].w);
            out[3] = out[3] + (cube[i].y > cube[i].w);
            out[4] = out[4] + (cube[i].z < -cube[i].w);
            out[5] = out[5] + (cube[i].z > cube[i].w);
        }

        if(out[0] == 8 || out[1] == 8 || out[2] == 8 || out[3] == 8 || out[4] == 8 || out[5] == 8) return false;

        return true;
    }

    
};

// utilitaire. creation d'une grille / repere.
Mesh make_grid( const int n= 10 )
{
    Mesh grid= Mesh(GL_LINES);
    
    // grille
    grid.color(White());
    for(int x= 0; x < n; x++)
    {
        float px= float(x) - float(n)/2 + .5f;
        grid.vertex(Point(px, 0, - float(n)/2 + .5f)); 
        grid.vertex(Point(px, 0, float(n)/2 - .5f));
    }

    for(int z= 0; z < n; z++)
    {
        float pz= float(z) - float(n)/2 + .5f;
        grid.vertex(Point(- float(n)/2 + .5f, 0, pz)); 
        grid.vertex(Point(float(n)/2 - .5f, 0, pz)); 
    }

    
    // axes XYZ
    grid.color(Red());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(1, .1, 0));
    
    grid.color(Green());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, 1, 0));
    
    grid.color(Blue());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, .1, 1));
    
    glLineWidth(2);
    
    return grid;
}

Mesh make_bbox(const Point & min, const Point & max) {
    auto box = boxify(min, max);
    Mesh bbox(GL_LINES);
    
    bbox.vertex(box[0]);
    bbox.vertex(box[1]);

    bbox.vertex(box[0]);
    bbox.vertex(box[2]);

    bbox.vertex(box[0]);
    bbox.vertex(box[4]);

    bbox.vertex(box[7]);
    bbox.vertex(box[6]);

    bbox.vertex(box[7]);
    bbox.vertex(box[5]);

    bbox.vertex(box[7]);
    bbox.vertex(box[3]);
    

    bbox.vertex(box[1]);
    bbox.vertex(box[3]);

    bbox.vertex(box[1]);
    bbox.vertex(box[5]);

    bbox.vertex(box[2]);
    bbox.vertex(box[3]);

    bbox.vertex(box[2]);
    bbox.vertex(box[6]);

    bbox.vertex(box[4]);
    bbox.vertex(box[5]);

    bbox.vertex(box[4]);
    bbox.vertex(box[6]);

    return bbox;
}
class TP : public AppCamera
{
public:
    
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppCamera(1900, 1000), param(Param(Vector(-15.3,7.22501,3.45), Vector(0, 0, 0), White(), 3.5, 1.0, false)){}

    void readObject(const std::string & path) {
        auto objet = read_indexed_mesh(path.c_str());
        printf("%d materials.\n", objet.materials().count());
        Materials & materials = objet.materials();
       
        for(size_t i = 0; i < materials.filename_count(); i++)
        {
            m_textures.push_back(read_texture(0, materials.filename(i)));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        
        printf("%d textures.\n", materials.filename_count());

        m_groups.push_back(objet.groups());

        for(auto & group : m_groups.back()) {
            objet.bounds(group.first/3, (group.first + group.n)/3, group.min, group.max);
            m_bboxs.push_back(make_bbox(group.min, group.max));
        }

        m_meshes.push_back(objet);
    }

    void readObject(Mesh & objet) {
        printf("%d materials.\n", objet.materials().count());
        Materials & materials = objet.materials();
       
        for(size_t i = 0; i < materials.filename_count(); i++)
        {
            m_textures.push_back(read_texture(0, materials.filename(i)));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        
        printf("%d textures.\n", materials.filename_count());
        
        m_groups.push_back(objet.groups());

        for(auto & group : m_groups.back()) {
            objet.bounds(group.first/3, (group.first + group.n)/3, group.min, group.max);
            m_bboxs.push_back(make_bbox(group.min, group.max));
        }

        m_meshes.push_back(objet);
    }
    
    // creation des objets de l'application
    int init( )
    {
        // decrire un repere / grille 
        m_repere = make_grid(10);

        UI.init(m_window);

        // charge un objet

        
        lightShader = read_program("projets/tp/bf.glsl");
        if(program_print_errors(lightShader)) {
            exit(0);
        }


        shadowShader = read_program("projets/tp/shadow.glsl");
        program_print_errors(shadowShader);
        
        // etat openGL par defaut
        glClearColor(0.4f, 0.4f, 0.75f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        camera().projection(1900, 1000, 45);

        // charge aussi une texture neutre pour les matieres sans texture...
        m_white_texture= read_texture(0, "data/white.jpg");  // !! utiliser une vraie image blanche...




        // readObject("data/cube.obj");
        // m_models.push_back(Scale(10, 0.1, 10));
        readObject("data/bigguy.obj");
        m_models.push_back(Scale(0.1, 0.1, 0.1) * Translation(0, 9.8, 0));

        //readObject("data/exterior.obj");
        //m_models.push_back(Scale(0.1, 0.1, 0.1) * Translation(0, 9.8, 0));

        float w = 5.f;
        camera().lookat(Point(-w, -w, -w), Point(w, w, w));
        param.shadowTexture = make_depth_texture(0, param.fbWidth, param.fbHeight);

        param.frameBuffer = 0;
        glGenFramebuffers(1, &param.frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, param.frameBuffer);

        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, param.shadowTexture, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout<<"ERROR FRAMEBUFFER"<<std::endl;
            return -1;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::cout<<"----INIT DONE---"<<std::endl;


        param.lightPos = Vector(0, 100, 100);
        param.lightDir = normalize(Vector(0, -1, -1));

        return 0;
    }

    void hotReload() {
        glDeleteProgram(lightShader);
        glDeleteProgram(shadowShader);
        lightShader = read_program("projets/tp/bf.glsl");
        shadowShader = read_program("projets/tp/shadow.glsl");
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        // etape 1 : detruire les objets openGL
        m_repere.release();
        for(auto & mesh : m_meshes) {
            mesh.release();
        }
        for(auto & bbox : m_bboxs) {
            bbox.release();
        }
        for(auto & texture : m_textures) {
            glDeleteTextures(1, &texture);
        }
        glDeleteTextures(1, &m_white_texture);
        glDeleteTextures(1, &param.shadowTexture);
        glDeleteFramebuffers(1, &param.frameBuffer);
        glDeleteProgram(lightShader);
        glDeleteProgram(shadowShader);

        return 0;   // pas d'erreur
    }
    
    void doKeyboard() {
    }

    int drawMesh(int meshId, GLuint program, const Transform & view, const Transform & projection, const Transform & vpInv, const Transform & model = Identity()) {
        int draw_call = 0;
        for(int i = 0; i < m_groups[meshId].size(); i++) {
            Transform m = model * m_models[meshId];
            Transform mv = view * m;
            Transform mvp = projection * mv;

            program_uniform(program, "mvpMatrix", mvp);

            if(program == lightShader) {
                program_uniform(program, "modelViewMatrix", mv);
                program_uniform(program, "modelMatrix", m);

                if(param.performCulling && !m_frustum.isInFrustum(m_groups[meshId][i].min, m_groups[meshId][i].max, mvp, vpInv, m)) continue;
                if(m_groups[meshId][i].index >= 0 && m_groups[meshId][i].index < m_meshes[meshId].materials().count()) {
                    const Material& material= m_meshes[meshId].materials()(m_groups[meshId][i].index);
                    program_use_texture(program, "tm_diffuse", 0, material.diffuse_texture != -1? m_textures[material.diffuse_texture] : m_white_texture);
                    program_uniform(program, "has_diffuse", material.diffuse_texture != -1);
                    program_use_texture(program, "tm_emission", 1, material.emission_texture != -1? m_textures[material.emission_texture] : m_white_texture);
                    program_uniform(program, "has_emission", material.emission_texture != -1);
                    program_use_texture(program, "tm_normal", 2, material.normal_texture != -1? m_textures[material.normal_texture] : m_white_texture);
                    program_uniform(program, "has_normal", material.normal_texture != -1);
                    program_use_texture(program, "tm_specular", 3, material.specular_texture != -1? m_textures[material.specular_texture] : m_white_texture);
                    program_uniform(program, "has_specular", material.specular_texture != -1);

                } else {
                    program_uniform(program, "has_diffuse", 0);
                    program_uniform(program, "has_emission", 0);
                    program_uniform(program, "has_normal", 0);
                    program_uniform(program, "has_specular", 0);
                }
                program_use_texture(program, "shadowMap", 4, param.shadowTexture);
                program_uniform(program, "shadowFactor", param.shadowFactor);
            }
           
            m_meshes[meshId].draw(m_groups[meshId][i].first, m_groups[meshId][i].n, program, true, true, true, m_meshes[meshId].has_color(), m_meshes[meshId].has_material_index());
            if(param.drawBbox) draw(m_bboxs[i], m, camera());
            draw_call++;
        }
        return draw_call;
    }

    void saveTexture(GLuint & texture, size_t w, size_t h, size_t nbChannels, GLuint format, const std::string & path) {
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<unsigned char> pixels(w * h * nbChannels, 0);
        glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, pixels.data());
        Image im(w, h, Black());
        if(nbChannels == 1) {
            for(int i = 0; i < param.fbWidth * param.fbHeight; i++) {
                Color c = Color(pixels[i]/255.0, pixels[i]/255.0, pixels[i]/255.0);
                im(i) = c;
            }
        } else {
            for(int i = 0; i < param.fbWidth * param.fbHeight; i++) {
                Color c = Color(pixels[i*3]/255.0, pixels[i*3+1]/255.0, pixels[i*3+2]/255.0);
                im(i) = c;
            }
        }
        write_image(im, path.c_str());
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        param.drawCall = 0;
        //shadow draw
        if(true || param.generateShadow) {
            projectionLight = Ortho(-param.orthoSize, param.orthoSize, -param.orthoSize, param.orthoSize, -param.orthoSize, param.orthoSize);
            viewLight = Lookat(Point(param.lightPos), Point(param.lightPos) + Point(param.lightDir), Vector(0, 1, 0));
            vpInvLight = (projectionLight * viewLight).inverse();
            glBindFramebuffer(GL_FRAMEBUFFER, param.frameBuffer);


            glViewport(0, 0, param.fbWidth, param.fbHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(shadowShader);
            for(int mId = 0; mId < m_meshes.size(); mId++) {
                param.drawCall += drawMesh(mId, shadowShader, viewLight, projectionLight, vpInvLight, Translation(param.lightPos));
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            param.generateShadow = false;
        }
        
        auto begin = std::chrono::high_resolution_clock::now();
        glUseProgram(lightShader);
        program_uniform(lightShader, "source", param.lightPos);
        program_uniform(lightShader, "lightDir", param.lightDir);
        program_uniform(lightShader, "camera", camera().position());
        program_uniform(lightShader, "lightColor", param.lightColor);
        program_uniform(lightShader, "lightPower", param.lightIntensity);
        program_uniform(lightShader, "bias", param.shadowBias);
        program_uniform(lightShader, "F0", param.F0);
        program_uniform(lightShader, "useNormalMapping", (int)param.useNormalMapping);
        program_uniform(lightShader, "mvpLightMatrix", shadowvp * projectionLight * viewLight * Translation(param.lightPos));

        glViewport(0, 0, window_width(), window_height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Transform view = camera().view();
        Transform projection = camera().projection(window_width(), window_height(), 45);
        Transform vpInv = (projection * view).inverse();
        for(int mId = 0; mId < m_meshes.size(); mId++) {
            param.drawCall += drawMesh(mId, lightShader, view, projection, vpInv);
        }

        auto end = std::chrono::high_resolution_clock::now();
        // get elapsed time in milliseconds

        auto e = std::chrono::duration_cast<std::chrono::microseconds>(end - begin) / 1000.0;

        param.elapsedSum += e.count();

        param.elapsedCount++;
        

        if(param.elapsedCount > 60) {
            param.elapsedTime = param.elapsedSum / param.elapsedCount;
            param.elapsedCount = 0;
            param.elapsedSum = 0;
        }




        if(param.saveTexture) {
            saveTexture(param.shadowTexture, param.fbWidth, param.fbHeight, 1, GL_DEPTH_COMPONENT, "shadow.png");
            param.saveTexture = false;
        }
        doKeyboard();

        UI.createUIWindow(m_window, "UI");
        UI.paramUI(m_window, param);
        UI.hotReload(lightShader, "projets/tp/bf.glsl", "Light");
        UI.hotReload(shadowShader, "projets/tp/shadow.glsl", "Shadow");
        UI.renderUI();

        return 1;
    }

protected:
    Mesh m_repere;
    std::vector<Mesh> m_meshes; // liste des objets a afficher


    Param param;
    std::vector<std::vector<TriangleGroup>> m_groups;
    std::vector<Transform> m_models;
    std::vector<Mesh> m_bboxs;
    std::vector<GLuint> m_textures;
    GLuint m_white_texture;
    

    frustum m_frustum;

    Transform shadowvp = Viewport(1, 1);
    

    DUI UI;
    
    GLuint lightShader;
    GLuint shadowShader;

    Transform projectionLight;
    Transform viewLight;
    Transform vpInvLight;

    bool isWireframe = false;

    float tick = 0;
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}
