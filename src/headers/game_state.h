#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <miniaudio.h>

inline constexpr unsigned int SCR_WIDTH = 1024;
inline constexpr unsigned int SCR_HEIGHT = 768;
inline constexpr int MAP_WIDTH = 50;
inline constexpr int MAP_HEIGHT = 50;

enum GameState { MENU, PLAYING, GAMEOVER };

struct Entity {
  glm::vec3 pos;
  int type; // 0=Log, 1=Bateria, 2=Entidad, 3=ObjetoAmbiental, 4=Mesa,
            // 5=Monitor, 6=Maquina, 7=Portal, 8=TarjetaNv1, 9=TarjetaNv2
  bool active;
  std::string text;
  float seed;
};

extern std::string currentHUDMessage;
extern float hudMessageTimer;
extern bool isReadingDocument;
extern std::string currentDocumentTitle;
extern std::string currentDocumentBody;

extern glm::vec3 mensBpos;
extern glm::vec3 mensBrot;
extern glm::vec3 mensBscale;

extern glm::vec3 girlBpos;
extern glm::vec3 girlBrot;
extern glm::vec3 girlBscale;
extern glm::vec3 mirrorBGpos;
extern glm::vec3 mirrorBGRot;
extern glm::vec3 mirrorBGScale;

extern glm::vec3 azulejoPos;
extern glm::vec3 azulejoRot;
extern glm::vec3 azulejoScale;
extern glm::vec3 mirrorPos;
extern glm::vec3 mirrorRot;
extern glm::vec3 mirrorScale;
extern glm::vec3 mirrorPos2;
extern glm::vec3 mirrorRot2;
extern glm::vec3 mirrorScale2;
extern glm::vec3 mirrorPos3;
extern glm::vec3 mirrorRot3;
extern glm::vec3 mirrorScale3;
extern glm::vec3 mirrorPos4;
extern glm::vec3 mirrorRot4;
extern glm::vec3 mirrorScale4;
extern glm::vec3 ligthbathroom2Pos;
extern glm::vec3 ligthbathroom2Rot;
extern glm::vec3 ligthbathroom2Scale;
extern bool ligthbathroomDebugVisible2;
extern glm::vec3 lamp3Pos;
extern glm::vec3 lamp3Rot;
extern glm::vec3 lamp3Scale;
extern glm::vec3 lamp4Pos;
extern glm::vec3 lamp4Rot;
extern glm::vec3 lamp4Scale;
extern glm::vec3 ligthbathroomPos;
extern glm::vec3 ligthbathroomRot;
extern glm::vec3 ligthbathroomScale;
extern bool ligthbathroomDebugVisible;
extern glm::vec3 banoPos;
extern glm::vec3 banoRot;
extern glm::vec3 banoScale;
extern glm::vec3 banoPos2;
extern glm::vec3 banoRot2;
extern glm::vec3 banoScale2;
extern glm::vec3 banoPos3;
extern glm::vec3 banoRot3;
extern glm::vec3 banoScale3;
extern glm::vec3 banoPos4;
extern glm::vec3 banoRot4;
extern glm::vec3 banoScale4;

extern glm::vec3 banoPos5;
extern glm::vec3 banoRot5;
extern glm::vec3 banoScale5;

extern glm::vec3 banoPos6;
extern glm::vec3 banoRot6;
extern glm::vec3 banoScale6;

extern glm::vec3 banoPos7;
extern glm::vec3 banoRot7;
extern glm::vec3 banoScale7;

extern glm::vec3 banoPos8;
extern glm::vec3 banoRot8;
extern glm::vec3 banoScale8;
extern glm::vec3 lavamanosPos;
extern glm::vec3 lavamanosRot;
extern glm::vec3 lavamanosScale;
extern glm::vec3 lavamanosPos2;
extern glm::vec3 lavamanosRot2;
extern glm::vec3 lavamanosScale2;
extern glm::vec3 lavamanosPos3;
extern glm::vec3 lavamanosRot3;
extern glm::vec3 lavamanosScale3;
extern glm::vec3 lavamanosPos4;
extern glm::vec3 lavamanosRot4;
extern glm::vec3 lavamanosScale4;

extern glm::vec3 lavamanosPos5;
extern glm::vec3 lavamanosRot5;
extern glm::vec3 lavamanosScale5;

extern glm::vec3 lavamanosPos6;
extern glm::vec3 lavamanosRot6;
extern glm::vec3 lavamanosScale6;

extern glm::vec3 lavamanosPos7;
extern glm::vec3 lavamanosRot7;
extern glm::vec3 lavamanosScale7;

