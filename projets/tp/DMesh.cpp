#include "DMesh.h"

Transform DMesh::getModel() const {
    return (Translation(T) *
            RotationX(R.x * 180 / M_PI) * 
            RotationY(R.y * 180 / M_PI) * 
            RotationZ(R.z * 180 / M_PI) * 
            Scale(S.x, S.y, S.z) * Identity());
}

DMesh::DMesh(const Mesh & m, const Vector & t, const Vector & r, const Vector & s) : VAO(0), VBO(0), T(t), R(r), S(s) {
    vertexCount = m.vertex_count();
    std::cout<<"Initializing dmesh from gkit mesh"<<std::endl;
    if(!(m.vertex_buffer_size())) {
        printf("LocalMesh::init error: mesh has no vertex buffer\n");
        return;
    }

    materials.resize(8, White());

    for(int i= 0; i < m.materials().count(); i++) {
        materials[i] = m.materials().material(i).diffuse;
    }


    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    int size = m.vertex_buffer_size() + m.has_texcoord() * m.texcoord_buffer_size() + m.has_normal() * m.normal_buffer_size() + m.has_material_index() * m.vertex_count() * sizeof(uint);
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW); // Utilisation par draw, sans modifications

    size = 0;

    vertexCount = m.vertex_count();

    vertices.clear();
    vertices.insert(vertices.end(), m.m_positions.begin(), m.m_positions.end());
    std::cout<<"vertices size: "<<vertices.size()<<std::endl;
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float) * 3, vertices.data());

    size = vertices.size() * sizeof(float) * 3;

    if(m.has_texcoord()) {
        uvs.clear();
        uvs.insert(uvs.end(), m.m_texcoords.begin(), m.m_texcoords.end());
        std::cout<<"uvs size: "<<uvs.size()<<std::endl;
        glBufferSubData(GL_ARRAY_BUFFER, size, uvs.size() * sizeof(float) * 2, uvs.data());
        size += uvs.size() * sizeof(float) * 2;
    }

    if(m.has_normal()) {
        normals.clear();
        normals.insert(normals.end(), m.m_normals.begin(), m.m_normals.end());
        std::cout<<"normals size: "<<normals.size()<<std::endl;
        glBufferSubData(GL_ARRAY_BUFFER, size, normals.size() * sizeof(float) * 3, normals.data());
        size += normals.size() * sizeof(float) * 3;
    }

    if(m.has_color()) {
        colors.clear();
        colors.insert(colors.end(), m.m_colors.begin(), m.m_colors.end());
        std::cout<<"colors size: "<<colors.size()<<std::endl;
        glBufferSubData(GL_ARRAY_BUFFER, size, colors.size() * sizeof(float) * 4, colors.data());
        size += colors.size() * sizeof(float) * 4;
    }

    if(m.has_material_index()) {
        this->materialsId.clear();
        this->materialsId.resize(m.vertex_count());
        for(int triangle_id= 0; triangle_id < m.triangle_count(); triangle_id++)
        {
            int material_id = m.triangle_material_index(triangle_id);
            unsigned a= triangle_id*3;
            unsigned b= triangle_id*3 +1;
            unsigned c= triangle_id*3 +2;
            
            materialsId[a]= material_id;
            materialsId[b]= material_id;
            materialsId[c]= material_id;
        }
        std::cout<<"materialsId size: "<<materialsId.size()<<std::endl;
        glBufferSubData(GL_ARRAY_BUFFER, size, materialsId.size() * sizeof(uint), materialsId.data());
        size += materialsId.size() * sizeof(uint);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    size = m.vertex_buffer_size();

    if(m.has_texcoord()) {
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)size);
        glEnableVertexAttribArray(1);
        size += m.texcoord_buffer_size();
    }

    if(m.has_normal()) {
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)size);
        glEnableVertexAttribArray(2);
        size += m.normal_buffer_size();
    }

    if(m.has_color()) {
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)size);
        glEnableVertexAttribArray(3);
        size += m.color_buffer_size();
    }

    if(m.has_material_index()) {
        glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, 0, (void*)size);
        glEnableVertexAttribArray(4);
        size += m.vertex_count() * sizeof(uint);
    }

    glBindVertexArray(0);


    // etat openGL par defaut
    glClearColor(0.2f, 0.2f, 0.2f, 1.f); // couleur par defaut de la fenetre

    glClearDepth(1.f);       // profondeur par defaut
    glDepthFunc(GL_LESS);    // ztest, conserver l'intersection la plus proche de la camera
    glEnable(GL_DEPTH_TEST); // activer le ztest

    std::cout<<"DMesh initialized"<<std::endl;
}

DMesh::DMesh(const DMesh & mesh) : VAO(mesh.VAO), VBO(mesh.VBO), vertices(mesh.vertices), normals(mesh.normals), uvs(mesh.uvs), colors(mesh.colors), materialsId(mesh.materialsId), materials(mesh.materials), vertexCount(mesh.vertexCount), T(mesh.T), R(mesh.R), S(mesh.S) {
    std::cout<<"---------- copy dmesh -------------"<<std::endl;
}

DMesh & DMesh::operator=(const DMesh & mesh) {
    std::cout<<"---------- assign dmesh -------------"<<std::endl;
    VAO = mesh.VAO;
    VBO = mesh.VBO;
    vertices = mesh.vertices;
    normals = mesh.normals;
    uvs = mesh.uvs;
    colors = mesh.colors;
    materialsId = mesh.materialsId;
    materials = mesh.materials;
    vertexCount = mesh.vertexCount;
    T = mesh.T;
    R = mesh.R;
    S = mesh.S;
    return *this;
}

void DMesh::setUniforms(GLuint shaderProg, Orbiter & cam, const Param & param) const {
    Transform model = getModel();

    // get location of uniform variables from the list bellow
    GLint mvpMatrix = glGetUniformLocation(shaderProg, "mvpMatrix");
    GLint modelMatrix = glGetUniformLocation(shaderProg, "modelMatrix");
    GLint source = glGetUniformLocation(shaderProg, "source");
    GLint camera = glGetUniformLocation(shaderProg, "camera");
    GLint lightColor = glGetUniformLocation(shaderProg, "lightColor");
    GLint materials = glGetUniformLocation(shaderProg, "materials");
    GLint lightPower = glGetUniformLocation(shaderProg, "lightPower");

    // send uniform variables to the shader
    glUniformMatrix4fv(mvpMatrix, 1, GL_TRUE, (cam.projection() * cam.view() * model).data());
    glUniformMatrix4fv(modelMatrix, 1, GL_TRUE, model.data());
    glUniform3fv(source, 1, &param.lightPos.x);
    Point camPos = cam.position();
    glUniform3fv(camera, 1, &camPos.x);
    glUniform4fv(lightColor, 1, &param.lightColor.r);
    glUniform4fv(materials, this->materials.size(), &this->materials[0].r);
    glUniform1f(lightPower, param.lightIntensity);
}

void DMesh::draw(GLuint shaderProg) const {
    glBindVertexArray(VAO);
    glUseProgram(shaderProg);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

DMesh::~DMesh()
{
    std::cout<<"---------- destroy dmesh -------------"<<std::endl;
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}