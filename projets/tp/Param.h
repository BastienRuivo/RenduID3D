#pragma once 

#include "vec.h"
#include "color.h"
struct Param
{
    float lightIntensity;
    Color lightColor;
    Vector lightPos;
    Vector lightDir;
    bool wireframe;
    int drawCall = 0;

    float F0 = 0.5f;
    float alpha = 1.0f;
    
    float shadowFactor = 0.5f;
    bool saveTexture = false;
    int orthoSize = 11;
    uint shadowTexture = 0;
    int fbWidth = 4096;
    int fbHeight = 4096;
    bool generateShadow = true;
    float shadowBias = 0.001f;
    uint frameBuffer = 0;

    bool useNormalMapping = true;
    bool drawBbox = false;
    bool performCulling = true;

    double elapsedTime = 0.0;
    double elapsedSum = 0.0;
    int elapsedCount = 0;

    Param(const Vector & lightPos, const Vector & lightDir, const Color & lightColor, float lightIntensity, float shininess, bool wireframe) : lightPos(lightPos), lightDir(lightDir), lightColor(lightColor), lightIntensity(lightIntensity), wireframe(wireframe) {}
};
