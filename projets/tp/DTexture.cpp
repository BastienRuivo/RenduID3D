#include "DTexture.h"

DTexture::DTexture(const ImageData & image, const int unit)
{
        
    glGenTextures(1, &this->texture);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    GLenum format;
    switch(image.channels)
    {
        case 1: format= GL_RED; break;
        case 2: format= GL_RG; break;
        case 3: format= GL_RGB; break;
        case 4: format= GL_RGBA; break;
        default: format= GL_RGBA; 
    }
    
    GLenum type;
    switch(image.size)
    {
        case 1: type= GL_UNSIGNED_BYTE; break;  
        case 4: type= GL_FLOAT; break;
        default: type= GL_UNSIGNED_BYTE;
    }


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, type, image.data());    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glGenerateMipmap(GL_TEXTURE_2D);

    this->width = image.width;
    this->height = image.height;
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DTexture::use(GLuint program, const std::string & uniform, const int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glUniform1i(glGetUniformLocation(program, uniform.c_str()), unit);
}

DTexture::~DTexture()
{
}