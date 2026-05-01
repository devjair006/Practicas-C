#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Librerías de matemáticas 3D (GLM)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

// Implementación de stb_image para cargar texturas
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Configuración de la ventana
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ==========================================
// SHADERS (El programa de la tarjeta gráfica)
// ==========================================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        // Multiplicación de matrices para convertir 3D a pantalla 2D
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;

    uniform sampler2D texture1;
    uniform vec3 objectColor;

    void main() {
        // Mezclamos la textura con un color base
        FragColor = texture(texture1, TexCoord) * vec4(objectColor, 1.0);
    }
)";

// ==========================================
// VARIABLES DE LA CÁMARA (ESTILO FPS)
// ==========================================
glm::vec3 cameraPos   = glm::vec3(4.0f, 0.0f, 4.0f); // Posición inicial
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Hacia dónde mira
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); // Cuál es "arriba"

bool firstMouse = true;
float yaw   = -90.0f;	// Mirar izq/der
float pitch =  0.0f;	// Mirar arriba/abajo
float lastX = SCR_WIDTH / 2.0;
float lastY = SCR_HEIGHT / 2.0;

// Control de tiempo (para movimiento suave independientemente de los FPS)
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// ==========================================
// ESTADOS DEL JUEGO
// ==========================================
enum GameState {
    MENU,    // Pantalla de inicio
    PLAYING  // Jugando
};
GameState gameState = MENU; // Iniciamos en el menú

// Control de estado del ratón
bool isCursorLocked = false; // Iniciamos con el cursor libre en el menú
bool tabKeyWasPressed = false;

// ==========================================
// MAPA DEL NIVEL (1 = Pared, 0 = Pasillo)---
// ==========================================
int worldMap[8][8] = {
    {1,1,1,1,1,1,1,1}, 
    {1,0,0,0,0,0,0,1}, 
    {1,0,1,0,0,1,0,1}, 
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1}, 
    {1,0,1,1,0,1,0,1}, 
    {1,0,0,0,0,0,0,1}, 
    {1,1,1,1,1,1,1,1}
};

