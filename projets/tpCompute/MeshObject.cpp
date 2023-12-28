#include "MeshObject.h"


void MeshObject::addInstance(const Transform & t, int groupIndex) {
    struct GLSL::IndirectParam p;
    struct GLSL::Object o;
    
    if(groupIndex == -1) {
        p = GLSL::makeIndirectParam(mesh.index_count(), 0);
        o = GLSL::makeObject(pmin, pmax, mesh.index_count());
    } else {
        auto & g = mesh.groups()[groupIndex];
        p = GLSL::makeIndirectParam(g);
        o = GLSL::makeObject(g);
    }
    indirectParams.push_back(p);
    objects.push_back(o);
}

MeshObject::MeshObject(const std::string & filename) {
    mesh = read_indexed_mesh(filename.c_str());
    mesh.create_buffers(mesh.has_texcoord(), mesh.has_normal(), mesh.has_color(), mesh.has_material_index());
    mesh.bounds(pmin, pmax);
    auto & materials = mesh.materials();

    std::vector<TriangleGroup> groups = mesh.groups();
    for(auto & g : groups) {
        struct GLSL::IndirectParam p = GLSL::makeIndirectParam(g);
        indirectParams.push_back(p);
        struct GLSL::Object o = GLSL::makeObject(g);    
        objects.push_back(o);
    }

    for(auto & m : materials.materials) {
        GLSL::MaterialGPU mat = {
            m.diffuse,
            m.ns,
            m.diffuse_texture,
            m.ns_texture
        };
        materialsGPU.push_back(mat);
    }


    if(materials.count() == 0) {
        std::cout << "No material found" << std::endl;
        auto m = materials.default_material();
        m.ns = 2.5;
        GLSL::MaterialGPU mat = {
            m.diffuse,
            m.ns,
            m.diffuse_texture,
            m.ns_texture
        };
        materialsGPU.push_back(mat);
    }

    std::cout<<mesh.m_triangle_materials[0]<<std::endl;

    // glGenBuffers(1, &indirectBuffer);
    // glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    // glBufferData(GL_DRAW_INDIRECT_BUFFER, indirectParams.size() * sizeof(GLSL::IndirectParam), indirectParams.data(), GL_STATIC_READ);
    InitBuffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW, drawCountBuffer, sizeof(GLuint), nullptr);
    InitBuffer(GL_DRAW_INDIRECT_BUFFER, GL_DYNAMIC_DRAW, indirectBuffer, indirectParams.size() * sizeof(GLSL::IndirectParam), indirectParams.data());
    InitBuffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW, objectBuffer, objects.size() * sizeof(GLSL::Object), objects.data());
    InitBuffer(GL_SHADER_STORAGE_BUFFER, GL_STATIC_READ, materialBuffer, materialsGPU.size() * sizeof(GLSL::MaterialGPU), materialsGPU.data());
    InitTextureArray(textureArray, materials);
}

MeshObject::~MeshObject() {

    std::cout<<"Mesh object release "<<std::endl;
    glDeleteBuffers(1, &indirectBuffer);
    glDeleteBuffers(1, &objectBuffer);
    glDeleteBuffers(1, &materialBuffer);
    glDeleteBuffers(1, &drawCountBuffer);
    glDeleteTextures(1, &textureArray);

    mesh.release();
}

void MeshObject::InitBuffer(GLenum primitive, GLenum dataMode, GLuint & buffer, size_t size, const void * data) {
    glGenBuffers(1, &buffer);
    glBindBuffer(primitive, buffer);
    glBufferData(primitive, size, data, dataMode);
    glBindBuffer(primitive, 0);
}

void MeshObject::InitTextureArray(GLuint & textureBuffer, Materials & materials) {
    if(materials.filename_count() == 0) {
        std::cout << "No texture found" << std::endl;
        return;
    }
    glGenTextures(1, &textureBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureBuffer);
    int texW, texH;
    texW = texH = 1024;
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, texW, texH, materials.filename_count());
    for(size_t i = 0; i < materials.filename_count(); i++) {
        ImageData image= read_image_data(materials.filename(i));
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, texW, texH, 1, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    }
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

int MeshObject::FrustumCulling(const Param & param, const Transform & mvp, const Transform & vpInv) {
    glUseProgram(param.frustumShader);
    BindObject(0);
    BindIndirect(1);
    BindDrawCount(2);
    program_uniform(param.frustumShader, "mvpMatrix", mvp);
    program_uniform(param.frustumShader, "vpInverseMatrix", vpInv);

    int nb = 256;
    int groups = (size() + nb) / nb;
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read draw count
    int draw;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawCountBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), &draw);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return draw;
}

int MeshObject::draw(const Param & param, GLuint program, const Transform & view, const Transform & projection, const Transform & vpInv, const Transform & mm) {
    auto m = mm * model;
    Transform mv = view * m;
    Transform mvp = projection * mv;
    
    // Handle frustum culling
    OcclusionCulling(param, mvp, vpInv);
    int draw = FrustumCulling(param, mvp, vpInv);

    glBindVertexArray(mesh.m_vao);
    glUseProgram(program);
    BindTexture(0);
    BindMaterial(1);
    program_uniform(program, "modelMatrix", m);
    program_uniform(program, "mvpMatrix", mvp);


    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, indirectParams.size(), 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);

    //glDrawElements(GL_TRIANGLES, mesh.index_count(), GL_UNSIGNED_INT, 0);

    return draw;
}

void MeshObject::OcclusionCulling(const Param & param, const Transform & mvp, const Transform & vpInv) {
    glUseProgram(param.occlusionShader);
    BindObject(0);
    BindIndirect(1);
    BindDrawCount(2);
    program_uniform(param.occlusionShader, "mvpMatrix", mvp);

    int nb = 256;
    int groups = (size() + nb) / nb;
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
