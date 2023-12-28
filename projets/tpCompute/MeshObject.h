#ifndef _MESH_OBJECT_H
#define _MESH_OBJECT_H
#include "GLSL.h"

#include "mesh.h"
#include "wavefront.h"
#include "texture.h"
#include "uniforms.h"
#include "Param.h"


#include <chrono>

class MeshObject
{
private:
    Mesh mesh;
    // OpenGL buffers
    GLuint indirectBuffer;
    GLuint objectBuffer;
    GLuint materialBuffer;
    GLuint drawCountBuffer;

    // Texture
    GLuint textureArray;

    // Data
    std::vector<GLSL::Object> objects;
    std::vector<GLSL::IndirectParam> indirectParams;
    std::vector<GLSL::MaterialGPU> materialsGPU;

    Point pmin, pmax;
    

    static void InitBuffer(GLenum primitive, GLenum dataMode, GLuint & buffer, size_t size, const void * data);
    static void InitTextureArray(GLuint & textureBuffer, Materials & materials);

    int FrustumCulling(const Param & param, const Transform & mvp, const Transform & vpInv);
    void OcclusionCulling(const Param & param, const Transform & mvp, const Transform & vpInv);

public:
    Transform model = Identity();
    
    MeshObject() = default;
    MeshObject(const std::string & filename);
    void addInstance(const Transform & t, int groupIndex = -1);

    inline Point min() const { return pmin; }
    inline Point max() const { return pmax; }
    inline size_t size() const { return objects.size(); }

    inline GLuint texture() const { return textureArray; }
    inline GLuint indirect() const { return indirectBuffer; }
    inline GLuint object() const { return objectBuffer; }
    inline GLuint material() const { return materialBuffer; }
    inline GLuint vao() const { return mesh.m_vao; }

    inline void BindBuffer(GLenum type, GLuint buffer, uint index) const{
        glBindBufferBase(type, index, buffer);
    }
    inline void BindObject(unsigned int index) const {
        BindBuffer(GL_SHADER_STORAGE_BUFFER, objectBuffer, index);
    }
    inline void BindMaterial(unsigned int index) const {
        BindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer, index);
    }
    inline void BindIndirect(unsigned int index) const {
        BindBuffer(GL_SHADER_STORAGE_BUFFER, indirectBuffer, index);
    }
    inline void BindTexture(unsigned int index) const {
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    }
    inline void BindIndirectDraw(unsigned int index) const {
        BindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer, index);
    }
    inline void BindDrawCount(unsigned int index) const {
        BindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountBuffer, index);
    }

    int draw(const Param & param, GLuint program, const Transform & view, const Transform & projection, const Transform & vpInv, const Transform & m);

    ~MeshObject();
};
#endif