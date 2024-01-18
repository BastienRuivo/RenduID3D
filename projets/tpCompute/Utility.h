#ifndef _UTILITY_H
#define _UTILITY_H

#include "wavefront.h"
#include "uniforms.h"
#include "texture.h"
#include "orbiter.h"
#include "draw.h"        
#include "app_camera.h"        // classe Application a deriver

#include <array>

// utilitaire. creation d'une grille / repere.

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

struct frustum
{
    const std::array<Point, 8> frustum = boxify(Point(-1, -1, -1), Point(1, 1, 1));
    bool isInFrustum(const Point & min, const Point & max, const Transform & mvp, const Transform & vpInv) {

        auto cube = boxify(min, max);
        
        // proective space
        int out[6] = {0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < 8; i++)
        {
            vec4 p = mvp(cube[i]);
            if(p.x < -p.w) out[0]++;
            else if(p.x > p.w) out[1]++;
            if(p.y < -p.w) out[2]++;
            else if(p.y > p.w) out[3]++;
            if(p.z < -p.w) out[4]++;
            else if(p.z > p.w) out[5]++;
        }

        for(int i = 0; i < 6; i++) {
            if(out[i] == 8) return false;
            out[i] = 0;
        }


        //std::cout<<"step 1 :: " << out[0] << " " << out[1] << " " << out[2] << " " << out[3] << " " << out[4] << " " << out[5] << std::endl;

        for (size_t i = 0; i < 8; i++)
        {
            vec4 p = vpInv(frustum[i]);
            if(p.x < -p.w) out[0]++;
            else if(p.x > p.w) out[1]++;
            if(p.y < -p.w) out[2]++;
            else if(p.y > p.w) out[3]++;
            if(p.z < -p.w) out[4]++;
            else if(p.z > p.w) out[5]++;
        }
        //std::cout<<"step 2 :: " << out[0] << " " << out[1] << " " << out[2] << " " << out[3] << " " << out[4] << " " << out[5] << std::endl;
        for(int i = 0; i < 6; i++) {
            if(out[i] == 8) return false;
        }
        return true;
           
    }

    
};

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
    
    bbox.color(Blue());
    bbox.vertex(box[0]);
    bbox.vertex(box[1]);

    bbox.color(Blue());
    bbox.vertex(box[0]);
    bbox.vertex(box[2]);

    bbox.color(Blue());
    bbox.vertex(box[0]);
    bbox.vertex(box[4]);

    bbox.color(Blue());
    bbox.vertex(box[7]);
    bbox.vertex(box[6]);

    bbox.color(Blue());
    bbox.vertex(box[7]);
    bbox.vertex(box[5]);

    bbox.color(Blue());
    bbox.vertex(box[7]);
    bbox.vertex(box[3]);
    
    bbox.color(Blue());
    bbox.vertex(box[1]);
    bbox.vertex(box[3]);

    bbox.color(Blue());
    bbox.vertex(box[1]);
    bbox.vertex(box[5]);

    bbox.color(Blue());
    bbox.vertex(box[2]);
    bbox.vertex(box[3]);

    bbox.color(Blue());
    bbox.vertex(box[2]);
    bbox.vertex(box[6]);

    bbox.color(Blue());
    bbox.vertex(box[4]);
    bbox.vertex(box[5]);

    bbox.color(Blue());
    bbox.vertex(box[4]);
    bbox.vertex(box[6]);

    return bbox;
}

namespace Scene {
    void Init(Color color) {
        // etat openGL par defaut
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glDepthRange(0, 1.0);

        glClearColor(color.r, color.g, color.b, color.a);        // couleur par defaut de la fenetre
        glClearDepth(1.f);                          // profondeur par defaut
    }
    void InitShader(const std::string & path, GLuint & shader) {
        shader = read_program(path.c_str());
        if(program_print_errors(shader)) {
            exit(0);
        }
    }
    void InitFramebuffer(GLuint & framebuffer, GLuint & texture, GLenum attachement) {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachement, texture, 0);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout<<"ERROR FRAMEBUFFER"<<std::endl;
            exit(1);
        } 
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint makeMipmapTexture(size_t w, size_t h) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glGenerateMipmap(GL_TEXTURE_2D);

        return texture;
    }

    void setDepthSampler(GLuint texture, int unit = 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    void setMipmapTexture(GLuint texture, int unit, int lvl, GLenum access, int tw = -1, int th = -1) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
        if(tw != -1) {
            glTexImage2D(GL_TEXTURE_2D, lvl, GL_R32F, tw, th, 0, GL_RED, GL_FLOAT, nullptr);
        }
        glBindImageTexture(unit, texture, lvl, GL_FALSE, 0, access, GL_R32F);
    }
}


#endif