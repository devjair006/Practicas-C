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

glm::vec3 sillasPos = glm::vec3(30.3f, -0.35f, 8.6f); 
glm::vec3 sillasRot = glm::vec3(0.0f, 90.0f, 0.0f); 
glm::vec3 sillasScale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas2Pos = glm::vec3(26.7f, -0.35f, 8.6f);
glm::vec3 sillas2Rot = glm::vec3(0.0f, 90.0f, 0.0f);   
glm::vec3 sillas2Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas3Pos = glm::vec3(31.8f, -0.35f, 8.6f); 
glm::vec3 sillas3Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas3Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas4Pos = glm::vec3(24.0f, -0.35f, 8.6f); 
glm::vec3 sillas4Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas4Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sillas5Pos = glm::vec3(21.3f, -0.35f, 8.6f); 
glm::vec3 sillas5Rot = glm::vec3(0.0f, 90.0f, 0.0f);
glm::vec3 sillas5Scale = glm::vec3(0.65f, 0.65f, 0.65f);

glm::vec3 sofaPos = glm::vec3(13.7f, -0.40f, 1.7f);
glm::vec3 sofaRot = glm::vec3(90.0f, 180.0f, 0.0f);
glm::vec3 sofaScale = glm::vec3(0.5f, 0.5f, 0.5f);


glm::vec3 monitorPos = glm::vec3(32.75f, 0.0f, 5.2f);    
glm::vec3 monitorRot = glm::vec3(0.0f, -180.0f, 0.0f); 
glm::vec3 monitorScale = glm::vec3(1.5f, 1.5f, 1.5f);  

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
GLTFModel *monitorGLTF = nullptr;
GLTFModel *esquinerosGLTF = nullptr;
GLTFModel *generadorGLTF = nullptr;

GLTFModel *lamparaContencionGLTF = nullptr;
GLTFModel *panelControlGLTF = nullptr;

glm::vec3 teslaPos(44.800f, -0.500f, 14.200f);
glm::vec3 teslaRot(-88.000f, 0.0f, 0.0f);
glm::vec3 teslaScale(0.150f, 0.120f, 0.090f);

glm::vec3 lamparaContencionPos(46.450f, 1.050f, 18.650f);
glm::vec3 lamparaContencionRot(-88.000f, 0.0f, 89.000f);
glm::vec3 lamparaContencionScale(0.570f, 0.720f, 0.900f);

glm::vec3 panelControlPos(48.350f, -0.500f, 17.450f);
glm::vec3 panelControlRot(-91.000f, 0.000f, -180.000f);
glm::vec3 panelControlScale(0.450f, 0.490f, 0.180f);

glm::vec3 esquinerosPos(48.133f, 0.200f, 12.601f);
glm::vec3 esquinerosRot(-0.500f, -63.000f, -1.000f);
glm::vec3 esquinerosScale(0.890f, 0.390f, 0.750f);

glm::vec3 generadorPos[3] = {glm::vec3(36.234f, -0.600f, 15.550f),
                             glm::vec3(36.266f, -0.600f, 17.254f),
                             glm::vec3(36.261f, -0.600f, 18.923f)};
glm::vec3 generadorRot[3] = {glm::vec3(-1.500f, 92.500f, -0.500f),
                             glm::vec3(0.000f, 92.500f, 1.500f),
                             glm::vec3(0.000f, 95.500f, 0.000f)};
glm::vec3 generadorScale[3] = {glm::vec3(1.0f, 1.0f, 1.0f),
                               glm::vec3(1.0f, 1.0f, 1.0f),
                               glm::vec3(1.0f, 1.0f, 1.0f)};

glm::vec3 esquineros2Pos(48.383f, 0.250f, 20.251f);
glm::vec3 esquineros2Rot(-2.500f, -144.000f, -0.500f);
glm::vec3 esquineros2Scale(1.070f, 0.410f, 0.830f);

glm::vec3 esquineros3Pos(34.583f, 0.450f, 12.851f);
glm::vec3 esquineros3Rot(-0.500f, 28.000f, -0.500f);
glm::vec3 esquineros3Scale(0.930f, 0.520f, 0.840f);

glm::vec3 esquineros4Pos(34.833f, 0.200f, 20.451f);
glm::vec3 esquineros4Rot(0.500f, 120.000f, -0.500f);
glm::vec3 esquineros4Scale(1.000f, 0.400f, 0.620f);

// paredes rojas
std::vector<WallDef> paredesList = {
    {glm::vec3(48.750000f, -0.500000f, 14.350000f),
     glm::vec3(-90.000000f, 1.000000f, -88.500000f),
     glm::vec3(1.010000f, 0.730000f, 0.410000f)},
    {glm::vec3(48.799999f, -0.500000f, 18.450001f),
     glm::vec3(-90.000000f, 1.000000f, -88.500000f),
     glm::vec3(1.040000f, 0.520000f, 0.410000f)},
    {glm::vec3(34.150002f, -0.550000f, 14.600000f),
     glm::vec3(-90.000000f, 1.000000f, 91.500000f),
     glm::vec3(1.040000f, 0.730000f, 0.420000f)},
    {glm::vec3(34.200001f, -0.500000f, 18.600000f),
     glm::vec3(-90.000000f, 1.000000f, 91.500000f),
     glm::vec3(0.970000f, 1.350000f, 0.400000f)},
    {glm::vec3(35.849998f, -0.500000f, 12.150000f),
     glm::vec3(-90.000000f, 0.000000f, 0.000000f),
     glm::vec3(0.680000f, 1.940000f, 0.400000f)},
    {glm::vec3(46.500000f, -0.500000f, 12.200000f),
     glm::vec3(-90.000000f, 0.000000f, 0.000000f),
     glm::vec3(1.010000f, 1.000000f, 0.400000f)},
    {glm::vec3(42.417999f, -0.500000f, 12.185000f),
     glm::vec3(-91.500000f, 0.000000f, 0.000000f),
     glm::vec3(1.100000f, 1.000000f, 0.400000f)},
    {glm::vec3(46.250000f, -0.500000f, 20.750000f),
     glm::vec3(-90.000000f, 0.000000f, 180.000000f),
     glm::vec3(1.120000f, 0.060000f, 0.380000f)},
    {glm::vec3(42.049999f, -0.500000f, 20.799999f),
     glm::vec3(-90.000000f, 0.000000f, 180.000000f),
     glm::vec3(0.990000f, 0.610000f, 0.390000f)},
    {glm::vec3(35.850f, -0.500000f, 20.850f),
     glm::vec3(-90.000000f, 0.000000f, -180.000f),
     glm::vec3(0.830f, 0.290f, 0.400f)}};

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
