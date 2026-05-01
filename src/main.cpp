#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Librerías de matemáticas 3D (GLM)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>

// Implementación de stb_image para cargar texturas
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Implementación de miniaudio para sonido
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

// Configuración de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// ==========================================
// SHADERS
// ==========================================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    uniform int dimensionAlterna;
    uniform float time;

    void main() {
        vec3 finalPos = aPos;
        // Glitches visuales severos (Escena 8) si la dimensión cambió
        if (dimensionAlterna == 1) {
            finalPos.x += sin(time * 50.0 + aPos.y) * 0.05;
            finalPos.y += cos(time * 30.0 + aPos.z) * 0.02;
        }

        FragPos = vec3(model * vec4(finalPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        TexCoord = aTexCoord;
        
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;

    uniform sampler2D texture1;
    uniform vec3 objectColor;

    // --- VARIABLES DE LA LINTERNA ---
    uniform vec3 lightPos;      
    uniform vec3 lightDir;      
    uniform float cutOff;       
    uniform float outerCutOff;  
    uniform int flashlightOn;   

    uniform int dimensionAlterna;
    uniform int currentZone; // 1: Pasillo, 2: Control, 3: Lab
    uniform float time;
    uniform vec2 resolution;

    void main() {
        float ambientStrength = 0.05;
        vec3 ambientColor = vec3(1.0);
        vec3 flashColor = vec3(1.0);

        // Colorimetría base según la habitación (Escenas 1, 2, 3)
        if (currentZone == 1) {
            ambientColor = vec3(0.6, 0.7, 0.8); // Blanco frío
            flashColor = vec3(0.9, 0.9, 1.0);
            ambientStrength = 0.1 + (sin(time * 10.0) * 0.02); // Leve parpadeo amarillo tenue
        } else if (currentZone == 2) {
            ambientColor = vec3(0.4, 0.9, 0.5); // Verde fosforescente
            flashColor = vec3(0.8, 1.0, 0.8);
            ambientStrength = 0.15;
        } else if (currentZone == 3) {
            ambientColor = vec3(0.3, 0.5, 1.0); // Azul eléctrico
            flashColor = vec3(1.0, 1.0, 1.0); // Blanco intenso
            ambientStrength = 0.2;
        }

        // Si entramos a la dimensión distorsionada (Escena 5 y 8)
        if (dimensionAlterna == 1) {
            ambientColor = vec3(0.6, 0.0, 0.2); // Rojo oscuro / Morado
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
        if (texColor.a < 0.1) {
            discard; 
        }
        
        vec3 result = (ambient + diffuse) * objectColor;
        
        if (dimensionAlterna == 1) {
            // Viñeta de sombras profundas para la fase final
            vec2 uv = gl_FragCoord.xy / resolution;
            float distToCenter = distance(uv, vec2(0.5));
            result *= smoothstep(0.9, 0.2, distToCenter);
        }
        
        FragColor = texColor * vec4(result, 1.0);
    }
)";

// ==========================================
// VARIABLES DE LA CÁMARA
// ==========================================
glm::vec3 cameraPos   = glm::vec3(12.0f, 0.0f, 22.0f); // Inicio en el pasillo Sur
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // Mirando hacia el Norte
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

bool firstMouse = true;
float yaw   = -90.0f;	// Mirar al norte (Z negativo)
float pitch =  0.0f;	
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;

float deltaTime = 0.0f;	
float lastFrame = 0.0f;
    
enum GameState { MENU, PLAYING, GAMEOVER };
GameState gameState = MENU; 

bool isCursorLocked = false; 
bool tabKeyWasPressed = false;
bool eKeyWasPressed = false;
bool isFlashlightOn = true;
bool fKeyWasPressed = false;

ma_engine audioEngine;

// ==========================================
// ENTIDADES
// ==========================================
struct Entity {
    glm::vec3 pos;  
    int type;       // 0 = Log, 1 = Batería, 2 = Entidad, 3 = Cable
    bool active;    
    std::string text;
};

std::vector<Entity> gameEntities = {
    // Zona 1: Pasillo (Escena 1)
    {glm::vec3(12.0f, -0.2f, 18.0f), 3, true, "[CABLE SUELTO]: Un cable de alta tension cortado de tajo."},
    {glm::vec3(12.0f, -0.2f, 16.0f), 1, true, ""}, // Batería 1
    
    // Zona 2: Control (Escena 2)
    {glm::vec3(9.0f, -0.2f, 11.0f), 0, true, "REGISTRO 1: 'La senal responde... pero no es un eco. Esta replicando estructuras... con errores.'"},
    {glm::vec3(15.0f, -0.2f, 10.0f), 1, true, ""}, // Batería 2
    
    // Zona 3: Laboratorio (Escena 3)
    {glm::vec3(8.0f, -0.2f, 5.0f), 0, true, "REGISTRO 2: 'La copia ya no sigue instrucciones. Intenta replicar comportamiento humano.'"},
    {glm::vec3(16.0f, -0.2f, 3.0f), 1, true, ""}, // Batería 3
    
    // La Entidad (Escena 6) - Spawnea cerca del portal, inactiva al inicio
    {glm::vec3(12.0f, 0.0f, 2.0f), 2, true, ""}
};

int bateriasRecolectadas = 0;
bool dimensionAlterna = false;
bool portalActivado = false;
int currentZone = 1;

// ==========================================
// MAPA LINEAL (24x24) - Laboratorio Estructurado
// 0=Vacío, 1=Pasillo, 2=Control, 3=Lab, 4=Puerta Falsa, 5=Portal
// ==========================================
const int MAP_WIDTH = 24;
const int MAP_HEIGHT = 24;

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    // NORTE: ESCENA 3 - LABORATORIO PRINCIPAL (Z=0 a 7)
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,5,5,0,0,0,0,0,0,0,0,0,0,3}, // Portal Central
    {3,0,0,0,0,0,0,0,0,0,0,5,5,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
    {3,3,3,3,3,3,3,3,3,3,3,0,0,3,3,3,3,3,3,3,3,3,3,3}, // Puertas (4) convertidas a (0)
    
    // MEDIO: ESCENA 2 - SALA DE CONTROL (Z=8 a 14)
    {2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,2}, // Mesas Computadoras
    {2,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
    {2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2}, // Puertas (4) convertidas a (0)

    // SUR: ESCENA 1 - PASILLO DE ACCESO (Z=15 a 23)
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1},
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
    
    // Si toca una pared (1,2,3), una puerta (4) o el portal (5), no avanza
    if (worldMap[minZ][minX] > 0) return true;
    if (worldMap[minZ][maxX] > 0) return true;
    if (worldMap[maxZ][minX] > 0) return true;
    if (worldMap[maxZ][maxX] > 0) return true;
    
    return false; 
}

void updateZone() {
    if (cameraPos.z >= 15.0f) currentZone = 1;
    else if (cameraPos.z >= 8.0f) currentZone = 2;
    else currentZone = 3;
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
            std::cout << "\n--- ESCENA 1: PASILLO DE ACCESO ---" << std::endl;
            std::cout << "El entorno es silencioso y vacio. Avanza." << std::endl;
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

    float cameraSpeed = 3.5f * deltaTime; 
    glm::vec3 moveDir(0.0f); 
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += glm::normalize(glm::cross(cameraFront, cameraUp));
    
    moveDir.y = 0.0f;
    
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir) * cameraSpeed; 
        if (!checkCollision(cameraPos.x + moveDir.x, cameraPos.z)) cameraPos.x += moveDir.x;
        if (!checkCollision(cameraPos.x, cameraPos.z + moveDir.z)) cameraPos.z += moveDir.z;
    }
    
    // Detectar en qué cuarto estamos
    int prevZone = currentZone;
    updateZone();
    if (prevZone != currentZone) {
        if (currentZone == 2 && !dimensionAlterna) std::cout << "\n--- ESCENA 2: SALA DE CONTROL ---\nLuz verde tenue. Computadoras desordenadas." << std::endl;
        if (currentZone == 3 && !dimensionAlterna) std::cout << "\n--- ESCENA 3: LABORATORIO PRINCIPAL ---\nEncuentras la esfera central del experimento." << std::endl;
    }

    // INTERACCIÓN CON CONSOLA (Escena 4)
    if (!portalActivado && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyWasPressed) {
            // Consola frente a la esfera
            float distA_Consola = glm::length(cameraPos - glm::vec3(12.0f, 0.0f, 6.0f));
            if (distA_Consola < 2.5f) {
                if (bateriasRecolectadas >= 3) {
                    portalActivado = true;
                    dimensionAlterna = true; // Escena 5 y 8
                    std::cout << "\n--- ESCENA 4 & 5: ACTIVACION Y DISTORSION ---" << std::endl;
                    std::cout << "[SISTEMA REACTIVADO]... ADVERTENCIA. ANOMALIA DETECTADA." << std::endl;
                    std::cout << "El entorno pierde estabilidad. Estructuras anómalas. No es una copia... esta aprendiendo." << std::endl;
                    ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
                } else {
                    std::cout << "\n[CONSOLA]: Energia insuficiente. Se requieren " << 3 - bateriasRecolectadas << " baterias.\n" << std::endl;
                }
            }
            eKeyWasPressed = true;
        }
    } else {
        eKeyWasPressed = false;
    }

    // LÓGICA DE ENTIDADES
    for (auto& entity : gameEntities) {
        if (entity.active) {
            float distancia = glm::length(entity.pos - cameraPos);
            
            // Coleccionables (0=Log, 1=Bateria, 3=Objeto Insignificante)
            if (entity.type != 2) { 
                if (distancia < 1.0f) {
                    entity.active = false;
                    ma_engine_play_sound(&audioEngine, "assets/collect.wav", NULL);
                    
                    if (entity.type == 0 || entity.type == 3) { 
                        std::cout << "\n" << entity.text << "\n" << std::endl;
                    } else if (entity.type == 1) { 
                        bateriasRecolectadas++;
                        std::cout << "\n[BATERIA RECOLECTADA]: Tienes " << bateriasRecolectadas << " / 3\n" << std::endl;
                    }
                }
            } 
            // La Entidad (Fase 6 Weeping Angel)
            else if (entity.type == 2 && portalActivado) {
                glm::vec3 dirToPlayer = glm::normalize(cameraPos - entity.pos);
                float lookAngle = glm::dot(cameraFront, -dirToPlayer);
                
                // Si NO la estamos mirando
                if (lookAngle < 0.5f) {
                    float speed = 3.5f * deltaTime; // Se mueve más rápido mientras más avanza el juego
                    
                    // Solo se mueve si no choca con las paredes
                    if (!checkCollision(entity.pos.x + dirToPlayer.x * speed, entity.pos.z)) entity.pos.x += dirToPlayer.x * speed;
                    if (!checkCollision(entity.pos.x, entity.pos.z + dirToPlayer.z * speed)) entity.pos.z += dirToPlayer.z * speed;
                    entity.pos.y = 0.0f; 
                }
                
                // Escena 9: FINAL
                if (distancia < 0.9f) {
                    gameState = GAMEOVER;
                    std::cout << "\n===================================" << std::endl;
                    std::cout << "--- ESCENA 9: FINAL ---" << std::endl;
                    std::cout << "La entidad ha imitado perfectamente tu postura." << std::endl;
                    std::cout << "COPIA COMPLETA. HAS SIDO REEMPLAZADO." << std::endl;
                    std::cout << "===================================\n" << std::endl;
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
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 4);
    if (data) {
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
    } else {
        std::cout << "Textura falló: " << path << std::endl;
    }
    return textureID;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto Confidencial", NULL, NULL);
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
        // Posición           // Normal            // TexUV
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

    unsigned int wallTex1 = loadTexture("assets/paredesH.png"); 
    unsigned int wallTex2 = loadTexture("assets/paredes.png");  
    unsigned int wallTex3 = loadTexture("assets/wall.png");     
    // Texturas nuevas sugeridas por el mundo (puerta y portal)
    // Usaremos texturas existentes y las tintaremos en el shader si no existen
    unsigned int doorTex = loadTexture("assets/paredes.png"); 
    unsigned int portalTex = loadTexture("assets/clue.png");

    unsigned int floorTexture = loadTexture("assets/pisoH.jpg");
    unsigned int logoTexture = loadTexture("assets/logo.png"); 
    unsigned int clueTexture = loadTexture("assets/clue.png"); 
    unsigned int enemyTexture = loadTexture("assets/enemy.png");

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

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Fondo negro total como un abismo (más inmersivo)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

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
            // Caer al piso (Replica perfecta te mató)
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
        glUniform1f(cutOffLoc, glm::cos(glm::radians(15.5f))); // Linterna más amplia
        glUniform1f(outerCutOffLoc, glm::cos(glm::radians(22.5f)));
        glUniform1i(flashlightOnLoc, isFlashlightOn ? 1 : 0);
        
        glUniform1i(dimAlternaLoc, dimensionAlterna ? 1 : 0);
        glUniform1i(zoneLoc, currentZone);
        glUniform1f(timeLoc, currentFrame);
        glUniform2f(resLoc, (float)currentWidth, (float)currentHeight);

        // Dibujar el mapa
        for (int z = 0; z < MAP_HEIGHT; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                int blockType = worldMap[z][x];
                
                if (blockType > 0) { 
                    if (blockType == 1) glBindTexture(GL_TEXTURE_2D, wallTex1);
                    else if (blockType == 2) glBindTexture(GL_TEXTURE_2D, wallTex2);
                    else if (blockType == 3) glBindTexture(GL_TEXTURE_2D, wallTex3);
                    else if (blockType == 4) glBindTexture(GL_TEXTURE_2D, doorTex); // Puerta
                    else if (blockType == 5) glBindTexture(GL_TEXTURE_2D, portalTex); // Portal

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3((float)x, 0.0f, (float)z));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    
                    if (blockType == 4) glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); // Oscurecer puerta
                    else if (blockType == 5) glUniform3f(colorLoc, 0.2f, 0.6f, 1.0f); // Tintar azul
                    else glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                
                // Solo dibujar piso donde hay espacios vacios o debajo de paredes para evitar huecos
                glBindTexture(GL_TEXTURE_2D, floorTexture);
                glm::mat4 floorModel = glm::mat4(1.0f);
                floorModel = glm::translate(floorModel, glm::vec3((float)x, -1.0f, (float)z));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
                
                if (dimensionAlterna) glUniform3f(colorLoc, 0.4f, 0.1f, 0.1f);
                else glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
                
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // Techo para mayor claustrofobia (Y=1.0)
                if (blockType == 0) {
                    glm::mat4 roofModel = glm::mat4(1.0f);
                    roofModel = glm::translate(roofModel, glm::vec3((float)x, 1.0f, (float)z));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(roofModel));
                    glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); // Techo oscuro
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
        } 

        glBindVertexArray(quadVAO);
        glDisable(GL_CULL_FACE);

        for (auto& entity : gameEntities) {
            if (!entity.active) continue; 
            if (entity.type == 2 && !portalActivado) continue; // No mostrar a la entidad hasta que se active

            if (entity.type == 0 || entity.type == 1 || entity.type == 3) glBindTexture(GL_TEXTURE_2D, clueTexture);
            else glBindTexture(GL_TEXTURE_2D, enemyTexture);

            glm::mat4 entityModel = glm::mat4(1.0f);
            entityModel = glm::translate(entityModel, entity.pos);
            
            float anguloHaciaCamara = atan2(cameraPos.x - entity.pos.x, cameraPos.z - entity.pos.z);
            entityModel = glm::rotate(entityModel, anguloHaciaCamara, glm::vec3(0.0f, 1.0f, 0.0f));

            float escala = (entity.type != 2) ? 0.3f : 0.9f; 
            entityModel = glm::scale(entityModel, glm::vec3(escala, escala, escala));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
            
            if (entity.type == 2 && dimensionAlterna) {
                // Entidad pulsante oscura
                float pulse = 0.5f + 0.5f * sin(currentFrame * 10.0f);
                glUniform3f(colorLoc, pulse, 0.1f, 0.1f);
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
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ma_sound_uninit(&bgm);
    ma_engine_uninit(&audioEngine);
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}