// ============================================================================
//  OPENGL NDC - DEFINITIVE EDUCATIONAL EDITION (FINAL FIX)
// ============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <math.h>

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform vec2 offset;
    uniform float rotation;
    uniform float aspect;
    uniform bool fixAspect;

    void main() {
        float s = sin(rotation);
        float c = cos(rotation);
        vec2 rotatedPos = vec2(aPos.x * c - aPos.y * s, aPos.x * s + aPos.y * c);
        vec2 pos = rotatedPos + offset;
        if(fixAspect) pos.x /= aspect; 
        gl_Position = vec4(pos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 ourColor;
    void main() { FragColor = vec4(ourColor, 1.0); }
)";

struct Shape {
    unsigned int vao, vbo;
    int count;
    void setup(const std::vector<float>& vertices) {
        count = (int)(vertices.size() / 2);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    void draw(GLenum mode, float r, float g, float b, int colorLoc, float ox=0, float oy=0, int offLoc=-1) {
        glUniform3f(colorLoc, r, g, b);
        if(offLoc != -1) glUniform2f(offLoc, ox, oy);
        glBindVertexArray(vao);
        glDrawArrays(mode, 0, (GLsizei)count);
    }
};

// --- TEXTO DIGITAL ---
// Usa su propio VAO/VBO dedicado, inicializado una vez
static unsigned int textVao = 0, textVbo = 0;

void initTextBuffers() {
    glGenVertexArrays(1, &textVao);
    glGenBuffers(1, &textVbo);
    glBindVertexArray(textVao);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    // Reservar espacio suficiente para cualquier dígito (max 20 floats = 10 vértices)
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void drawDigit(int d, float x, float y, float size, int cLoc, int oLoc) {
    std::vector<float> v;
    float w = size * 0.7f; float h = size;
    if (d==0) v={0,0, w,0, w,0, w,h, w,h, 0,h, 0,h, 0,0};
    else if (d==1) v={w/2,0, w/2,h};
    else if (d==2) v={0,h, w,h, w,h, w,h/2, w,h/2, 0,h/2, 0,h/2, 0,0, 0,0, w,0};
    else if (d==3) v={0,h, w,h, w,h, w,0, w,0, 0,0, 0,h/2, w,h/2};
    else if (d==4) v={0,h, 0,h/2, 0,h/2, w,h/2, w,h, w,0};
    else if (d==5) v={w,h, 0,h, 0,h, 0,h/2, 0,h/2, w,h/2, w,h/2, w,0, w,0, 0,0};
    else if (d==6) v={w,h, 0,h, 0,h, 0,0, 0,0, w,0, w,0, w,h/2, w,h/2, 0,h/2};
    else if (d==7) v={0,h, w,h, w,h, w,0};
    else if (d==8) v={0,0, w,0, w,0, w,h, w,h, 0,h, 0,h, 0,0, 0,h/2, w,h/2};
    else if (d==9) v={w,h/2, 0,h/2, 0,h/2, 0,h, 0,h, w,h, w,h, w,0};
    else if (d==-1) v={0, h/2, w*0.6f, h/2};  // signo menos
    else if (d==-2) v={0, 0, 0.01f, 0.01f};    // punto decimal
    else return;

    glBindVertexArray(textVao);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(float), v.data());
    glUniform2f(oLoc, x, y);
    glDrawArrays(GL_LINES, 0, (GLsizei)(v.size()/2));
}

void drawValue(float val, float x, float y, int cLoc, int oLoc) {
    glUniform3f(cLoc, 0.0f, 1.0f, 1.0f);
    if(val < 0) { drawDigit(-1, x, y, 0.06f, cLoc, oLoc); x+=0.06f; val = -val; }
    int entero = (int)val;
    int decimal = (int)((val - entero) * 100);
    drawDigit(entero % 10, x, y, 0.06f, cLoc, oLoc);
    drawDigit(-2, x + 0.05f, y, 0.06f, cLoc, oLoc);
    drawDigit(std::abs(decimal/10)%10, x + 0.07f, y, 0.06f, cLoc, oLoc);
    drawDigit(std::abs(decimal)%10, x + 0.12f, y, 0.06f, cLoc, oLoc);
}

// --- Función de conversión: coordenadas grandes (píxeles) -> NDC ---
// Fórmula: ndc = (2 * coordenada / resolución) - 1
float toNDC(float coord, float resolution) {
    return (2.0f * coord / resolution) - 1.0f;
}

// Globales
int lesson = 1; bool fix = false;
float px = 0, py = 0, prot = 0;
double lastMouseX = 400.0;
bool mouseInitialized = false;

int world[8][8] = {
    {1,1,1,1,1,1,1,1}, {1,0,0,0,0,0,0,1}, {1,0,1,0,0,1,0,1}, {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1}, {1,0,1,1,0,1,0,1}, {1,0,0,0,0,0,0,1}, {1,1,1,1,1,1,1,1}
};

// Helper: check if position is walkable
bool canWalk(float nx, float ny) {
    int ix = (int)nx, iy = (int)ny;
    if(ix < 0 || ix >= 8 || iy < 0 || iy >= 8) return false;
    return world[iy][ix] == 0;
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 800, "NDC Academy", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL); glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL); glCompileShader(fs);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glUseProgram(prog);

    int cLoc = glGetUniformLocation(prog, "ourColor");
    int oLoc = glGetUniformLocation(prog, "offset");
    int rLoc = glGetUniformLocation(prog, "rotation");
    int aLoc = glGetUniformLocation(prog, "aspect");
    int fLoc = glGetUniformLocation(prog, "fixAspect");

    // Inicializar buffers de texto
    initTextBuffers();

    Shape grid, bounds, square, bigTri, circle;
    grid.setup({-1,0.5, 1,0.5, -1,-0.5, 1,-0.5, 0.5,-1, 0.5,1, -0.5,-1, -0.5,1, -1,0, 1,0, 0,-1, 0,1});
    bounds.setup({-1,-1, 1,-1, 1,1, -1,1});
    square.setup({-0.05f,-0.05f, 0.05f,-0.05f, 0.05f,0.05f, -0.05f,0.05f});
    bigTri.setup({-1.2f, -1.2f, 1.2f, -1.2f, 0.0f, 1.2f});
    
    std::vector<float> cv; cv.push_back(0); cv.push_back(0);
    for(int i=0; i<=360; i+=10) { cv.push_back(cos(i*3.1415f/180.0f)*0.5f); cv.push_back(sin(i*3.1415f/180.0f)*0.5f); }
    circle.setup(cv);

    // === LESSON 5: Shapes ===
    // Lado izquierdo: Estrella con vértices DIRECTOS en NDC (-1 a 1)
    // Estos valores ya están en el rango que OpenGL acepta
    Shape ndcStar;
    ndcStar.setup({
        // Estrella de 5 puntas - vértices directos en NDC
         0.0f,  0.85f,   // punta superior
        -0.2f,  0.25f,   // interior izq-sup
        -0.8f,  0.25f,   // punta izquierda
        -0.3f, -0.1f,    // interior izq-inf
        -0.5f, -0.7f,    // punta inferior-izq
         0.0f, -0.3f,    // interior inferior
         0.5f, -0.7f,    // punta inferior-der
         0.3f, -0.1f,    // interior der-inf
         0.8f,  0.25f,   // punta derecha
         0.2f,  0.25f    // interior der-sup
    });

    // Lado derecho: Hexágono con vértices en PÍXELES convertidos a NDC
    // Suponiendo una ventana de 800x800, el hexágono está centrado en (600, 400)
    // con radio de 150 píxeles
    float winW = 800.0f, winH = 800.0f;
    float cx5 = 600.0f, cy5 = 400.0f, rad5 = 150.0f;
    Shape convertedHex;
    std::vector<float> hexVerts;
    hexVerts.push_back(toNDC(cx5, winW)); hexVerts.push_back(toNDC(cy5, winH)); // centro
    for(int i = 0; i <= 6; i++) {
        float angle = i * 3.1415926f * 2.0f / 6.0f;
        float hx = cx5 + cos(angle) * rad5;  // coordenada en píxeles
        float hy = cy5 + sin(angle) * rad5;  // coordenada en píxeles
        hexVerts.push_back(toNDC(hx, winW)); // convertido a NDC
        hexVerts.push_back(toNDC(hy, winH)); // convertido a NDC
    }
    convertedHex.setup(hexVerts);

    // Línea divisoria vertical para separar las dos mitades
    Shape divider;
    divider.setup({0, -1, 0, 1});

    // === Labels con líneas para indicar qué lado es cuál ===
    // Indicador "NDC" lado izquierdo (flecha apuntando izq)
    Shape arrowLeft;
    arrowLeft.setup({
        -0.05f, 0.95f,  -0.9f, 0.95f,  // línea horizontal
        -0.9f, 0.95f,   -0.8f, 0.98f,  // punta flecha arriba
        -0.9f, 0.95f,   -0.8f, 0.92f   // punta flecha abajo
    });
    // Indicador "CONV" lado derecho (flecha apuntando der)
    Shape arrowRight;
    arrowRight.setup({
        0.05f, 0.95f,   0.9f, 0.95f,   // línea horizontal
        0.9f, 0.95f,    0.8f, 0.98f,   // punta flecha arriba
        0.9f, 0.95f,    0.8f, 0.92f    // punta flecha abajo
    });

    // Pre-crear VAO/VBO para los rayos del modo Doom
    // Cada rayo = 4 vértices (quad como triangle strip), 2 floats por vértice
    unsigned int rayVao, rayVbo;
    glGenVertexArrays(1, &rayVao);
    glGenBuffers(1, &rayVbo);
    glBindVertexArray(rayVao);
    glBindBuffer(GL_ARRAY_BUFFER, rayVbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    const int NUM_RAYS = 400;
    const float FOV = 1.0f; // campo de visión en radianes (~57 grados)

    while (!glfwWindowShouldClose(window)) {
        if(glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) { lesson=1; px=0; py=0; prot=0; mouseInitialized=false; glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
        if(glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) { lesson=2; px=0; py=0; prot=0; mouseInitialized=false; glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
        if(glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) { lesson=3; px=0; py=0; prot=0; mouseInitialized=false; glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
        if(glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) { 
            lesson=4; px=1.5f; py=1.5f; prot=0; 
            mouseInitialized=false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if(glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) { lesson=5; px=0; py=0; prot=0; mouseInitialized=false; glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
        if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) { fix=!fix; glfwWaitEventsTimeout(0.2); }

        float s = 0.015f;
        if(lesson != 4 && lesson != 5) {
            if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) px -= s;
            if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) px += s;
            if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) py += s;
            if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) py -= s;
        } else if(lesson == 4) {
            // === MOUSE: controla la cámara ===
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            if(!mouseInitialized) {
                lastMouseX = mouseX;
                mouseInitialized = true;
            }
            double deltaX = mouseX - lastMouseX;
            lastMouseX = mouseX;
            prot += (float)deltaX * 0.003f; // sensibilidad del mouse

            // === A/D: strafe izquierda/derecha ===
            float strafeSpeed = 0.04f;
            if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                float nx = px - cos(prot) * strafeSpeed;
                float ny = py + sin(prot) * strafeSpeed;
                if(canWalk(nx, ny)) { px = nx; py = ny; }
            }
            if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                float nx = px + cos(prot) * strafeSpeed;
                float ny = py - sin(prot) * strafeSpeed;
                if(canWalk(nx, ny)) { px = nx; py = ny; }
            }

            // === W/S: avanzar/retroceder ===
            float move = 0.0f;
            if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move = 0.05f;
            if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move = -0.05f;
            
            if(move != 0.0f) {
                float nx = px + sin(prot) * move;
                float ny = py + cos(prot) * move;
                if(canWalk(nx, ny)) { px = nx; py = ny; }
            }

            // ESC para soltar el mouse
            if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        // Lesson 5: sin controles de movimiento

        int fw, fh; glfwGetFramebufferSize(window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glUniform1f(aLoc, (float)fw/fh); glUniform1i(fLoc, (lesson==3 && fix));
        glUniform1f(rLoc, 0);

        if(lesson < 4) {
            glUniform2f(oLoc, 0, 0);
            bounds.draw(GL_LINE_LOOP, 1, 1, 1, cLoc);
            drawValue(px, -0.95f, 0.9f, cLoc, oLoc);
            drawValue(py, -0.95f, 0.82f, cLoc, oLoc);

            // IMPORTANTE: resetear offset a (0,0) después de drawValue
            glUniform2f(oLoc, 0, 0);
            
            if(lesson == 1) {
                grid.draw(GL_LINES, 0.2f, 0.2f, 0.2f, cLoc, 0, 0, oLoc);
                square.draw(GL_TRIANGLE_FAN, 0.2f, 1.0f, 0.5f, cLoc, px, py, oLoc);
            } else if(lesson == 2) {
                bigTri.draw(GL_TRIANGLES, 0.3f, 0.6f, 1.0f, cLoc, px, py, oLoc);
            } else if(lesson == 3) {
                circle.draw(GL_TRIANGLE_FAN, 1.0f, 0.9f, 0.3f, cLoc, px, py, oLoc);
            }
        } else if(lesson == 5) {
            // === LESSON 5: NDC directo vs Conversión de coordenadas ===
            glUniform2f(oLoc, 0, 0);
            glUniform1f(rLoc, 0);
            glUniform1i(fLoc, 0);

            // Línea divisoria central
            divider.draw(GL_LINES, 0.5f, 0.5f, 0.5f, cLoc, 0, 0, oLoc);

            // Flechas indicadoras
            arrowLeft.draw(GL_LINES, 0.0f, 1.0f, 0.5f, cLoc, 0, 0, oLoc);   // verde - NDC directo
            arrowRight.draw(GL_LINES, 1.0f, 0.5f, 0.0f, cLoc, 0, 0, oLoc);  // naranja - convertido

            // --- LADO IZQUIERDO: Estrella con vértices NDC directos ---
            // Los vértices ya están en rango [-1, 1], escalados a la mitad izquierda
            // Dibujamos la estrella centrada en la mitad izquierda
            ndcStar.draw(GL_TRIANGLE_FAN, 0.2f, 0.9f, 0.4f, cLoc, -0.5f, 0, oLoc);
            // Contorno
            ndcStar.draw(GL_LINE_LOOP, 0.0f, 1.0f, 0.5f, cLoc, -0.5f, 0, oLoc);

            // --- LADO DERECHO: Hexágono con coordenadas convertidas ---
            // Los vértices fueron creados en píxeles (ej: centro 600,400, radio 150px)
            // y convertidos a NDC con: ndc = (2*coord/800) - 1
            convertedHex.draw(GL_TRIANGLE_FAN, 0.9f, 0.5f, 0.1f, cLoc, 0, 0, oLoc);
            // Contorno
            convertedHex.draw(GL_LINE_LOOP, 1.0f, 0.6f, 0.0f, cLoc, 0, 0, oLoc);

            // Mostrar textos informativos con drawValue
            // Lado izquierdo: mostrar rango NDC (-1 a 1)
            glUniform3f(cLoc, 0.0f, 1.0f, 0.5f);
            drawDigit(-1, -0.95f, -0.95f, 0.05f, cLoc, oLoc); // "-1"
            drawDigit(1, -0.88f, -0.95f, 0.05f, cLoc, oLoc);  // "1" -> muestra -1
            
            // Lado derecho: mostrar valor convertido ejemplo
            glUniform3f(cLoc, 1.0f, 0.6f, 0.0f);
            drawDigit(8, 0.1f, -0.95f, 0.05f, cLoc, oLoc);  // "800" px
            drawDigit(0, 0.15f, -0.95f, 0.05f, cLoc, oLoc);
            drawDigit(0, 0.2f, -0.95f, 0.05f, cLoc, oLoc);

            // Resetear offset
            glUniform2f(oLoc, 0, 0);
        } else if(lesson == 4) {
            // === MODO DOOM ===
            // Dibujar techo y piso
            float ceilingVerts[] = { -1, 0, 1, 0, 1, 1, -1, 1 };
            float floorVerts[] = { -1, -1, 1, -1, 1, 0, -1, 0 };

            glBindVertexArray(rayVao);
            glBindBuffer(GL_ARRAY_BUFFER, rayVbo);
            glUniform2f(oLoc, 0, 0);
            glUniform1f(rLoc, 0);

            // Techo
            glBufferData(GL_ARRAY_BUFFER, sizeof(ceilingVerts), ceilingVerts, GL_STREAM_DRAW);
            glUniform3f(cLoc, 0.05f, 0.05f, 0.12f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            // Piso
            glBufferData(GL_ARRAY_BUFFER, sizeof(floorVerts), floorVerts, GL_STREAM_DRAW);
            glUniform3f(cLoc, 0.08f, 0.06f, 0.04f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            // Raycast walls
            float rayWidth = 2.0f / (float)NUM_RAYS;
            for(int x = 0; x < NUM_RAYS; x++) {
                float rayAngle = prot - FOV/2.0f + ((float)x / (float)NUM_RAYS) * FOV;
                float rx = px, ry = py;
                float vx = sin(rayAngle), vy = cos(rayAngle);
                float d = 0;
                while(d < 16.0f) {
                    rx += vx * 0.01f; 
                    ry += vy * 0.01f; 
                    d += 0.01f;
                    int ix = (int)rx, iy = (int)ry;
                    if(ix < 0 || ix >= 8 || iy < 0 || iy >= 8 || world[iy][ix] != 0) break;
                }

                // Corregir fisheye
                float correctedDist = d * cos(rayAngle - prot);
                if(correctedDist < 0.01f) correctedDist = 0.01f;

                float wallH = 0.8f / correctedDist;
                if(wallH > 1.0f) wallH = 1.0f;

                // Posición X de este rayo en pantalla
                float screenX = -1.0f + x * rayWidth;

                // Quad: 4 vértices como triangle strip
                float quad[] = {
                    screenX,            -wallH,
                    screenX + rayWidth,  -wallH,
                    screenX,             wallH,
                    screenX + rayWidth,   wallH
                };

                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

                // Color con atenuación por distancia
                float bright = 1.0f / (correctedDist * 0.4f + 0.3f);
                if(bright > 1.0f) bright = 1.0f;
                glUniform3f(cLoc, 0.25f * bright, 0.45f * bright, 0.85f * bright);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &rayVao);
    glDeleteBuffers(1, &rayVbo);
    glDeleteVertexArrays(1, &textVao);
    glDeleteBuffers(1, &textVbo);
    glfwTerminate(); return 0;
}
