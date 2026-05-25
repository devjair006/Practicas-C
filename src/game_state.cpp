#include "headers/game_state.h"

std::string currentHUDMessage = "";
float hudMessageTimer = 0.0f;
bool isReadingDocument = false;
std::string currentDocumentTitle = "";
std::string currentDocumentBody = "";

glm::vec3 mensBpos(35.231f, 0.250f, 9.150f);
glm::vec3 mensBrot(90.000f, 0.000f, 0.000f);
glm::vec3 mensBscale(1.700f, 1.700f, 1.700f);

glm::vec3 girlBpos(38.776f, 0.250f, 9.150f);
glm::vec3 girlBrot(90.000f, 0.000f, 0.000f);
glm::vec3 girlBscale(1.700f, 1.700f, 1.700f);

glm::vec3 mirrorBGpos(40.815f, 0.150f, 5.55f);
glm::vec3 mirrorBGRot(0.0f, 0.5f, -90.0f);
glm::vec3 mirrorBGScale(1.30f, 1.0f, 3.0f);

glm::vec3 azulejoPos(35.160f, 0.200f, 7.00f);
glm::vec3 azulejoRot(90.0f, -90.0f, 0.0f);
glm::vec3 azulejoScale(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos(33.160f, 0.100f, 3.00f);
glm::vec3 mirrorRot(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos2(33.160f, 0.100f, 4.00f);
glm::vec3 mirrorRot2(0.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale2(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos3(33.160f, 0.100f, 5.00f);
glm::vec3 mirrorRot3(90.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale3(0.540f, 0.520f, 0.630f);

glm::vec3 mirrorPos4(33.160f, 0.1f, 6.00f);
glm::vec3 mirrorRot4(0.0f, -90.0f, 0.0f);
glm::vec3 mirrorScale4(0.540f, 0.520f, 0.630f);

glm::vec3 ligthbathroom2Pos(35.160f, 0.480f, 3.403f);
glm::vec3 ligthbathroom2Rot(0.0f, -180.0f, 0.0f);
glm::vec3 ligthbathroom2Scale(0.520f, 0.490f, 1.0f);
bool ligthbathroomDebugVisible2 = true;

glm::vec3 lamp3Pos(38.160f, 0.480f, 3.403f);
glm::vec3 lamp3Rot(0.0f, -180.0f, 0.0f);
glm::vec3 lamp3Scale(0.520f, 0.490f, 1.0f);

glm::vec3 lamp4Pos(38.160f, 0.480f, 7.403f);
glm::vec3 lamp4Rot(0.0f, -180.0f, 0.0f);
glm::vec3 lamp4Scale(0.520f, 0.490f, 1.0f);

glm::vec3 ligthbathroomPos(35.160f, 0.480f, 7.403f);
glm::vec3 ligthbathroomRot(0.0f, -180.0f, 0.0f);
glm::vec3 ligthbathroomScale(0.520f, 0.490f, 1.0f);
bool ligthbathroomDebugVisible = true;

glm::vec3 banoPos(35.6f, -0.5f, 1.740f);
glm::vec3 banoRot(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos2(36.150f, -0.5f, 1.740f);
glm::vec3 banoRot2(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale2(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos3(35.050f, -0.5f, 1.740f);
glm::vec3 banoRot3(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale3(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos4(34.500f, -0.5f, 1.740f);
glm::vec3 banoRot4(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale4(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos5(38.400f, -0.5f, 1.740f);
glm::vec3 banoRot5(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale5(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos6(37.850f, -0.5f, 1.740f);
glm::vec3 banoRot6(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale6(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos7(38.950f, -0.5f, 1.740f);
glm::vec3 banoRot7(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale7(0.5f, 0.4f, 0.4f);

glm::vec3 banoPos8(39.500f, -0.5f, 1.740f);
glm::vec3 banoRot8(-90.0f, 0.0f, 0.0f);
glm::vec3 banoScale8(0.5f, 0.4f, 0.4f);

glm::vec3 lavamanosPos(33.250f, -0.300f, 3.00f);
glm::vec3 lavamanosRot(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos2(33.250f, -0.300f, 4.00f);
glm::vec3 lavamanosRot2(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale2(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos3(33.250f, -0.300f, 5.00f);
glm::vec3 lavamanosRot3(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale3(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos4(33.250f, -0.300f, 6.00f);
glm::vec3 lavamanosRot4(0.0f, -90.0f, 0.0f);
glm::vec3 lavamanosScale4(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos5(40.750f, -0.300f, 3.00f);
glm::vec3 lavamanosRot5(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale5(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos6(40.750f, -0.300f, 4.00f);
glm::vec3 lavamanosRot6(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale6(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos7(40.750f, -0.300f, 5.00f);
glm::vec3 lavamanosRot7(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale7(0.540f, 0.520f, 0.630f);

glm::vec3 lavamanosPos8(40.750f, -0.300f, 6.00f);
glm::vec3 lavamanosRot8(0.0f, 90.0f, 0.0f);
glm::vec3 lavamanosScale8(0.540f, 0.520f, 0.630f);

glm::vec3 urinarioPos(36.8f, -0.54f, 5.2f);
glm::vec3 urinarioRot(-90.5f, -2.5f, -90.0f);
glm::vec3 urinarioScale(0.4f, 0.4f, 0.4f);

glm::vec3 cameraPos = glm::vec3(6.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float headBobTimer = 0.0f;
float baseCameraY = 0.0f;
bool isMoving = false;
float stamina = 100.0f;
bool isSprinting = false;
bool isExhausted = false;
GameState gameState = MENU;
bool isCursorLocked = false;
bool tabKeyWasPressed = false;
bool eKeyWasPressed = false;
bool isFlashlightOn = true;
bool fKeyWasPressed = false;

ma_engine audioEngine;

int bateriasRecolectadas = 0;
bool hasKeycardLvl1 = false;
bool hasKeycardLvl2 = false;
bool dimensionAlterna = false;
bool portalActivado = false;
int currentZone = 1;
bool showDebugGUI = true;

GLTFModel *banoGLTF = nullptr;
GLTFModel *lavamanosGLTF = nullptr;
GLTFModel *urinarioGLTF = nullptr;
GLTFModel *mensBGLTF = nullptr;
GLTFModel *girlBGLTF = nullptr;
// area de contencion
GLTFModel *teslaGLTF = nullptr;
GLTFModel *paredesGLTF = nullptr;

glm::vec3 teslaPos(44.800f, -0.500f, 14.200f);
glm::vec3 teslaRot(-88.000f, 0.0f, 0.0f);
glm::vec3 teslaScale(0.150f, 0.120f, 0.090f);

std::vector<WallDef> paredesList = {
    // pared sur (x=48.75) - 2 paneles que cubren Z≈13..21
    {glm::vec3(48.750f, -0.500f, 15.000f),
     glm::vec3(-90.000f, 1.000f, -88.500f), glm::vec3(1.040f, 3.000f, 0.520f)},
    {glm::vec3(48.750f, -0.500f, 19.000f),
     glm::vec3(-90.000f, 1.000f, -88.500f), glm::vec3(1.040f, 3.000f, 0.520f)},

    // pared norte (x=34.25) - 2 paneles que cubren Z≈13..21
    {glm::vec3(34.250f, -0.500f, 15.000f), glm::vec3(-90.000f, 1.000f, 91.500f),
     glm::vec3(1.040f, 3.000f, 0.520f)},
    {glm::vec3(34.250f, -0.500f, 19.000f), glm::vec3(-90.000f, 1.000f, 91.500f),
     glm::vec3(1.040f, 3.000f, 0.520f)}};

std::vector<Entity> gameEntities = {
    {glm::vec3(8.0f, -0.4f, 4.0f), 3, true,
     "[CABLE SUELTO]:Hay un cable pelado aqui.", 0.0f},
    {glm::vec3(20.0f, -0.4f, 5.0f), 0, true,
     "LOG 1 (Arrugado): 'Apagon general. Las compuertas se bloquearon.'", 0.0f},
    {glm::vec3(42.0f, -0.2f, 5.0f), 8, true, "", 0.0f},
    {glm::vec3(12.0f, -0.2f, 6.0f), 1, true, "", 0.0f},
    {glm::vec3(24.0f, -0.5f, 6.0f), 4, true, "", 1.0f},
    {glm::vec3(24.0f, 0.0f, 6.0f), 5, true,
     "[MONITOR AUXILIAR]: 'Sistema inestable.'", 1.5f},
    {glm::vec3(10.0f, -0.5f, 15.0f), 4, true, "", 2.0f},
    {glm::vec3(10.0f, 0.0f, 15.0f), 5, true,
     "[PANTALLA ERROR]: 'Falla de contencion.'", 2.5f},
    {glm::vec3(28.0f, 0.0f, 16.0f), 6, true, "[MAQUINA]: Unidad Frigorifica.",
     5.0f},
    {glm::vec3(42.0f, -0.4f, 17.0f), 9, true, "", 0.0f},
    {glm::vec3(42.0f, -0.2f, 15.0f), 0, true,
     "LOG 2 (Sangriento): 'La muestra escapo.'", 0.0f},
    {glm::vec3(15.0f, -0.2f, 18.0f), 1, true, "", 0.0f},
    {glm::vec3(10.0f, -0.4f, 28.0f), 3, true,
     "[MANCHA]: Rastro oscuro hacia ventilacion.", 0.0f},
    {glm::vec3(10.0f, -0.5f, 38.0f), 4, true, "", 3.0f},
    {glm::vec3(10.0f, 0.0f, 38.0f), 5, true,
     "[REGISTRO MAESTRO]: 'EVACUACION INMEDIATA.'", 3.5f},
    {glm::vec3(28.0f, 0.0f, 38.0f), 6, true, "[GENERADOR]: Requiere reinicio.",
     6.0f},
    {glm::vec3(22.0f, -0.2f, 40.0f), 1, true, "", 0.0f},
    {glm::vec3(24.0f, 1.0f, 46.0f), 7, true,
     "[PALANCA MAESTRA]: Energia restaurada.", 0.0f},
    {glm::vec3(25.0f, 0.0f, 3.0f), 2, true, "", 0.0f}};

float wallWidth = 0.3f;
float wallHeight = 1.0f;
float door1Anim = 0.0f;
bool door1Opening = false;
float door2Anim = 0.0f;
bool door2Opening = false;

int worldMap[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
     0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
     0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
    //////////////////////
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 8, 8, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
     1, 1, 1, 9, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
