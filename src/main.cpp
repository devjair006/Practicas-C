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
#include "headers/shader.h"
#include "headers/texture.h"

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Variables de HUD (ImGui)----
std::string currentHUDMessage = "";
float hudMessageTimer = 0.0f;
bool isReadingDocument = false;
std::string currentDocumentTitle = "";
std::string currentDocumentBody = "";

glm::vec3 ligthbathroomPos(34.8f, 1.8f, 4.0f);
glm::vec3 ligthbathroomRot(-90.0f, 0.0f, 0.0f);
glm::vec3 ligthbathroomScale(0.4f, 0.4f, 0.4f);
bool ligthbathroomDebugVisible = true;

glm::vec3 banoPos(35.6f, -0.5f, 1.740f);
glm::vec3 banoRot(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale(0.5f, 0.4f, 0.4f);

glm::vec3 lavamanosPos(34.7f, -0.590f, 3.400f);
glm::vec3 lavamanosRot(-90.0f, 0.0f, -90.0f);
glm::vec3 lavamanosScale(0.4f, 0.4f, 0.4f);

glm::vec3 urinarioPos(33.2f, -0.5f, 5.2f);
glm::vec3 urinarioRot(-90.0f, 0.0f, 90.0f);
glm::vec3 urinarioScale(0.4f, 0.4f, 0.4f);


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

        vec4 texColor = texture(texture1, TexCoord);
        if (useSolidColor == 1) {
            texColor = vec4(ObjColor, 1.0); // Usar el color empaquetado del OBJ
        } else if (texColor.a < 0.1) {
            discard; 
        }
        
        vec3 result = (ambient + diffuse) * objectColor;
        
        if (dimensionAlterna == 1) {
            vec2 uv = gl_FragCoord.xy / resolution;
            float distToCenter = distance(uv, vec2(0.5));
            result *= smoothstep(0.9, 0.2, distToCenter);
        }
        
        FragColor = texColor * vec4(result, 1.0);
    }
)";

