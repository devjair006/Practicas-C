#ifndef GLTF_MODEL_H
#define GLTF_MODEL_H

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "headers/texture.h"

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

struct GLTFTexture {
    unsigned int id;
    std::string type;
};

struct GLTFVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Color;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];

    GLTFVertex() {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            m_BoneIDs[i] = -1;
            m_Weights[i] = 0.0f;
        }
    }
};

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Declaración global para que main.cpp pueda usarla
unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

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
            glUniform1i(solidColorLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
        } else {
            glUniform1i(solidColorLoc, 1);
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
        
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(GLTFVertex), (void*)offsetof(GLTFVertex, m_BoneIDs));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(GLTFVertex), (void*)offsetof(GLTFVertex, m_Weights));

        glBindVertexArray(0);
    }
};

class GLTFModel {
public:
    std::vector<GLTFMesh> meshes;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    const aiScene* m_Scene = nullptr;
    glm::mat4 m_GlobalInverseTransform;
    Assimp::Importer m_Importer; // Ahora es persistente

    AABB localAABB;

    void CalculateLocalAABB() {
        if (meshes.empty()) return;
        glm::vec3 minPoint(1e9f);
        glm::vec3 maxPoint(-1e9f);
        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                minPoint = (glm::min)(minPoint, vertex.Position);
                maxPoint = (glm::max)(maxPoint, vertex.Position);
            }
        }
        localAABB.min = minPoint;
        localAABB.max = maxPoint;
    }

    AABB GetWorldAABB(const glm::mat4& modelMatrix) const {
        glm::vec3 min = localAABB.min;
        glm::vec3 max = localAABB.max;
        glm::vec3 corners[8] = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(max.x, max.y, max.z)
        };
        glm::vec3 worldMin(1e9f);
        glm::vec3 worldMax(-1e9f);
        for (int i = 0; i < 8; i++) {
            glm::vec3 worldCorner = glm::vec3(modelMatrix * glm::vec4(corners[i], 1.0f));
            worldMin = (glm::min)(worldMin, worldCorner);
            worldMax = (glm::max)(worldMax, worldCorner);
        }
        return { worldMin, worldMax };
    }

    GLTFModel(std::string path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram, int solidColorLoc) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram, solidColorLoc);
    }

    void DrawInstanced(unsigned int shaderProgram, int solidColorLoc, const std::vector<glm::mat4>& instanceModels) {
        if (instanceModels.empty()) return;
        
        static std::map<unsigned int, int> useInstancingLocCache;
        static std::map<unsigned int, int> instanceModelsLocCache;
        auto getCachedLocation = [](std::map<unsigned int, int>& cache,
                                    unsigned int program,
                                    const char* name) {
            auto it = cache.find(program);
            if (it != cache.end()) return it->second;
            int location = glGetUniformLocation(program, name);
            cache[program] = location;
            return location;
        };

        int useInstancingLoc =
            getCachedLocation(useInstancingLocCache, shaderProgram, "useInstancing");
        glUniform1i(useInstancingLoc, 1);
        
        int baseLoc =
            getCachedLocation(instanceModelsLocCache, shaderProgram, "instanceModels");
        glUniformMatrix4fv(baseLoc, static_cast<GLsizei>(instanceModels.size()),
                           GL_FALSE, glm::value_ptr(instanceModels[0]));

        for(unsigned int i = 0; i < meshes.size(); i++) {
            if (meshes[i].textures.size() > 0) {
                glUniform1i(solidColorLoc, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, meshes[i].textures[0].id);
            } else {
                glUniform1i(solidColorLoc, 1);
            }
            
            glBindVertexArray(meshes[i].VAO);
            glDrawElementsInstanced(
                GL_TRIANGLES, static_cast<GLsizei>(meshes[i].indices.size()),
                GL_UNSIGNED_INT, 0,
                static_cast<GLsizei>(instanceModels.size()));
            glBindVertexArray(0);
        }
        
        glUniform1i(useInstancingLoc, 0);
    }

    int GetAnimationCount() const {
        if (!m_Scene || !m_Scene->HasAnimations()) return 0;
        return (int)m_Scene->mNumAnimations;
    }

    std::string GetAnimationName(int index) const {
        if (!m_Scene || !m_Scene->HasAnimations() || index < 0 || index >= (int)m_Scene->mNumAnimations) {
            return "";
        }
        return m_Scene->mAnimations[index]->mName.C_Str();
    }

    float GetAnimationDuration(int index) const {
        if (!m_Scene || !m_Scene->HasAnimations() || index < 0 || index >= (int)m_Scene->mNumAnimations) {
            return 0.0f;
        }
        return (float)m_Scene->mAnimations[index]->mDuration;
    }

    float GetAnimationLengthSeconds(int index) const {
        if (!m_Scene || !m_Scene->HasAnimations() || index < 0 || index >= (int)m_Scene->mNumAnimations) {
            return 0.0f;
        }
        const aiAnimation* animation = m_Scene->mAnimations[index];
        float ticksPerSecond = (float)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
        return ticksPerSecond > 0.0f ? (float)animation->mDuration / ticksPerSecond : 0.0f;
    }

    int CountBonesInMeshes() const {
        if (!m_Scene) return 0;
        int totalBones = 0;
        for (unsigned int i = 0; i < m_Scene->mNumMeshes; i++) {
            totalBones += (int)m_Scene->mMeshes[i]->mNumBones;
        }
        return totalBones;
    }

    int FindAnimationIndexContains(const std::string& needle) const {
        if (!m_Scene || !m_Scene->HasAnimations()) return -1;
        std::string needleLower = ToLower(needle);
        for (unsigned int i = 0; i < m_Scene->mNumAnimations; i++) {
            std::string animName = m_Scene->mAnimations[i]->mName.C_Str();
            std::string animNameLower = ToLower(animName);
            if (animNameLower.find(needleLower) != std::string::npos) {
                return (int)i;
            }
        }
        return -1;
    }

    void UpdateAnimation(float timeInSeconds, std::vector<glm::mat4>& transforms, int animIndex = 0) {
        transforms.resize(MAX_BONES, glm::mat4(1.0f)); // Inicializar con identidad

        if (!m_Scene || !m_Scene->HasAnimations() || animIndex >= m_Scene->mNumAnimations) {
            // Si no hay animación, enviamos matrices identidad para que se vea en pose estática
            return;
        }

        float TicksPerSecond = (float)(m_Scene->mAnimations[animIndex]->mTicksPerSecond != 0 ? m_Scene->mAnimations[animIndex]->mTicksPerSecond : 25.0f);
        float TimeInTicks = timeInSeconds * TicksPerSecond;
        float AnimationTime = fmod(TimeInTicks, (float)m_Scene->mAnimations[animIndex]->mDuration);

        ReadNodeHierarchy(AnimationTime, m_Scene->mRootNode, glm::mat4(1.0f), animIndex);

        for (auto const& [name, info] : m_BoneInfoMap) {
            transforms[info.id] = m_FinalTransforms[info.id];
        }
    }
    void DrawAnimated(float timeInSeconds, int animIndex, unsigned int shaderProgram, int modelLoc, int solidColorLoc, const glm::mat4& baseModelMatrix) {
        if (!m_Scene) return;

        float AnimationTime = 0.0f;
        if (m_Scene->HasAnimations() && animIndex < m_Scene->mNumAnimations) {
            float TicksPerSecond = (float)(m_Scene->mAnimations[animIndex]->mTicksPerSecond != 0 ? m_Scene->mAnimations[animIndex]->mTicksPerSecond : 25.0f);
            float TimeInTicks = timeInSeconds * TicksPerSecond;
            AnimationTime = fmod(TimeInTicks, (float)m_Scene->mAnimations[animIndex]->mDuration);
        }

        DrawNodeAnimated(m_Scene->mRootNode, glm::mat4(1.0f), glm::mat4(1.0f), AnimationTime, animIndex, shaderProgram, modelLoc, solidColorLoc, baseModelMatrix);
    }