extern glm::vec3 lavamanosPos8;
extern glm::vec3 lavamanosRot8;
extern glm::vec3 lavamanosScale8;
extern glm::vec3 urinarioPos;
extern glm::vec3 urinarioRot;
extern glm::vec3 urinarioScale;
extern glm::vec3 sillasPos;
extern glm::vec3 sillasRot;
extern glm::vec3 sillasScale;
extern glm::vec3 sofaPos;
extern glm::vec3 sofaRot;
extern glm::vec3 sofaScale;
extern glm::vec3 sillas2Pos;
extern glm::vec3 sillas2Rot;
extern glm::vec3 sillas2Scale; 
extern glm::vec3 sillas3Pos; 
extern glm::vec3 sillas3Rot; 
extern glm::vec3 sillas3Scale;
extern glm::vec3 sillas4Pos; 
extern glm::vec3 sillas4Rot; 
extern glm::vec3 sillas4Scale;
extern glm::vec3 sillas5Pos;
extern glm::vec3 sillas5Rot;
extern glm::vec3 sillas5Scale;
extern glm::vec3 monitorPos;
extern glm::vec3 monitorRot;
extern glm::vec3 monitorScale;
extern glm::vec3 monitor2Pos;
extern glm::vec3 monitor2Rot;
extern glm::vec3 monitor2Scale;
extern glm::vec3 lockerPos;
extern glm::vec3 lockerRot;
extern glm::vec3 lockerScale;
extern glm::vec3 locker2Pos;
extern glm::vec3 locker2Rot;
extern glm::vec3 locker2Scale;
extern glm::vec3 locker3Pos;
extern glm::vec3 locker3Rot;
extern glm::vec3 locker3Scale;
extern glm::vec3 locker4Pos;
extern glm::vec3 locker4Rot;
extern glm::vec3 locker4Scale;
extern glm::vec3 maquinaPos;
extern glm::vec3 maquinaRot;
extern glm::vec3 maquinaScale;
extern glm::vec3 deskPos;
extern glm::vec3 deskRot;
extern glm::vec3 deskScale;
extern glm::vec3 desk2Pos;
extern glm::vec3 desk2Rot;
extern glm::vec3 desk2Scale;
extern glm::vec3 desk3Pos;
extern glm::vec3 desk3Rot;
extern glm::vec3 desk3Scale;

extern glm::vec3 desk4Pos;
extern glm::vec3 desk4Rot;
extern glm::vec3 desk4Scale;

extern glm::vec3 desk5Pos;
extern glm::vec3 desk5Rot;
extern glm::vec3 desk5Scale;

extern glm::vec3 desk6Pos;
extern glm::vec3 desk6Rot;
extern glm::vec3 desk6Scale;

extern glm::vec3 silla1Pos;
extern glm::vec3 silla1Rot;
extern glm::vec3 silla1Scale;

extern glm::vec3 silla2Pos;
extern glm::vec3 silla2Rot;
extern glm::vec3 silla2Scale;

extern glm::vec3 silla3Pos;
extern glm::vec3 silla3Rot;
extern glm::vec3 silla3Scale;

extern glm::vec3 silla4Pos;
extern glm::vec3 silla4Rot;
extern glm::vec3 silla4Scale;

extern glm::vec3 silla5Pos;
extern glm::vec3 silla5Rot;
extern glm::vec3 silla5Scale;

extern glm::vec3 silla6Pos;
extern glm::vec3 silla6Rot;
extern glm::vec3 silla6Scale;

extern glm::vec3 estantePos;
extern glm::vec3 estanteRot;
extern glm::vec3 estanteScale;

extern glm::vec3 estante2Pos;
extern glm::vec3 estante2Rot;
extern glm::vec3 estante2Scale;

extern glm::vec3 estante3Pos;
extern glm::vec3 estante3Rot;
extern glm::vec3 estante3Scale;

extern glm::vec3 monitorsciPos;
extern glm::vec3 monitorsciRot;
extern glm::vec3 monitorsciScale;

extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;

extern bool firstMouse;
extern float yaw;
extern float pitch;
extern float lastX;
extern float lastY;
extern float deltaTime;
extern float lastFrame;
extern float headBobTimer;
extern float baseCameraY;
extern bool isMoving;
extern float stamina;
extern bool isSprinting;
extern bool isExhausted;
extern GameState gameState;
extern bool isCursorLocked;
extern bool tabKeyWasPressed;
extern bool eKeyWasPressed;
extern bool isFlashlightOn;
extern bool fKeyWasPressed;
extern ma_engine audioEngine;
extern int bateriasRecolectadas;
extern bool hasKeycardLvl1;
extern bool hasKeycardLvl2;
extern bool dimensionAlterna;
extern bool portalActivado;
extern int currentZone;
extern bool showDebugGUI;

class GLTFModel;
extern GLTFModel *banoGLTF;
extern GLTFModel *lavamanosGLTF;
extern GLTFModel *urinarioGLTF;
extern GLTFModel *mensBGLTF;
extern GLTFModel *girlBGLTF;
// area de contencion
extern GLTFModel *teslaGLTF;
extern glm::vec3 teslaPos;
extern glm::vec3 teslaRot;
extern glm::vec3 teslaScale;

extern GLTFModel *esquinerosGLTF;
extern glm::vec3 esquinerosPos;
extern glm::vec3 esquinerosRot;
extern glm::vec3 esquinerosScale;

extern glm::vec3 esquineros2Pos;
extern glm::vec3 esquineros2Rot;
extern glm::vec3 esquineros2Scale;

extern glm::vec3 esquineros3Pos;
extern glm::vec3 esquineros3Rot;
extern glm::vec3 esquineros3Scale;

extern glm::vec3 esquineros4Pos;
extern glm::vec3 esquineros4Rot;
extern glm::vec3 esquineros4Scale;


struct WallDef {
  glm::vec3 pos;
  glm::vec3 rot;
  glm::vec3 scale;
};

// Una zona rectangular del mapa con texturas propias.
// Las coords son celdas del worldMap (mismo sistema de coordenadas que el
// mundo).
struct RoomZone {
  int x1, z1, x2, z2;
  unsigned int wallTex = 0;  // 0 = usa blockType por defecto
  unsigned int floorTex = 0; // 0 = usa floorTexture por defecto
  glm::vec3 ceilColor = {0.3f, 0.3f, 0.3f};
  bool overrideWall = false;
  bool overrideFloor = false;
  bool overrideCeil = false;
};

extern GLTFModel *paredesGLTF;
extern std::vector<WallDef> paredesList;

extern std::vector<Entity> gameEntities;

extern float wallWidth;
extern float wallHeight;
extern float door1Anim;
extern bool door1Opening;
extern float door2Anim;
extern bool door2Opening;
extern int worldMap[MAP_HEIGHT][MAP_WIDTH];


