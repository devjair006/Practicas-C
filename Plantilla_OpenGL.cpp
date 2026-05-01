// ================================================================
//  DEMO: Espacio Mundo  vs  NDC
//
//  IDEA CENTRAL (una sola oración):
//    Sin importar que un triángulo tenga coordenadas enormes (50, -40...)
//    o coordenadas normalizadas (0.5, -0.4...), OpenGL siempre termina
//    dibujando en el mismo lugar porque AMBOS llegan a NDC [-1,+1].
//
//  QUÉ VES EN PANTALLA:
//    Izquierda (AZUL)   = triángulo con coordenadas GRANDES + matrices MVP
//    Derecha  (NARANJA) = triángulo con coordenadas NDC directas, sin matrices
//    Son visualmente iguales — eso ES la demostración.
//
//  CONTROLES:
//    WASD = mover   |   R = resetear   |   ESC = salir
// ================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// ----------------------------------------------------------------
// PASO 1 — Shaders escritos directamente en el código (sin archivos)
//
// Así el programa compila y corre sin depender de rutas de archivos.
// Cada shader es solo un string con código GLSL.
// ----------------------------------------------------------------

// Shader del lado IZQUIERDO (Espacio Mundo):
// Recibe vértices enormes y los multiplica por las matrices MVP.
// Las matrices son las que los "caben" en NDC.
const char* SHADER_VERT_MUNDO = R"(
    #version 330 core
    layout(location = 0) in vec3 pos;   // coordenadas GRANDES (ej: 50, -40...)

    uniform mat4 model;       // transforma: local → mundo
    uniform mat4 view;        // transforma: mundo → cámara
    uniform mat4 projection;  // transforma: cámara → NDC

    void main() {
        // Esta línea hace TODO el trabajo de normalización.
        // Sin ella, los vértices de 50 unidades nunca cabrían en pantalla.
        gl_Position = projection * view * model * vec4(pos, 1.0);
    }
)";

// Shader del lado DERECHO (NDC directo):
// Recibe vértices ya normalizados. No necesita hacer nada con ellos.
const char* SHADER_VERT_NDC = R"(
    #version 330 core
    layout(location = 0) in vec2 pos;   // coordenadas YA en NDC (ej: 0.5, -0.4...)

    uniform vec2 offset;   // movimiento del triángulo en NDC

    void main() {
        // Sin matrices. Los vértices ya están donde OpenGL los espera.
        // gl_Position recibe directamente los valores NDC.
        gl_Position = vec4(pos + offset, 0.0, 1.0);
    }
)";

// Fragment shader compartido por ambos lados.
// Solo pinta el color que le mandamos desde C++.
const char* SHADER_FRAG = R"(
    #version 330 core
    out vec4 color_final;
    uniform vec3 color;

    void main() {
        color_final = vec4(color, 1.0);
    }
)";

// ----------------------------------------------------------------
// PASO 2 — Estado de la demo (posición del triángulo)
// ----------------------------------------------------------------
float offsetX = 0.0f;
float offsetY = 0.0f;

// ----------------------------------------------------------------
// PASO 3 — Funciones mínimas de ayuda
// ----------------------------------------------------------------

// Compila un shader y devuelve su ID.
// Si falla, imprime el error y devuelve 0.
unsigned int compilarShader(unsigned int tipo, const char* codigo) {
    unsigned int id = glCreateShader(tipo);
    glShaderSource(id, 1, &codigo, nullptr);
    glCompileShader(id);

    int ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char error[512];
        glGetShaderInfoLog(id, 512, nullptr, error);
        std::cerr << "Error en shader: " << error << "\n";
    }
    return id;
}

// Une vertex + fragment shader en un programa OpenGL.
unsigned int crearPrograma(const char* vert, const char* frag) {
    unsigned int v = compilarShader(GL_VERTEX_SHADER,   vert);
    unsigned int f = compilarShader(GL_FRAGMENT_SHADER, frag);

    unsigned int programa = glCreateProgram();
    glAttachShader(programa, v);
    glAttachShader(programa, f);
    glLinkProgram(programa);

    glDeleteShader(v);  // ya no los necesitamos después de linkear
    glDeleteShader(f);
    return programa;
}

