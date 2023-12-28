#pragma once 

#include "mesh.h"
#include "vec.h"
#include "color.h"
struct Param
{
    // Light parameters
    float lightIntensity = 1.0;
    Color lightColor = White();
    Vector lightPos = Vector(0, 100, 100);
    Vector lightDir = normalize(Vector(0, 1, 1));;

    // Draw parameters
    bool wireframe = false;
    int drawCall = 0;

    // Shadow map Texture parameters
    GLuint shadowTexture = 0;
    bool saveTexture = false;
    int fbWidth = 4096;
    int fbHeight = 4096;
    bool generateShadow = true;
    
    // Shadow map FrameBuffer parameters
    GLuint shadowFrameBuffer = 0;
    float shadowFactor = 1.f;
    int orthoSize = 20;
    float shadowBias = 0.001f;

    // Occlusion map Texture parameters
    GLuint occTexture = 0;
    bool occSaveTexture = false;
    int occWidth = 1024;
    int occHeight = 1024;

    // Occ map FrameBuffer parameters
    GLuint occFrameBuffer = 0;


    GLuint occlusionShader;
    GLuint frustumShader;
};
