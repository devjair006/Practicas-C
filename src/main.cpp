#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "obj_loader.h"

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Variables de HUD (ImGui)----
std::string currentHUDMessage = "";
float hudMessageTimer = 0.0f;

// ==========================================
// SHADERS
// ==========================================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    layout (location = 3) in vec3 aObjColor;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;
    out vec3 ObjColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    uniform int dimensionAlterna;
    uniform float time;

    void main() {
        vec3 finalPos = aPos;
        if (dimensionAlterna == 1) {
            finalPos.x += sin(time * 50.0 + aPos.y) * 0.05;
            finalPos.y += cos(time * 30.0 + aPos.z) * 0.02;
        }

        FragPos = vec3(model * vec4(finalPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
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

glm::vec3 cameraPos   = glm::vec3(12.0f, 0.0f, 22.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float yaw   = -90.0f;	
float pitch =  0.0f;	
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;

float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// Headbobbing (Movimiento de cámara al caminar)
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
    int type; // 0=Log, 1=Batería, 2=Entidad, 3=ObjetoAmbiental, 4=Mesa, 5=Monitor, 6=Máquina, 7=Portal, 8=TarjetaNv1, 9=TarjetaNv2
    bool active;    
    std::string text;
    float seed;
};

std::vector<Entity> gameEntities = {
    // --- ESCENA 1: PASILLO ---
    {glm::vec3(12.0f, -0.4f, 21.0f), 3, true, "[CABLE SUELTO]:  Hay un cable pelado aqui. Quien lo corto lo hizo con prisa.", 0.0f},
    {glm::vec3(12.0f, -0.4f, 18.0f), 0, true, "LOG 1 (Arrugado): 'El proyecto se suponia que predeciria catastrofes. Solo veo una frente a mi.'", 0.0f},
    {glm::vec3(11.0f, -0.4f, 16.0f), 8, true, "", 0.0f}, // TARJETA NV 1 (Amarilla) (Mover de X=10 a X=11)
    {glm::vec3(12.0f, -0.2f, 16.0f), 1, true, "", 0.0f}, // Batería 1 (Mover de X=14 a X=12)
    
    // --- ESCENA 2: CONTROL ---
    {glm::vec3(10.0f, -0.5f, 12.0f), 4, true, "", 1.0f},
    {glm::vec3(10.0f, 0.0f, 12.0f), 5, true, "[PANTALLA VERDE]: 'La senal responde... pero no es un eco.'", 1.5f},
    
    {glm::vec3(14.0f, -0.5f, 12.0f), 4, true, "", 2.0f},
    {glm::vec3(14.0f, 0.0f, 12.0f), 5, true, "[PANTALLA ERROR]: 'Esta replicando estructuras... con errores en la masa. Falta algo.'", 2.5f},
    {glm::vec3(14.0f, -0.4f, 11.5f), 9, true, "", 0.0f}, // TARJETA NV 2 (Roja)
    
    {glm::vec3(10.0f, -0.5f, 10.0f), 4, true, "", 3.0f},
    {glm::vec3(10.0f, 0.0f, 10.0f), 5, true, "[PANTALLA APAGADA]: Solo hay estatica...", 3.5f},
    
    {glm::vec3(14.0f, -0.5f, 10.0f), 4, true, "", 4.0f},
    {glm::vec3(14.0f, 0.0f, 10.0f), 5, true, "[REGISTRO MAESTRO]: 'EVACUACION INMEDIATA. NO MIREN ATRAS.'", 4.5f},
    
    {glm::vec3(12.0f, -0.2f, 11.0f), 0, true, "LOG 2 (Sangriento): 'Se suponia que copiara la habitacion, pero... empezo a copiar mis movimientos.'", 0.0f},
    {glm::vec3(16.0f, -0.2f, 9.0f), 1, true, "", 0.0f}, // Batería 2
    {glm::vec3(8.0f, -0.4f, 9.0f), 3, true, "[MANCHA]: Un charco oscuro de procedencia dudosa.", 0.0f},
    
    // --- ESCENA 3: LABORATORIO ---
    {glm::vec3(8.0f, 0.0f, 6.0f), 6, true, "[MAQUINA]: Nivel de radiacion al 900%. Inestable.", 5.0f},
    {glm::vec3(16.0f, 0.0f, 6.0f), 6, true, "[MAQUINA]: Cables arrancados. Alguien intento apagarlo a la fuerza.", 6.0f},
    {glm::vec3(8.0f, 0.0f, 2.0f), 6, true, "[MAQUINA]: 'ERROR CRITICO DE SIMETRIA DIMENSIONAL'.", 7.0f},
    {glm::vec3(16.0f, 0.0f, 2.0f), 6, true, "[MAQUINA]: ...", 8.0f},
    
    {glm::vec3(12.0f, 1.0f, 4.0f), 7, true, "[EL PORTAL]: Una esfera masiva flotando, emitiendo energia cruda.", 0.0f},
    
    {glm::vec3(9.0f, -0.2f, 5.0f), 0, true, "LOG 3 (Rasgado): 'La copia ya no sigue instrucciones. Intenta replicar comportamiento humano.'", 0.0f},
    {glm::vec3(16.0f, -0.2f, 3.0f), 1, true, "", 0.0f}, // Batería 3
    
    // La Entidad
    {glm::vec3(12.0f, 0.0f, 2.0f), 2, true, "", 0.0f}
};

// ==========================================
// MAPA LINEAL (24x24) - Laboratorio Estructurado
// 0=Vacío, 1=Pasillo, 2=Control, 3=Lab, 4=Bloque sólido invisible, 8=Puerta Nivel 1, 9=Puerta Nivel 2
// ==========================================
const int MAP_WIDTH = 24;
const int MAP_HEIGHT = 24;

// --- Animacion de Puertas ---
float door1Anim = 0.0f; // 0.0 a 90.0 grados
bool door1Opening = false;
float door2Anim = 0.0f;
bool door2Opening = false;

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    // NORTE: ESCENA 3 - LABORATORIO PRINCIPAL (Z=0 a 7)
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,3,3,3,3,3,3,3,3,3,3,9,9,3,3,3,3,3,3,3,3,3,3,3}, // Puerta Nivel 2 (Roja)
    
    // MEDIO: ESCENA 2 - SALA DE CONTROL (Z=8 a 14)
    {2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,2,2,2,2,2,2,2,2,2,2,8,8,2,2,2,2,2,2,2,2,2,2,2}, // Puerta Nivel 1 (Amarilla)

    // SUR: ESCENA 1 - PASILLO DE ACCESO (Z=15 a 23)
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// ==========================================
// FUNCIONES DE CONTROL
// ==========================================
bool checkCollision(float x, float z) {
    float playerRadius = 0.25f; 
    int minX = (int)round(x - playerRadius);
    int maxX = (int)round(x + playerRadius);
    int minZ = (int)round(z - playerRadius);
    int maxZ = (int)round(z + playerRadius);
    
    if (minX < 0 || maxX >= MAP_WIDTH || minZ < 0 || maxZ >= MAP_HEIGHT) return true;
    
    if (worldMap[minZ][minX] > 0) return true;
    if (worldMap[minZ][maxX] > 0) return true;
    if (worldMap[maxZ][minX] > 0) return true;
    if (worldMap[maxZ][maxX] > 0) return true;
    
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
    if (cameraPos.z >= 15.0f) currentZone = 1;
    else if (cameraPos.z >= 8.0f) currentZone = 2;
    else currentZone = 3;
}

void printTypewriter(std::string text) {
    currentHUDMessage = text;
    hudMessageTimer = 5.0f; // Mostrar por 5 segundos
    std::cout << "\n> " << text << "\n" << std::endl; // Mantenemos el log por si acaso
}

void tryOpenDoor(GLFWwindow *window) {
    // Escaneo de los bloques frente a la cámara (rango 1.5)
    glm::vec3 checkPos = cameraPos + cameraFront * 1.5f;
    int gridX = (int)round(checkPos.x);
    int gridZ = (int)round(checkPos.z);
    
    if (gridX >= 0 && gridX < MAP_WIDTH && gridZ >= 0 && gridZ < MAP_HEIGHT) {
        int targetBlock = worldMap[gridZ][gridX];
        
        if (targetBlock == 8) { // Puerta Amarilla
            if (hasKeycardLvl1) {
                // La puerta amarilla abarca x=11 y x=12. Los cambiamos ambos a -8.
                if (gridX == 11 || gridX == 12) {
                    worldMap[gridZ][11] = -8; 
                    worldMap[gridZ][12] = -8; 
                }
                door1Opening = true; // Iniciar animacion
                printTypewriter("[PUERTA]: Tarjeta Nivel 1 Aceptada. Accediendo a Sala de Control.");
                ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
            } else {
                printTypewriter("[PUERTA BLOQUEADA]: Se requiere Tarjeta Amarilla (Nivel 1).");
            }
        } else if (targetBlock == 9) { // Puerta Roja
            if (hasKeycardLvl2) {
                // La puerta roja también abarca x=11 y x=12. Los cambiamos ambos a -9.
                if (gridX == 11 || gridX == 12) {
                    worldMap[gridZ][11] = -9;
                    worldMap[gridZ][12] = -9;
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
        glfwSetWindowShouldClose(window, true);

    if (gameState == GAMEOVER) return;

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
        cameraSpeed = 6.0f; // Corre rápido
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
        if (currentZone == 2 && !dimensionAlterna) printTypewriter("ESCENA 2: SALA DE CONTROL\nLuz verde tenue. Computadoras encendidas solas.");
        if (currentZone == 3 && !dimensionAlterna) printTypewriter("ESCENA 3: LABORATORIO PRINCIPAL\nEncuentras la esfera central del experimento. Necesitas baterias.");
    }

    // --- INTERACCIÓN GENERAL (TECLA E) ---
    bool justPressedE = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyWasPressed) {
            justPressedE = true;
            eKeyWasPressed = true;
            tryOpenDoor(window); // Intenta abrir puertas
            
            // Consola frente a la esfera (Solo se puede interactuar si estas en el laboratorio)
            if (!portalActivado && currentZone == 3) {
                float distA_Consola = glm::length(cameraPos - glm::vec3(12.0f, 0.0f, 6.0f)); 
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

    // LÓGICA DE ENTIDADES E INSPECCIÓN
    for (auto& entity : gameEntities) {
        if (entity.active) {
            float distancia = glm::length(entity.pos - cameraPos);
            glm::vec3 dirToEntity = glm::normalize(glm::vec3(entity.pos.x, cameraPos.y, entity.pos.z) - cameraPos); 
            // Para objetos altos o bajos, la dirección varía. Usamos la posición real para el ángulo
            glm::vec3 realDirToEntity = glm::normalize(entity.pos - cameraPos);
            float lookAngle = glm::dot(cameraFront, realDirToEntity);
            
            // Recolectables (Por cercanía y mirando hacia ellos)
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
                    } else if (entity.type == 9) {
                        hasKeycardLvl2 = true;
                        std::cout << "\n[OBJETO CLAVE]: Has obtenido la TARJETA ROJA (Nivel 2).\n" << std::endl;
                    }
                }
            } 
            // Objetos Inspectables Estáticos (Mesa, Monitor, Máquina, Cable)
            else if (entity.type == 3 || entity.type == 4 || entity.type == 5 || entity.type == 6 || entity.type == 7) {
                // Precision Raycast Approximation (lookAngle > 0.95 significa mirar casi exactamente al objeto)
                if (distancia < 3.0f && lookAngle > 0.92f && justPressedE) {
                    if (entity.text != "") { // Solo si tiene texto
                        printTypewriter(entity.text);
                    } else if (entity.type == 4) { // Es una mesa sin texto, interactuar abre un cajón (simulado)
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

unsigned int loadTexture(char const * path) {
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 4);
    if (data) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        GLenum format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);
        return textureID;
    } else {
        std::cout << "Textura falló: " << path << std::endl;
        return 0;
    }
}

unsigned int loadTextureWithFallback(char const * path, unsigned int fallback) {
    unsigned int tex = loadTexture(path);
    return tex == 0 ? fallback : tex;
}

int main() {
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
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

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

    std::vector<float> objVertices;
    unsigned int objVAO = 0, objVBO = 0;
    int objVertexCount = 0;
    if (loadOBJ("assets/laptop.obj", objVertices)) {
        objVertexCount = objVertices.size() / 11;
        glGenVertexArrays(1, &objVAO);
        glGenBuffers(1, &objVBO);
        glBindVertexArray(objVAO);
        glBindBuffer(GL_ARRAY_BUFFER, objVBO);
        glBufferData(GL_ARRAY_BUFFER, objVertices.size() * sizeof(float), &objVertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

    std::vector<float> cablesVertices;
    unsigned int cablesVAO = 0, cablesVBO = 0;
    int cablesVertexCount = 0;
    if (loadOBJ("assets/cables.obj", cablesVertices)) {
        cablesVertexCount = cablesVertices.size() / 11;
        glGenVertexArrays(1, &cablesVAO);
        glGenBuffers(1, &cablesVBO);
        glBindVertexArray(cablesVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cablesVBO);
        glBufferData(GL_ARRAY_BUFFER, cablesVertices.size() * sizeof(float), &cablesVertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

    std::vector<float> cartaVertices;
    unsigned int cartaVAO = 0, cartaVBO = 0;
    int cartaVertexCount = 0;
    if (loadOBJ("assets/carta.obj", cartaVertices)) {
        cartaVertexCount = cartaVertices.size() / 11;
        glGenVertexArrays(1, &cartaVAO);
        glGenBuffers(1, &cartaVBO);
        glBindVertexArray(cartaVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cartaVBO);
        glBufferData(GL_ARRAY_BUFFER, cartaVertices.size() * sizeof(float), &cartaVertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

    std::vector<float> pIzquiVertices, pDereVertices;
    unsigned int pIzquiVAO = 0, pIzquiVBO = 0, pDereVAO = 0, pDereVBO = 0;
    int pIzquiCount = 0, pDereCount = 0;

    if (loadOBJ("assets/puertaizqui.obj", pIzquiVertices)) {
        pIzquiCount = pIzquiVertices.size() / 11;
        glGenVertexArrays(1, &pIzquiVAO);
        glGenBuffers(1, &pIzquiVBO);
        glBindVertexArray(pIzquiVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pIzquiVBO);
        glBufferData(GL_ARRAY_BUFFER, pIzquiVertices.size() * sizeof(float), &pIzquiVertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

    if (loadOBJ("assets/puertadere.obj", pDereVertices)) {
        pDereCount = pDereVertices.size() / 11;
        glGenVertexArrays(1, &pDereVAO);
        glGenBuffers(1, &pDereVBO);
        glBindVertexArray(pDereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pDereVBO);
        glBufferData(GL_ARRAY_BUFFER, pDereVertices.size() * sizeof(float), &pDereVertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }

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

    // Texturas específicas con fallback
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
                    
                    if (is3DDoor && x == 12) {
                        // Skip
                    } else if (is3DDoor && x == 11) {
                        glm::mat4 baseModel = glm::mat4(1.0f);
                        
                        // 4. Mover al centro del hueco (11.5) y anclar al piso de la pared (-0.5)
                        baseModel = glm::translate(baseModel, glm::vec3(11.5f, -0.5f, (float)z));
                        
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
                                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Amarillo Sólido
                            } else if (renderBlock == 9 || renderBlock == -9) {
                                glUniform3f(colorLoc, 0.9f, 0.1f, 0.1f); // Rojo Sólido
                            } else {
                                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                            }
                        }

                        // Obtener ángulo actual de la animación
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

                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, glm::vec3((float)x, 0.0f, (float)z));
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

                if (blockType == 0) {
                    glm::mat4 roofModel = glm::mat4(1.0f);
                    roofModel = glm::translate(roofModel, glm::vec3((float)x, 1.0f, (float)z));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(roofModel));
                    glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); 
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        } 

        // --- DIBUJAR ENTIDADES 3D ---
        glBindVertexArray(VAO);
        glEnable(GL_DEPTH_TEST);

        for (auto& entity : gameEntities) {
            if (!entity.active || (entity.type != 0 && entity.type != 3 && entity.type < 4) || entity.type == 8 || entity.type == 9) continue; 
            if (entity.type > 0 && entity.type < 3) continue; // Solo procesar tipos 0, 3, 4, 5, 6, 7 aquí

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
                
                // Ajuste de posición para que toque el suelo
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
                entityModel = glm::rotate(entityModel, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Estático
                entityModel = glm::scale(entityModel, glm::vec3(0.6f, 0.6f, 0.6f));
                if (dimensionAlterna) glUniform3f(colorLoc, 1.0f, 0.4f, 0.4f);
                else glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Usar colores 100% reales de Blender
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
                glDrawArrays(GL_TRIANGLES, 0, objVertexCount);
                
                glUniform1i(solidColorLoc, 0); // Restaurar texturas normales
                
                glBindVertexArray(VAO); // Restaurar VAO
            } else if (entity.type == 6) { // Máquina Lab
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
            
            // Flotación sutil de objetos clave para visibilidad---
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

            glBindTexture(GL_TEXTURE_2D, clueTexture); // Textura blanca genérica
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        // --- RENDER IMGUI HUD ---
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