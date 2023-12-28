#pragma once

#include "image_io.h"
#include "glcore.h"
#include "string"

class DTexture
{
private:
    GLuint texture;
    int width, height;
public:
    DTexture() : texture(0), width(0), height(0) {}
    DTexture(const ImageData & image, const int unit = 0);
    void use(GLuint program, const std::string & uniform, const int unit);
    ~DTexture();
};