// Manejo de teclado
void teclado(GLFWwindow* ventana, int tecla, int, int accion, int) {
    if (tecla == GLFW_KEY_ESCAPE && accion == GLFW_PRESS)
        glfwSetWindowShouldClose(ventana, true);

    if (tecla == GLFW_KEY_R && accion == GLFW_PRESS) {
        offsetX = 0.0f;
        offsetY = 0.0f;
    }
}

void procesarInput(GLFWwindow* ventana) {
    const float velocidad = 0.01f;
    if (glfwGetKey(ventana, GLFW_KEY_W) == GLFW_PRESS) offsetY += velocidad;
    if (glfwGetKey(ventana, GLFW_KEY_S) == GLFW_PRESS) offsetY -= velocidad;
    if (glfwGetKey(ventana, GLFW_KEY_A) == GLFW_PRESS) offsetX -= velocidad;
    if (glfwGetKey(ventana, GLFW_KEY_D) == GLFW_PRESS) offsetX += velocidad;
}

// ================================================================
// MAIN — todo el programa en orden lineal, sin sorpresas
// ================================================================
int main() {

    // ============================================================
    // BLOQUE A — Inicializar ventana OpenGL
    // (código de instalación — no es el concepto, solo preparación)
    // ============================================================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* ventana = glfwCreateWindow(1200, 600, "Mundo vs NDC  |  WASD=mover  R=reset  ESC=salir", nullptr, nullptr);
    glfwMakeContextCurrent(ventana);
    glfwSetKeyCallback(ventana, teclado);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    std::cout << "\n--- DEMO: Espacio Mundo vs NDC ---\n";
    std::cout << "AZUL   (izq) = coordenadas grandes + matrices MVP\n";
    std::cout << "NARANJA(der) = coordenadas NDC directas, sin matrices\n";
    std::cout << "Ambos dibujan en el mismo lugar. Esa es la idea.\n\n";

    // ============================================================
    // BLOQUE B — Preparar los dos shaders
    // ============================================================
    unsigned int shaderMundo = crearPrograma(SHADER_VERT_MUNDO, SHADER_FRAG);
    unsigned int shaderNDC   = crearPrograma(SHADER_VERT_NDC,   SHADER_FRAG);

    // ============================================================
    // BLOQUE C — Definir los vértices
    //
    // IZQUIERDA: coordenadas de "mundo real" — números grandes.
    // Si los mandas directo a OpenGL sin matrices, no verías NADA
    // porque 50 unidades está muy fuera del cubo NDC [-1, +1].
    // ============================================================
    float verticesMundo[] = {
         0.0f,  50.0f, 0.0f,   // arriba    (50 unidades hacia arriba)
       -40.0f, -30.0f, 0.0f,   // abajo-izq (40 unidades a la izquierda)
        40.0f, -30.0f, 0.0f,   // abajo-der (40 unidades a la derecha)
    };

    // DERECHA: exactamente el mismo triángulo, pero ya normalizado.
    // 50 unidades de mundo → 0.5 en NDC.
    // 40 unidades de mundo → 0.4 en NDC.
    float verticesNDC[] = {
         0.0f,  0.5f,   // arriba    (equivale a Y=50)
        -0.4f, -0.3f,   // abajo-izq (equivale a X=-40, Y=-30)
         0.4f, -0.3f,   // abajo-der (equivale a X=+40, Y=-30)
    };

    // Subir vértices MUNDO a la GPU
    unsigned int vaoMundo, vboMundo;
    glGenVertexArrays(1, &vaoMundo);
    glGenBuffers(1, &vboMundo);
    glBindVertexArray(vaoMundo);
    glBindBuffer(GL_ARRAY_BUFFER, vboMundo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesMundo), verticesMundo, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // Subir vértices NDC a la GPU
    unsigned int vaoNDC, vboNDC;
    glGenVertexArrays(1, &vaoNDC);
    glGenBuffers(1, &vboNDC);
    glBindVertexArray(vaoNDC);
    glBindBuffer(GL_ARRAY_BUFFER, vboNDC);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesNDC), verticesNDC, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // ============================================================
    // BLOQUE D — Loop principal
    // Cada frame: input → construir matrices → dibujar izq → dibujar der
    // ============================================================
    while (!glfwWindowShouldClose(ventana)) {

        glfwPollEvents();
        procesarInput(ventana);

        int anchoTotal, alto;
        glfwGetFramebufferSize(ventana, &anchoTotal, &alto);
        int mitad = anchoTotal / 2;

        // Limpiar pantalla
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Actualizar título con los valores NDC actuales
        char titulo[200];
        snprintf(titulo, sizeof(titulo),
            "Mundo vs NDC  |  offset NDC: x=%.2f  y=%.2f  |  WASD=mover  R=reset",
            offsetX, offsetY);
        glfwSetWindowTitle(ventana, titulo);

        // --------------------------------------------------------
        // DIBUJAR LADO IZQUIERDO — Espacio Mundo con matrices MVP
        //
        // Aquí construimos las 3 matrices que necesita OpenGL
        // para convertir los vértices grandes a NDC.
        // --------------------------------------------------------
        glViewport(0, 0, mitad, alto);   // solo la mitad izquierda

        // Matriz MODEL: dónde está el objeto en el mundo
        // Usamos el offset * 100 para convertir unidades NDC → mundo
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(offsetX * 100.0f, offsetY * 100.0f, 0.0f));

        // Matriz VIEW: dónde está la cámara
        // La cámara mira desde Z=150 hacia el origen
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 150.0f),  // posición de la cámara
            glm::vec3(0.0f, 0.0f,   0.0f),  // mira hacia aquí
            glm::vec3(0.0f, 1.0f,   0.0f)   // "arriba" es Y+
        );

        // Matriz PROJECTION: cómo se aplana el mundo 3D en la pantalla 2D
        // Esta matriz ES la que "cabe" los vértices de 50 unidades en [-1,+1]
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),            // ángulo de visión
            (float)mitad / (float)alto,     // proporción de la pantalla
            0.1f,                           // distancia mínima visible
            500.0f                          // distancia máxima visible
        );

        // Mandar las matrices al shader y dibujar
        glUseProgram(shaderMundo);
        glUniformMatrix4fv(glGetUniformLocation(shaderMundo, "model"),      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderMundo, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderMundo, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shaderMundo, "color"), 0.25f, 0.55f, 0.95f);  // azul

        glBindVertexArray(vaoMundo);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --------------------------------------------------------
        // DIBUJAR LADO DERECHO — NDC directo, sin matrices
        //
        // Los vértices ya están normalizados.
        // Solo mandamos el offset para que se mueva igual que el izquierdo.
        // --------------------------------------------------------
        glViewport(mitad, 0, mitad, alto);   // solo la mitad derecha

        glUseProgram(shaderNDC);
        glUniform2f(glGetUniformLocation(shaderNDC, "offset"), offsetX, offsetY);
        glUniform3f(glGetUniformLocation(shaderNDC, "color"), 0.95f, 0.55f, 0.15f);  // naranja

        glBindVertexArray(vaoNDC);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --------------------------------------------------------
        // DIBUJAR LÍNEA DIVISORIA
        // Volvemos al viewport completo para la línea del medio
        // --------------------------------------------------------
        glViewport(0, 0, anchoTotal, alto);

        float lineaVerts[] = { 0.0f, -1.0f,  0.0f, 1.0f };
        unsigned int vaoLinea, vboLinea;
        glGenVertexArrays(1, &vaoLinea);
        glGenBuffers(1, &vboLinea);
        glBindVertexArray(vaoLinea);
        glBindBuffer(GL_ARRAY_BUFFER, vboLinea);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineaVerts), lineaVerts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glUniform2f(glGetUniformLocation(shaderNDC, "offset"), 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderNDC, "color"), 1.0f, 1.0f, 1.0f);  // blanco
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 2);

        glDeleteVertexArrays(1, &vaoLinea);
        glDeleteBuffers(1, &vboLinea);

        glfwSwapBuffers(ventana);
    }

    // ============================================================
    // BLOQUE E — Limpieza
    // ============================================================
    glDeleteVertexArrays(1, &vaoMundo);
    glDeleteBuffers(1, &vboMundo);
    glDeleteVertexArrays(1, &vaoNDC);
    glDeleteBuffers(1, &vboNDC);
    glDeleteProgram(shaderMundo);
    glDeleteProgram(shaderNDC);
    glfwTerminate();

    return 0;
}