#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#undef MINIAUDIO_IMPLEMENTATION

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "headers/obj_mesh.h"
#include "headers/game_state.h"
#include "headers/gameplay.h"
#include "headers/shader.h"
#include "headers/texture.h"

// SHADERS edwedew

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    layout (location = 3) in vec3 aObjColor;
    layout (location = 4) in ivec4 boneIds; 
    layout (location = 5) in vec4 weights;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;
    out vec3 ObjColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    uniform mat4 finalBonesMatrices[100];
    uniform int isAnimated;

    uniform int dimensionAlterna;
    uniform float time;

    void main() {
        vec4 totalPosition = vec4(0.0f);
        vec3 totalNormal = vec3(0.0f);
        bool hasBoneInfluence = false;

        if (isAnimated == 1) {
            for(int i = 0 ; i < 4 ; i++) {
                if(boneIds[i] == -1) continue;
                if(boneIds[i] >= 100) break;

                hasBoneInfluence = true;
                vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos, 1.0f);
                totalPosition += localPosition * weights[i];
                vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
                totalNormal += localNormal * weights[i];
            }
        }

        // Si no hubo influencia de huesos, usar posiciÃ³n original
        if (!hasBoneInfluence) {
            totalPosition = vec4(aPos, 1.0f);
            totalNormal = aNormal;
        }

        vec3 finalPos = totalPosition.xyz;
        if (dimensionAlterna == 1) {
            finalPos.x += sin(time * 50.0 + totalPosition.y) * 0.05;
            finalPos.y += cos(time * 30.0 + totalPosition.z) * 0.02;
        }

        FragPos = vec3(model * vec4(finalPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * totalNormal;  
        TexCoord = aTexCoord;
        ObjColor = aObjColor;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;
    in vec3 ObjColor;

    uniform sampler2D texture1;
    uniform vec3 objectColor;

    uniform vec3 lightPos;      
    uniform vec3 lightDir;      
    uniform float cutOff;       
    uniform float outerCutOff;  
    uniform int flashlightOn;   

    uniform int dimensionAlterna;
    uniform int currentZone; 
    uniform float time;
    uniform vec2 resolution;
    uniform int useSolidColor;
    uniform float emissiveStrength;

    struct PointLight {
        vec3 position;
        vec3 color;
    };
    #define MAX_POINT_LIGHTS 4
    uniform int numPointLights;
    uniform PointLight pointLights[MAX_POINT_LIGHTS];

    void main() {
        float ambientStrength = 0.05;
        vec3 ambientColor = vec3(1.0);
        vec3 flashColor = vec3(1.0);

        if (currentZone == 1) {
            ambientColor = vec3(0.6, 0.7, 0.8); 
            flashColor = vec3(0.9, 0.9, 1.0);
            ambientStrength = 0.1 + (sin(time * 10.0) * 0.02); 
        } else if (currentZone == 2) {
            ambientColor = vec3(0.4, 0.9, 0.5); 
            flashColor = vec3(0.8, 1.0, 0.8);
            ambientStrength = 0.15;
        } else if (currentZone == 3) {
            ambientColor = vec3(0.3, 0.5, 1.0); 
            flashColor = vec3(1.0, 1.0, 1.0); 
            ambientStrength = 0.2;
        }

        if (dimensionAlterna == 1) {
            ambientColor = vec3(0.6, 0.0, 0.2); 
            ambientStrength = 0.1 + (sin(time * 20.0) * 0.05) + (cos(time * 50.0) * 0.03);
            if(ambientStrength < 0.02) ambientStrength = 0.02;
            flashColor = vec3(1.0, 0.3, 0.3) * (0.7 + 0.3 * sin(time * 40.0));
        }

        vec3 ambient = ambientStrength * ambientColor;
        vec3 diffuse = vec3(0.0);
        
        if (flashlightOn == 1) {
            vec3 norm = normalize(Normal);
            vec3 lightDirVec = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDirVec), 0.0);
            diffuse = diff * flashColor;

            float theta = dot(lightDirVec, normalize(-lightDir));
            float epsilon = cutOff - outerCutOff;
            float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

            float distance = length(lightPos - FragPos);
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

            diffuse *= intensity * attenuation;
        }

        // agregar las luces puntuales definidas en pointLights{}
        vec3 pointLightsDiffuse = vec3(0.0);
        vec3 norm = normalize(Normal);
        for(int i = 0; i < numPointLights; i++) {
            vec3 lightDirVec = normalize(pointLights[i].position - FragPos);
            float diff = max(dot(norm, lightDirVec), 0.0);
            
            float distance = length(pointLights[i].position - FragPos);
            
            // Fórmula de atenuación suave  (copiada de unreal engine)
            // La luz muere exactamente en el "radius" sin cortes feos
            float radius = 4.0; 
            float falloff = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
            float attenuation = falloff * falloff; // Suavizado cuadrático
            
            pointLightsDiffuse += diff * pointLights[i].color * attenuation;
        }
        diffuse += pointLightsDiffuse;

        vec4 texColor = texture(texture1, TexCoord);
        if (useSolidColor == 1) {
            texColor = vec4(ObjColor, 1.0); // Usar el color empaquetado del OBJ
        } else if (texColor.a < 0.1) {
            discard; 
        }
        
        vec3 result = (ambient + diffuse) * objectColor;
        
        // Sumamos emisión pura para que las lámparas brillen en la oscuridad
        if (emissiveStrength > 0.0) {
            result += ObjColor * emissiveStrength;
        }
        
        if (dimensionAlterna == 1) {
            vec2 uv = gl_FragCoord.xy / resolution;
            float distToCenter = distance(uv, vec2(0.5));
            result *= smoothstep(0.9, 0.2, distToCenter);
        }
        
        FragColor = texColor * vec4(result, 1.0);
    }
)";

#include "gltf_model.h"

static const char* getEntityTypeLabel(int type) {
    switch (type) {
        case 0: return "Log";
        case 1: return "Bateria";
        case 2: return "Entidad";
        case 3: return "Cable/Pista";
        case 4: return "Mesa";
        case 5: return "Monitor";
        case 6: return "Maquina";
        case 7: return "Portal";
        case 8: return "Tarjeta Nv1";
        case 9: return "Tarjeta Nv2";
        default: return "Desconocido";
    }
}

static bool isCollectibleEntityType(int type) {
    return type == 0 || type == 1 || type == 8 || type == 9;
}

static bool isInspectableEntityType(int type) {
    return type == 3 || type == 4 || type == 5 || type == 6 || type == 7;
}

static int findFocusedEntityIndex(float* outDistance = nullptr, float* outLookAngle = nullptr) {
    int focusedIndex = -1;
    float bestScore = -9999.0f;

    for (int i = 0; i < (int)gameEntities.size(); ++i) {
        const Entity& entity = gameEntities[i];
        if (!entity.active) continue;

        float distance = glm::length(entity.pos - cameraPos);
        glm::vec3 dir = glm::normalize(entity.pos - cameraPos);
        float lookAngle = glm::dot(cameraFront, dir);

        bool valid = false;
        if (isCollectibleEntityType(entity.type)) valid = distance < 2.0f;
        else if (isInspectableEntityType(entity.type)) valid = distance < 3.0f && lookAngle > 0.80f;
        else if (entity.type == 2) valid = distance < 12.0f;

        if (!valid) continue;

        float score = lookAngle * 10.0f - distance;
        if (score > bestScore) {
            bestScore = score;
            focusedIndex = i;
            if (outDistance) *outDistance = distance;
            if (outLookAngle) *outLookAngle = lookAngle;
        }
    }

    return focusedIndex;
}

static bool findDoorAhead(int& gridX, int& gridZ, int& blockType, float distance = 1.5f) {
    glm::vec3 checkPos = cameraPos + cameraFront * distance;
    gridX = (int)round(checkPos.x);
    gridZ = (int)round(checkPos.z);
    blockType = 0;

    if (gridX < 0 || gridX >= MAP_WIDTH || gridZ < 0 || gridZ >= MAP_HEIGHT) return false;
    blockType = worldMap[gridZ][gridX];
    return blockType == 8 || blockType == 9 || blockType == -8 || blockType == -9;
}

static const char* getDoorDebugLabel(int blockType) {
    switch (blockType) {
        case 8: return "Puerta Nv1 cerrada";
        case -8: return "Puerta Nv1 abierta";
        case 9: return "Puerta Nv2 cerrada";
        case -9: return "Puerta Nv2 abierta";
        default: return "Sin puerta";
    }
}