// ==========================================
// FUNCIONES DE CONTROL
// ==========================================
bool checkCollision(float x, float z) {
    // Radio del hitbox del jugador (qué tan cerca de la pared puede llegar)
    float playerRadius = 0.25f; 
    
    // Calculamos qué bloques ocupa la hitbox del jugador (x, z convertidos a índices del mapa)
    int minX = (int)round(x - playerRadius);
    int maxX = (int)round(x + playerRadius);
    int minZ = (int)round(z - playerRadius);
    int maxZ = (int)round(z + playerRadius);
    
    // Evitar que el jugador salga del mapa (fuera del arreglo)
    if (minX < 0 || maxX > 7 || minZ < 0 || maxZ > 7) return true;
    
    // Si alguna parte de la hitbox toca una pared (valor 1), bloqueamos el paso
    if (worldMap[minZ][minX] == 1) return true;
    if (worldMap[minZ][maxX] == 1) return true;
    if (worldMap[maxZ][minX] == 1) return true;
    if (worldMap[maxZ][maxX] == 1) return true;
    
    return false; // Camino libre
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- LÓGICA DE MENÚ ---
    if (gameState == MENU) {
        // Presionar ESPACIO o ENTER para iniciar el juego
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS|| glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            gameState = PLAYING;
            isCursorLocked = true; // Bloqueamos ratón al entrar al juego
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;     // Prevenir saltos de cámara
        }
        return; // Bloqueamos controles de movimiento mientras estamos en el menú
    }

    // --- LÓGICA DE JUEGO ---
    // Activar / Desactivar el ratón con la tecla TAB (Tabulador)
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyWasPressed) {
            isCursorLocked = !isCursorLocked;
            if (isCursorLocked) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true; // Evitar que la cámara salte de golpe al regresar
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            tabKeyWasPressed = true;
        }
    } else {
        tabKeyWasPressed = false;
    }

    // Si el ratón está desbloqueado, no movemos al jugador
    if (!isCursorLocked) return;

    float cameraSpeed = 2.5f * deltaTime; // Velocidad al caminar
    glm::vec3 moveDir(0.0f); // Dirección a la que nos queremos mover
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += glm::normalize(glm::cross(cameraFront, cameraUp));
    
    // Ignoramos el eje Y para no volar
    moveDir.y = 0.0f;
    
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir) * cameraSpeed; // Normalizar para no correr super rápido en diagonal
        
        // TRUCO DE FPS: Revisamos colisiones en los ejes X y Z por separado
        // Esto permite que el jugador "resbale" por las paredes en lugar de quedarse atascado.
        
        // Comprobar eje X
        if (!checkCollision(cameraPos.x + moveDir.x, cameraPos.z)) {
            cameraPos.x += moveDir.x;
        }
        
        // Comprobar eje Z
        if (!checkCollision(cameraPos.x, cameraPos.z + moveDir.z)) {
            cameraPos.z += moveDir.z;
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
    float yoffset = lastY - ypos; // Invertido porque las coordenadas Y van de abajo hacia arriba
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Limitar para que el cuello no dé un giro de 360 grados
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

// Cargar una textura desde un archivo usando stb_image
unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // OpenGL espera que el píxel (0,0) esté abajo, pero las imágenes lo tienen arriba
    stbi_set_flip_vertically_on_load(true); 
    
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Parámetros para que se vea retro / pixelado (GL_NEAREST)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
    } else {
        std::cout << "Textura falló al cargar en la ruta: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

// ==========================================
// FUNCIÓN PRINCIPAL
// ==========================================
int main() {
    // 1. Inicializar GLFW y crear la ventana
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Mi Juego 3D (Fase 1)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Fallo al crear la ventana GLFW" << std::endl;
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Capturar el ratón (Inicia libre en el menú, se bloquea al jugar)
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // 2. Inicializar GLAD (Carga punteros de OpenGL)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Fallo al inicializar GLAD" << std::endl;
        return -1;
    }

    // IMPORTANTISIMO para 3D: Activar el Z-Buffer (Prueba de Profundidad)
    glEnable(GL_DEPTH_TEST);

    // 3. Compilar Shaders
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

    // 4. Definir Vértices del Cubo (Posición + Textura UV)
    float vertices[] = {
        // Cara Trasera
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Cara Frontal
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        // Cara Izquierda
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        // Cara Derecha
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        // Cara Inferior (Suelo)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        // Cara Superior (Techo)
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f
    };

    // Crear VAO y VBO
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributo de Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Atributo de Textura UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Cargar texturas reales
    unsigned int wallTexture = loadTexture("assets/paredes.png");
    unsigned int floorTexture = loadTexture("assets/floor.jpg");
    unsigned int logoTexture = loadTexture("assets/startt.png"); // El Logo del inicio

    // ==========================================
    // CONFIGURAR VAO PARA LA UI 2D (LOGO)
    // ==========================================
    float quadVertices[] = {
        // Posiciones x,y,z    Texturas u,v
        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,    0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,

        -0.5f,  0.5f, 0.0f,    0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,    1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,    1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Referencias a los uniforms del Shader
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    // ==========================================
    // BUCLE DE JUEGO (Game Loop)
    // ==========================================
    while (!glfwWindowShouldClose(window)) {
        // Lógica de tiempo
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Entradas (Teclado)
        processInput(window);

        // Limpiar la pantalla y el buffer de profundidad (cielo)
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activar el Shader
        glUseProgram(shaderProgram);

        // --- EFECTO DEL MENÚ (Cámara Rotando Sola) ---
        if (gameState == MENU) {
            yaw += 15.0f * deltaTime; // Girar la cabeza 15 grados por segundo
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }

        glBindVertexArray(VAO);

        // Actualizar el tamaño de la pantalla dinámicamente si el usuario maximiza
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1; // Evitar división por cero

        // 1. Matriz de Proyección (Campo de visión de 45 grados)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)currentWidth / (float)currentHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // 2. Matriz de Vista (La Cámara)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // 3. Dibujar el mapa recorriendo el arreglo worldMap
        for (int z = 0; z < 8; z++) {
            for (int x = 0; x < 8; x++) {
                if (worldMap[z][x] == 1) { // Si hay una pared
                    glBindTexture(GL_TEXTURE_2D, wallTexture);
                    glm::mat4 model = glm::mat4(1.0f);
                    // Trasladar el cubo a su posición x, z (La altura 'y' es 0)
                    model = glm::translate(model, glm::vec3((float)x, 0.0f, (float)z));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    
                    // Darle un ligero tinte a las paredes (o dejarlo en blanco para que tome el color real)
                    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                
                // Dibujar el suelo en TODAS las posiciones (Y = -1.0)
                glBindTexture(GL_TEXTURE_2D, floorTexture);
                glm::mat4 floorModel = glm::mat4(1.0f);
                floorModel = glm::translate(floorModel, glm::vec3((float)x, -1.0f, (float)z));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
                // Suelo de color (un poco oscuro para dar atmósfera)
                glUniform3f(colorLoc, 0.7f, 0.7f, 0.7f);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        } // FIN DEL DIBUJADO 3D

        // ==========================================
        // DIBUJAR INTERFAZ DE USUARIO (2D)
        // ==========================================
        if (gameState == MENU) {
            // Desactivamos la profundidad (para pintar encima de todo)
            // y activamos el canal Alpha para la transparencia de la imagen
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBindTexture(GL_TEXTURE_2D, logoTexture);
            glBindVertexArray(quadVAO);

            // 1. Proyección Ortográfica 2D (ignora la profundidad Z)
            float aspect = (float)currentWidth / (float)currentHeight;
            glm::mat4 orthoProj = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProj));

            // 2. Vista plana estática (sin movimiento)
            glm::mat4 orthoView = glm::mat4(1.0f);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(orthoView));

            // 3. Modelo: Posición y Escala (Efecto Latido para que llame la atención)
            glm::mat4 orthoModel = glm::mat4(1.0f);
            float scale = 0.8f + sin(glfwGetTime() * 3.0f) * 0.05f; // Pulso de latido
            orthoModel = glm::scale(orthoModel, glm::vec3(scale, scale, 1.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(orthoModel));

            // Mostrar el logo flotante sin tintes de color
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Restaurar estado para el siguiente Frame
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        // Intercambiar buffers y procesar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberar recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}