#ifndef GLTF_MODEL_H
#define GLTF_MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <iostream>

// #include "stb_image.h" // Removed because main.cpp already defines it and caused macro redefinitions

struct GLTFTexture {
    unsigned int id;
    std::string type;
};

struct GLTFVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Color;
};

class GLTFMesh {
public:
    std::vector<GLTFVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<GLTFTexture> textures;
    unsigned int VAO;
    
    GLTFMesh(std::vector<GLTFVertex> vertices, std::vector<unsigned int> indices, std::vector<GLTFTexture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        setupMesh();
    }

    void Draw(unsigned int shaderProgram, int solidColorLoc) {
        if (textures.size() > 0) {
            glUniform1i(solidColorLoc, 0); // Usar texturas
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
        } else {
            glUniform1i(solidColorLoc, 1); // Usar color plano si no hay textura
        }
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
private:
    unsigned int VBO, EBO;
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLTFVertex), &vertices[0], GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLTFVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLTFVertex), (void*)offsetof(GLTFVertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLTFVertex), (void*)offsetof(GLTFVertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GLTFVertex), (void*)offsetof(GLTFVertex, Color));
        
        glBindVertexArray(0);
    }
};

class GLTFModel {
public:
    std::vector<GLTFMesh> meshes;
    
    GLTFModel(std::string path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram, int solidColorLoc) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram, solidColorLoc);
    }

private:
    void loadModel(std::string path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PopulateArmatureData);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode *node, const aiScene *scene) {
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    GLTFMesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<GLTFVertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<GLTFTexture> textures;
        
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            GLTFVertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals()) vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }

        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<GLTFTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            
            // Si Blender exportó como BaseColor (PBR):
            if (textures.empty()) {
                std::vector<GLTFTexture> baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
                textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());
            }
        }
        return GLTFMesh(vertices, indices, textures);
    }

    std::vector<GLTFTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene *scene) {
        std::vector<GLTFTexture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
            if (embeddedTexture) {
                GLTFTexture texture;
                texture.id = loadEmbeddedTexture(embeddedTexture);
                texture.type = typeName;
                textures.push_back(texture);
            } else {
                std::cout << "Advertencia: El material apunta a textura externa (" << str.C_Str() << ") no soportado temporalmente.\n";
            }
        }
        return textures;
    }

    unsigned int loadEmbeddedTexture(const aiTexture* embeddedTexture) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        
        int width, height, nrComponents;
        unsigned char *data = nullptr;
        
        if (embeddedTexture->mHeight == 0) {
            // Textura comprimida (PNG/JPG embebido)
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, &width, &height, &nrComponents, 4);
        } else {
            // Raw data
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth * embeddedTexture->mHeight * 4, &width, &height, &nrComponents, 4);
        }
        
        if (data) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            stbi_image_free(data);
        } else {
            std::cout << "Fallo al cargar textura embebida del GLB." << std::endl;
        }
        return textureID;
    }
};

#endif