int main() {
    std::cout << "--- PRUEBA DE ASSIMP (GNOME) ---" << std::endl;
    std::string diagnosticPath = "assets/gnome.glb";
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(diagnosticPath, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_PopulateArmatureData |
        aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        diagnosticPath = "assets/gnome.glb";
        scene = importer.ReadFile(diagnosticPath,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_PopulateArmatureData |
            aiProcess_LimitBoneWeights);
    }

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
    } else {
        std::cout << "EXITO: El gnomo se cargo correctamente desde " << diagnosticPath << std::endl;
        std::cout << "--- INFO DEL MODELO ---" << std::endl;
        std::cout << "Animaciones: " << scene->mNumAnimations << std::endl;
        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            std::cout << "  [" << i << "] Nombre: " << scene->mAnimations[i]->mName.C_Str() << " (Duracion: " << scene->mAnimations[i]->mDuration << ", TicksPerSecond: " << scene->mAnimations[i]->mTicksPerSecond << ")" << std::endl;
        }
        unsigned int totalBones = 0;
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            totalBones += scene->mMeshes[i]->mNumBones;
        }
        std::cout << "Mallas (Meshes): " << scene->mNumMeshes << " | Huesos totales en mallas: " << totalBones << std::endl;
        // std::cout << "--- JERARQUIA DE NODOS ---" << std::endl;
        // printNodeHierarchy(scene->mRootNode, 0);
    }
    std::cout << "------------------------" << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto Confidencial..", NULL, NULL);
    if (window == NULL) {
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    if (ma_engine_init(NULL, &audioEngine) != MA_SUCCESS) return -1;

    ma_sound bgm;
    ma_sound_init_from_file(&audioEngine, "assets/music.mp3", MA_SOUND_FLAG_STREAM, NULL, NULL, &bgm);
    ma_sound_set_looping(&bgm, MA_TRUE);
    ma_sound_start(&bgm);

    // --- SETUP IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    {
        int success; char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::VERTEX_SHADER::" << infoLog << std::endl;
        }
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    {
        int success; char infoLog[512];
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::FRAGMENT_SHADER::" << infoLog << std::endl;
        }
    }
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    {
        int success; char infoLog[512];
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER_LINK::" << infoLog << std::endl;
        } else {
            std::cout << "SHADER: Compilado y enlazado correctamente." << std::endl;
        }
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        // Back face (-Z)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,         
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 
        // Front face (+Z)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f, 
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 
        // Left face (-X)
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f, 
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, 
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 
        // Right face (+X)
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, 
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,         
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f, 
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,      
        // Bottom face (-Y)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, 
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f, 
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, 
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f, 
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f, 
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f, 
        // Top face (+Y)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, 
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,     
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f, 
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f  
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int objVAO = 0, objVBO = 0;
    int objVertexCount = 0;
    loadOBJMesh("assets/laptop.obj", objVAO, objVBO, objVertexCount);

    unsigned int cablesVAO = 0, cablesVBO = 0;
    int cablesVertexCount = 0;
    loadOBJMesh("assets/cables.obj", cablesVAO, cablesVBO, cablesVertexCount);

    unsigned int cartaVAO = 0, cartaVBO = 0;
    int cartaVertexCount = 0;
    loadOBJMesh("assets/carta.obj", cartaVAO, cartaVBO, cartaVertexCount);

    unsigned int pIzquiVAO = 0, pIzquiVBO = 0, pDereVAO = 0, pDereVBO = 0;
    int pIzquiCount = 0, pDereCount = 0;
    loadOBJMesh("assets/puertaizqui.obj", pIzquiVAO, pIzquiVBO, pIzquiCount);
    loadOBJMesh("assets/puertadere.obj", pDereVAO, pDereVBO, pDereCount);

    std::string gnomeModelPath = "assets/gnome.glb";
    GLTFModel* gnomeGLTF = new GLTFModel(gnomeModelPath);
    if (gnomeGLTF->meshes.empty()) {
        delete gnomeGLTF;
        gnomeModelPath = "assets/gnome.glb";
        gnomeGLTF = new GLTFModel(gnomeModelPath);
    }
    std::cout << "[SISTEMA] Modelo activo del gnomo: " << gnomeModelPath << std::endl;

    GLTFModel* azulejoGLTF = new GLTFModel("assets/azule.glb");
    GLTFModel* mirrorGLTF = new GLTFModel("assets/mirror.glb");
    GLTFModel* ligthbathroom2GLTF = new GLTFModel("assets/ligthbathroom.glb");
    GLTFModel* ligthbathroomGLTF = new GLTFModel("assets/ligthbathroom.glb");
    GLTFModel* banoGLTF = new GLTFModel("assets/Bano.glb");
    GLTFModel* bano2GLTF = new GLTFModel("assets/Bano.glb");
    GLTFModel* bano3GLTF = new GLTFModel("assets/Bano.glb");
    GLTFModel* bano4GLTF = new GLTFModel("assets/Bano.glb");
    GLTFModel* lavamanosGLTF = new GLTFModel("assets/lavamanos.glb");
    GLTFModel* urinarioGLTF = new GLTFModel("assets/urinario.glb");
    std::cout << "[SISTEMA] Props baño cargados: "
              << "Lampara(" << ligthbathroomGLTF->meshes.size() << "), "
              << "Bano(" << banoGLTF->meshes.size() << "), "
              << "Lavamanos(" << lavamanosGLTF->meshes.size() << "), "
              << "Urinario(" << urinarioGLTF->meshes.size() << ")" << std::endl;
    
    unsigned int wallTex1 = loadTexture("assets/paredesH.png"); 
    unsigned int wallTex2 = loadTexture("assets/paredes.png");  
    unsigned int wallTex3 = loadTexture("assets/wall.png");     
    
    // Textura de metal generada para las puertas
    unsigned int doorTex = loadTextureWithFallback("assets/puerta_metal.png", 0); 
    
    unsigned int portalTex = loadTexture("assets/clue.png");

    unsigned int floorTexture = loadTexture("assets/pisoH.jpg");
    unsigned int logoTexture = loadTexture("assets/logo.png"); 
    unsigned int clueTexture = loadTexture("assets/clue.png"); 
    unsigned int enemyTexture = loadTexture("assets/enemy.png");

    // Texturas especÃ­ficas con fallback
    unsigned int batteryTex = loadTextureWithFallback("assets/battery.png", clueTexture);
    unsigned int keycardTex = loadTextureWithFallback("assets/keycard.png", clueTexture);
    unsigned int pcTex = loadTextureWithFallback("assets/pc.png", wallTex2);

    float quadVertices[] = {
        -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,

        -0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    
    int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    int cutOffLoc = glGetUniformLocation(shaderProgram, "cutOff");
    int outerCutOffLoc = glGetUniformLocation(shaderProgram, "outerCutOff");
    int flashlightOnLoc = glGetUniformLocation(shaderProgram, "flashlightOn");
    
    int dimAlternaLoc = glGetUniformLocation(shaderProgram, "dimensionAlterna");
    int zoneLoc = glGetUniformLocation(shaderProgram, "currentZone");
    int timeLoc = glGetUniformLocation(shaderProgram, "time");
    int resLoc = glGetUniformLocation(shaderProgram, "resolution");
    int solidColorLoc = glGetUniformLocation(shaderProgram, "useSolidColor");
    int isAnimatedLoc = glGetUniformLocation(shaderProgram, "isAnimated");
    int finalBonesLoc = glGetUniformLocation(shaderProgram, "finalBonesMatrices[0]");
    
    int numPointLightsLoc = glGetUniformLocation(shaderProgram, "numPointLights");
    int pointLightPosLoc[4];
    int pointLightColLoc[4];
    for(int i = 0; i < 4; i++) {
        std::string posStr = "pointLights[" + std::to_string(i) + "].position";
        std::string colStr = "pointLights[" + std::to_string(i) + "].color";
        pointLightPosLoc[i] = glGetUniformLocation(shaderProgram, posStr.c_str());
        pointLightColLoc[i] = glGetUniformLocation(shaderProgram, colStr.c_str());
    }
    
    int emissiveStrengthLoc = glGetUniformLocation(shaderProgram, "emissiveStrength");
    
    std::vector<glm::mat4> gnomeBoneTransforms;
    glm::vec3 gnomePos = glm::vec3(4.1f, -0.4f, 4.0f);
    float gnomeStunTimer = 0.0f;
    bool isGnomeActive = true;
    unsigned int gnomeTexture = 0;
    bool gnomeTextureFailed = false;
    int gnomeDebugAnimIndex = 0;
    float gnomeAnimSpeed = 1.0f;
    bool gnomeAnimLoop = true;
    float gnomeAnimPreviewTime = 0.0f;
    bool gnomeForceAnimation = false;

    glm::vec3 debugSpawnPos = cameraPos;
    float debugSpawnYaw = yaw;
    float debugSpawnPitch = pitch;
    bool showCollisionViewer = false;
    bool showInteractionDebugger = true;
    bool showAnimationTester = true;
    bool showSpawnInspector = true;
    bool collisionShowWalls = true;
    bool collisionShowProps = true;
    float collisionViewerRadius = 8.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (hudMessageTimer > 0.0f) {
            hudMessageTimer -= deltaTime;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f); // Default obj color para otros VAOs
        glUniform1f(emissiveStrengthLoc, 0.0f); // Por defecto nada emite luz propia

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1;

        if (gameState == MENU) {
            yaw += 15.0f * deltaTime; 
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        } else if (gameState == GAMEOVER) {
            pitch -= 15.0f * deltaTime;
            cameraPos.y -= 1.5f * deltaTime;
            if (pitch < -89.0f) pitch = -89.0f;
            if (cameraPos.y < -0.5f) cameraPos.y = -0.5f;
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glBindVertexArray(VAO);

        glm::mat4 projection = glm::perspective(glm::radians(55.0f), (float)currentWidth / (float)currentHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniform3fv(lightPosLoc, 1, glm::value_ptr(cameraPos));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(cameraFront));
        glUniform1f(cutOffLoc, glm::cos(glm::radians(15.5f)));
        glUniform1f(outerCutOffLoc, glm::cos(glm::radians(22.5f)));
        glUniform1i(flashlightOnLoc, isFlashlightOn ? 1 : 0);
        
        glUniform1i(dimAlternaLoc, dimensionAlterna ? 1 : 0);
        glUniform1i(zoneLoc, currentZone);
        glUniform1f(timeLoc, currentFrame);
        glUniform2f(resLoc, (float)currentWidth, (float)currentHeight);
        //espacio donde se le da las luces a las lamparas antes de poner su figura .gltf o .obj

        glUniform1i(numPointLightsLoc, 2);
        
        // color de la lampara y posicion para ponerla en un color amarillento poner colores mas altos en vez de 0.8 0.9 1.0 a unos 0.8 0.6 0.2 para que se vea mas amarillento
        glUniform3fv(pointLightPosLoc[0], 1, glm::value_ptr(ligthbathroomPos));
        glUniform3f(pointLightColLoc[0], 0.8f, 0.6f, 0.2f);
        
       // lampara 2 
        glUniform3fv(pointLightPosLoc[1], 1, glm::value_ptr(ligthbathroom2Pos));
        glUniform3f(pointLightColLoc[1], 0.8f, 0.6f, 0.2f);

        // --- MAPA ---
        for (int z = 0; z < MAP_HEIGHT; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                int blockType = worldMap[z][x];
                
                // Consideramos la puerta visible tanto si esta cerrada (>0) como abierta (<0)
                int renderBlock = worldMap[z][x];
                if (renderBlock != 0 && (blockType > 0 || renderBlock == -8 || renderBlock == -9)) { 
                    bool is3DDoor = (renderBlock == 8 || renderBlock == 9 || renderBlock == -8 || renderBlock == -9);
                    
                    // Detectar si esta celda es la primera o segunda de un par de puertas
                    bool isSecondDoorCell = false;
                    if (is3DDoor && x > 0) {
                        int prevBlock = worldMap[z][x - 1];
                        if (prevBlock == renderBlock) isSecondDoorCell = true;
                    }
                    
                    if (is3DDoor && isSecondDoorCell) {
                        // Skip: la segunda celda de la puerta, ya se dibuja desde la primera
                    } else if (is3DDoor && !isSecondDoorCell) {
                        glm::mat4 baseModel = glm::mat4(1.0f);
                        
                        // 4. Mover al centro del hueco y anclar al piso de la pared (-0.5)
                        baseModel = glm::translate(baseModel, glm::vec3((float)x + 0.5f, -0.5f, (float)z));
                        
                        // 3. Escalar para encajar en el juego.
                        // Ancho de ensamble: 1.81 -> Juego: 2.0 (Escala 1.1)
                        // Alto Blender: 1.535 -> Juego: 1.0 (Escala 0.651)
                        baseModel = glm::scale(baseModel, glm::vec3(1.1f, 0.651f, 1.1f));
                        
                        // 2. Rotar (dejaremos 0 grados asumiendo que los nuevos estan de frente)
                        // Si se ven las texturas por detras, cambiaremos este valor a 180 despues.
                        // baseModel = glm::rotate(baseModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        
                        // 1. Compensar el offset original de Blender para centrar el ensamble en (0,0,0)
                        // Centro X = 0.455, Centro Z = 0.059
                        baseModel = glm::translate(baseModel, glm::vec3(-0.455f, 0.0f, -0.059f));

                        if (doorTex > 0) {
                            glUniform1i(solidColorLoc, 0); // No ignorar textura
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, doorTex);
                            
                            // Aplicar tinte sobre el metal
                            if (renderBlock == 8 || renderBlock == -8) {
                                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Metal Amarillo
                            } else if (renderBlock == 9 || renderBlock == -9) {
                                glUniform3f(colorLoc, 0.9f, 0.1f, 0.1f); // Metal Rojo
                            } else {
                                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                            }
                        } else {
                            glUniform1i(solidColorLoc, 1);
                            if (renderBlock == 8 || renderBlock == -8) {
                                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Amarillo SÃ³lido
                            } else if (renderBlock == 9 || renderBlock == -9) {
                                glUniform3f(colorLoc, 0.9f, 0.1f, 0.1f); // Rojo SÃ³lido
                            } else {
                                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                            }
                        }

                        // Obtener Ã¡ngulo actual de la animaciÃ³n
                        float currentAnim = (renderBlock == 8 || renderBlock == -8) ? door1Anim : door2Anim;
                        
                        // Rotacion: La izquierda gira hacia adelante (negativo), la derecha gira hacia el otro lado (positivo)
                        float angleL = glm::radians(-currentAnim);
                        float angleR = glm::radians(currentAnim);

                        if (pIzquiCount > 0) {
                            glBindVertexArray(pIzquiVAO);
                            glm::mat4 modelL = baseModel;
                            // Bisagra de la puerta izquierda en Blender (Min X = -0.45)
                            glm::vec3 hingeL(-0.45f, 0.0f, 0.057f);
                            modelL = glm::translate(modelL, hingeL);
                            modelL = glm::rotate(modelL, angleL, glm::vec3(0.0f, 1.0f, 0.0f));
                            modelL = glm::translate(modelL, -hingeL);
                            
                            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelL));
                            glDrawArrays(GL_TRIANGLES, 0, pIzquiCount);
                        }
                        if (pDereCount > 0) {
                            glBindVertexArray(pDereVAO);
                            glm::mat4 modelR = baseModel;
                            // Bisagra de la puerta derecha en Blender (Max X = 1.359)
                            glm::vec3 hingeR(1.359f, 0.0f, 0.061f);
                            modelR = glm::translate(modelR, hingeR);
                            modelR = glm::rotate(modelR, angleR, glm::vec3(0.0f, 1.0f, 0.0f));
                            modelR = glm::translate(modelR, -hingeR);
                            
                            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelR));
                            glDrawArrays(GL_TRIANGLES, 0, pDereCount);
                        }
                        
                        glUniform1i(solidColorLoc, 0);
                        glBindVertexArray(VAO);
                    } else if (blockType > 0) {
                        // Bloques normales (paredes)
                        if (blockType == 1) glBindTexture(GL_TEXTURE_2D, wallTex1);
                        else if (blockType == 2) glBindTexture(GL_TEXTURE_2D, wallTex2);
                        else if (blockType == 3) glBindTexture(GL_TEXTURE_2D, wallTex3);
                        glBindVertexArray(VAO);

                        // Escalar paredes inteligentemente segÃºn vecinos
                        float scaleX = wallWidth;
                        float scaleZ = wallWidth;
                        bool hasLeft  = (x > 0 && worldMap[z][x-1] > 0);
                        bool hasRight = (x < MAP_WIDTH-1 && worldMap[z][x+1] > 0);
                        bool hasUp    = (z > 0 && worldMap[z-1][x] > 0);
                        bool hasDown  = (z < MAP_HEIGHT-1 && worldMap[z+1][x] > 0);
                        if (hasLeft || hasRight) scaleX = 1.0f;
                        if (hasUp || hasDown) scaleZ = 1.0f;

                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f, (float)z));
                        model = glm::scale(model, glm::vec3(scaleX, wallHeight, scaleZ));
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
                
                glBindTexture(GL_TEXTURE_2D, floorTexture);
                glm::mat4 floorModel = glm::mat4(1.0f);
                floorModel = glm::translate(floorModel, glm::vec3((float)x, -1.0f, (float)z));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
                
                if (dimensionAlterna) glUniform3f(colorLoc, 0.4f, 0.1f, 0.1f);
                else glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
                
                glDrawArrays(GL_TRIANGLES, 0, 36);

                { // Techo en TODAS las celdas (incluidas las de pared, para cubrir huecos de paredes delgadas)
                    glm::mat4 roofModel = glm::mat4(1.0f);
                    roofModel = glm::translate(roofModel, glm::vec3((float)x, wallHeight, (float)z));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(roofModel));
                    glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); 
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        } 

        // --- LÃ“GICA Y DIBUJO DEL GNOMO ACOSADOR (AI) ---
        if (gnomeGLTF) {
            // Cargar textura una sola vez si no existe
            if (gnomeTexture == 0 && !gnomeTextureFailed) {
                gnomeTexture = loadTexture("assets/Gnome_Albedo.png"); 
                if (gnomeTexture == 0) {
                    gnomeTextureFailed = true;
                    std::cout << "[SISTEMA] No se encontró assets/Gnome_Albedo.png. El gnomo usará sus colores por defecto." << std::endl;
                }
            }

            if (isGnomeActive) {
                float distToPlayer = glm::length(cameraPos - gnomePos);
                glm::vec3 dirToGnome = glm::normalize(gnomePos - cameraPos);
                
                // 1. DETECTAR SI LA LINTERNA LO APUNTA DIRECTAMENTE
                bool beingLookedAt = false;
                if (isFlashlightOn) {
                    float angle = glm::dot(cameraFront, dirToGnome);
                    // 0.98 significa que lo miras casi al centro (un cono muy cerrado)
                    if (angle > 0.98f && distToPlayer < 10.0f) { 
                        beingLookedAt = true;
                    }
                }

                // 2. LÃ“GICA DEL TEMPORIZADOR
                if (beingLookedAt) {
                    gnomeStunTimer += deltaTime;
                    if (gnomeStunTimer >= 2.0f) {
                        isGnomeActive = false; // El gnomo se asusta y desaparece (o se detiene)
                        std::cout << "[SISTEMA]: Gnomo ahuyentado por la luz   ." << std::endl;
                    }
                } else {
                    gnomeStunTimer = (std::max)(0.0f, gnomeStunTimer - deltaTime); // El timer baja si dejas de mirarlo
                }

                // 3. MOVIMIENTO (Solo si NO lo está mirando o no ha sido aturdido)
                float speed = 1.8f; 
                bool isMoving = false;
                if (!beingLookedAt && distToPlayer > 0.8f) {
                    glm::vec3 moveDir = glm::normalize(cameraPos - gnomePos);
                    moveDir.y = 0; // Mantenerlo en el suelo
                    gnomePos += moveDir * speed * deltaTime;
                    isMoving = true;
                }

                // 4. RENDERIZADO
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                
                bool hasSkinningBones = gnomeGLTF->CountBonesInMeshes() > 0;

                glm::mat4 gnomeModel = glm::mat4(1.0f);
                gnomeModel = glm::translate(gnomeModel, gnomePos);
                
                // Rotar en el eje Y para mirar al jugador (se aplica DESPUÉS de levantarse en espacio global)--
                float angle = atan2(cameraPos.x - gnomePos.x, cameraPos.z - gnomePos.z);
                gnomeModel = glm::rotate(gnomeModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));
                
                if (!hasSkinningBones) {
                    // Corrección legacy para modelos exportados sin skinning
                    gnomeModel = glm::rotate(gnomeModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    gnomeModel = glm::scale(gnomeModel, glm::vec3(0.006f, 0.006f, 0.006f));
                } else {
                    // GLB actual: escala de depuración más visible y sin giro extra
                    gnomeModel = glm::scale(gnomeModel, glm::vec3(0.07f, 0.07f, 0.07f));
                }
                
                // Aplicar textura forzada
                glUniform1i(solidColorLoc, gnomeTexture == 0 ? 1 : 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, gnomeTexture);
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

                // Determinar animación a reproducir
                int animCount = gnomeGLTF->GetAnimationCount();
                int idleAnimIndex = gnomeGLTF->FindAnimationIndexContains("idle");
                if (idleAnimIndex < 0) idleAnimIndex = 0;

                int stunAnimIndex = gnomeGLTF->FindAnimationIndexContains("stun");
                if (stunAnimIndex < 0) stunAnimIndex = idleAnimIndex;

                int moveAnimIndex = gnomeGLTF->FindAnimationIndexContains("move");
                if (moveAnimIndex < 0) moveAnimIndex = gnomeGLTF->FindAnimationIndexContains("walk");
                if (moveAnimIndex < 0) moveAnimIndex = gnomeGLTF->FindAnimationIndexContains("run");
                if (moveAnimIndex < 0) moveAnimIndex = idleAnimIndex;

                int currentAnimIndex = idleAnimIndex;
                if (gnomeForceAnimation && animCount > 0) {
                    if (gnomeDebugAnimIndex < 0) gnomeDebugAnimIndex = 0;
                    if (gnomeDebugAnimIndex >= animCount) gnomeDebugAnimIndex = animCount - 1;
                    currentAnimIndex = gnomeDebugAnimIndex;
                } else {
                    if (beingLookedAt) {
                        currentAnimIndex = stunAnimIndex;
                    } else if (isMoving) {
                        currentAnimIndex = moveAnimIndex;
                    }
                }

                float gnomeAnimLength = currentAnimIndex >= 0 ? gnomeGLTF->GetAnimationLengthSeconds(currentAnimIndex) : 0.0f;
                float gnomeRenderTime = currentFrame * gnomeAnimSpeed;
                if (gnomeForceAnimation) {
                    if (gnomeAnimLoop && gnomeAnimLength > 0.0f) {
                        gnomeRenderTime = fmod(gnomeAnimPreviewTime, gnomeAnimLength);
                    } else {
                        gnomeRenderTime = gnomeAnimPreviewTime;
                    }
                }

                // Actualizar y enviar matrices de huesos para skinning (solo si el modelo realmente trae huesos)
                if (hasSkinningBones) {
                    gnomeGLTF->UpdateAnimation(gnomeRenderTime, gnomeBoneTransforms, currentAnimIndex);
                    if (finalBonesLoc >= 0 && !gnomeBoneTransforms.empty()) {
                        glUniformMatrix4fv(finalBonesLoc, (GLsizei)gnomeBoneTransforms.size(), GL_FALSE, glm::value_ptr(gnomeBoneTransforms[0]));
                    }
                }
                if (isAnimatedLoc >= 0) {
                    glUniform1i(isAnimatedLoc, hasSkinningBones ? 1 : 0);
                }

                if (hasSkinningBones) {
                    // En modelos con skinning, evitar doble transformación (nodos + huesos)
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(gnomeModel));
                    gnomeGLTF->Draw(shaderProgram, solidColorLoc);
                } else {
                    // Fallback para modelos sin pesos de hueso exportados
                    gnomeGLTF->DrawAnimated(gnomeRenderTime, currentAnimIndex, shaderProgram, modelLoc, -1, gnomeModel);
                }
            }
        }

        // --- DECORACIÓN BAÑO (GLB estáticos) ---
        // Limpiar estado incondicionalmente antes de dibujar los props para evitar heredar colores o estados
        glActiveTexture(GL_TEXTURE0); 
        if (isAnimatedLoc >= 0) glUniform1i(isAnimatedLoc, 0);
        glUniform1i(solidColorLoc, 0);
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // MUY IMPORTANTE: Resetear color que pudo dejar el techo oscuro del mapa
        glBindVertexArray(VAO);

        if (banoGLTF && !banoGLTF->meshes.empty()) {
            glm::mat4 banoModel = glm::mat4(1.0f);
            banoModel = glm::translate(banoModel, banoPos);
            banoModel = glm::rotate(banoModel, glm::radians(banoRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            banoModel = glm::rotate(banoModel, glm::radians(banoRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            banoModel = glm::rotate(banoModel, glm::radians(banoRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            banoModel = glm::scale(banoModel, banoScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(banoModel));
            banoGLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (azulejoGLTF && !azulejoGLTF->meshes.empty()) {
            glm::mat4 azulejoModel = glm::mat4(1.0f);
            azulejoModel = glm::translate(azulejoModel, azulejoPos);
            azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            azulejoModel = glm::scale(azulejoModel, azulejoScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(azulejoModel));
            azulejoGLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (lavamanosGLTF && !lavamanosGLTF->meshes.empty()) {
            // Lavamanos 1
            glm::mat4 lavamanosModel = glm::mat4(1.0f);
            lavamanosModel = glm::translate(lavamanosModel, lavamanosPos);
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            lavamanosModel = glm::scale(lavamanosModel, lavamanosScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lavamanosModel));
            lavamanosGLTF->Draw(shaderProgram, solidColorLoc);

            // Lavamanos 2
            glm::mat4 lavamanosModel2 = glm::mat4(1.0f);
            lavamanosModel2 = glm::translate(lavamanosModel2, lavamanosPos2);
            lavamanosModel2 = glm::rotate(lavamanosModel2, glm::radians(lavamanosRot2.x), glm::vec3(1.0f, 0.0f, 0.0f));
            lavamanosModel2 = glm::rotate(lavamanosModel2, glm::radians(lavamanosRot2.y), glm::vec3(0.0f, 1.0f, 0.0f));
            lavamanosModel2 = glm::rotate(lavamanosModel2, glm::radians(lavamanosRot2.z), glm::vec3(0.0f, 0.0f, 1.0f));
            lavamanosModel2 = glm::scale(lavamanosModel2, lavamanosScale2);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lavamanosModel2));
            lavamanosGLTF->Draw(shaderProgram, solidColorLoc);

            // Lavamanos 3
            glm::mat4 lavamanosModel3 = glm::mat4(1.0f);
            lavamanosModel3 = glm::translate(lavamanosModel3, lavamanosPos3);
            lavamanosModel3 = glm::rotate(lavamanosModel3, glm::radians(lavamanosRot3.x), glm::vec3(1.0f, 0.0f, 0.0f));
            lavamanosModel3 = glm::rotate(lavamanosModel3, glm::radians(lavamanosRot3.y), glm::vec3(0.0f, 1.0f, 0.0f));
            lavamanosModel3 = glm::rotate(lavamanosModel3, glm::radians(lavamanosRot3.z), glm::vec3(0.0f, 0.0f, 1.0f));
            lavamanosModel3 = glm::scale(lavamanosModel3, lavamanosScale3);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lavamanosModel3));
            lavamanosGLTF->Draw(shaderProgram, solidColorLoc);

            // Lavamanos 4
            glm::mat4 lavamanosModel4 = glm::mat4(1.0f);
            lavamanosModel4 = glm::translate(lavamanosModel4, lavamanosPos4);
            lavamanosModel4 = glm::rotate(lavamanosModel4, glm::radians(lavamanosRot4.x), glm::vec3(1.0f, 0.0f, 0.0f));
            lavamanosModel4 = glm::rotate(lavamanosModel4, glm::radians(lavamanosRot4.y), glm::vec3(0.0f, 1.0f, 0.0f));
            lavamanosModel4 = glm::rotate(lavamanosModel4, glm::radians(lavamanosRot4.z), glm::vec3(0.0f, 0.0f, 1.0f));
            lavamanosModel4 = glm::scale(lavamanosModel4, lavamanosScale4);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lavamanosModel4));
            lavamanosGLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (mirrorGLTF && !mirrorGLTF->meshes.empty()) {
            glm::mat4 mirrorModel = glm::mat4(1.0f);
            mirrorModel = glm::translate(mirrorModel, mirrorPos);
            mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            mirrorModel = glm::scale(mirrorModel, mirrorScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel));
            mirrorGLTF->Draw(shaderProgram, solidColorLoc);

            // Mirror 2
            glm::mat4 mirrorModel2 = glm::mat4(1.0f);
            mirrorModel2 = glm::translate(mirrorModel2, mirrorPos2);
            mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.x), glm::vec3(1.0f, 0.0f, 0.0f));
            mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.y), glm::vec3(0.0f, 1.0f, 0.0f));
            mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.z), glm::vec3(0.0f, 0.0f, 1.0f));
            mirrorModel2 = glm::scale(mirrorModel2, mirrorScale2);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel2));
            mirrorGLTF->Draw(shaderProgram, solidColorLoc);

            // Mirror 3
            glm::mat4 mirrorModel3 = glm::mat4(1.0f);
            mirrorModel3 = glm::translate(mirrorModel3, mirrorPos3);
            mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.x), glm::vec3(1.0f, 0.0f, 0.0f));
            mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.y), glm::vec3(0.0f, 1.0f, 0.0f));
            mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.z), glm::vec3(0.0f, 0.0f, 1.0f));
            mirrorModel3 = glm::scale(mirrorModel3, mirrorScale3);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel3));
            mirrorGLTF->Draw(shaderProgram, solidColorLoc);

            // Mirror 4
            glm::mat4 mirrorModel4 = glm::mat4(1.0f);
            mirrorModel4 = glm::translate(mirrorModel4, mirrorPos4);
            mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.x), glm::vec3(1.0f, 0.0f, 0.0f));
            mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.y), glm::vec3(0.0f, 1.0f, 0.0f));
            mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.z), glm::vec3(0.0f, 0.0f, 1.0f));
            mirrorModel4 = glm::scale(mirrorModel4, mirrorScale4);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel4));
            mirrorGLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (bano2GLTF && !bano2GLTF->meshes.empty()) {
            glm::mat4 bano2Model = glm::mat4(1.0f);
            bano2Model = glm::translate(bano2Model, banoPos2);
            bano2Model = glm::rotate(bano2Model, glm::radians(banoRot2.x), glm::vec3(1.0f, 0.0f, 0.0f));
            bano2Model = glm::rotate(bano2Model, glm::radians(banoRot2.y), glm::vec3(0.0f, 1.0f, 0.0f));
            bano2Model = glm::rotate(bano2Model, glm::radians(banoRot2.z), glm::vec3(0.0f, 0.0f, 1.0f));
            bano2Model = glm::scale(bano2Model, banoScale2);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bano2Model));
            bano2GLTF->Draw(shaderProgram, solidColorLoc);
        }
        
        if (bano3GLTF && !bano3GLTF->meshes.empty()) {
            glm::mat4 bano3Model = glm::mat4(1.0f);
            bano3Model = glm::translate(bano3Model, banoPos3);
            bano3Model = glm::rotate(bano3Model, glm::radians(banoRot3.x), glm::vec3(1.0f, 0.0f, 0.0f));
            bano3Model = glm::rotate(bano3Model, glm::radians(banoRot3.y), glm::vec3(0.0f, 1.0f, 0.0f));
            bano3Model = glm::rotate(bano3Model, glm::radians(banoRot3.z), glm::vec3(0.0f, 0.0f, 1.0f));
            bano3Model = glm::scale(bano3Model, banoScale3);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bano3Model));
            bano3GLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (bano4GLTF && !bano4GLTF->meshes.empty()) {
            glm::mat4 bano4Model = glm::mat4(1.0f);
            bano4Model = glm::translate(bano4Model, banoPos4);
            bano4Model = glm::rotate(bano4Model, glm::radians(banoRot4.x), glm::vec3(1.0f, 0.0f, 0.0f));
            bano4Model = glm::rotate(bano4Model, glm::radians(banoRot4.y), glm::vec3(0.0f, 1.0f, 0.0f));
            bano4Model = glm::rotate(bano4Model, glm::radians(banoRot4.z), glm::vec3(0.0f, 0.0f, 1.0f));
            bano4Model = glm::scale(bano4Model, banoScale4);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bano4Model));
            bano4GLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (ligthbathroomGLTF && !ligthbathroomGLTF->meshes.empty()) {
            glm::mat4 ligthbathroomModel = glm::mat4(1.0f);
            ligthbathroomModel = glm::translate(ligthbathroomModel, ligthbathroomPos);
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            ligthbathroomModel = glm::scale(ligthbathroomModel, ligthbathroomScale);
            
            // Compensar el offset original del modelo en Blender para centrarlo en su pivote real
            ligthbathroomModel = glm::translate(ligthbathroomModel, glm::vec3(-0.423f, -2.7725f, 2.622f));


            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ligthbathroomModel));
            glUniform1f(emissiveStrengthLoc, 1.5f); // Hacer que brille la lampara
            ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
            glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

            glm::mat4 ligthbathroom3Model = glm::mat4(1.0f);
            ligthbathroom3Model = glm::translate(ligthbathroom3Model, lamp3Pos);
            ligthbathroom3Model = glm::rotate(ligthbathroom3Model, glm::radians(lamp3Pos.x), glm::vec3(1.0f, 0.0f, 0.0f));
            ligthbathroom3Model = glm::rotate(ligthbathroom3Model, glm::radians(lamp3Pos.y), glm::vec3(0.0f, 1.0f, 0.0f));
            ligthbathroom3Model = glm::rotate(ligthbathroom3Model, glm::radians(lamp3Pos.z), glm::vec3(0.0f, 0.0f, 1.0f));
            ligthbathroom3Model = glm::scale(ligthbathroom3Model, lamp3Scale);
            
            // Compensar el offset original del modelo en Blender para centrarlo en su pivote real
            ligthbathroom3Model = glm::translate(ligthbathroom3Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ligthbathroom3Model));
            glUniform1f(emissiveStrengthLoc, 1.5f); // Hacer que brille la lampara
            ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
            glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

            glm::mat4 ligthbathroom4Model = glm::mat4(1.0f);
            ligthbathroom4Model = glm::translate(ligthbathroom4Model, lamp4Pos);
            ligthbathroom4Model = glm::rotate(ligthbathroom4Model, glm::radians(lamp4Pos.x), glm::vec3(1.0f, 0.0f, 0.0f));
            ligthbathroom4Model = glm::rotate(ligthbathroom4Model, glm::radians(lamp4Pos.y), glm::vec3(0.0f, 1.0f, 0.0f));
            ligthbathroom4Model = glm::rotate(ligthbathroom4Model, glm::radians(lamp4Pos.z), glm::vec3(0.0f, 0.0f, 1.0f));
            ligthbathroom4Model = glm::scale(ligthbathroom4Model, lamp4Scale);
            
            // Compensar el offset original del modelo en Blender para centrarlo en su pivote real
            ligthbathroom4Model = glm::translate(ligthbathroom4Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ligthbathroom4Model));
            glUniform1f(emissiveStrengthLoc, 1.5f); // Hacer que brille la lampara
            ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
            glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear
        }

        if (ligthbathroom2GLTF && !ligthbathroom2GLTF->meshes.empty()) {
            glm::mat4 ligthbathroom2Model = glm::mat4(1.0f);
            ligthbathroom2Model = glm::translate(ligthbathroom2Model, ligthbathroom2Pos);
            ligthbathroom2Model = glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            ligthbathroom2Model = glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            ligthbathroom2Model = glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            ligthbathroom2Model = glm::scale(ligthbathroom2Model, ligthbathroom2Scale);
            
            // Compensar el offset original del modelo en Blender para centrarlo en su pivote real
            ligthbathroom2Model = glm::translate(ligthbathroom2Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ligthbathroom2Model));
            glUniform1f(emissiveStrengthLoc, 1.5f); // Hacer que brille la lampara
            ligthbathroom2GLTF->Draw(shaderProgram, solidColorLoc);
            glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear
        }

        if (urinarioGLTF && !urinarioGLTF->meshes.empty()) {
            glm::mat4 urinarioModel = glm::mat4(1.0f);
            urinarioModel = glm::translate(urinarioModel, urinarioPos);
            urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            urinarioModel = glm::scale(urinarioModel, urinarioScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(urinarioModel));
            urinarioGLTF->Draw(shaderProgram, solidColorLoc);
        }
        glUniform1i(solidColorLoc, 0);

        // --- DIBUJAR ENTIDADES 3D ---
        glBindVertexArray(VAO);
        glEnable(GL_DEPTH_TEST);

        for (auto& entity : gameEntities) {
            if (!entity.active || (entity.type != 0 && entity.type != 3 && entity.type < 4) || entity.type == 8 || entity.type == 9) continue; 
            if (entity.type > 0 && entity.type < 3) continue; // Solo procesar tipos 0, 3, 4, 5, 6, 7 aquÃ­

            glm::mat4 entityModel = glm::mat4(1.0f);
            float floatY = 0.0f;
            
            if (dimensionAlterna && entity.type != 7) { 
                floatY = (sin(currentFrame * 2.0f + entity.seed) * 0.8f) + 0.2f;
            }
            
            entityModel = glm::translate(entityModel, glm::vec3(entity.pos.x, entity.pos.y + floatY, entity.pos.z));
            
            if (entity.type == 0 && cartaVertexCount > 0) { // Carta/Papel 3D
                glBindVertexArray(cartaVAO);
                glBindTexture(GL_TEXTURE_2D, clueTexture); 
                glUniform1i(solidColorLoc, 1); 
                entityModel = glm::translate(entityModel, glm::vec3(0.0f, -0.06f, 0.0f)); // Subirla para verla
                entityModel = glm::rotate(entityModel, entity.seed, glm::vec3(0.0f, 1.0f, 0.0f)); 
                entityModel = glm::scale(entityModel, glm::vec3(1.0f, 1.0f, 1.0f)); // Agrandarla
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, cartaVertexCount);
                glUniform1i(solidColorLoc, 0);
                glBindVertexArray(VAO);
            } else if (entity.type == 3 && cablesVertexCount > 0) { // Cables 3D
                glBindVertexArray(cablesVAO);
                glBindTexture(GL_TEXTURE_2D, pcTex); 
                glUniform1i(solidColorLoc, 1); 
                
                // Ajuste de posiciÃ³n para que toque el suelo
                entityModel = glm::translate(entityModel, glm::vec3(-0.99f, -0.12f, 0.0f)); 
                entityModel = glm::scale(entityModel, glm::vec3(0.5f, 0.5f, 0.5f));
                
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, cablesVertexCount);
                glUniform1i(solidColorLoc, 0);
                glBindVertexArray(VAO);
            } else if (entity.type == 4) { // Mesa
                glBindVertexArray(VAO);
                glBindTexture(GL_TEXTURE_2D, wallTex1);
                entityModel = glm::scale(entityModel, glm::vec3(1.2f, 0.8f, 0.8f));
                glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            } else if (entity.type == 5 && objVertexCount > 0) { // PC 3D
                glBindVertexArray(objVAO);
                glBindTexture(GL_TEXTURE_2D, pcTex);
                
                glUniform1i(solidColorLoc, 1); // Ignorar la textura cargada
                
                entityModel = glm::translate(entityModel, glm::vec3(0.0f, -0.1f, 0.0f)); // Bajar a la mesa
                entityModel = glm::rotate(entityModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // EstÃ¡tico
                entityModel = glm::scale(entityModel, glm::vec3(0.6f, 0.6f, 0.6f));
                if (dimensionAlterna) glUniform3f(colorLoc, 1.0f, 0.4f, 0.4f);
                else glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Usar colores 100% reales de Blender
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, objVertexCount);
                
                glUniform1i(solidColorLoc, 0); // Restaurar texturas normales
                
                glBindVertexArray(VAO); // Restaurar VAO
            } else if (entity.type == 6) { // MÃ¡quina Lab
                glBindVertexArray(VAO);
                glBindTexture(GL_TEXTURE_2D, wallTex3);
                entityModel = glm::scale(entityModel, glm::vec3(0.8f, 2.0f, 0.8f));
                glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            } else if (entity.type == 7) { // Portal
                glBindVertexArray(VAO);
                glBindTexture(GL_TEXTURE_2D, portalTex);
                entityModel = glm::scale(entityModel, glm::vec3(1.5f, 1.5f, 1.5f));
                if (portalActivado) {
                    entityModel = glm::rotate(entityModel, currentFrame * 2.0f, glm::vec3(1.0f, 1.0f, 1.0f));
                    float pulse = 0.8f + 0.2f * sin(currentFrame * 15.0f);
                    glUniform3f(colorLoc, pulse, 0.0f, 0.0f); 
                } else {
                    entityModel = glm::rotate(entityModel, currentFrame * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
                    glUniform3f(colorLoc, 0.2f, 0.5f, 1.0f); 
                }
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        if (showCollisionViewer) {
            glBindVertexArray(VAO);
            glBindTexture(GL_TEXTURE_2D, wallTex1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_CULL_FACE);
            glLineWidth(2.0f);

            if (collisionShowWalls) {
                for (int z = 0; z < MAP_HEIGHT; ++z) {
                    for (int x = 0; x < MAP_WIDTH; ++x) {
                        int blockType = worldMap[z][x];
                        if (blockType <= 0) continue;

                        glm::vec2 cellCenter((float)x, (float)z);
                        if (glm::length(cellCenter - glm::vec2(cameraPos.x, cameraPos.z)) > collisionViewerRadius) continue;

                        float scaleX = wallWidth;
                        float scaleZ = wallWidth;
                        bool hasLeft = (x > 0 && worldMap[z][x - 1] > 0);
                        bool hasRight = (x < MAP_WIDTH - 1 && worldMap[z][x + 1] > 0);
                        bool hasUp = (z > 0 && worldMap[z - 1][x] > 0);
                        bool hasDown = (z < MAP_HEIGHT - 1 && worldMap[z + 1][x] > 0);
                        if (hasLeft || hasRight) scaleX = 1.0f;
                        if (hasUp || hasDown) scaleZ = 1.0f;

                        glm::mat4 debugModel = glm::mat4(1.0f);
                        debugModel = glm::translate(debugModel, glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f, (float)z));
                        debugModel = glm::scale(debugModel, glm::vec3(scaleX + 0.02f, wallHeight + 0.02f, scaleZ + 0.02f));
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));

                        if (blockType == 8 || blockType == 9) glUniform3f(colorLoc, 1.0f, 0.4f, 0.1f);
                        else glUniform3f(colorLoc, 0.1f, 1.0f, 0.2f);

                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }

            if (collisionShowProps) {
                for (const auto& entity : gameEntities) {
                    if (!entity.active) continue;
                    if (entity.type != 4 && entity.type != 6) continue;
                    if (glm::length(glm::vec2(entity.pos.x - cameraPos.x, entity.pos.z - cameraPos.z)) > collisionViewerRadius) continue;

                    glm::vec3 scale = (entity.type == 4) ? glm::vec3(1.6f, 1.0f, 1.2f) : glm::vec3(1.2f, 2.2f, 1.2f);
                    glm::mat4 debugModel = glm::mat4(1.0f);
                    debugModel = glm::translate(debugModel, glm::vec3(entity.pos.x, entity.pos.y + 0.2f, entity.pos.z));
                    debugModel = glm::scale(debugModel, scale);
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
                    glUniform3f(colorLoc, 1.0f, 0.9f, 0.2f);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }

            glLineWidth(1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // --- DIBUJAR ENTIDADES 2D ---
        glBindVertexArray(quadVAO);
        glDisable(GL_CULL_FACE);

        for (auto& entity : gameEntities) {
            if (!entity.active || (entity.type >= 3 && entity.type != 8 && entity.type != 9)) continue; 
            if (entity.type == 0 || entity.type == 3) continue; // Ya se dibujaron en 3D
            if (entity.type == 2 && !portalActivado) continue; 

            if (entity.type == 1) {
                glBindTexture(GL_TEXTURE_2D, batteryTex);
            } else if (entity.type == 8 || entity.type == 9) {
                glBindTexture(GL_TEXTURE_2D, keycardTex);
            } else if (entity.type == 0 || entity.type == 3) {
                glBindTexture(GL_TEXTURE_2D, clueTexture);
            } else {
                glBindTexture(GL_TEXTURE_2D, enemyTexture);
            }

            glm::mat4 entityModel = glm::mat4(1.0f);
            
            // FlotaciÃ³n sutil de objetos clave para visibilidad---
            float bounce = 0.0f;
            if(entity.type == 8 || entity.type == 9) bounce = sin(currentFrame * 3.0f) * 0.1f;
            
            float targetY = entity.pos.y + bounce;

            entityModel = glm::translate(entityModel, glm::vec3(entity.pos.x, targetY, entity.pos.z));
            
            float anguloHaciaCamara = atan2(cameraPos.x - entity.pos.x, cameraPos.z - entity.pos.z);
            entityModel = glm::rotate(entityModel, anguloHaciaCamara, glm::vec3(0.0f, 1.0f, 0.0f));

            float escala = (entity.type != 2) ? 0.3f : 0.9f; 

            entityModel = glm::scale(entityModel, glm::vec3(escala, escala, escala));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
            
            if (entity.type == 2 && dimensionAlterna) {
                float pulse = 0.5f + 0.5f * sin(currentFrame * 10.0f);
                glUniform3f(colorLoc, pulse, 0.1f, 0.1f);
            } else if (entity.type == 8) {
                glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // Tarjeta amarilla
            } else if (entity.type == 9) {
                glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // Tarjeta roja
            } else {
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            }
            
            glDrawArrays(GL_TRIANGLES, 0, 6); 
        }

        if (gameState == MENU) {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBindTexture(GL_TEXTURE_2D, logoTexture);
            glBindVertexArray(quadVAO);

            float aspect = (float)currentWidth / (float)currentHeight;
            glm::mat4 orthoProj = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProj));

            glm::mat4 orthoView = glm::mat4(1.0f);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(orthoView));

            glm::mat4 orthoModel = glm::mat4(1.0f);
            float scale = 0.8f + sin(glfwGetTime() * 3.0f) * 0.05f; 
            orthoModel = glm::scale(orthoModel, glm::vec3(scale, scale, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(orthoModel));

            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        } else if (gameState == PLAYING) {
            // --- DIBUJAR CROSSHAIR (HUD) ---
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // Color invertido para que se vea siempre

            glBindVertexArray(quadVAO);
            float aspect = (float)currentWidth / (float)currentHeight;
            glm::mat4 orthoProj = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProj));
            
            glm::mat4 orthoView = glm::mat4(1.0f);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(orthoView));

            glm::mat4 orthoModel = glm::mat4(1.0f);
            orthoModel = glm::scale(orthoModel, glm::vec3(0.015f, 0.015f * aspect, 1.0f)); // Puntito en el centro
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(orthoModel));

            glBindTexture(GL_TEXTURE_2D, clueTexture); // Textura blanca genÃ©rica
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        // --- RENDER IMGUI HUD ---
        // Display coordinates top-left
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
        ImGui::SetNextWindowSize(ImVec2(180.0f, 50.0f));
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("Coords", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "POSICION ACTUAL:");
        ImGui::Text("X: %.1f  |  Z: %.1f", cameraPos.x, cameraPos.z);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2((float)currentWidth - 350.0f, 10.0f));
        ImGui::SetNextWindowSize(ImVec2(340.0f, 340.0f));
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("Editor Bano", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("Ajuste en tiempo real (sin recompilar)");
        ImGui::Separator();
        ImGui::Text("Bano");
        ImGui::DragFloat3("Bano Pos", &banoPos.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Bano Rot", &banoRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Bano Scale", &banoScale.x, 0.01f, 0.05f, 2.0f);
        ImGui::Separator();
        ImGui::Text("Lavamanos");
        ImGui::DragFloat3("Lava Pos", &lavamanosPos.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Lava Rot", &lavamanosRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Lava Scale", &lavamanosScale.x, 0.01f, 0.05f, 2.0f);
        if (ImGui::Button("Traer lavamanos frente a camara")) {
            lavamanosPos = cameraPos + cameraFront * 0.8f;
            lavamanosPos.y = cameraPos.y;
            lavamanosRot = glm::vec3(0.0f, 0.0f, 0.0f);
            lavamanosScale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        ImGui::Separator();
        ImGui::Text("Urinario");
        ImGui::DragFloat3("Uri Pos", &urinarioPos.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Uri Rot", &urinarioRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Uri Scale", &urinarioScale.x, 0.01f, 0.05f, 2.0f);// los parametros de dragfloat3 son name, escalas y posicion inicial
        ImGui::Separator();
        ImGui::Text("Bano 2");
        ImGui::DragFloat3("Bano 2 Pos", &banoPos2.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Bano 2 Rot", &banoRot2.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Bano 2 Scale", &banoScale2.x, 0.01f, 0.05f, 2.0f);
        ImGui::Separator();
        ImGui::Text("Bano 3");
        ImGui::DragFloat3("Bano 3 Pos", &banoPos3.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Bano 3 Rot", &banoRot3.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Bano 3 Scale", &banoScale3.x, 0.01f, 0.05f, 2.0f);
        ImGui::Separator();
        ImGui::Text("Bano 4");
        ImGui::DragFloat3("Bano 4 Pos", &banoPos4.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Bano 4 Rot", &banoRot4.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Bano 4 Scale", &banoScale4.x, 0.01f, 0.05f, 2.0f);
        ImGui::Separator();

        ImGui::Text("Lampara bano");
        ImGui::DragFloat3("Lampara bano Pos", &ligthbathroomPos.x, 0.05f);
        ImGui::DragFloat3("Lampara bano Rot", &ligthbathroomRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Lampara bano Scale", &ligthbathroomScale.x, 0.01f, 0.05f, 2.0f);
        ImGui::Checkbox("Lampara modo debug visible", &ligthbathroomDebugVisible);
        ImGui::Separator();
        ImGui::Text("Azulejo");
        ImGui::DragFloat3("Azulejo Pos", &azulejoPos.x, 0.05f);
        ImGui::DragFloat3("Azulejo Rot", &azulejoRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Azulejo Scale", &azulejoScale.x, 0.01f, 0.05f, 2.0f);
        if (ImGui::Button("Traer azulejo frente a camara")) {
            azulejoPos = cameraPos + cameraFront * 0.8f;
            azulejoPos.y = cameraPos.y;
            azulejoRot = glm::vec3(0.0f, 0.0f, 0.0f);
            azulejoScale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        ImGui::Separator();
       

        ImGui::End();

        static int selectedEntityIndex = 0;
        static bool entityOnlyCollectibles = false;
        if (!gameEntities.empty()) {
            if (selectedEntityIndex < 0) selectedEntityIndex = 0;
            if (selectedEntityIndex >= (int)gameEntities.size()) selectedEntityIndex = (int)gameEntities.size() - 1;
        }

        ImGui::SetNextWindowPos(ImVec2((float)currentWidth - 350.0f, 360.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(340.0f, 330.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.78f);
        ImGui::Begin("Editor Entidades", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("Mover pickups y props en runtime");
        ImGui::Separator();

        if (gameEntities.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No hay entidades cargadas.");
        } else {
            ImGui::Checkbox("Solo pickups clave", &entityOnlyCollectibles);

            if (ImGui::BeginListBox("Entidades", ImVec2(0.0f, 120.0f))) {
                for (int i = 0; i < (int)gameEntities.size(); ++i) {
                    const Entity& entity = gameEntities[i];
                    if (entityOnlyCollectibles && !(entity.type == 0 || entity.type == 1 || entity.type == 8 || entity.type == 9)) continue;

                    std::string label = std::to_string(i) + " - " + getEntityTypeLabel(entity.type);
                    label += entity.active ? " [ON]" : " [OFF]";
                    if (!entity.text.empty()) label += " *";

                    bool isSelected = (selectedEntityIndex == i);
                    if (ImGui::Selectable(label.c_str(), isSelected)) selectedEntityIndex = i;
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            Entity& selectedEntity = gameEntities[selectedEntityIndex];
            ImGui::Text("Tipo: %s", getEntityTypeLabel(selectedEntity.type));
            ImGui::Text("Indice: %d", selectedEntityIndex);
            ImGui::Checkbox("Activa", &selectedEntity.active);
            ImGui::DragFloat3("Posicion", &selectedEntity.pos.x, 0.05f);
            ImGui::DragFloat("Seed", &selectedEntity.seed, 0.05f, -100.0f, 100.0f);

            if (ImGui::Button("Traer frente a camara")) {
                selectedEntity.pos = cameraPos + cameraFront * 1.2f;
                selectedEntity.pos.y = cameraPos.y - 0.2f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Ir a entidad")) {
                cameraPos = selectedEntity.pos - glm::normalize(cameraFront) * 1.5f;
                cameraPos.y = baseCameraY;
            }

            std::string exportLine =
                "{glm::vec3(" + std::to_string(selectedEntity.pos.x) + "f, " +
                std::to_string(selectedEntity.pos.y) + "f, " +
                std::to_string(selectedEntity.pos.z) + "f), " +
                std::to_string(selectedEntity.type) + ", " +
                (selectedEntity.active ? "true" : "false") + ", \"" +
                selectedEntity.text + "\", " +
                std::to_string(selectedEntity.seed) + "f},";

            ImGui::Separator();
            ImGui::TextWrapped("Export rapido:");
            ImGui::InputTextMultiline("##entity_export", exportLine.data(), exportLine.size() + 1, ImVec2(0.0f, 70.0f), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::Button("Copiar linea")) {
                ImGui::SetClipboardText(exportLine.c_str());
            }

            if (!selectedEntity.text.empty()) {
                ImGui::Separator();
                ImGui::TextWrapped("Texto:");
                ImGui::BeginChild("entity_text_preview", ImVec2(0.0f, 45.0f), true);
                ImGui::TextWrapped("%s", selectedEntity.text.c_str());
                ImGui::EndChild();
            }
        }

        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(10.0f, 70.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240.0f, 205.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.62f);
        ImGui::Begin("Debug Juego", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("Zona: %d", currentZone);
        ImGui::Text("Baterias: %d / 3", bateriasRecolectadas);
        ImGui::Text("Tarjeta N1: %s", hasKeycardLvl1 ? "SI" : "NO");
        ImGui::Text("Tarjeta N2: %s", hasKeycardLvl2 ? "SI" : "NO");
        ImGui::Text("Portal: %s", portalActivado ? "ACTIVO" : "INACTIVO");
        ImGui::Text("Dimension: %s", dimensionAlterna ? "ALTERNA" : "NORMAL");
        ImGui::Separator();
        ImGui::Checkbox("Collision Viewer", &showCollisionViewer);
        ImGui::Checkbox("Interaction Debug", &showInteractionDebugger);
        ImGui::Checkbox("Animation Tester", &showAnimationTester);
        ImGui::Checkbox("Spawn Inspector", &showSpawnInspector);
        ImGui::End();

        if (showInteractionDebugger) {
            float focusedDistance = 0.0f;
            float focusedAngle = 0.0f;
            int focusedIndex = findFocusedEntityIndex(&focusedDistance, &focusedAngle);
            int doorGridX = 0;
            int doorGridZ = 0;
            int doorBlockType = 0;
            bool hasDoorAhead = findDoorAhead(doorGridX, doorGridZ, doorBlockType);

            ImGui::SetNextWindowPos(ImVec2(10.0f, 285.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(290.0f, 220.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.72f);
            ImGui::Begin("Interaction Debugger", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("Camara dir: %.2f %.2f %.2f", cameraFront.x, cameraFront.y, cameraFront.z);
            ImGui::Separator();

            if (focusedIndex >= 0) {
                const Entity& focusedEntity = gameEntities[focusedIndex];
                ImGui::Text("Entidad enfocada: %d", focusedIndex);
                ImGui::Text("Tipo: %s", getEntityTypeLabel(focusedEntity.type));
                ImGui::Text("Distancia: %.2f", focusedDistance);
                ImGui::Text("Dot mira: %.3f", focusedAngle);
                if (!focusedEntity.text.empty()) {
                    ImGui::TextWrapped("Texto: %s", focusedEntity.text.c_str());
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "No hay entidad enfocada.");
            }

            ImGui::Separator();
            if (hasDoorAhead) {
                bool hasAccess = (doorBlockType == 8 || doorBlockType == -8) ? hasKeycardLvl1 : hasKeycardLvl2;
                ImGui::Text("Puerta enfrente: %s", getDoorDebugLabel(doorBlockType));
                ImGui::Text("Celda: %d, %d", doorGridX, doorGridZ);
                ImGui::Text("Acceso actual: %s", hasAccess ? "SI" : "NO");
            } else {
                ImGui::Text("Puerta enfrente: no");
            }
            ImGui::End();
        }

        if (showAnimationTester) {
            ImGui::SetNextWindowPos(ImVec2(260.0f, 70.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(310.0f, 300.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.78f);
            ImGui::Begin("Animation Tester", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            if (!gnomeGLTF) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No se cargo gnome.glb");
            } else {
                int animCount = gnomeGLTF->GetAnimationCount();
                ImGui::Text("Gnomo activo: %s", isGnomeActive ? "SI" : "NO");
                ImGui::DragFloat3("Gnomo Pos", &gnomePos.x, 0.05f);
                ImGui::Checkbox("Forzar animacion", &gnomeForceAnimation);
                ImGui::SliderFloat("Velocidad", &gnomeAnimSpeed, 0.0f, 3.0f, "%.2f");
                ImGui::Checkbox("Loop manual", &gnomeAnimLoop);

                if (animCount > 0) {
                    if (gnomeDebugAnimIndex < 0) gnomeDebugAnimIndex = 0;
                    if (gnomeDebugAnimIndex >= animCount) gnomeDebugAnimIndex = animCount - 1;

                    ImGui::SliderInt("Indice anim", &gnomeDebugAnimIndex, 0, animCount - 1);
                    std::string animName = gnomeGLTF->GetAnimationName(gnomeDebugAnimIndex);
                    float animLength = gnomeGLTF->GetAnimationLengthSeconds(gnomeDebugAnimIndex);
                    if (animName.empty()) animName = "(sin nombre)";
                    ImGui::TextWrapped("Anim actual: %s", animName.c_str());
                    ImGui::Text("Duracion aprox: %.2f s", animLength);

                    float maxPreviewTime = animLength > 0.05f ? animLength : 10.0f;
                    if (gnomeAnimPreviewTime > maxPreviewTime) gnomeAnimPreviewTime = maxPreviewTime;
                    ImGui::SliderFloat("Pose / tiempo", &gnomeAnimPreviewTime, 0.0f, maxPreviewTime, "%.2f s");

                    if (ImGui::Button("Reset pose")) {
                        gnomeAnimPreviewTime = 0.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Frente a camara")) {
                        gnomePos = cameraPos + cameraFront * 2.0f;
                        gnomePos.y = -0.4f;
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "El modelo no trae animaciones.");
                }
            }
            ImGui::End();
        }

        if (showSpawnInspector) {
            auto teleportNear = [&](const glm::vec3& target) {
                cameraPos = target + glm::vec3(0.0f, 0.0f, 1.8f);
                cameraPos.y = baseCameraY;
                updateZone();
            };

            auto findEntityByType = [&](int type) -> int {
                for (int i = 0; i < (int)gameEntities.size(); ++i) {
                    if (gameEntities[i].type == type && gameEntities[i].active) return i;
                }
                return -1;
            };

            ImGui::SetNextWindowPos(ImVec2(260.0f, 380.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(310.0f, 250.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.78f);
            ImGui::Begin("Spawn Inspector", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("Spawn guardado");
            ImGui::Text("Pos: %.1f %.1f %.1f", debugSpawnPos.x, debugSpawnPos.y, debugSpawnPos.z);
            ImGui::Text("Yaw/Pitch: %.1f / %.1f", debugSpawnYaw, debugSpawnPitch);
            if (ImGui::Button("Guardar actual")) {
                debugSpawnPos = cameraPos;
                debugSpawnYaw = yaw;
                debugSpawnPitch = pitch;
            }
            ImGui::SameLine();
            if (ImGui::Button("Ir a guardado")) {
                cameraPos = debugSpawnPos;
                yaw = debugSpawnYaw;
                pitch = debugSpawnPitch;
                updateZone();
            }

            ImGui::Separator();
            if (ImGui::Button("Inicio")) {
                cameraPos = glm::vec3(6.0f, baseCameraY, 5.0f);
                yaw = -90.0f;
                pitch = 0.0f;
                updateZone();
            }
            ImGui::SameLine();
            if (ImGui::Button("Gnomo")) {
                teleportNear(gnomePos);
            }

            int yellowKeycardIndex = findEntityByType(8);
            int redKeycardIndex = findEntityByType(9);
            int portalIndex = findEntityByType(7);

            if (ImGui::Button("Tarjeta amarilla") && yellowKeycardIndex >= 0) teleportNear(gameEntities[yellowKeycardIndex].pos);
            ImGui::SameLine();
            if (ImGui::Button("Tarjeta roja") && redKeycardIndex >= 0) teleportNear(gameEntities[redKeycardIndex].pos);

            if (ImGui::Button("Portal") && portalIndex >= 0) teleportNear(gameEntities[portalIndex].pos);
            ImGui::SameLine();
            if (!gameEntities.empty() && ImGui::Button("Entidad seleccionada")) {
                teleportNear(gameEntities[selectedEntityIndex].pos);
            }
            ImGui::End();
        }

        if (showCollisionViewer) {
            ImGui::SetNextWindowPos(ImVec2(10.0f, 515.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(290.0f, 120.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.72f);
            ImGui::Begin("Collision Viewer", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Paredes", &collisionShowWalls);
            ImGui::Checkbox("Props bloqueantes", &collisionShowProps);
            ImGui::SliderFloat("Radio", &collisionViewerRadius, 2.0f, 20.0f, "%.1f");
            ImGui::End();
        }


        if (hudMessageTimer > 0.0f) {
            ImGui::SetNextWindowPos(ImVec2(currentWidth * 0.1f, currentHeight * 0.8f));
            ImGui::SetNextWindowSize(ImVec2(currentWidth * 0.8f, currentHeight * 0.2f));
            ImGui::SetNextWindowBgAlpha(0.0f); // Transparente
            ImGui::Begin("HUD", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
            ImGui::SetWindowFontScale(1.2f);
            
            // Centrar el texto
            float textWidth = ImGui::CalcTextSize(currentHUDMessage.c_str()).x;
            ImGui::SetCursorPosX((currentWidth * 0.8f - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", currentHUDMessage.c_str());
            ImGui::End();
        }

        if (isReadingDocument) {
            ImGui::SetNextWindowPos(ImVec2(currentWidth * 0.18f, currentHeight * 0.16f));
            ImGui::SetNextWindowSize(ImVec2(currentWidth * 0.64f, currentHeight * 0.5f));
            ImGui::SetNextWindowBgAlpha(0.94f);
            ImGui::Begin("Documento", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
            ImGui::SetWindowFontScale(1.15f);
            ImGui::TextWrapped("%s", currentDocumentTitle.c_str());
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("%s", currentDocumentBody.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "E o ESC para cerrar");
            ImGui::End();
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    ma_sound_uninit(&bgm);
    ma_engine_uninit(&audioEngine);
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
    glfwTerminate();
    return 0;
}
