#pragma once

#include "glcore.h"
#include "mesh.h"
#include <math.h>
#include "Param.h"
#include "orbiter.h"
class DMesh
{
private:
    GLuint VAO, VBO;
    std::vector<vec3> vertices, normals;
    std::vector<vec2> uvs;
    std::vector<vec4> colors;
    std::vector<uint> materialsId;
    std::vector<Color> materials;

    uint vertexCount = 0;

    void initBuffer();
public:
    DMesh() : VAO(0), VBO(0) {}
    DMesh(const Mesh & m, const Vector & translation = Vector(0, 0, 0), const Vector & rotation = Vector(0, 0, 0), const Vector & scale = Vector(1.0, 1.0, 1.0));
    DMesh(const DMesh & mesh);
    DMesh & operator=(const DMesh & mesh);
    Vector T;
    Vector R;
    Vector S;
    void setUniforms(GLuint shaderProg, Orbiter & cam, const Param & param) const;
    Transform getModel() const;
    void draw(GLuint shaderProg) const;

    int getVertexCount() const { return vertexCount; }
    ~DMesh();
};