glm::vec3 cameraPos   = glm::vec3(6.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float yaw   = -90.0f;	
float pitch =  0.0f;	
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;

float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// Headbobbing (Movimiento de cÃ¡mara al caminar)
float headBobTimer = 0.0f;
float baseCameraY = 0.0f;
bool isMoving = false;

// Stamina y Sprint
float stamina = 100.0f;
bool isSprinting = false;
bool isExhausted = false;

enum GameState { MENU, PLAYING, GAMEOVER };
GameState gameState = MENU; 

bool isCursorLocked = false; 
bool tabKeyWasPressed = false;
bool eKeyWasPressed = false;
bool isFlashlightOn = true;
bool fKeyWasPressed = false;

ma_engine audioEngine;

// Inventario
int bateriasRecolectadas = 0;
bool hasKeycardLvl1 = false; // Llave amarilla (Control)
bool hasKeycardLvl2 = false; // Llave roja (Lab)
bool dimensionAlterna = false;
bool portalActivado = false;
int currentZone = 1;

// ==========================================
// ENTIDADES
// ==========================================
struct Entity {
    glm::vec3 pos;  
    int type; // 0=Log, 1=BaterÃ­a, 2=Entidad, 3=ObjetoAmbiental, 4=Mesa, 5=Monitor, 6=MÃ¡quina, 7=Portal, 8=TarjetaNv1, 9=TarjetaNv2
    bool active;    
    std::string text;
    float seed;
};

std::vector<Entity> gameEntities = {
    // --- ZONA NORTE (z=2 a 11) : INICIO, OFICINAS, BAÃ‘OS ---
    {glm::vec3(8.0f, -0.4f, 4.0f), 3, true, "[CABLE SUELTO]:Hay un cable pelado aqui.", 0.0f},
    {glm::vec3(20.0f, -0.4f, 5.0f), 0, true, "LOG 1 (Arrugado): 'Apagon general. Las compuertas se bloquearon.'", 0.0f},
    {glm::vec3(42.0f, -0.2f, 5.0f), 8, true, "", 0.0f}, // TARJETA NV 1 (Amarilla) en BaÃ±os
    {glm::vec3(12.0f, -0.2f, 6.0f), 1, true, "", 0.0f}, // Bateria 1 en Sala Descanso
    {glm::vec3(24.0f, -0.5f, 6.0f), 4, true, "", 1.0f}, // Mesa en Oficinas
    {glm::vec3(24.0f, 0.0f, 6.0f), 5, true, "[MONITOR AUXILIAR]: 'Sistema inestable.'", 1.5f},
    
    // --- ZONA MEDIA (z=13 a 20) : LABS, FRIGORIFICO, CONTENCION ---
    {glm::vec3(10.0f, -0.5f, 15.0f), 4, true, "", 2.0f}, // Mesa en Labs
    {glm::vec3(10.0f, 0.0f, 15.0f), 5, true, "[PANTALLA ERROR]: 'Falla de contencion.'", 2.5f},
    {glm::vec3(28.0f, 0.0f, 16.0f), 6, true, "[MAQUINA]: Unidad Frigorifica.", 5.0f}, // Maquina en Frigorifico
    {glm::vec3(42.0f, -0.4f, 17.0f), 9, true, "", 0.0f}, // TARJETA NV 2 (Roja) en Contencion
    {glm::vec3(42.0f, -0.2f, 15.0f), 0, true, "LOG 2 (Sangriento): 'La muestra escapo.'", 0.0f},
    {glm::vec3(15.0f, -0.2f, 18.0f), 1, true, "", 0.0f}, // Bateria 2 en Labs
    
    // --- ZONA SUR (z=25 a 43) : VENTILACION, PRUEBAS, GENERADORES ---
    {glm::vec3(10.0f, -0.4f, 28.0f), 3, true, "[MANCHA]: Rastro oscuro hacia ventilacion.", 0.0f},
    {glm::vec3(10.0f, -0.5f, 38.0f), 4, true, "", 3.0f}, // Mesa en Sala Pruebas
    {glm::vec3(10.0f, 0.0f, 38.0f), 5, true, "[REGISTRO MAESTRO]: 'EVACUACION INMEDIATA.'", 3.5f},
    {glm::vec3(28.0f, 0.0f, 38.0f), 6, true, "[GENERADOR]: Requiere reinicio.", 6.0f},
    {glm::vec3(22.0f, -0.2f, 40.0f), 1, true, "", 0.0f}, // Bateria 3 en Generadores
    
    // --- CORREDOR FINAL (z=45 a 47) ---
    {glm::vec3(24.0f, 1.0f, 46.0f), 7, true, "[PALANCA MAESTRA]: Energia restaurada.", 0.0f},

    // La Entidad
    {glm::vec3(25.0f, 0.0f, 3.0f), 2, true, "", 0.0f}
};

// ==========================================
// MAPA EXPANDIDO (24x32) - Laboratorio Estructurado
// 0=VacÃ­o, 1=Pasillo, 2=Control, 3=Lab, 4=Bloque sÃ³lido invisible, 8=Puerta Nivel 1, 9=Puerta Nivel 2
// ==========================================
const int MAP_WIDTH = 50;
const int MAP_HEIGHT = 50;

// --- ConfiguraciÃ³n de dimensiones de paredes ---
float wallWidth = 0.3f;  // Grosor visual de las paredes (se estira automÃ¡ticamente si hay vecinos)
float wallHeight = 1.0f; // 1.0 es la altura estÃ¡ndar

// --- Animacion de Puertas ---
float door1Anim = 0.0f; // 0.0 a 90.0 grados
bool door1Opening = false;
float door2Anim = 0.0f;
bool door2Opening = false;

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    // ============ PROYECTO ÃTOMO - NIVEL -4 ============
    // 0=VacÃ­o, 1=Pared, 8=Puerta Nivel 1, 9=Puerta Nivel 2
    // NORTE (z=0): Borde superior
    //0000 esos q estan ahi van a equivales a los asensores 
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    // z=1: Acceso desde Nivel -3 (entrada superior) + inicio de salas
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    // z=2-8: SALA DE DESCANSO(1) | ZONA DE OFICINAS(2) | BAÃ‘OS(3)
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    // z=9: Pared entre salas norte y corredor principal (con entradas a cada sala)
    {1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1},
    // z=10-11: CORREDOR PRINCIPAL ESTE-OESTE
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    // z=12: Pared sur del corredor (puerta 8=Nivel1 hacia Labs ClÃ­nicos)
    {1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,8,8,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1},
    // z=13-20: SALA VIGILANCIA | LABS CLÃNICOS(4) | C.FRIGORÃFICA(5) | CONTENCIÃ“N(6)
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    // z=21: Pared divisoria medio-sur (entradas a VentilaciÃ³n y corredor)
    {1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1},
    // z=22-23: CORREDOR CENTRAL
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    // z=24: Pared con entradas a zona sur
    {1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1},
    // z=25-33: CONDUCTOS VENTILACIÃ“N(7) izq | SALA DE PRUEBAS(8) centro-izq
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    // z=34: Pared divisoria con puerta 9 (Nivel 2) hacia Generadores
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    // z=35-43: SALA DE PRUEBAS(8) izq | SALA DE GENERADORES(9) der | SALIDA(10)
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    // z=44: Pared sur con entrada al corredor de interruptores
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    // z=45-47: CORREDOR DE INTERRUPTORES + PALANCA MAESTRA
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    // z=48-49: Pared inferior + borde
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// ==========================================
// FUNCIONES DE CONTROL
// ==========================================
bool checkCollision(float x, float z) {
    float playerRadius = 0.25f; 
    
    // Verificar celdas vecinas
    int startX = (int)floor(x - 1.0f);
    int endX   = (int)ceil(x + 1.0f);
    int startZ = (int)floor(z - 1.0f);
    int endZ   = (int)ceil(z + 1.0f);

    for (int cz = startZ; cz <= endZ; cz++) {
        for (int cx = startX; cx <= endX; cx++) {
            if (cx < 0 || cx >= MAP_WIDTH || cz < 0 || cz >= MAP_HEIGHT) {
                return true; // LÃ­mites sÃ³lidos del mapa
            }
            
            if (worldMap[cz][cx] > 0) {
                // AABB de la pared: delgada por defecto, se estira si tiene vecinos
                float halfX = wallWidth / 2.0f;
                float halfZ = wallWidth / 2.0f;
                bool leftW  = (cx > 0 && worldMap[cz][cx-1] > 0);
                bool rightW = (cx < MAP_WIDTH-1 && worldMap[cz][cx+1] > 0);
                bool upW    = (cz > 0 && worldMap[cz-1][cx] > 0);
                bool downW  = (cz < MAP_HEIGHT-1 && worldMap[cz+1][cx] > 0);
                if (leftW || rightW) halfX = 0.5f;
                if (upW || downW) halfZ = 0.5f;

                float wallMinX = (float)cx - halfX;
                float wallMaxX = (float)cx + halfX;
                float wallMinZ = (float)cz - halfZ;
                float wallMaxZ = (float)cz + halfZ;

                // Encontrar el punto mÃ¡s cercano en la caja al jugador
                float closestX = glm::clamp(x, wallMinX, wallMaxX);
                float closestZ = glm::clamp(z, wallMinZ, wallMaxZ);

                float dx = x - closestX;
                float dz = z - closestZ;
                if ((dx * dx + dz * dz) <= (playerRadius * playerRadius)) return true;
            }
        }
    }
    
    // ColisiÃ³n con entidades (objetos grandes como mesas)
    for (auto& entity : gameEntities) {
        if (!entity.active) continue;
        if (entity.type == 4 || entity.type == 6) { 
            float dist = glm::length(glm::vec2(x - entity.pos.x, z - entity.pos.z));
            if (dist < 0.8f) return true; 
        }
    }
    
    return false; 
}

void updateZone() {
    if (cameraPos.z >= 35.0f) currentZone = 1;
    else if (cameraPos.z >= 15.0f) currentZone = 2;
    else currentZone = 3;
}

void printTypewriter(std::string text) {
    currentHUDMessage = text;
    hudMessageTimer = 5.0f; // Mostrar por 5 segundos
    std::cout << "\n> " << text << "\n" << std::endl; // Mantenemos el log por si acaso
}

void openDocument(const std::string& title, const std::string& body) {
    currentDocumentTitle = title;
    currentDocumentBody = body;
    isReadingDocument = true;
}

void closeDocument() {
    isReadingDocument = false;
    currentDocumentTitle.clear();
    currentDocumentBody.clear();
}

void tryOpenDoor(GLFWwindow *window) {
    // Escaneo de los bloques frente a la cÃ¡mara (rango 1.5)
    glm::vec3 checkPos = cameraPos + cameraFront * 1.5f;
    int gridX = (int)round(checkPos.x);
    int gridZ = (int)round(checkPos.z);
    
    if (gridX >= 0 && gridX < MAP_WIDTH && gridZ >= 0 && gridZ < MAP_HEIGHT) {
        int targetBlock = worldMap[gridZ][gridX];
        
        if (targetBlock == 8) { // Puerta Amarilla
            if (hasKeycardLvl1) {
                // Abrir ambas celdas de la puerta en esta fila
                for (int cx = 0; cx < MAP_WIDTH; cx++) {
                    if (worldMap[gridZ][cx] == 8) worldMap[gridZ][cx] = -8;
                }
                door1Opening = true; // Iniciar animacion
                printTypewriter("[PUERTA]: Tarjeta Nivel 1 Aceptada. Accediendo a Sala de Control.");
                ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
            } else {
                printTypewriter("[PUERTA BLOQUEADA]: Se requiere Tarjeta Amarilla (Nivel 1).");
            }
        } else if (targetBlock == 9) { // Puerta Roja
            if (hasKeycardLvl2) {
                // Abrir ambas celdas de la puerta en esta fila
                for (int cx = 0; cx < MAP_WIDTH; cx++) {
                    if (worldMap[gridZ][cx] == 9) worldMap[gridZ][cx] = -9;
                }
                door2Opening = true; // Iniciar animacion
                printTypewriter("[PUERTA]: Tarjeta Nivel 2 Aceptada. Peligro: Zona de Alta Radiacion.");
                ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
            } else {
                printTypewriter("[PUERTA BLOQUEADA]: Se requiere Tarjeta Roja (Nivel 2).");
            }
        }
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        if (isReadingDocument) closeDocument();
        else glfwSetWindowShouldClose(window, true);

    if (gameState == GAMEOVER) return;

    if (isReadingDocument) {
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            if (!eKeyWasPressed) {
                closeDocument();
                eKeyWasPressed = true;
            }
        } else {
            eKeyWasPressed = false;
        }
        return;
    }

    if (gameState == MENU) {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            gameState = PLAYING;
            isCursorLocked = true; 
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;     
            ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
            std::cout << "=========================================================" << std::endl;
            std::cout << "               PROYECTO CONFIDENCIAL - REINICIO          " << std::endl;
            std::cout << "=========================================================\n" << std::endl;
            printTypewriter("ESCENA 1: PASILLO DE ACCESO");
            std::cout << "El entorno es silencioso y vacio." << std::endl;
            std::cout << "Moverte: W A S D  | Mirar: MOUSE | Sprint: SHIFT" << std::endl;
            std::cout << "Interactuar/Abrir Puertas: E | Linterna: F" << std::endl;
            std::cout << "Busca TARJETAS DE ACCESO para avanzar a las siguientes salas." << std::endl;
        }
        return; 
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyWasPressed) {
            isCursorLocked = !isCursorLocked;
            if (isCursorLocked) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            tabKeyWasPressed = true;
        }
    } else {
        tabKeyWasPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyWasPressed) {
            isFlashlightOn = !isFlashlightOn; 
            fKeyWasPressed = true;
            ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
        }
    } else {
        fKeyWasPressed = false;
    }

    if (!isCursorLocked) return;

    // --- SPRINT Y ESTAMINA ---
    float cameraSpeed = 3.5f; 
    isSprinting = false;
    
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && stamina > 0.0f && !isExhausted) {
        cameraSpeed = 6.0f; // Corre rÃ¡pido
        stamina -= 30.0f * deltaTime;
        isSprinting = true;
        if (stamina <= 0.0f) {
            isExhausted = true;
            std::cout << "\n[AGITADO]: Te has quedado sin aliento.\n" << std::endl;
        }
    } else {
        stamina += 15.0f * deltaTime;
        if (stamina > 100.0f) {
            stamina = 100.0f;
            isExhausted = false;
        }
    }
    
    cameraSpeed *= deltaTime;

    glm::vec3 moveDir(0.0f); 
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += glm::normalize(glm::cross(cameraFront, cameraUp));
    
    moveDir.y = 0.0f;
    isMoving = false;
    
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir) * cameraSpeed; 
        if (!checkCollision(cameraPos.x + moveDir.x, cameraPos.z)) { cameraPos.x += moveDir.x; isMoving = true; }
        if (!checkCollision(cameraPos.x, cameraPos.z + moveDir.z)) { cameraPos.z += moveDir.z; isMoving = true; }
    }
    
    // --- HEADBOBBING ---
    if (isMoving) {
        float bobSpeed = isSprinting ? 15.0f : 10.0f;
        headBobTimer += deltaTime * bobSpeed;
        cameraPos.y = baseCameraY + sin(headBobTimer) * 0.1f;
    } else {
        // Suavizado hacia el centro
        cameraPos.y = glm::mix(cameraPos.y, baseCameraY, deltaTime * 5.0f);
        headBobTimer = 0.0f;
    }

    int prevZone = currentZone;
    updateZone();
    if (prevZone != currentZone) {
        if (currentZone == 2 && !dimensionAlterna) printTypewriter("ESCENA 2: SALA DE CONTROL \nLuz verde tenue. Computadoras encendidas solas.");
        if (currentZone == 3 && !dimensionAlterna) printTypewriter("ESCENA 3: LABORATORIO PRINCIPAL\nEncuentras la esfera central del experimento. Necesitas baterias.");
    }

    // --- INTERACCIÃ“N GENERAL (TECLA E) ---
    bool justPressedE = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyWasPressed) {
            justPressedE = true;
            eKeyWasPressed = true;
            tryOpenDoor(window); // Intenta abrir puertas
            
            // Consola frente a la esfera (Solo se puede interactuar si estas en el laboratorio)
            if (!portalActivado && currentZone == 3) {
                float distA_Consola = glm::length(cameraPos - glm::vec3(25.0f, 0.0f, 7.0f)); 
                if (distA_Consola < 2.0f) {
                    if (bateriasRecolectadas >= 3) {
                        portalActivado = true;
                        dimensionAlterna = true; 
                        std::cout << "\n=========================================================" << std::endl;
                        printTypewriter("ESCENA 4 & 5: ACTIVACION Y DISTORSION DE LA REALIDAD");
                        std::cout << "[SISTEMA REACTIVADO]... INICIANDO SECUENCIA DE COPIA." << std::endl;
                        std::cout << "[ADVERTENCIA]... ANOMALIA DETECTADA EN LA REPLICACION." << std::endl;
                        std::cout << "El entorno pierde estabilidad. Los objetos empiezan a flotar." << std::endl;
                        printTypewriter("NO ES UNA COPIA... ESTA APRENDIENDO. CORRE.");
                        std::cout << "=========================================================\n" << std::endl;
                        ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
                    } else {
                        std::cout << "\n[CONSOLA]: Energia principal fuera de linea. Faltan " << 3 - bateriasRecolectadas << " Baterias.\n" << std::endl;
                    }
                }
            }
        }
    } else {
        eKeyWasPressed = false;
    }

    // LÃ“GICA DE ENTIDADES E INSPECCIÃ“N
    for (auto& entity : gameEntities) {
        if (entity.active) {
            float distancia = glm::length(entity.pos - cameraPos);
            glm::vec3 dirToEntity = glm::normalize(glm::vec3(entity.pos.x, cameraPos.y, entity.pos.z) - cameraPos); 
            // Para objetos altos o bajos, la direcciÃ³n varÃ­a. Usamos la posiciÃ³n real para el Ã¡ngulo
            glm::vec3 realDirToEntity = glm::normalize(entity.pos - cameraPos);
            float lookAngle = glm::dot(cameraFront, realDirToEntity);
            
            // Recolectables (Por cercanÃ­a y mirando hacia ellos)
            if (entity.type == 0 || entity.type == 1 || entity.type == 8 || entity.type == 9) { 
                // Eliminamos la necesidad de apuntar exacto para no frustrar la recoleccion
                if (distancia < 1.5f && justPressedE) {
                    entity.active = false;
                    ma_engine_play_sound(&audioEngine, "assets/collect.wav", NULL);
                    
                    if (entity.type == 0) { 
                        printTypewriter(entity.text);
                    } else if (entity.type == 1) { 
                        bateriasRecolectadas++;
                        std::cout << "\n[BATERIA RECOLECTADA]: Tienes " << bateriasRecolectadas << " / 3\n" << std::endl;
                    } else if (entity.type == 8) {
                        hasKeycardLvl1 = true;
                        std::cout << "\n[OBJETO CLAVE]: Has obtenido la TARJETA AMARILLA (Nivel 1).\n" << std::endl;
                        openDocument(
                            "TARJETA AMARILLA - NIVEL 1",
                            "Autorizacion: Sala de Control.\n\n"
                            "Personal permitido: mantenimiento y soporte.\n"
                            "Observacion manuscrita:\n"
                            "\"Si la puerta se abre sola, no entres.\""
                        );
                    } else if (entity.type == 9) {
                        hasKeycardLvl2 = true;
                        std::cout << "\n[OBJETO CLAVE]: Has obtenido la TARJETA ROJA (Nivel 2).\n" << std::endl;
                        openDocument(
                            "TARJETA ROJA - NIVEL 2",
                            "Autorizacion: Laboratorio principal.\n\n"
                            "Acceso restringido a personal senior.\n"
                            "Nota de emergencia:\n"
                            "\"No activen el nucleo sin las baterias. La copia ya no obedece.\""
                        );
                    }
                }
            } 
            // Objetos Inspectables EstÃ¡ticos (Mesa, Monitor, MÃ¡quina, Cable)
            else if (entity.type == 3 || entity.type == 4 || entity.type == 5 || entity.type == 6 || entity.type == 7) {
                // Precision Raycast Approximation (lookAngle > 0.95 significa mirar casi exactamente al objeto)
                if (distancia < 3.0f && lookAngle > 0.92f && justPressedE) {
                    if (entity.text != "") { // Solo si tiene texto
                        printTypewriter(entity.text);
                    } else if (entity.type == 4) { // Es una mesa sin texto, interactuar abre un cajÃ³n (simulado)
                        printTypewriter("[CAJON]: Esta vacio o atascado.");
                    }
                }
            }
            
            // Procesar animaciones de puertas
        if (door1Opening && door1Anim < 90.0f) {
            door1Anim += 120.0f * deltaTime; // Abre a 120 grados por segundo
            if (door1Anim > 90.0f) door1Anim = 90.0f;
        }
        if (door2Opening && door2Anim < 90.0f) {
            door2Anim += 120.0f * deltaTime;
            if (door2Anim > 90.0f) door2Anim = 90.0f;
        }

        // DRAW CALLS
            else if (entity.type == 2 && portalActivado) {
                float entityLookAngle = glm::dot(cameraFront, -realDirToEntity);
                
                if (entityLookAngle < 0.5f) { // Se mueve si no la miras
                    float speed = 4.5f * deltaTime; 
                    if (!checkCollision(entity.pos.x + realDirToEntity.x * speed, entity.pos.z)) entity.pos.x += realDirToEntity.x * speed;
                    if (!checkCollision(entity.pos.x, entity.pos.z + realDirToEntity.z * speed)) entity.pos.z += realDirToEntity.z * speed;
                    entity.pos.y = 0.0f; 
                }
                
                if (distancia < 0.9f) {
                    gameState = GAMEOVER;
                    std::cout << "\n=========================================================" << std::endl;
                    printTypewriter("ESCENA 9: FALLO TOTAL");
                    std::cout << "La silueta humanoide se retuerce frente a ti." << std::endl;
                    std::cout << "Sus facciones se asientan. Son... las tuyas." << std::endl;
                    std::cout << "La entidad ha imitado perfectamente tu postura." << std::endl;
                    printTypewriter("COPIA COMPLETA. HAS SIDO REEMPLAZADO.");
                    std::cout << "=========================================================\n" << std::endl;
                }
            }
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (gameState != PLAYING || !isCursorLocked) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}



#include "gltf_model.h"
void printNodeHierarchy(const aiNode* node, int depth) {
    if (!node || depth > 12) return;
    for (int i = 0; i < depth; i++) std::cout << "  ";
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);
    std::cout << "- Node: " << node->mName.C_Str() << " | Pos: (" << position.x << ", " << position.y << ", " << position.z 
              << ") | Scale: (" << scaling.x << ", " << scaling.y << ", " << scaling.z << ")" << std::endl;
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        printNodeHierarchy(node->mChildren[i], depth + 1);
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

    GLTFModel* ligthbathroomGLTF = new GLTFModel("assets/ligthbathroom.glb");
    GLTFModel* banoGLTF = new GLTFModel("assets/Bano.glb");
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
    std::vector<glm::mat4> gnomeBoneTransforms;

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
            static glm::vec3 gnomePos = glm::vec3(4.1f, -0.4f, 4.0f);
            static float stunTimer = 0.0f;
            static bool isGnomeActive = true;
            static unsigned int gnomeTexture = 0;

            static bool textureFailed = false;
            // Cargar textura una sola vez si no existe
            if (gnomeTexture == 0 && !textureFailed) {
                gnomeTexture = loadTexture("assets/Gnome_Albedo.png"); 
                if (gnomeTexture == 0) {
                    textureFailed = true;
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
                    stunTimer += deltaTime;
                    if (stunTimer >= 2.0f) {
                        isGnomeActive = false; // El gnomo se asusta y desaparece (o se detiene)
                        std::cout << "[SISTEMA]: Gnomo ahuyentado por la luz." << std::endl;
                    }
                } else {
                    stunTimer = (std::max)(0.0f, stunTimer - deltaTime); // El timer baja si dejas de mirarlo
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
                
                float gnomeTime = (float)glfwGetTime();

                bool hasSkinningBones = gnomeGLTF->CountBonesInMeshes() > 0;

                glm::mat4 gnomeModel = glm::mat4(1.0f);
                gnomeModel = glm::translate(gnomeModel, gnomePos);
                
                // Rotar en el eje Y para mirar al jugador (se aplica DESPUÉS de levantarse en espacio global)
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
                if (beingLookedAt) {
                    currentAnimIndex = stunAnimIndex;
                } else if (isMoving) {
                    currentAnimIndex = moveAnimIndex;
                }

                // Actualizar y enviar matrices de huesos para skinning (solo si el modelo realmente trae huesos)
                if (hasSkinningBones) {
                    gnomeGLTF->UpdateAnimation(gnomeTime, gnomeBoneTransforms, currentAnimIndex);
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
                    gnomeGLTF->DrawAnimated(gnomeTime, currentAnimIndex, shaderProgram, modelLoc, -1, gnomeModel);
                }
                
                // Limpiar estado
                glActiveTexture(GL_TEXTURE0); // FIX: Resetear la unidad de textura activa
                if (isAnimatedLoc >= 0) {
                    glUniform1i(isAnimatedLoc, 0);
                }
                glUniform1i(solidColorLoc, 0);
                glBindVertexArray(VAO);
            }
        }

        // --- DECORACIÓN BAÑO (GLB estáticos) ---
        if (isAnimatedLoc >= 0) glUniform1i(isAnimatedLoc, 0);

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
        if (lavamanosGLTF && !lavamanosGLTF->meshes.empty()) {
            glm::mat4 lavamanosModel = glm::mat4(1.0f);
            lavamanosModel = glm::translate(lavamanosModel, lavamanosPos);
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            lavamanosModel = glm::rotate(lavamanosModel, glm::radians(lavamanosRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            lavamanosModel = glm::scale(lavamanosModel, lavamanosScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lavamanosModel));
            lavamanosGLTF->Draw(shaderProgram, solidColorLoc);
        }

        if (ligthbathroomGLTF && !ligthbathroomGLTF->meshes.empty()) {
            glm::mat4 ligthbathroomModel = glm::mat4(1.0f);
            ligthbathroomModel = glm::translate(ligthbathroomModel, ligthbathroomPos);
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
            ligthbathroomModel = glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
            ligthbathroomModel = glm::scale(ligthbathroomModel, ligthbathroomScale);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ligthbathroomModel));
            if (ligthbathroomDebugVisible) {
                glDisable(GL_CULL_FACE);
                glUniform1i(solidColorLoc, 1);
                glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f);
                ligthbathroomGLTF->Draw(shaderProgram, -1);
            } else {
                ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
            }
            if (ligthbathroomDebugVisible) {
                glEnable(GL_CULL_FACE);
                glUniform1i(solidColorLoc, 0);
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            }
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
        ImGui::Separator();
        ImGui::Text("Urinario");
        ImGui::DragFloat3("Uri Pos", &urinarioPos.x, 0.05f, 33.0f, 40.0f);
        ImGui::DragFloat3("Uri Rot", &urinarioRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Uri Scale", &urinarioScale.x, 0.01f, 0.05f, 2.0f);
        ImGui::Separator();
        ImGui::Text("Lampara bano");
        ImGui::DragFloat3("Lampara bano Pos", &ligthbathroomPos.x, 0.05f);
        ImGui::DragFloat3("Lampara bano Rot", &ligthbathroomRot.x, 0.5f, -180.0f, 180.0f);
        ImGui::DragFloat3("Lampara bano Scale", &ligthbathroomScale.x, 0.01f, 0.05f, 2.0f);
        ImGui::Checkbox("Lampara modo debug visible", &ligthbathroomDebugVisible);
        if (ImGui::Button("Traer lampara frente a camara")) {
            ligthbathroomPos = cameraPos + cameraFront * 0.8f;
            ligthbathroomPos.y = cameraPos.y;
            ligthbathroomRot = glm::vec3(0.0f, 0.0f, 0.0f);
            ligthbathroomScale = glm::vec3(1.8f, 1.8f, 1.8f);
        }

        ImGui::End();


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
