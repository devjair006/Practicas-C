#ifndef OBJ_MESH_H
#define OBJ_MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

#include "obj_loader.h"

inline bool loadOBJMesh(const char* path, unsigned int& vao, unsigned int& vbo, int& vertexCount) {
    std::vector<float> vertices;
    if (!loadOBJ(path, vertices)) {
        return false;
    }

    vertexCount = static_cast<int>(vertices.size() / 11);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    return true;
}

#endif