private:
    static std::string ToLower(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return (char)std::tolower(c);
        });
        return result;
    }

    void DrawNodeAnimated(const aiNode* pNode, const glm::mat4& ParentTransform, const glm::mat4& DefaultParentTransform, float AnimationTime, int animIndex, unsigned int shaderProgram, int modelLoc, int solidColorLoc, const glm::mat4& baseModelMatrix) {
        std::string NodeName(pNode->mName.data);
        glm::mat4 NodeTransformation = ConvertMatrixToGLMFormat(pNode->mTransformation);
        glm::mat4 DefaultNodeTransformation = NodeTransformation;

        if (m_Scene && m_Scene->HasAnimations() && animIndex < m_Scene->mNumAnimations) {
            const aiAnimation* pAnimation = m_Scene->mAnimations[animIndex];
            const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);
            if (pNodeAnim) {
                aiVector3D Scaling;
                CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
                glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Scaling.x, Scaling.y, Scaling.z));

                aiQuaternion RotationQ;
                CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
                glm::mat4 RotationM = glm::toMat4(glm::quat(RotationQ.w, RotationQ.x, RotationQ.y, RotationQ.z));

                aiVector3D Translation;
                CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
                glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

                NodeTransformation = TranslationM * RotationM * ScalingM;
            }
        }

        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;
        glm::mat4 DefaultGlobalTransformation = DefaultParentTransform * DefaultNodeTransformation;

        for (unsigned int i = 0; i < pNode->mNumMeshes; i++) {
            unsigned int meshIndex = pNode->mMeshes[i];
            
            // Revertimos la transformación global por defecto para situarnos en el espacio del nodo antes de aplicar la animada
            glm::mat4 invDefault = glm::inverse(DefaultGlobalTransformation);
            glm::mat4 finalModel = baseModelMatrix * GlobalTransformation * invDefault;
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(finalModel));
            meshes[meshIndex].Draw(shaderProgram, solidColorLoc);
        }

        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            DrawNodeAnimated(pNode->mChildren[i], GlobalTransformation, DefaultGlobalTransformation, AnimationTime, animIndex, shaderProgram, modelLoc, solidColorLoc, baseModelMatrix);
        }
    }
    glm::mat4 m_FinalTransforms[MAX_BONES];

    void loadModel(std::string path) {
        const aiScene* scene = m_Importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_PopulateArmatureData |
            aiProcess_LimitBoneWeights);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << m_Importer.GetErrorString() << std::endl;
            return;
        }
        m_Scene = scene;
        m_GlobalInverseTransform = ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation);
        m_GlobalInverseTransform = glm::inverse(m_GlobalInverseTransform);

        processNode(scene->mRootNode, scene);
        
        CalculateLocalAABB();
        
        std::cout << "[SISTEMA] Huesos cargados en m_BoneInfoMap: " << m_BoneInfoMap.size() << std::endl;
        
        // Inicializar matrices finales con identidad por si acaso
        for(int i=0; i<MAX_BONES; i++) m_FinalTransforms[i] = glm::mat4(1.0f);
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
        
        glm::vec3 matColor(1.0f, 1.0f, 1.0f);
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
            
            // Try getting glTF PBR base color first, then fallback to diffuse
            if (AI_SUCCESS == material->Get(AI_MATKEY_BASE_COLOR, color)) {
                matColor = glm::vec3(color.r, color.g, color.b);
            } else if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
                matColor = glm::vec3(color.r, color.g, color.b);
            }
            
            aiColor3D emissive(0.0f, 0.0f, 0.0f);
            if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive)) {
                matColor += glm::vec3(emissive.r, emissive.g, emissive.b);
            }
            
            // Si Assimp falla al leer el color emisivo, solo forzamos blanco si el material parece ser la luz
            aiString matName;
            material->Get(AI_MATKEY_NAME, matName);
            std::string nameStr = matName.C_Str();
            // Transform to lowercase for easier matching
            std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
            
            if (glm::length(matColor) < 0.1f) {
                if (nameStr.find("led") != std::string::npos || 
                    nameStr.find("light") != std::string::npos || 
                    nameStr.find("emis") != std::string::npos || 
                    nameStr.find("auto") != std::string::npos) {
                    matColor = glm::vec3(1.0f, 1.0f, 1.0f);
                }
            }
            
            // clamp for sanity
            matColor = glm::clamp(matColor, 0.0f, 1.0f);
        }
        
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            GLTFVertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals()) vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            
            if(mesh->mTextureCoords[0]) vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            
            vertex.Color = matColor;
            vertices.push_back(vertex);
        }

        // Load Bones
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            int boneID = -1;
            std::string boneName = mesh->mBones[i]->mName.C_Str();
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCounter;
                m_BoneCounter++;
            } else {
                boneID = m_BoneInfoMap[boneName].id;
            }

            auto weights = mesh->mBones[i]->mWeights;
            int numWeights = mesh->mBones[i]->mNumWeights;
            for (int weightIndex = 0; weightIndex < numWeights; weightIndex++) {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
                    if (vertices[vertexId].m_BoneIDs[j] < 0) {
                        vertices[vertexId].m_Weights[j] = weight;
                        vertices[vertexId].m_BoneIDs[j] = boneID;
                        break;
                    }
                }
            }
        }

        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Material loading remains the same...
        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<GLTFTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            if (textures.empty()) {
                std::vector<GLTFTexture> baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
                textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());
            }
        }
        return GLTFMesh(vertices, indices, textures);
    }

    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform, int animIndex = 0) {
        std::string NodeName(pNode->mName.data);
        const aiAnimation* pAnimation = m_Scene->mAnimations[animIndex];
        glm::mat4 NodeTransformation = ConvertMatrixToGLMFormat(pNode->mTransformation);
        const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

        if (pNodeAnim) {
            // Interpolate scaling, rotation and translation
            aiVector3D Scaling;
            CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
            glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Scaling.x, Scaling.y, Scaling.z));

            aiQuaternion RotationQ;
            CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
            glm::mat4 RotationM = glm::toMat4(glm::quat(RotationQ.w, RotationQ.x, RotationQ.y, RotationQ.z));

            aiVector3D Translation;
            CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
            glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

            NodeTransformation = TranslationM * RotationM * ScalingM;
        }

        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

        if (m_BoneInfoMap.find(NodeName) != m_BoneInfoMap.end()) {
            int BoneIndex = m_BoneInfoMap[NodeName].id;
            m_FinalTransforms[BoneIndex] = GlobalTransformation * m_BoneInfoMap[NodeName].offset;
        }

        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation, animIndex);
        }
    }

    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName) {
        for (unsigned int i = 0; i < pAnimation->mNumChannels; i++) {
            const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
            if (std::string(pNodeAnim->mNodeName.data) == NodeName) return pNodeAnim;
        }
        return NULL;
    }

    // Helper interpolation functions...
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        if (pNodeAnim->mNumScalingKeys == 1) { Out = pNodeAnim->mScalingKeys[0].mValue; return; }
        unsigned int ScalingIndex = 0;
        for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
            if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) { ScalingIndex = i; break; }
        }
        unsigned int NextScalingIndex = (ScalingIndex + 1);
        float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
        float Factor = 0.0f;
        if (DeltaTime > 0.0f) {
            Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
            if (Factor < 0.0f) Factor = 0.0f;
            if (Factor > 1.0f) Factor = 1.0f;
        }
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        Out = Start + Factor * (End - Start);
    }

    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        if (pNodeAnim->mNumRotationKeys == 1) { Out = pNodeAnim->mRotationKeys[0].mValue; return; }
        unsigned int RotationIndex = 0;
        for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
            if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) { RotationIndex = i; break; }
        }
        unsigned int NextRotationIndex = (RotationIndex + 1);
        float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
        float Factor = 0.0f;
        if (DeltaTime > 0.0f) {
            Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
            if (Factor < 0.0f) Factor = 0.0f;
            if (Factor > 1.0f) Factor = 1.0f;
        }
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
        Out = Out.Normalize();
    }

    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
        if (pNodeAnim->mNumPositionKeys == 1) { Out = pNodeAnim->mPositionKeys[0].mValue; return; }
        unsigned int PositionIndex = 0;
        for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
            if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) { PositionIndex = i; break; }
        }
        unsigned int NextPositionIndex = (PositionIndex + 1);
        float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
        float Factor = 0.0f;
        if (DeltaTime > 0.0f) {
            Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
            if (Factor < 0.0f) Factor = 0.0f;
            if (Factor > 1.0f) Factor = 1.0f;
        }
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        Out = Start + Factor * (End - Start);
    }

    static glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    // Material texture loading... (Same as before)
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
                std::string filename = std::string("assets/") + str.C_Str();
                unsigned int texID = loadTexture(filename.c_str());
                if (texID != 0) {
                    GLTFTexture texture;
                    texture.id = texID;
                    texture.type = typeName;
                    textures.push_back(texture);
                }
            }
        }
        return textures;
    }

    unsigned int loadEmbeddedTexture(const aiTexture* embeddedTexture) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        int width, height, nrComponents;
        unsigned char *data = nullptr;
        extern unsigned char* stbi_load_from_memory(unsigned char const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
        extern void stbi_image_free(void *retval_from_load);
        if (embeddedTexture->mHeight == 0) data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, &width, &height, &nrComponents, 4);
        else data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth * embeddedTexture->mHeight * 4, &width, &height, &nrComponents, 4);
        if (data) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
        }
        return textureID;
    }
};

#endif
