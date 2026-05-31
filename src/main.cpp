#define GLM_ENABLE_EXPERIMENTAL
// GLAD debe incluirse SIEMPRE antes que GLFW (y antes de cualquier header OpenGL)
#include <glad/glad.h>
// GLFW_INCLUDE_NONE evita que GLFW incluya <GL/gl.h> por su cuenta en Windows
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "headers/game_state.h"
#include "headers/gameplay.h"
#include "headers/obj_mesh.h"
#include "headers/shader.h"
#include "headers/texture.h"

#include "headers/gltf_model.h"

static const char *getEntityTypeLabel(int type) {
  switch (type) {
  case 0:
    return "Log";
  case 1:
    return "Bateria";
  case 2:
    return "Entidad";
  case 3:
    return "Cable/Pista";
  case 4:
    return "Mesa";
  case 5:
    return "Monitor";
  case 6:
    return "Maquina";
  case 7:
    return "Portal";
  case 8:
    return "Tarjeta Nv1";
  case 9:
    return "Tarjeta Nv2";
  default:
    return "Desconocido";
  }
}

static bool isCollectibleEntityType(int type) {
  return type == 0 || type == 1 || type == 8 || type == 9;
}

static bool isInspectableEntityType(int type) {
  return type == 3 || type == 4 || type == 5 || type == 6 || type == 7;
}

static int findFocusedEntityIndex(float *outDistance = nullptr,
                                  float *outLookAngle = nullptr) {
  int focusedIndex = -1;
  float bestScore = -9999.0f;

  for (int i = 0; i < (int)gameEntities.size(); ++i) {
    const Entity &entity = gameEntities[i];
    if (!entity.active)
      continue;

    float distance = glm::length(entity.pos - cameraPos);
    glm::vec3 dir = glm::normalize(entity.pos - cameraPos);
    float lookAngle = glm::dot(cameraFront, dir);

    bool valid = false;
    if (isCollectibleEntityType(entity.type))
      valid = distance < 2.0f;
    else if (isInspectableEntityType(entity.type))
      valid = distance < 3.0f && lookAngle > 0.80f;
    else if (entity.type == 2)
      valid = distance < 12.0f;

    if (!valid)
      continue;

    float score = lookAngle * 10.0f - distance;
    if (score > bestScore) {
      bestScore = score;
      focusedIndex = i;
      if (outDistance)
        *outDistance = distance;
      if (outLookAngle)
        *outLookAngle = lookAngle;
    }
  }

  return focusedIndex;
}

static bool findDoorAhead(int &gridX, int &gridZ, int &blockType,
                          float distance = 1.5f) {
  glm::vec3 checkPos = cameraPos + cameraFront * distance;
  gridX = (int)round(checkPos.x);
  gridZ = (int)round(checkPos.z);
  blockType = 0;

  if (gridX < 0 || gridX >= MAP_WIDTH || gridZ < 0 || gridZ >= MAP_HEIGHT)
    return false;
  blockType = worldMap[gridZ][gridX];
  return blockType == 8 || blockType == 9 || blockType == -8 || blockType == -9;
}

static const char *getDoorDebugLabel(int blockType) {
  switch (blockType) {
  case 8:
    return "Puerta Nv1 cerrada";
  case -8:
    return "Puerta Nv1 abierta";
  case 9:
    return "Puerta Nv2 cerrada";
  case -9:
    return "Puerta Nv2 abierta";
  default:
    return "Sin puerta";
  }
}

int main() {
  std::cout << "--- PRUEBA DE ASSIMP (GNOME) ---" << std::endl;
  std::string diagnosticPath = "assets/gnome.glb";
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      diagnosticPath, aiProcess_Triangulate | aiProcess_FlipUVs |
                          aiProcess_PopulateArmatureData |
                          aiProcess_LimitBoneWeights);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    diagnosticPath = "assets/gnome.glb";
    scene = importer.ReadFile(diagnosticPath,
                              aiProcess_Triangulate | aiProcess_FlipUVs |
                                  aiProcess_PopulateArmatureData |
                                  aiProcess_LimitBoneWeights);
  }

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
  } else {
    std::cout << "EXITO: El gnomo se cargo correctamente desde "
              << diagnosticPath << std::endl;
    std::cout << "--- INFO DEL MODELO ---" << std::endl;
    std::cout << "Animaciones: " << scene->mNumAnimations << std::endl;
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
      std::cout << "  [" << i
                << "] Nombre: " << scene->mAnimations[i]->mName.C_Str()
                << " (Duracion: " << scene->mAnimations[i]->mDuration
                << ", TicksPerSecond: "
                << scene->mAnimations[i]->mTicksPerSecond << ")" << std::endl;
    }
    unsigned int totalBones = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
      totalBones += scene->mMeshes[i]->mNumBones;
    }
    std::cout << "Mallas (Meshes): " << scene->mNumMeshes
              << " | Huesos totales en mallas: " << totalBones << std::endl;
    // std::cout << "--- JERARQUIA DE NODOS ---" << std::endl;
    // printNodeHierarchy(scene->mRootNode, 0);
  }
  std::cout << "------------------------" << std::endl;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
                                        "Proyecto Confidencial..", NULL, NULL);
  if (window == NULL) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return -1;
  if (ma_engine_init(NULL, &audioEngine) != MA_SUCCESS)
    return -1;

  ma_sound bgm;
  ma_sound_init_from_file(&audioEngine, "assets/music.mp3",
                          MA_SOUND_FLAG_STREAM, NULL, NULL, &bgm);
  ma_sound_set_looping(&bgm, MA_TRUE);
  ma_sound_start(&bgm);

  // --- SETUP IMGUI ---
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  glEnable(GL_DEPTH_TEST);

  Shader mainShader("src/shaders/vertex.vert", "src/shaders/fragment.frag");
  unsigned int shaderProgram = mainShader.id();

  float vertices[] = {
      // Back face (-Z)
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f,
      0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
      1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, -0.5f,
      -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f,
      0.0f, -1.0f, 0.0f, 1.0f,
      // Front face (+Z)
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 0.0f,
      0.0f, 1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
      0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f,
      0.0f, 1.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      // Left face (-X)
      -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f,
      -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -0.5f,
      -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, -1.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      // Right face (+X)
      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f,
      0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
      0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      // Bottom face (-Y)
      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f,
      0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f,
      0.0f, 0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -0.5f, -0.5f,
      0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, -1.0f,
      0.0f, 0.0f, 1.0f,
      // Top face (+Y)
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f,
      1.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f,
      1.0f, 0.0f, 0.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
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
  GLTFModel *gnomeGLTF = new GLTFModel(gnomeModelPath);
  if (gnomeGLTF->meshes.empty()) {
    delete gnomeGLTF;
    gnomeModelPath = "assets/gnome.glb";
    gnomeGLTF = new GLTFModel(gnomeModelPath);
  }
  std::cout << "[SISTEMA] Modelo activo del gnomo: " << gnomeModelPath
            << std::endl;

  //----------------------------------------------------------------------------AGREGAR
  // LOS ARCHIVOS GLTF/OBJ
  // AQUI--------------------------------------------------------------------------------

  GLTFModel *azulejoGLTF = new GLTFModel("assets/azule.glb");
  GLTFModel *mirrorGLTF = new GLTFModel("assets/mirror.glb");
  GLTFModel *mirrorBGGLTF = new GLTFModel("assets/mirrorBG.glb");
  GLTFModel *ligthbathroomGLTF = new GLTFModel("assets/ligthbathroom.glb");
  GLTFModel *ligthbathroom2GLTF = ligthbathroomGLTF;
  mensBGLTF = new GLTFModel("assets/mensB.glb");
  girlBGLTF = new GLTFModel("assets/girlB.glb");
  banoGLTF = new GLTFModel("assets/Bano.glb");
  lavamanosGLTF = new GLTFModel("assets/lavamanos.glb");
  urinarioGLTF = new GLTFModel("assets/urinario.glb");

  // Modelos de contencion
  teslaGLTF = new GLTFModel("assets/contencion/tesla.glb");
  sarcofagoGLTF = new GLTFModel("assets/contencion/sarcofago.glb");
  cablePisoGLTF = new GLTFModel("assets/contencion/cables_piso.glb");
  cableTechoGLTF = new GLTFModel("assets/contencion/cables_techo.glb");

  esquinerosGLTF = new GLTFModel("assets/contencion/esquineros.glb", true);
  generadorGLTF = new GLTFModel("assets/contencion/generador.glb");
  lamparaContencionGLTF = new GLTFModel("assets/contencion/lampara.glb");
  lampara2GLTF = new GLTFModel("assets/contencion/lampara2.glb");
  lampara3GLTF = lampara2GLTF;
  emergencyGLTF = new GLTFModel("assets/contencion/emergency.glb");
  reactorGLTF = new GLTFModel("assets/contencion/reactor.glb");
  panelControlGLTF = new GLTFModel("assets/contencion/panel-control.glb");
  lamparaReactorGLTF = new GLTFModel("assets/contencion/lampara-reactor.glb");
  warningGLTF = new GLTFModel("assets/contencion/warning.glb");
  cajonesOFGLTF = new GLTFModel("assets/oficinas/cajonesOF.glb");
  reactorControlGLTF = new GLTFModel("assets/contencion/reactor-control.glb");

  // Registrar en modelRegistry
  modelRegistry["cajonesOF"] = cajonesOFGLTF;
  modelRegistry["sarcofago"] = sarcofagoGLTF;
  modelRegistry["tesla"] = teslaGLTF;
  modelRegistry["reactor"] = reactorGLTF;
  modelRegistry["reactorControl"] = reactorControlGLTF;
  modelRegistry["panelControl"] = panelControlGLTF;
  modelRegistry["warning"] = warningGLTF;
  modelRegistry["esquineros"] = esquinerosGLTF;
  modelRegistry["esquineros2"] = esquinerosGLTF;
  modelRegistry["esquineros3"] = esquinerosGLTF;
  modelRegistry["esquineros4"] = esquinerosGLTF;

  // Cargar propiedades desde archivo
  loadLevelProps("assets/config_posiciones.txt");
  std::cout << "[SISTEMA] Props oficinas cargados: CajonesOF(" << cajonesOFGLTF->meshes.size() << ")" << std::endl;
  std::cout << "[SISTEMA] Props baño cargados: "
            << "Lampara(" << ligthbathroomGLTF->meshes.size() << "), "
            << "Bano(" << banoGLTF->meshes.size() << "), "
            << "Lavamanos(" << lavamanosGLTF->meshes.size() << "), "
            << "Urinario(" << urinarioGLTF->meshes.size() << ")" << std::endl;
  std::cout << "[SISTEMA] Props contención cargados: "
            << "Tesla(" << teslaGLTF->meshes.size() << "), "
            << "Reactor(" << reactorGLTF->meshes.size() << "), "
            << "Esquineros(" << esquinerosGLTF->meshes.size() << ")"
            << std::endl;
  unsigned int wallTex1 = loadTexture("assets/paredesH.png");
  unsigned int wallTex2 = loadTexture("assets/paredbanosT.png");
  unsigned int wallTex3 = loadTexture("assets/wall.png");
  unsigned int wallTex4 = loadTexture("assets/paredbanosGT.png");
  unsigned int wallTex5 = loadTexture("assets/pisosBanos.png");

  // Texturas de área de contención
  unsigned int wallContencionTex =
      loadTexture("assets/contencion/paredes-contencion.png");
  unsigned int floorContencionTex =
      loadTexture("assets/contencion/piso-contencion.png");
  unsigned int roofContencionTex =
      loadTexture("assets/contencion/techo-contencion.png");

  // Textura de metal generada para las puertas
  unsigned int doorTex = loadTextureWithFallback("assets/puerta_metal.png", 0);

  unsigned int portalTex = loadTexture("assets/clue.png");

  unsigned int floorTexture = loadTexture("assets/pisoH.jpg");
  unsigned int logoTexture = loadTexture("assets/logo.png");
  unsigned int clueTexture = loadTexture("assets/clue.png");
  unsigned int enemyTexture = loadTexture("assets/enemy.png");

  // Texturas especÃ­ficas con fallback
  unsigned int batteryTex =
      loadTextureWithFallback("assets/battery.png", clueTexture);
  unsigned int keycardTex =
      loadTextureWithFallback("assets/keycard.png", clueTexture);
  unsigned int pcTex = loadTextureWithFallback("assets/pc.png", wallTex2);

  //------------------------------------------------------------------------------------------------------------------------------------------------------------

  float quadVertices[] = {-0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                          -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                          0.5f,  -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,

                          -0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                          0.5f,  -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                          0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
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
  int texture1Loc = glGetUniformLocation(shaderProgram, "texture1");
  int isAnimatedLoc = glGetUniformLocation(shaderProgram, "isAnimated");
  int finalBonesLoc =
      glGetUniformLocation(shaderProgram, "finalBonesMatrices[0]");

  int numPointLightsLoc = glGetUniformLocation(shaderProgram, "numPointLights");
  int pointLightPosLoc[12];
  int pointLightColLoc[12];
  int pointLightRadLoc[12];
  for (int i = 0; i < 12; i++) {
    std::string posStr = "pointLights[" + std::to_string(i) + "]";
    posStr += ".position";
    std::string colStr = "pointLights[" + std::to_string(i) + "]";
    colStr += ".color";
    std::string radStr = "pointLights[" + std::to_string(i) + "]";
    radStr += ".radius";
    pointLightPosLoc[i] = glGetUniformLocation(shaderProgram, posStr.c_str());
    pointLightColLoc[i] = glGetUniformLocation(shaderProgram, colStr.c_str());
    pointLightRadLoc[i] = glGetUniformLocation(shaderProgram, radStr.c_str());
  }

  int numSpotLightsLoc = glGetUniformLocation(shaderProgram, "numSpotLights");
  int spotLightPosLoc[4];
  int spotLightDirLoc[4];
  int spotLightColLoc[4];
  int spotLightCutOffLoc[4];
  int spotLightOuterCutOffLoc[4];
  int spotLightRadLoc[4];
  for (int i = 0; i < 4; i++) {
    std::string base = "spotLights[" + std::to_string(i) + "]";
    spotLightPosLoc[i] = glGetUniformLocation(shaderProgram, (base + ".position").c_str());
    spotLightDirLoc[i] = glGetUniformLocation(shaderProgram, (base + ".direction").c_str());
    spotLightColLoc[i] = glGetUniformLocation(shaderProgram, (base + ".color").c_str());
    spotLightCutOffLoc[i] = glGetUniformLocation(shaderProgram, (base + ".cutOff").c_str());
    spotLightOuterCutOffLoc[i] = glGetUniformLocation(shaderProgram, (base + ".outerCutOff").c_str());
    spotLightRadLoc[i] = glGetUniformLocation(shaderProgram, (base + ".radius").c_str());
  }

  int emissiveStrengthLoc =
      glGetUniformLocation(shaderProgram, "emissiveStrength");

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
  const bool gnomeHasSkinningBones =
      gnomeGLTF && gnomeGLTF->CountBonesInMeshes() > 0;
  const int gnomeAnimationCount =
      gnomeGLTF ? gnomeGLTF->GetAnimationCount() : 0;
  int gnomeIdleAnimIndex =
      gnomeGLTF ? gnomeGLTF->FindAnimationIndexContains("idle") : -1;
  if (gnomeIdleAnimIndex < 0) {
    gnomeIdleAnimIndex = 0;
  }
  int gnomeStunAnimIndex =
      gnomeGLTF ? gnomeGLTF->FindAnimationIndexContains("stun") : -1;
  if (gnomeStunAnimIndex < 0) {
    gnomeStunAnimIndex = gnomeIdleAnimIndex;
  }
  int gnomeMoveAnimIndex =
      gnomeGLTF ? gnomeGLTF->FindAnimationIndexContains("move") : -1;
  if (gnomeMoveAnimIndex < 0) {
    gnomeMoveAnimIndex =
        gnomeGLTF ? gnomeGLTF->FindAnimationIndexContains("walk") : -1;
  }
  if (gnomeMoveAnimIndex < 0) {
    gnomeMoveAnimIndex =
        gnomeGLTF ? gnomeGLTF->FindAnimationIndexContains("run") : -1;
  }
  if (gnomeMoveAnimIndex < 0) {
    gnomeMoveAnimIndex = gnomeIdleAnimIndex;
  }

  glm::vec3 debugSpawnPos = cameraPos;
  float debugSpawnYaw = yaw;
  float debugSpawnPitch = pitch;
  bool showInteractionDebugger = true;
  bool showAnimationTester = true;
  bool showSpawnInspector = true;

  // --- ZONAS DE HABITACION ---
  // Cada zona define un rectángulo del worldMap con texturas propias para
  // paredes, piso y techo. Las coords son celdas del grid (igual que el
  // worldMap). Tip: camina al cuarto en el juego y lee cameraPos.x /
  // cameraPos.z en el editor para encontrar las coordenadas exactas del área
  // que quieres cubrir.
  //
  // (Las texturas wallTex4 y wallTex5 ahora se cargan arriba con loadTexture)

  struct RoomZone {
    int x1, z1, x2, z2;
    unsigned int wallTex;
    unsigned int floorTex;
    unsigned int ceilTex;
    glm::vec3 ceilColor;
    bool overrideWall;
    bool overrideFloor;
    bool overrideCeil;
  };

  //  { x1, z1, x2, z2,  wallTex,  floorTex, ceilColor, overrideWall,
  //  overrideFloor, overrideCeil }
  std::vector<RoomZone> roomZones = {
      {33,
       0,
       37,
       8,
       wallTex2,
       wallTex5,
       floorTexture,
       {0.15f, 0.15f, 0.2f},
       true,
       true,
       false}, // Baños (cambia a tu textura de azulejo)
      {38,
       0,
       41,
       8,
       wallTex4,
       wallTex5,
       floorTexture,
       {0.15f, 0.15f, 0.2f},
       true,
       true,
       false}, // banos de girls
       
      {34, 12, 49, 21, wallContencionTex, floorContencionTex, roofContencionTex, {1.0f, 1.0f, 1.0f}, false, true, true}, // Area contencion

      {}, // pisos
  };

  auto getZone = [&](int gx, int gz) -> const RoomZone * {
    for (const auto &rz : roomZones)
      if (gx >= rz.x1 && gx <= rz.x2 && gz >= rz.z1 && gz <= rz.z2)
        return &rz;
    return nullptr;
  };


    // --- STATIC MAP BATCHING ---
    struct MapBatch {
        unsigned int textureID;
        glm::vec3 baseColor;
        std::vector<float> vertices;
        unsigned int VAO, VBO;
    };
    std::vector<MapBatch> mapBatches;

    auto getBatch = [&](unsigned int tex, glm::vec3 color) -> MapBatch& {
        for (auto& b : mapBatches) {
            if (b.textureID == tex && abs(b.baseColor.x - color.x) < 0.01f && abs(b.baseColor.y - color.y) < 0.01f && abs(b.baseColor.z - color.z) < 0.01f) {
                return b;
            }
        }
        mapBatches.push_back({tex, color, {}, 0, 0});
        return mapBatches.back();
    };

    auto addCubeToBatch = [&](MapBatch& batch, glm::mat4 model) {
        for (int i = 0; i < 36; i++) {
            glm::vec4 pos(vertices[i*8+0], vertices[i*8+1], vertices[i*8+2], 1.0f);
            pos = model * pos;
            
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            glm::vec3 normal(vertices[i*8+3], vertices[i*8+4], vertices[i*8+5]);
            normal = glm::normalize(normalMatrix * normal);

            batch.vertices.push_back(pos.x);
            batch.vertices.push_back(pos.y);
            batch.vertices.push_back(pos.z);
            batch.vertices.push_back(normal.x);
            batch.vertices.push_back(normal.y);
            batch.vertices.push_back(normal.z);
            batch.vertices.push_back(vertices[i*8+6]); // u
            batch.vertices.push_back(vertices[i*8+7]); // v
        }
    };

    auto buildStaticMapBatches = [&]() {
        for (int z = 0; z < MAP_HEIGHT; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                int blockType = worldMap[z][x];
                const RoomZone *zone = getZone(x, z);
                
                // Paredes
                if (blockType > 0 && blockType < 8) {
                    float scaleX = wallWidth;
                    float scaleZ = wallWidth;
                    bool hasLeft = (x > 0 && worldMap[z][x - 1] > 0);
                    bool hasRight = (x < MAP_WIDTH - 1 && worldMap[z][x + 1] > 0);
                    bool hasUp = (z > 0 && worldMap[z - 1][x] > 0);
                    bool hasDown = (z < MAP_HEIGHT - 1 && worldMap[z + 1][x] > 0);
                    if (hasLeft || hasRight) scaleX = 1.0f;
                    if (hasUp || hasDown) scaleZ = 1.0f;

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f, (float)z));
                    model = glm::scale(model, glm::vec3(scaleX, wallHeight, scaleZ));

                    unsigned int tex = wallTex1;
                    if (blockType == 1) tex = wallTex1;
                    else if (blockType == 2) tex = wallTex2;
                    else if (blockType == 3) tex = wallTex3;
                    else if (blockType == 4) tex = wallContencionTex;
                    
                    if (zone && zone->overrideWall) tex = zone->wallTex;
                    
                    addCubeToBatch(getBatch(tex, glm::vec3(1.0f, 1.0f, 1.0f)), model);
                }

                // Piso y Techo
                
                // Piso
                unsigned int fTex = floorTexture;
                if (zone && zone->overrideFloor) fTex = zone->floorTex;

                glm::vec3 fCol = glm::vec3(0.5f, 0.5f, 0.5f);
                glm::mat4 floorModel = glm::mat4(1.0f);
                floorModel = glm::translate(floorModel, glm::vec3((float)x, -1.0f, (float)z));
                addCubeToBatch(getBatch(fTex, fCol), floorModel);

                // Techo
                unsigned int cTex = floorTexture;
                glm::vec3 cCol = glm::vec3(0.3f, 0.3f, 0.3f);
                if (zone && zone->overrideCeil) {
                    cCol = zone->ceilColor;
                    cTex = zone->ceilTex;
                }

                glm::mat4 roofModel = glm::mat4(1.0f);
                roofModel = glm::translate(roofModel, glm::vec3((float)x, wallHeight, (float)z));
                addCubeToBatch(getBatch(cTex, cCol), roofModel);
            }
        }

        // Bind batches
        for (auto& b : mapBatches) {
            glGenVertexArrays(1, &b.VAO);
            glGenBuffers(1, &b.VBO);
            glBindVertexArray(b.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, b.VBO);
            glBufferData(GL_ARRAY_BUFFER, b.vertices.size() * sizeof(float), b.vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        glBindVertexArray(0);
    };
    buildStaticMapBatches();
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
    glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f);  // Default obj color para otros VAOs
    glUniform1f(emissiveStrengthLoc, 0.0f); // Por defecto nada emite luz propia

    //-----------------------------------------------------------CONTROLAR LUCES
    // DE
    // BANO----------------------------------------------------------------------------
    int currentWidth, currentHeight;
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
    if (currentHeight == 0)
      currentHeight = 1;

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
      if (pitch < -89.0f)
        pitch = -89.0f;
      if (cameraPos.y < -0.5f)
        cameraPos.y = -0.5f;
      glm::vec3 front;
      front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      front.y = sin(glm::radians(pitch));
      front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
      cameraFront = glm::normalize(front);
    }
    //------------------------------------------------------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------BACK FACE
    // CULLING----------------------------------------------------------------------------

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(VAO);
    //------------------------------------------------------------------------------------------------------------------------------------------------------------

    glm::mat4 projection = glm::perspective(
        glm::radians(55.0f), (float)currentWidth / (float)currentHeight, 0.1f,
        100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glUniform3fv(lightPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(cameraFront));
    glUniform1f(cutOffLoc, glm::cos(glm::radians(15.5f)));
    glUniform1f(outerCutOffLoc, glm::cos(glm::radians(22.5f)));
    glUniform1i(flashlightOnLoc, (isFlashlightOn && selectedHotbarSlot == 0) ? 1 : 0);

    glUniform1i(dimAlternaLoc, dimensionAlterna ? 1 : 0);
    glUniform1i(zoneLoc, currentZone);
    glUniform1f(timeLoc, currentFrame);
    glUniform2f(resLoc, (float)currentWidth, (float)currentHeight);
    // espacio donde se le da las luces a las lamparas antes de poner su figura
    // .gltf o .obj

    // Efecto de parpadeo (flicker) para las 4 lámparas (desfasadas e
    // independientes)
    auto getFlicker = [](float t, float offset) -> float {
      float val = t + offset - 1.0f;
      float n1 = sin(val * 14.0f) * cos(val * 8.0f);
      float n2 = sin(val * 20.0f);
      if (n1 > 0.65f)
        return 0.15f; // Caída repentina de tensión
      if (n2 > 0.88f)
        return 0.0f; // Apagón total momentáneo
      if (n2 < -0.92f)
        return 0.40f;                        // Parpadeo tenue
      return 0.9f + 0.1f * sin(val * 55.0f); // Vibración/hum sutil constante
    };

    float flicker1 = getFlicker(currentFrame, 0.0f);
    float flicker2 = getFlicker(currentFrame, 12.4f);
    float flicker3 = getFlicker(currentFrame, 28.7f);
    float flicker4 = getFlicker(currentFrame, 45.2f);

    float lampFlicker = 0.8f + 0.2f * sin(currentFrame * 10.0f);
    float emergencyPulse = (sin(currentFrame * 5.0f) * 0.5f) + 0.5f;


    glUniform1i(numPointLightsLoc, 8);

    // color de la lampara y posicion para ponerla en un color amarillento poner
    // colores mas altos en vez de 0.8 0.9 1.0 a unos 0.8 0.6 0.2 para que se
    // vea mas amarillento
    glUniform3fv(pointLightPosLoc[0], 1, glm::value_ptr(ligthbathroomPos));
    glUniform3f(pointLightColLoc[0], 0.8f * flicker1, 0.6f * flicker1,
                0.2f * flicker1);
    glUniform1f(pointLightRadLoc[0], 4.0f);

    // Lámpara 2
    glUniform3fv(pointLightPosLoc[1], 1, glm::value_ptr(ligthbathroom2Pos));
    glUniform3f(pointLightColLoc[1], 0.8f * flicker2, 0.6f * flicker2,
                0.2f * flicker2);
    glUniform1f(pointLightRadLoc[1], 4.0f);

    // Lámpara 3 (baño)
    glUniform3fv(pointLightPosLoc[2], 1, glm::value_ptr(lamp3Pos));
    glUniform3f(pointLightColLoc[2], 0.8f * flicker3, 0.6f * flicker3,
                0.2f * flicker3);
    glUniform1f(pointLightRadLoc[2], 4.0f);

    // Lámpara 4 (baño)
    glUniform3fv(pointLightPosLoc[3], 1, glm::value_ptr(lamp4Pos));
    glUniform3f(pointLightColLoc[3], 0.8f * flicker4, 0.6f * flicker4,
                0.2f * flicker4);
    glUniform1f(pointLightRadLoc[3], 4.0f);

    // lampara contencion 1
    glUniform3fv(pointLightPosLoc[4], 1, glm::value_ptr(lamparaContencionPos));
    glUniform3f(pointLightColLoc[4], 0.8f * lampFlicker, 0.9f * lampFlicker,
                1.0f * lampFlicker);
    glUniform1f(pointLightRadLoc[4], 4.0f);

    // lampara contencion 2 (tenue)
    glUniform3fv(pointLightPosLoc[5], 1, glm::value_ptr(lampara2Pos));
    glUniform3f(pointLightColLoc[5], 0.4f * lampFlicker, 0.45f * lampFlicker,
                0.5f * lampFlicker);
    glUniform1f(pointLightRadLoc[5], 3.0f);

    // lampara contencion 3 (tenue)
    glUniform3fv(pointLightPosLoc[6], 1, glm::value_ptr(lampara3Pos));
    glUniform3f(pointLightColLoc[6], 0.4f * lampFlicker, 0.45f * lampFlicker,
                0.5f * lampFlicker);
    glUniform1f(pointLightRadLoc[6], 3.0f);

    // --- LUCES DE EMERGENCIA DINAMICAS ---
    // Empezamos en el indice 7 (despues de las fijas)
    int currentLightIdx = 7;
    for (size_t i = 0; i < emergencyPos.size() && currentLightIdx < 11;
         i++) { // Dejamos el 11 para la Tesla
      glUniform3fv(pointLightPosLoc[currentLightIdx], 1,
                   glm::value_ptr(emergencyPos[i]));
      glUniform3f(pointLightColLoc[currentLightIdx], 1.0f * emergencyPulse,
                  0.0f, 0.0f);
      glUniform1f(pointLightRadLoc[currentLightIdx],
                  2.0f); // Radio corto para evitar traspasar paredes
      currentLightIdx++;
    }

    // --- LUZ DE LA TESLA (Indice 11) ---
    float teslaPulse = 0.5f + 0.5f * sin(currentFrame * 20.0f);
    glUniform3fv(pointLightPosLoc[11], 1, glm::value_ptr(teslaPos));
    glUniform3f(pointLightColLoc[11], 0.2f * teslaPulse, 0.4f * teslaPulse, 1.0f * teslaPulse);
    glUniform1f(pointLightRadLoc[11], 5.0f);

    // Informar al shader cuantas luces reales estamos usando
    // Usamos 12 si la tesla esta activa, o el currentLightIdx si no
    glUniform1i(numPointLightsLoc, 12);

    for (int i = currentLightIdx; i < 11; i++) {
      glUniform3f(pointLightColLoc[i], 0.0f, 0.0f, 0.0f);
      glUniform1f(pointLightRadLoc[i], 0.0f);
    }

    // --- SPOTLIGHTS PARA LÁMPARAS DE REACTOR ---
    glUniform1i(numSpotLightsLoc, 4);
    glm::vec3 lp[4] = { lamparaReactorPos, lamparaReactorPos2, lamparaReactorPos3, lamparaReactorPos4 };
    glm::vec3 lr[4] = { lamparaReactorRot, lamparaReactorRot2, lamparaReactorRot3, lamparaReactorRot4 };
    for (int i = 0; i < 4; i++) {
      glUniform3fv(spotLightPosLoc[i], 1, glm::value_ptr(lp[i]));
      
      glm::mat4 rotMat = glm::mat4(1.0f);
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
      glm::vec3 dir = glm::normalize(glm::vec3(rotMat * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
      
      glUniform3fv(spotLightDirLoc[i], 1, glm::value_ptr(dir));
      glUniform3f(spotLightColLoc[i], 1.0f, 0.9f, 0.6f); // Warm yellow
      glUniform1f(spotLightCutOffLoc[i], glm::cos(glm::radians(25.0f)));
      glUniform1f(spotLightOuterCutOffLoc[i], glm::cos(glm::radians(35.0f)));
      glUniform1f(spotLightRadLoc[i], 20.0f);
    }

    // --- CULLING HELPER ---
    glm::vec2 camDir2D = glm::length(glm::vec2(cameraFront.x, cameraFront.z)) > 0.001f ? glm::normalize(glm::vec2(cameraFront.x, cameraFront.z)) : glm::vec2(1,0);
    auto shouldRender = [&](float x, float z, float radius = 2.0f) -> bool {
        glm::vec2 objPos(x, z);
        glm::vec2 dir = objPos - glm::vec2(cameraPos.x, cameraPos.z);
        float dist = glm::length(dir);
        constexpr float kPropRenderDistance = 24.0f;
        if (dist > kPropRenderDistance + radius) return false;
        if (dist > radius + 3.0f) {
            dir /= dist;
            if (glm::dot(camDir2D, dir) < -0.4f) return false;
        }
        return true;
    };

    // --- DIBUJAR MAPA BATCHEADO (Optimizado) ---
    for (auto& b : mapBatches) {
        glBindVertexArray(b.VAO);
        glBindTexture(GL_TEXTURE_2D, b.textureID);
        if (dimensionAlterna)
            glUniform3f(colorLoc, 0.4f, 0.1f, 0.1f);
        else
            glUniform3f(colorLoc, b.baseColor.x, b.baseColor.y, b.baseColor.z);
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)b.vertices.size() / 8);
    }

    // --- MAPA (Solo objetos dinamicos/puertas) ---
    for (int z = 0; z < MAP_HEIGHT; z++) {
      for (int x = 0; x < MAP_WIDTH; x++) {
        if (!shouldRender((float)x, (float)z, 1.5f)) continue;
        int blockType = worldMap[z][x];
        const RoomZone *zone = getZone(x, z);

        // Solo procesamos puertas en el bucle dinámico
        if (blockType != 8 && blockType != 9 && blockType != -8 && blockType != -9) continue;

        // Consideramos la puerta visible tanto si esta cerrada (>0) como
        // abierta (<0)
        int renderBlock = worldMap[z][x];
        if (renderBlock != 0 &&
            (blockType > 0 || renderBlock == -8 || renderBlock == -9)) {
          bool is3DDoor = (renderBlock == 8 || renderBlock == 9 ||
                           renderBlock == -8 || renderBlock == -9);

          // Detectar si esta celda es la primera o segunda de un par de puertas
          bool isSecondDoorCell = false;
          if (is3DDoor && x > 0) {
            int prevBlock = worldMap[z][x - 1];
            if (prevBlock == renderBlock)
              isSecondDoorCell = true;
          }

          if (is3DDoor && isSecondDoorCell) {
            // Skip: la segunda celda de la puerta, ya se dibuja desde la
            // primera
          } else if (is3DDoor && !isSecondDoorCell) {
            glm::mat4 baseModel = glm::mat4(1.0f);

            // 4. Mover al centro del hueco y anclar al piso de la pared (-0.5)
            baseModel = glm::translate(
                baseModel, glm::vec3((float)x + 0.5f, -0.5f, (float)z));

            // 3. Escalar para encajar en el juego.
            // Ancho de ensamble: 1.81 -> Juego: 2.0 (Escala 1.1)
            // Alto Blender: 1.535 -> Juego: 1.0 (Escala 0.651)
            baseModel = glm::scale(baseModel, glm::vec3(1.1f, 0.651f, 1.1f));

            // 2. Rotar (dejaremos 0 grados asumiendo que los nuevos estan de
            // frente) Si se ven las texturas por detras, cambiaremos este valor
            // a 180 despues. baseModel = glm::rotate(baseModel,
            // glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            // 1. Compensar el offset original de Blender para centrar el
            // ensamble en (0,0,0) Centro X = 0.455, Centro Z = 0.059
            baseModel =
                glm::translate(baseModel, glm::vec3(-0.455f, 0.0f, -0.059f));

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
            float currentAnim =
                (renderBlock == 8 || renderBlock == -8) ? door1Anim : door2Anim;

            // Rotacion: La izquierda gira hacia adelante (negativo), la derecha
            // gira hacia el otro lado (positivo)
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
          }
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
          std::cout << "[SISTEMA] No se encontró assets/Gnome_Albedo.png. El "
                       "gnomo usará sus colores por defecto."
                    << std::endl;
        }
      }

      if (isGnomeActive) {
        float distToPlayer = glm::length(cameraPos - gnomePos);
        glm::vec3 dirToGnome = glm::normalize(gnomePos - cameraPos);

        // 1. DETECTAR SI LA LINTERNA LO APUNTA DIRECTAMENTE
        bool beingLookedAt = false;
        if (isFlashlightOn && selectedHotbarSlot == 0) {
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
            isGnomeActive =
                false; // El gnomo se asusta y desaparece (o se detiene)
            std::cout << "[SISTEMA]: Gnomo ahuyentado por la luz   ."
                      << std::endl;
          }
        } else if (gameState == PLAYING) {
          gnomeStunTimer =
              (std::max)(0.0f,
                         gnomeStunTimer -
                             deltaTime); // El timer baja si dejas de mirarlo
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

        bool hasSkinningBones = gnomeHasSkinningBones;

        glm::mat4 gnomeModel = glm::mat4(1.0f);
        gnomeModel = glm::translate(gnomeModel, gnomePos);

        // Rotar en el eje Y para mirar al jugador (se aplica DESPUÉS de
        // levantarse en espacio global)--
        float angle = atan2(cameraPos.x - gnomePos.x, cameraPos.z - gnomePos.z);
        gnomeModel =
            glm::rotate(gnomeModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        if (!hasSkinningBones) {
          // Corrección legacy para modelos exportados sin skinning
          gnomeModel = glm::rotate(gnomeModel, glm::radians(90.0f),
                                   glm::vec3(1.0f, 0.0f, 0.0f));
          gnomeModel =
              glm::scale(gnomeModel, glm::vec3(0.006f, 0.006f, 0.006f));
        } else {
          // GLB actual: escala de depuración más visible y sin giro extra
          gnomeModel = glm::scale(gnomeModel, glm::vec3(0.07f, 0.07f, 0.07f));
        }

        // Aplicar textura forzada
        glUniform1i(solidColorLoc, gnomeTexture == 0 ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gnomeTexture);
        if (texture1Loc >= 0) {
          glUniform1i(texture1Loc, 0);
        }

        // Determinar animación a reproducir
        int animCount = gnomeAnimationCount;
        int idleAnimIndex = gnomeIdleAnimIndex;
        int stunAnimIndex = gnomeStunAnimIndex;
        int moveAnimIndex = gnomeMoveAnimIndex;

        int currentAnimIndex = idleAnimIndex;
        if (gnomeForceAnimation && animCount > 0) {
          if (gnomeDebugAnimIndex < 0)
            gnomeDebugAnimIndex = 0;
          if (gnomeDebugAnimIndex >= animCount)
            gnomeDebugAnimIndex = animCount - 1;
          currentAnimIndex = gnomeDebugAnimIndex;
        } else {
          if (beingLookedAt) {
            currentAnimIndex = stunAnimIndex;
          } else if (isMoving) {
            currentAnimIndex = moveAnimIndex;
          }
        }

        float gnomeAnimLength =
            currentAnimIndex >= 0
                ? gnomeGLTF->GetAnimationLengthSeconds(currentAnimIndex)
                : 0.0f;
        float gnomeRenderTime = currentFrame * gnomeAnimSpeed;
        if (gnomeForceAnimation) {
          if (gnomeAnimLoop && gnomeAnimLength > 0.0f) {
            gnomeRenderTime = fmod(gnomeAnimPreviewTime, gnomeAnimLength);
          } else {
            gnomeRenderTime = gnomeAnimPreviewTime;
          }
        }

        // Actualizar y enviar matrices de huesos para skinning (solo si el
        // modelo realmente trae huesos)
        if (hasSkinningBones) {
          gnomeGLTF->UpdateAnimation(gnomeRenderTime, gnomeBoneTransforms,
                                     currentAnimIndex);
          if (finalBonesLoc >= 0 && !gnomeBoneTransforms.empty()) {
            glUniformMatrix4fv(finalBonesLoc,
                               (GLsizei)gnomeBoneTransforms.size(), GL_FALSE,
                               glm::value_ptr(gnomeBoneTransforms[0]));
          }
        }
        if (isAnimatedLoc >= 0) {
          glUniform1i(isAnimatedLoc, hasSkinningBones ? 1 : 0);
        }

        if (shouldRender(gnomePos.x, gnomePos.z, 2.0f)) {
          if (hasSkinningBones) {
            // En modelos con skinning, evitar doble transformación (nodos +
            // huesos)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(gnomeModel));
            gnomeGLTF->Draw(shaderProgram, solidColorLoc);
          } else {
            // Fallback para modelos sin pesos de hueso exportados
            gnomeGLTF->DrawAnimated(gnomeRenderTime, currentAnimIndex,
                                    shaderProgram, modelLoc, -1, gnomeModel);
          }
        }
      }
    }

    // --- DECORACIÓN BAÑO (GLB estáticos) ---
    // Limpiar estado incondicionalmente antes de dibujar los props para evitar
    // heredar colores o estados
    glActiveTexture(GL_TEXTURE0);
    if (isAnimatedLoc >= 0)
      glUniform1i(isAnimatedLoc, 0);
    glUniform1i(solidColorLoc, 0);
    glUniform3f(colorLoc, 1.0f, 1.0f,
                1.0f); // MUY IMPORTANTE: Resetear color que pudo dejar el techo
                       // oscuro del mapa
    glBindVertexArray(VAO);

    if (banoGLTF && !banoGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;
      glm::vec3 positions[8] = { banoPos, banoPos2, banoPos3, banoPos4, banoPos5, banoPos6, banoPos7, banoPos8 };
      glm::vec3 rotations[8] = { banoRot, banoRot2, banoRot3, banoRot4, banoRot5, banoRot6, banoRot7, banoRot8 };
      glm::vec3 scales[8] = { banoScale, banoScale2, banoScale3, banoScale4, banoScale5, banoScale6, banoScale7, banoScale8 };
      for (int i = 0; i < 8; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }
      if (!instanceModels.empty()) banoGLTF->DrawInstanced(shaderProgram, solidColorLoc, instanceModels);
    }

    if (mensBGLTF && !mensBGLTF->meshes.empty()) {
      glm::mat4 mensBModel = glm::mat4(1.0f);
      mensBModel = glm::translate(mensBModel, mensBpos);
      mensBModel = glm::rotate(mensBModel, glm::radians(mensBrot.x),
                               glm::vec3(1.0f, 0.0f, 0.0f));
      mensBModel = glm::rotate(mensBModel, glm::radians(mensBrot.y),
                               glm::vec3(0.0f, 1.0f, 0.0f));
      mensBModel = glm::rotate(mensBModel, glm::radians(mensBrot.z),
                               glm::vec3(0.0f, 0.0f, 1.0f));
      mensBModel = glm::scale(mensBModel, mensBscale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mensBModel));
      if (shouldRender(mensBpos.x, mensBpos.z, 3.0f)) mensBGLTF->Draw(shaderProgram, solidColorLoc);
    }

    if (azulejoGLTF && !azulejoGLTF->meshes.empty()) {
      glm::mat4 azulejoModel = glm::mat4(1.0f);
      azulejoModel = glm::translate(azulejoModel, azulejoPos);
      azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
      azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
      azulejoModel = glm::rotate(azulejoModel, glm::radians(azulejoRot.z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
      azulejoModel = glm::scale(azulejoModel, azulejoScale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(azulejoModel));
      if (shouldRender(azulejoPos.x, azulejoPos.z, 3.0f)) azulejoGLTF->Draw(shaderProgram, solidColorLoc);
    }

    if (lavamanosGLTF && !lavamanosGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;
      glm::vec3 positions[8] = { lavamanosPos, lavamanosPos2, lavamanosPos3, lavamanosPos4, lavamanosPos5, lavamanosPos6, lavamanosPos7, lavamanosPos8 };
      glm::vec3 rotations[8] = { lavamanosRot, lavamanosRot2, lavamanosRot3, lavamanosRot4, lavamanosRot5, lavamanosRot6, lavamanosRot7, lavamanosRot8 };
      glm::vec3 scales[8] = { lavamanosScale, lavamanosScale2, lavamanosScale3, lavamanosScale4, lavamanosScale5, lavamanosScale6, lavamanosScale7, lavamanosScale8 };
      for (int i = 0; i < 8; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }
      if (!instanceModels.empty()) lavamanosGLTF->DrawInstanced(shaderProgram, solidColorLoc, instanceModels);
    }

    if (mirrorGLTF && !mirrorGLTF->meshes.empty()) {
      glm::mat4 mirrorModel = glm::mat4(1.0f);
      mirrorModel = glm::translate(mirrorModel, mirrorPos);
      mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.x),
                                glm::vec3(1.0f, 0.0f, 0.0f));
      mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.y),
                                glm::vec3(0.0f, 1.0f, 0.0f));
      mirrorModel = glm::rotate(mirrorModel, glm::radians(mirrorRot.z),
                                glm::vec3(0.0f, 0.0f, 1.0f));
      mirrorModel = glm::scale(mirrorModel, mirrorScale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel));
      if (shouldRender(mirrorPos.x, mirrorPos.z, 3.0f)) mirrorGLTF->Draw(shaderProgram, solidColorLoc);

      // Mirror 2
      glm::mat4 mirrorModel2 = glm::mat4(1.0f);
      mirrorModel2 = glm::translate(mirrorModel2, mirrorPos2);
      mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
      mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
      mirrorModel2 = glm::rotate(mirrorModel2, glm::radians(mirrorRot2.z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
      mirrorModel2 = glm::scale(mirrorModel2, mirrorScale2);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel2));
      if (shouldRender(mirrorPos2.x, mirrorPos2.z, 3.0f)) mirrorGLTF->Draw(shaderProgram, solidColorLoc);

      // Mirror 3
      glm::mat4 mirrorModel3 = glm::mat4(1.0f);
      mirrorModel3 = glm::translate(mirrorModel3, mirrorPos3);
      mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
      mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
      mirrorModel3 = glm::rotate(mirrorModel3, glm::radians(mirrorRot3.z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
      mirrorModel3 = glm::scale(mirrorModel3, mirrorScale3);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel3));
      if (shouldRender(mirrorPos3.x, mirrorPos3.z, 3.0f)) mirrorGLTF->Draw(shaderProgram, solidColorLoc);

      // Mirror 4
      glm::mat4 mirrorModel4 = glm::mat4(1.0f);
      mirrorModel4 = glm::translate(mirrorModel4, mirrorPos4);
      mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
      mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
      mirrorModel4 = glm::rotate(mirrorModel4, glm::radians(mirrorRot4.z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
      mirrorModel4 = glm::scale(mirrorModel4, mirrorScale4);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorModel4));
      if (shouldRender(mirrorPos4.x, mirrorPos4.z, 3.0f)) mirrorGLTF->Draw(shaderProgram, solidColorLoc);
    }

    if (girlBGLTF && !girlBGLTF->meshes.empty()) {
      glm::mat4 girlBModel = glm::mat4(1.0f);
      girlBModel = glm::translate(girlBModel, girlBpos);
      girlBModel = glm::rotate(girlBModel, glm::radians(girlBrot.x),
                               glm::vec3(1.0f, 0.0f, 0.0f));
      girlBModel = glm::rotate(girlBModel, glm::radians(girlBrot.y),
                               glm::vec3(0.0f, 1.0f, 0.0f));
      girlBModel = glm::rotate(girlBModel, glm::radians(girlBrot.z),
                               glm::vec3(0.0f, 0.0f, 1.0f));
      girlBModel = glm::scale(girlBModel, girlBscale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(girlBModel));
      if (shouldRender(girlBpos.x, girlBpos.z, 3.0f)) girlBGLTF->Draw(shaderProgram, solidColorLoc);
    }


    if (ligthbathroomGLTF && !ligthbathroomGLTF->meshes.empty()) {
      glm::mat4 ligthbathroomModel = glm::mat4(1.0f);
      ligthbathroomModel = glm::translate(ligthbathroomModel, ligthbathroomPos);
      ligthbathroomModel =
          glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      ligthbathroomModel =
          glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      ligthbathroomModel =
          glm::rotate(ligthbathroomModel, glm::radians(ligthbathroomRot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      ligthbathroomModel = glm::scale(ligthbathroomModel, ligthbathroomScale);

      // Compensar el offset original del modelo en Blender para centrarlo en su
      // pivote real
      ligthbathroomModel = glm::translate(ligthbathroomModel,
                                          glm::vec3(-0.423f, -2.7725f, 2.622f));

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(ligthbathroomModel));
      glUniform1f(emissiveStrengthLoc,
                  1.5f * flicker1); // Hacer que brille la lampara
      if (shouldRender(ligthbathroomPos.x, ligthbathroomPos.z, 3.0f)) ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

      glm::mat4 ligthbathroom3Model = glm::mat4(1.0f);
      ligthbathroom3Model = glm::translate(ligthbathroom3Model, lamp3Pos);
      ligthbathroom3Model =
          glm::rotate(ligthbathroom3Model, glm::radians(lamp3Rot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      ligthbathroom3Model =
          glm::rotate(ligthbathroom3Model, glm::radians(lamp3Rot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      ligthbathroom3Model =
          glm::rotate(ligthbathroom3Model, glm::radians(lamp3Rot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      ligthbathroom3Model = glm::scale(ligthbathroom3Model, lamp3Scale);

      // Compensar el offset original del modelo en Blender para centrarlo en su
      // pivote real
      ligthbathroom3Model = glm::translate(
          ligthbathroom3Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(ligthbathroom3Model));
      glUniform1f(emissiveStrengthLoc,
                  1.5f * flicker3); // Hacer que brille la lampara
      if (shouldRender(lamp3Pos.x, lamp3Pos.z, 3.0f)) ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

      glm::mat4 ligthbathroom4Model = glm::mat4(1.0f);
      ligthbathroom4Model = glm::translate(ligthbathroom4Model, lamp4Pos);
      ligthbathroom4Model =
          glm::rotate(ligthbathroom4Model, glm::radians(lamp4Rot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      ligthbathroom4Model =
          glm::rotate(ligthbathroom4Model, glm::radians(lamp4Rot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      ligthbathroom4Model =
          glm::rotate(ligthbathroom4Model, glm::radians(lamp4Rot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      ligthbathroom4Model = glm::scale(ligthbathroom4Model, lamp4Scale);

      // Compensar el offset original del modelo en Blender para centrarlo en su
      // pivote real
      ligthbathroom4Model = glm::translate(
          ligthbathroom4Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(ligthbathroom4Model));
      glUniform1f(emissiveStrengthLoc,
                  1.5f * flicker4); // Hacer que brille la lampara
      if (shouldRender(lamp4Pos.x, lamp4Pos.z, 3.0f)) ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear
    }

    if (ligthbathroom2GLTF && !ligthbathroom2GLTF->meshes.empty()) {
      glm::mat4 ligthbathroom2Model = glm::mat4(1.0f);
      ligthbathroom2Model =
          glm::translate(ligthbathroom2Model, ligthbathroom2Pos);
      ligthbathroom2Model =
          glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      ligthbathroom2Model =
          glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      ligthbathroom2Model =
          glm::rotate(ligthbathroom2Model, glm::radians(ligthbathroom2Rot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      ligthbathroom2Model =
          glm::scale(ligthbathroom2Model, ligthbathroom2Scale);

      // Compensar el offset original del modelo en Blender para centrarlo en su
      // pivote real
      ligthbathroom2Model = glm::translate(
          ligthbathroom2Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(ligthbathroom2Model));
      glUniform1f(emissiveStrengthLoc,
                  1.5f * flicker2); // Hacer que brille la lampara
      if (shouldRender(ligthbathroom2Pos.x, ligthbathroom2Pos.z, 3.0f)) ligthbathroom2GLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear
    }

    if (mirrorBGGLTF && !mirrorBGGLTF->meshes.empty()) {
      glm::mat4 mirrorBGModel = glm::mat4(1.0f);
      mirrorBGModel = glm::translate(mirrorBGModel, mirrorBGpos);
      mirrorBGModel = glm::rotate(mirrorBGModel, glm::radians(mirrorBGRot.x),
                                  glm::vec3(1.0f, 0.0f, 0.0f));
      mirrorBGModel = glm::rotate(mirrorBGModel, glm::radians(mirrorBGRot.y),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
      mirrorBGModel = glm::rotate(mirrorBGModel, glm::radians(mirrorBGRot.z),
                                  glm::vec3(0.0f, 0.0f, 1.0f));
      mirrorBGModel = glm::scale(mirrorBGModel, mirrorBGScale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mirrorBGModel));
      if (shouldRender(mirrorBGpos.x, mirrorBGpos.z, 3.0f)) mirrorBGGLTF->Draw(shaderProgram, solidColorLoc);
    }

    if (urinarioGLTF && !urinarioGLTF->meshes.empty()) {
      glm::mat4 urinarioModel = glm::mat4(1.0f);
      urinarioModel = glm::translate(urinarioModel, urinarioPos);
      urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.x),
                                  glm::vec3(1.0f, 0.0f, 0.0f));
      urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.y),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
      urinarioModel = glm::rotate(urinarioModel, glm::radians(urinarioRot.z),
                                  glm::vec3(0.0f, 0.0f, 1.0f));
      urinarioModel = glm::scale(urinarioModel, urinarioScale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(urinarioModel));
      if (shouldRender(urinarioPos.x, urinarioPos.z, 3.0f)) urinarioGLTF->Draw(shaderProgram, solidColorLoc);
    }

    // --- BUCLE DINAMICO DE RENDERING DE PROPS ---
    for (const auto& prop : placedProps) {
      GLTFModel* model = modelRegistry[prop.modelName];
      if (!model || model->meshes.empty()) continue;

      glm::mat4 pModel = glm::mat4(1.0f);
      pModel = glm::translate(pModel, prop.pos);
      pModel = glm::rotate(pModel, glm::radians(prop.rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
      pModel = glm::rotate(pModel, glm::radians(prop.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
      pModel = glm::rotate(pModel, glm::radians(prop.rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
      pModel = glm::scale(pModel, prop.scale);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pModel));
      if (shouldRender(prop.pos.x, prop.pos.z, 3.0f)) {
        model->Draw(shaderProgram, solidColorLoc);
      }
    }
//*------------------
    if (cablePisoGLTF && !cablePisoGLTF->meshes.empty()) {
      for (size_t i = 0; i < cablePisoPos.size(); i++) {
        glm::mat4 cpModel = glm::mat4(1.0f);
        cpModel = glm::translate(cpModel, cablePisoPos[i]);
        cpModel = glm::rotate(cpModel, glm::radians(cablePisoRot[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
        cpModel = glm::rotate(cpModel, glm::radians(cablePisoRot[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
        cpModel = glm::rotate(cpModel, glm::radians(cablePisoRot[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
        cpModel = glm::scale(cpModel, cablePisoScale[i]);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cpModel));
      if (shouldRender(cablePisoPos[i].x, cablePisoPos[i].z, 3.0f)) cablePisoGLTF->Draw(shaderProgram, solidColorLoc);
      }
    }

    if (cableTechoGLTF && !cableTechoGLTF->meshes.empty()) {
      for (size_t i = 0; i < cableTechoPos.size(); i++) {
        glm::mat4 ctModel = glm::mat4(1.0f);
        ctModel = glm::translate(ctModel, cableTechoPos[i]);
        ctModel = glm::rotate(ctModel, glm::radians(cableTechoRot[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
        ctModel = glm::rotate(ctModel, glm::radians(cableTechoRot[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
        ctModel = glm::rotate(ctModel, glm::radians(cableTechoRot[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
        ctModel = glm::scale(ctModel, cableTechoScale[i]);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ctModel));
      if (shouldRender(cableTechoPos[i].x, cableTechoPos[i].z, 3.0f)) cableTechoGLTF->Draw(shaderProgram, solidColorLoc);
      }
    }


    if (lamparaContencionGLTF && !lamparaContencionGLTF->meshes.empty()) {
      glm::mat4 lampModel = glm::mat4(1.0f);
      lampModel = glm::translate(lampModel, lamparaContencionPos);
      lampModel = glm::rotate(lampModel, glm::radians(lamparaContencionRot.x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
      lampModel = glm::rotate(lampModel, glm::radians(lamparaContencionRot.y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
      lampModel = glm::rotate(lampModel, glm::radians(lamparaContencionRot.z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
      lampModel = glm::scale(lampModel, lamparaContencionScale);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lampModel));
      glUniform1f(emissiveStrengthLoc, 1.5f * lampFlicker);
      if (shouldRender(lamparaContencionPos.x, lamparaContencionPos.z, 3.0f)) lamparaContencionGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f);
    }

    if (lampara2GLTF && !lampara2GLTF->meshes.empty()) {
      glm::mat4 lamp2Model = glm::mat4(1.0f);
      lamp2Model = glm::translate(lamp2Model, lampara2Pos);
      lamp2Model = glm::rotate(lamp2Model, glm::radians(lampara2Rot.x),
                               glm::vec3(1.0f, 0.0f, 0.0f));
      lamp2Model = glm::rotate(lamp2Model, glm::radians(lampara2Rot.y),
                               glm::vec3(0.0f, 1.0f, 0.0f));
      lamp2Model = glm::rotate(lamp2Model, glm::radians(lampara2Rot.z),
                               glm::vec3(0.0f, 0.0f, 1.0f));
      lamp2Model = glm::scale(lamp2Model, lampara2Scale);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lamp2Model));
      glUniform1f(emissiveStrengthLoc, 0.8f * lampFlicker);
      if (shouldRender(lampara2Pos.x, lampara2Pos.z, 3.0f)) lampara2GLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f);
    }

    if (lampara3GLTF && !lampara3GLTF->meshes.empty()) {
      glm::mat4 lamp3Model = glm::mat4(1.0f);
      lamp3Model = glm::translate(lamp3Model, lampara3Pos);
      lamp3Model = glm::rotate(lamp3Model, glm::radians(lampara3Rot.x),
                               glm::vec3(1.0f, 0.0f, 0.0f));
      lamp3Model = glm::rotate(lamp3Model, glm::radians(lampara3Rot.y),
                               glm::vec3(0.0f, 1.0f, 0.0f));
      lamp3Model = glm::rotate(lamp3Model, glm::radians(lampara3Rot.z),
                               glm::vec3(0.0f, 0.0f, 1.0f));
      lamp3Model = glm::scale(lamp3Model, lampara3Scale);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lamp3Model));
      glUniform1f(emissiveStrengthLoc, 0.8f * lampFlicker);
      if (shouldRender(lampara3Pos.x, lampara3Pos.z, 3.0f)) lampara3GLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f);
    }
    if (emergencyGLTF && !emergencyGLTF->meshes.empty()) {
      for (size_t i = 0; i < emergencyPos.size(); i++) {
        glm::mat4 emergModel = glm::mat4(1.0f);
        emergModel = glm::translate(emergModel, emergencyPos[i]);
        emergModel = glm::rotate(emergModel, glm::radians(emergencyRot[i].x),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
        emergModel = glm::rotate(emergModel, glm::radians(emergencyRot[i].y),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
        emergModel = glm::rotate(emergModel, glm::radians(emergencyRot[i].z),
                                 glm::vec3(0.0f, 0.0f, 1.0f));
        emergModel = glm::scale(emergModel, emergencyScale[i]);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(emergModel));
        // Usar el pulso compartido y aumentar intensidad emisiva
        glUniform1f(emissiveStrengthLoc, 4.0f * emergencyPulse);
      if (shouldRender(emergencyPos[i].x, emergencyPos[i].z, 3.0f)) emergencyGLTF->Draw(shaderProgram, solidColorLoc);
        glUniform1f(emissiveStrengthLoc, 0.0f);
      }
    }



    if (lamparaReactorGLTF && !lamparaReactorGLTF->meshes.empty()) {
      glm::mat4 lrModel = glm::mat4(1.0f);
      lrModel = glm::translate(lrModel, lamparaReactorPos);
      lrModel = glm::rotate(lrModel, glm::radians(lamparaReactorRot.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
      lrModel = glm::rotate(lrModel, glm::radians(lamparaReactorRot.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
      lrModel = glm::rotate(lrModel, glm::radians(lamparaReactorRot.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
      lrModel = glm::scale(lrModel, lamparaReactorScale);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lrModel));
      if (shouldRender(lamparaReactorPos.x, lamparaReactorPos.z, 3.0f)) lamparaReactorGLTF->Draw(shaderProgram, solidColorLoc);

      glm::mat4 lrModel2 = glm::mat4(1.0f);
      lrModel2 = glm::translate(lrModel2, lamparaReactorPos2);
      lrModel2 = glm::rotate(lrModel2, glm::radians(lamparaReactorRot2.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
      lrModel2 = glm::rotate(lrModel2, glm::radians(lamparaReactorRot2.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
      lrModel2 = glm::rotate(lrModel2, glm::radians(lamparaReactorRot2.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
      lrModel2 = glm::scale(lrModel2, lamparaReactorScale2);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lrModel2));
      if (shouldRender(lamparaReactorPos2.x, lamparaReactorPos2.z, 3.0f)) lamparaReactorGLTF->Draw(shaderProgram, solidColorLoc);

      glm::mat4 lrModel3 = glm::mat4(1.0f);
      lrModel3 = glm::translate(lrModel3, lamparaReactorPos3);
      lrModel3 = glm::rotate(lrModel3, glm::radians(lamparaReactorRot3.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
      lrModel3 = glm::rotate(lrModel3, glm::radians(lamparaReactorRot3.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
      lrModel3 = glm::rotate(lrModel3, glm::radians(lamparaReactorRot3.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
      lrModel3 = glm::scale(lrModel3, lamparaReactorScale3);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lrModel3));
      if (shouldRender(lamparaReactorPos3.x, lamparaReactorPos3.z, 3.0f)) lamparaReactorGLTF->Draw(shaderProgram, solidColorLoc);

      glm::mat4 lrModel4 = glm::mat4(1.0f);
      lrModel4 = glm::translate(lrModel4, lamparaReactorPos4);
      lrModel4 = glm::rotate(lrModel4, glm::radians(lamparaReactorRot4.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
      lrModel4 = glm::rotate(lrModel4, glm::radians(lamparaReactorRot4.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
      lrModel4 = glm::rotate(lrModel4, glm::radians(lamparaReactorRot4.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
      lrModel4 = glm::scale(lrModel4, lamparaReactorScale4);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lrModel4));
      if (shouldRender(lamparaReactorPos4.x, lamparaReactorPos4.z, 3.0f)) lamparaReactorGLTF->Draw(shaderProgram, solidColorLoc);
    }



    if (generadorGLTF && !generadorGLTF->meshes.empty()) {
      for (int i = 0; i < 3; i++) { // Dibujamos los 3 ejemplares
        glm::mat4 generadorModel = glm::mat4(1.0f);
        generadorModel = glm::translate(generadorModel, generadorPos[i]);
        generadorModel =
            glm::rotate(generadorModel, glm::radians(generadorRot[i].x),
                        glm::vec3(1.0f, 0.0f, 0.0f));
        generadorModel =
            glm::rotate(generadorModel, glm::radians(generadorRot[i].y),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        generadorModel =
            glm::rotate(generadorModel, glm::radians(generadorRot[i].z),
                        glm::vec3(0.0f, 0.0f, 1.0f));
        generadorModel = glm::scale(generadorModel, generadorScale[i]);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                           glm::value_ptr(generadorModel));
      if (shouldRender(generadorPos[i].x, generadorPos[i].z, 3.0f)) generadorGLTF->Draw(shaderProgram, solidColorLoc);
      }
    }

    glUniform1i(solidColorLoc, 0);

    // --- DIBUJAR ENTIDADES 3D ---
    glBindVertexArray(VAO);
    glEnable(GL_DEPTH_TEST);

    for (auto &entity : gameEntities) {
      if (!entity.active ||
          (entity.type != 0 && entity.type != 3 && entity.type < 4) ||
          entity.type == 8 || entity.type == 9)
        continue;
      if (entity.type > 0 && entity.type < 3)
        continue; // Solo procesar tipos 0, 3, 4, 5, 6, 7 aquÃ­

      glm::mat4 entityModel = glm::mat4(1.0f);
      float floatY = 0.0f;

      if (dimensionAlterna && entity.type != 7) {
        floatY = (sin(currentFrame * 2.0f + entity.seed) * 0.8f) + 0.2f;
      }

      entityModel = glm::translate(
          entityModel,
          glm::vec3(entity.pos.x, entity.pos.y + floatY, entity.pos.z));

            if (entity.type == 0 && cartaVertexCount > 0) { // Carta/Papel 3D
              glBindVertexArray(cartaVAO);
              glBindTexture(GL_TEXTURE_2D, clueTexture);
              glUniform1i(solidColorLoc, 1);
              entityModel = glm::translate(
                  entityModel,
                  glm::vec3(0.0f, -0.06f, 0.0f)); // Subirla para verla
              entityModel = glm::rotate(entityModel, entity.seed,
                                        glm::vec3(0.0f, 1.0f, 0.0f));
              entityModel = glm::scale(
                  entityModel, glm::vec3(1.0f, 1.0f, 1.0f)); // Agrandarla
              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
              glDrawArrays(GL_TRIANGLES, 0, cartaVertexCount);
              glUniform1i(solidColorLoc, 0);
              glBindVertexArray(VAO);
            } else if (entity.type == 3 && cablesVertexCount > 0) { // Cables 3D
              glBindVertexArray(cablesVAO);
              glBindTexture(GL_TEXTURE_2D, pcTex);
              glUniform1i(solidColorLoc, 1);

              // Ajuste de posiciÃ³n para que toque el suelo
              entityModel =
                  glm::translate(entityModel, glm::vec3(-0.99f, -0.12f, 0.0f));
              entityModel =
                  glm::scale(entityModel, glm::vec3(0.5f, 0.5f, 0.5f));

              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
              glDrawArrays(GL_TRIANGLES, 0, cablesVertexCount);
              glUniform1i(solidColorLoc, 0);
              glBindVertexArray(VAO);
            } else if (entity.type == 4) { // Mesa
              glBindVertexArray(VAO);
              glBindTexture(GL_TEXTURE_2D, wallTex1);
              entityModel =
                  glm::scale(entityModel, glm::vec3(1.2f, 0.8f, 0.8f));
              glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f);
              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
              glDrawArrays(GL_TRIANGLES, 0, 36);
            } else if (entity.type == 5 && objVertexCount > 0) { // PC 3D
              glBindVertexArray(objVAO);
              glBindTexture(GL_TEXTURE_2D, pcTex);

              glUniform1i(solidColorLoc, 1); // Ignorar la textura cargada

              entityModel = glm::translate(
                  entityModel, glm::vec3(0.0f, -0.1f, 0.0f)); // Bajar a la mesa
              entityModel =
                  glm::rotate(entityModel, glm::radians(0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f)); // EstÃ¡tico
              entityModel =
                  glm::scale(entityModel, glm::vec3(0.6f, 0.6f, 0.6f));
              if (dimensionAlterna)
                glUniform3f(colorLoc, 1.0f, 0.4f, 0.4f);
              else
                glUniform3f(colorLoc, 1.0f, 1.0f,
                            1.0f); // Usar colores 100% reales de Blender
              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
              glDrawArrays(GL_TRIANGLES, 0, objVertexCount);

              glUniform1i(solidColorLoc, 0); // Restaurar texturas normales

              glBindVertexArray(VAO);      // Restaurar VAO
            } else if (entity.type == 6) { // MÃ¡quina Lab
              glBindVertexArray(VAO);
              glBindTexture(GL_TEXTURE_2D, wallTex3);
              entityModel =
                  glm::scale(entityModel, glm::vec3(0.8f, 2.0f, 0.8f));
              glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
              glDrawArrays(GL_TRIANGLES, 0, 36);
            } else if (entity.type == 7) { // Portal
              glBindVertexArray(VAO);
              glBindTexture(GL_TEXTURE_2D, portalTex);
              entityModel =
                  glm::scale(entityModel, glm::vec3(1.5f, 1.5f, 1.5f));
              if (portalActivado) {
                entityModel = glm::rotate(entityModel, currentFrame * 2.0f,
                                          glm::vec3(1.0f, 1.0f, 1.0f));
                float pulse = 0.8f + 0.2f * sin(currentFrame * 15.0f);
                glUniform3f(colorLoc, pulse, 0.0f, 0.0f);
              } else {
                entityModel = glm::rotate(entityModel, currentFrame * 0.5f,
                                          glm::vec3(0.0f, 1.0f, 0.0f));
                glUniform3f(colorLoc, 0.2f, 0.5f, 1.0f);
              }
              glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                 glm::value_ptr(entityModel));
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
                  if (blockType <= 0)
                    continue;

                  glm::vec2 cellCenter((float)x, (float)z);
                  if (glm::length(cellCenter -
                                  glm::vec2(cameraPos.x, cameraPos.z)) >
                      collisionViewerRadius)
                    continue;

                  float scaleX = wallWidth;
                  float scaleZ = wallWidth;
                  bool hasLeft = (x > 0 && worldMap[z][x - 1] > 0);
                  bool hasRight = (x < MAP_WIDTH - 1 && worldMap[z][x + 1] > 0);
                  bool hasUp = (z > 0 && worldMap[z - 1][x] > 0);
                  bool hasDown = (z < MAP_HEIGHT - 1 && worldMap[z + 1][x] > 0);
                  if (hasLeft || hasRight)
                    scaleX = 1.0f;
                  if (hasUp || hasDown)
                    scaleZ = 1.0f;

                  glm::mat4 debugModel = glm::mat4(1.0f);
                  debugModel = glm::translate(
                      debugModel,
                      glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f,
                                (float)z));
                  debugModel = glm::scale(
                      debugModel, glm::vec3(scaleX + 0.02f, wallHeight + 0.02f,
                                            scaleZ + 0.02f));
                  glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                     glm::value_ptr(debugModel));

                  if (blockType == 8 || blockType == 9)
                    glUniform3f(colorLoc, 1.0f, 0.4f, 0.1f);
                  else
                    glUniform3f(colorLoc, 0.1f, 1.0f, 0.2f);

                  glDrawArrays(GL_TRIANGLES, 0, 36);
                }
              }
            }

            if (collisionShowProps) {
              // 1. Dibujar entidades estándar (Mesas y Máquinas) en amarillo
              for (const auto &entity : gameEntities) {
                if (!entity.active)
                  continue;
                if (entity.type != 4 && entity.type != 6)
                  continue;
                if (glm::length(glm::vec2(entity.pos.x - cameraPos.x,
                                          entity.pos.z - cameraPos.z)) >
                    collisionViewerRadius)
                  continue;

                glm::vec3 scale = (entity.type == 4)
                                      ? glm::vec3(1.6f, 1.0f, 1.2f)
                                      : glm::vec3(1.2f, 2.2f, 1.2f);
                glm::mat4 debugModel = glm::mat4(1.0f);
                debugModel = glm::translate(
                    debugModel,
                    glm::vec3(entity.pos.x, entity.pos.y + 0.2f, entity.pos.z));
                debugModel = glm::scale(debugModel, scale);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                                   glm::value_ptr(debugModel));
                glUniform3f(colorLoc, 1.0f, 0.9f, 0.2f); // Amarillo para entidades
                glDrawArrays(GL_TRIANGLES, 0, 36);
              }

              // 2. Helper lambda: dibuja las mismas cajas por submesh que usa la colision real.
              auto drawModelHitbox = [&](GLTFModel* model, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl, glm::vec3 hitboxColor = glm::vec3(0.1f, 0.75f, 1.0f)) {
                if (!model || model->meshes.empty()) return;
                if (glm::length(glm::vec2(pos.x - cameraPos.x, pos.z - cameraPos.z)) > collisionViewerRadius)
                  return;

                glm::mat4 mat = glm::mat4(1.0f);
                mat = glm::translate(mat, pos);
                mat = glm::rotate(mat, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
                mat = glm::rotate(mat, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
                mat = glm::rotate(mat, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
                mat = glm::scale(mat, scl);

                // Dibujar como OBB: trasladamos y escalamos la matriz del modelo usando el centro y tamaño local AABB!
                for (const auto& mesh : model->meshes) {
                  glm::vec3 localCenter = (mesh.localAABB.min + mesh.localAABB.max) * 0.5f;
                  glm::vec3 localSize = mesh.localAABB.max - mesh.localAABB.min;

                  glm::mat4 debugModel = glm::translate(mat, localCenter);
                  debugModel = glm::scale(debugModel, localSize);

                  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
                  glUniform3f(colorLoc, hitboxColor.r, hitboxColor.g, hitboxColor.b); // Color personalizado para props GLTF
                  glDrawArrays(GL_TRIANGLES, 0, 36);
                }
              };

              auto drawManualOBBHitbox = [&](const glm::vec3& pos, float yawDegrees, const glm::vec3& halfSize) {
                if (glm::length(glm::vec2(pos.x - cameraPos.x, pos.z - cameraPos.z)) > collisionViewerRadius)
                  return;

                glm::mat4 debugModel = glm::mat4(1.0f);
                debugModel = glm::translate(debugModel, pos);
                debugModel = glm::rotate(debugModel, glm::radians(yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
                debugModel = glm::scale(debugModel, halfSize * 2.0f);

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
                glUniform3f(colorLoc, 1.0f, 0.55f, 0.05f);
                glDrawArrays(GL_TRIANGLES, 0, 36);
              };

              // --- Props del Baño ---
              glm::vec3 banoPositions[8] = { banoPos, banoPos2, banoPos3, banoPos4, banoPos5, banoPos6, banoPos7, banoPos8 };
              glm::vec3 banoRotations[8] = { banoRot, banoRot2, banoRot3, banoRot4, banoRot5, banoRot6, banoRot7, banoRot8 };
              glm::vec3 banoScales[8] = { banoScale, banoScale2, banoScale3, banoScale4, banoScale5, banoScale6, banoScale7, banoScale8 };
              for (int i = 0; i < 8; i++) {
                drawModelHitbox(banoGLTF, banoPositions[i], banoRotations[i], banoScales[i]);
              }

              glm::vec3 lavaPositions[8] = { lavamanosPos, lavamanosPos2, lavamanosPos3, lavamanosPos4, lavamanosPos5, lavamanosPos6, lavamanosPos7, lavamanosPos8 };
              glm::vec3 lavaRotations[8] = { lavamanosRot, lavamanosRot2, lavamanosRot3, lavamanosRot4, lavamanosRot5, lavamanosRot6, lavamanosRot7, lavamanosRot8 };
              glm::vec3 lavaScales[8] = { lavamanosScale, lavamanosScale2, lavamanosScale3, lavamanosScale4, lavamanosScale5, lavamanosScale6, lavamanosScale7, lavamanosScale8 };
              for (int i = 0; i < 8; i++) {
                drawModelHitbox(lavamanosGLTF, lavaPositions[i], lavaRotations[i], lavaScales[i]);
              }

              drawModelHitbox(urinarioGLTF, urinarioPos, urinarioRot, urinarioScale);
              drawModelHitbox(mensBGLTF, mensBpos, mensBrot, mensBscale);
              drawModelHitbox(girlBGLTF, girlBpos, girlBrot, girlBscale);

              // --- Props Dinámicos (placedProps) ---
              for (const auto& prop : placedProps) {
                GLTFModel* model = modelRegistry[prop.modelName];
                if (model) {
                  glm::vec3 color = prop.collisionActive ? glm::vec3(0.1f, 0.75f, 1.0f) : glm::vec3(1.0f, 0.25f, 0.25f);
                  drawModelHitbox(model, prop.pos, prop.rot, prop.scale, color);
                }
              }

              for (int i = 0; i < 3; i++) {
                drawModelHitbox(generadorGLTF, generadorPos[i], generadorRot[i], generadorScale[i]);
              }

              for (size_t i = 0; i < emergencyPos.size(); i++) {
                drawModelHitbox(emergencyGLTF, emergencyPos[i], emergencyRot[i], emergencyScale[i]);
              }
            }

            glLineWidth(1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          }

          // --- DIBUJAR ENTIDADES 2D ---
          glBindVertexArray(quadVAO);
          glDisable(GL_CULL_FACE);

          for (auto &entity : gameEntities) {
            if (!entity.active ||
                (entity.type >= 3 && entity.type != 8 && entity.type != 9))
              continue;
            if (entity.type == 0 || entity.type == 3)
              continue; // Ya se dibujaron en 3D
            if (entity.type == 2 && !portalActivado)
              continue;

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
            if (entity.type == 8 || entity.type == 9)
              bounce = sin(currentFrame * 3.0f) * 0.1f;

            float targetY = entity.pos.y + bounce;

            entityModel = glm::translate(
                entityModel, glm::vec3(entity.pos.x, targetY, entity.pos.z));

            float anguloHaciaCamara =
                atan2(cameraPos.x - entity.pos.x, cameraPos.z - entity.pos.z);
            entityModel = glm::rotate(entityModel, anguloHaciaCamara,
                                      glm::vec3(0.0f, 1.0f, 0.0f));

            float escala = (entity.type != 2) ? 0.3f : 0.9f;

            entityModel =
                glm::scale(entityModel, glm::vec3(escala, escala, escala));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                               glm::value_ptr(entityModel));

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
          } else if (gameState == PLAYING) {
            // --- DIBUJAR CROSSHAIR (HUD) ---
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_DST_COLOR,
                        GL_ZERO); // Color invertido para que se vea siempre

            glBindVertexArray(quadVAO);
            float aspect = (float)currentWidth / (float)currentHeight;
            glm::mat4 orthoProj =
                glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(orthoProj));

            glm::mat4 orthoView = glm::mat4(1.0f);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(orthoView));

            glm::mat4 orthoModel = glm::mat4(1.0f);
            orthoModel =
                glm::scale(orthoModel, glm::vec3(0.015f, 0.015f * aspect,
                                                 1.0f)); // Puntito en el centro
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                               glm::value_ptr(orthoModel));

            glBindTexture(GL_TEXTURE_2D,
                          clueTexture); // Textura blanca genÃ©rica
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
          }

          // --- RENDER IMGUI HUD ---
          if (gameState == MENU) {
            auto startGameFromMenu = [&]() {
              gameState = PLAYING;
              isCursorLocked = true;
              glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
              firstMouse = true;
              ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
              printTypewriter("ESCENA 1: PASILLO DE ACCESO");
            };

            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            ImVec2 screenMin(0.0f, 0.0f);
            ImVec2 screenMax((float)currentWidth, (float)currentHeight);
            float t = (float)glfwGetTime();

            drawList->AddRectFilledMultiColor(
                screenMin, screenMax,
                IM_COL32(3, 7, 11, 238), IM_COL32(8, 16, 22, 238),
                IM_COL32(1, 2, 5, 248), IM_COL32(2, 4, 7, 248));
            drawList->AddCircleFilled(
                ImVec2(currentWidth * 0.5f, currentHeight * 0.05f),
                currentWidth * 0.32f, IM_COL32(170, 215, 230, 20), 96);
            drawList->AddRectFilledMultiColor(
                ImVec2(0, 0), ImVec2((float)currentWidth, currentHeight * 0.34f),
                IM_COL32(190, 220, 230, 28), IM_COL32(90, 140, 160, 10),
                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
            for (int i = 0; i < 42; ++i) {
              float seed = (float)i * 37.13f;
              float x = fmodf(seed * 19.7f, (float)currentWidth);
              float y = fmodf(seed * 11.3f + t * (6.0f + (i % 5)), (float)currentHeight);
              float a = 20.0f + (float)(i % 4) * 12.0f;
              drawList->AddCircleFilled(ImVec2(x, y), 1.0f + (i % 3) * 0.35f,
                                        IM_COL32(210, 230, 235, (int)a), 8);
            }

            ImGui::SetNextWindowPos(screenMin);
            ImGui::SetNextWindowSize(screenMax);
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin("MainMenu", NULL,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings);

            float titleY = currentHeight * 0.16f;
            const char* title = "PROYECTO CONFIDENCIAL";
            const char* subtitle = "LABORATORIO DE CONTENCION";
            ImGui::SetWindowFontScale(3.0f);
            ImVec2 titleSize = ImGui::CalcTextSize(title);
            ImGui::SetCursorPos(ImVec2((currentWidth - titleSize.x) * 0.5f, titleY));
            ImGui::TextColored(ImVec4(0.88f, 0.97f, 1.0f, 1.0f), "%s", title);

            ImGui::SetWindowFontScale(1.0f);
            ImVec2 titleCenter(currentWidth * 0.5f, titleY + titleSize.y + 10.0f);
            drawList->AddLine(ImVec2(titleCenter.x - 230.0f, titleCenter.y),
                              ImVec2(titleCenter.x - 45.0f, titleCenter.y),
                              IM_COL32(220, 245, 255, 180), 1.5f);
            drawList->AddLine(ImVec2(titleCenter.x + 45.0f, titleCenter.y),
                              ImVec2(titleCenter.x + 230.0f, titleCenter.y),
                              IM_COL32(220, 245, 255, 180), 1.5f);
            drawList->AddCircle(titleCenter, 17.0f, IM_COL32(220, 245, 255, 180), 32, 1.3f);

            ImVec2 subSize = ImGui::CalcTextSize(subtitle);
            ImGui::SetCursorPos(ImVec2((currentWidth - subSize.x) * 0.5f, titleCenter.y + 24.0f));
            ImGui::TextColored(ImVec4(0.62f, 0.78f, 0.84f, 1.0f), "%s", subtitle);

            float menuWidth = 260.0f;
            float menuX = (currentWidth - menuWidth) * 0.5f;
            float menuY = currentHeight * 0.48f;
            ImGui::SetCursorPos(ImVec2(menuX, menuY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.05f, 0.07f, 0.28f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.75f, 0.82f, 0.22f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.92f, 1.0f, 0.35f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.96f, 1.0f, 1.0f));

            auto menuButton = [&](const char* label) {
              ImGui::SetCursorPosX(menuX);
              bool pressed = ImGui::Button(label, ImVec2(menuWidth, 34.0f));
              ImVec2 min = ImGui::GetItemRectMin();
              ImVec2 max = ImGui::GetItemRectMax();
              if (ImGui::IsItemHovered()) {
                drawList->AddLine(ImVec2(min.x + 34.0f, max.y + 2.0f),
                                  ImVec2(max.x - 34.0f, max.y + 2.0f),
                                  IM_COL32(220, 245, 255, 180), 1.0f);
              }
              return pressed;
            };

            if (menuButton("INICIAR JUEGO")) startGameFromMenu();
            menuButton("OPCIONES");
            menuButton("ARCHIVOS");
            menuButton("CREDITOS");
            if (menuButton("SALIR")) glfwSetWindowShouldClose(window, true);

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2);

            ImGui::SetWindowFontScale(0.85f);
            const char* hint = "ENTER / ESPACIO tambien inicia";
            ImVec2 hintSize = ImGui::CalcTextSize(hint);
            ImGui::SetCursorPos(ImVec2((currentWidth - hintSize.x) * 0.5f,
                                       currentHeight * 0.86f));
            ImGui::TextColored(ImVec4(0.5f, 0.65f, 0.7f, 0.85f), "%s", hint);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::End();
          } else if (showDebugGUI) {
            static int activeEditorPanel = 1;
            const float sideTabW = 36.0f;
            const float leftPanelX = sideTabW + 12.0f;
            const float panelTopY = 54.0f;
            const float panelGap = 8.0f;
            const float leftPanelW = 360.0f;
            const float availablePanelH = (float)currentHeight - panelTopY - 14.0f;
            const float leftStatusH = 126.0f;
            const float leftLevelY = panelTopY + leftStatusH + panelGap;
            const float leftLevelH = availablePanelH * 0.62f;
            const float leftSpawnY = leftLevelY + leftLevelH + panelGap;
            const float leftSpawnH = (glm::max)(120.0f, availablePanelH - leftStatusH - leftLevelH - panelGap * 2.0f);

            ImGui::SetNextWindowPos(ImVec2(0.0f, panelTopY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(sideTabW, 238.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.78f);
            ImGui::Begin("EditorDockTabs", NULL,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
            auto sideTab = [&](const char* label, int panel) {
              if (activeEditorPanel == panel) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.38f, 0.58f, 1.0f));
              }
              bool clicked = ImGui::Button(label, ImVec2(-1.0f, 58.0f));
              if (activeEditorPanel == panel) {
                ImGui::PopStyleColor();
              }
              if (clicked) activeEditorPanel = panel;
            };
            sideTab("I", 0);
            sideTab("M", 1);
            sideTab("S", 2);
            if (ImGui::Button("<", ImVec2(-1.0f, 34.0f))) {
              showDebugGUI = false;
            }
            ImGui::End();

            // Display coordinates top-left
            if (activeEditorPanel == 0) {
            ImGui::SetNextWindowPos(ImVec2(leftPanelX, panelTopY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(leftPanelW, 182.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.78f);
            ImGui::Begin("Coords", NULL,
                         ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                               "POSICION ACTUAL:");
            ImGui::Text("X: %.1f  |  Z: %.1f", cameraPos.x, cameraPos.z);
            ImGui::Spacing();
            ImGui::Checkbox("Ver Hitboxes (H)", &showCollisionViewer);
            ImGui::Spacing();
            if (ImGui::Button("Ocultar Editor (G)", ImVec2(-1, 0))) {
              showDebugGUI = false;
            }
            ImGui::End();
            }

            if (false) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::Begin("Editor Bano", NULL);
            ImGui::Text("Ajuste en tiempo real (sin recompilar)");
            ImGui::Separator();
            ImGui::Text("Bano");
            ImGui::DragFloat3("Bano Pos", &banoPos.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano Rot", &banoRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano Scale", &banoScale.x, 0.01f, 0.05f, 2.0f);
            ImGui::Separator();
            ImGui::Text("Lavamanos");
            ImGui::DragFloat3("Lava Pos", &lavamanosPos.x, 0.05f, 33.0f, 42.0f);
            ImGui::DragFloat3("Lava Rot", &lavamanosRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava Scale", &lavamanosScale.x, 0.01f, 0.05f,
                              2.0f);
            if (ImGui::Button("Traer lavamanos frente a camara")) {
              lavamanosPos = cameraPos + cameraFront * 0.8f;
              lavamanosPos.y = cameraPos.y;
              lavamanosRot = glm::vec3(0.0f, 0.0f, 0.0f);
              lavamanosScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lavamanos 2");
            ImGui::DragFloat3("Lava 2 Pos", &lavamanosPos2.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 2 Rot", &lavamanosRot2.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 2 Scale", &lavamanosScale2.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 3");
            ImGui::DragFloat3("Lava 3 Pos", &lavamanosPos3.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 3 Rot", &lavamanosRot3.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 3 Scale", &lavamanosScale3.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 4");
            ImGui::DragFloat3("Lava 4 Pos", &lavamanosPos4.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 4 Rot", &lavamanosRot4.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 4 Scale", &lavamanosScale4.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 5 (Mujeres 1)");
            ImGui::DragFloat3("Lava 5 Pos", &lavamanosPos5.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 5 Rot", &lavamanosRot5.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 5 Scale", &lavamanosScale5.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 6 (Mujeres 2)");
            ImGui::DragFloat3("Lava 6 Pos", &lavamanosPos6.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 6 Rot", &lavamanosRot6.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 6 Scale", &lavamanosScale6.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 7 (Mujeres 3)");
            ImGui::DragFloat3("Lava 7 Pos", &lavamanosPos7.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 7 Rot", &lavamanosRot7.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 7 Scale", &lavamanosScale7.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lavamanos 8 (Mujeres 4)");
            ImGui::DragFloat3("Lava 8 Pos", &lavamanosPos8.x, 0.05f, 33.0f,
                              42.0f);
            ImGui::DragFloat3("Lava 8 Rot", &lavamanosRot8.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lava 8 Scale", &lavamanosScale8.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();
            ImGui::Text("Urinario");
            ImGui::DragFloat3("Uri Pos", &urinarioPos.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Uri Rot", &urinarioRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Uri Scale", &urinarioScale.x, 0.01f, 0.05f,
                              2.0f); // los parametros de dragfloat3 son name,
                                     // escalas y posicion inicial
            ImGui::Separator();
            ImGui::Text("Bano 2");
            ImGui::DragFloat3("Bano 2 Pos", &banoPos2.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 2 Rot", &banoRot2.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 2 Scale", &banoScale2.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();
            ImGui::Text("Bano 3");
            ImGui::DragFloat3("Bano 3 Pos", &banoPos3.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 3 Rot", &banoRot3.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 3 Scale", &banoScale3.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();
            ImGui::Text("Bano 4");
            ImGui::DragFloat3("Bano 4 Pos", &banoPos4.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 4 Rot", &banoRot4.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 4 Scale", &banoScale4.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Bano 5 (Mujeres 1)");
            ImGui::DragFloat3("Bano 5 Pos", &banoPos5.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 5 Rot", &banoRot5.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 5 Scale", &banoScale5.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Bano 6 (Mujeres 2)");
            ImGui::DragFloat3("Bano 6 Pos", &banoPos6.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 6 Rot", &banoRot6.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 6 Scale", &banoScale6.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Bano 7 (Mujeres 3)");
            ImGui::DragFloat3("Bano 7 Pos", &banoPos7.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 7 Rot", &banoRot7.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 7 Scale", &banoScale7.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Bano 8 (Mujeres 4)");
            ImGui::DragFloat3("Bano 8 Pos", &banoPos8.x, 0.05f, 33.0f, 40.0f);
            ImGui::DragFloat3("Bano 8 Rot", &banoRot8.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Bano 8 Scale", &banoScale8.x, 0.01f, 0.05f,
                              2.0f);
            ImGui::Separator();

            ImGui::Text("Lampara bano");
            ImGui::DragFloat3("Lampara bano Pos", &ligthbathroomPos.x, 0.05f);
            ImGui::DragFloat3("Lampara bano Rot", &ligthbathroomRot.x, 0.5f,
                              -180.0f, 180.0f);
            ImGui::DragFloat3("Lampara bano Scale", &ligthbathroomScale.x,
                              0.01f, 0.05f, 2.0f);
            ImGui::Checkbox("Lampara modo debug visible",
                            &ligthbathroomDebugVisible);
            ImGui::Separator();
            ImGui::Text("mensB");
            ImGui::DragFloat3("mensB Pos", &mensBpos.x, 0.05f);
            ImGui::DragFloat3("mensB Rot", &mensBrot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("mensB Scale", &mensBscale.x, 0.01f, 0.05f, 2.0f);
            if (ImGui::Button("Traer mensB frente a camara")) {
              mensBpos = cameraPos + cameraFront * 0.8f;
              mensBpos.y = cameraPos.y;
              mensBrot = glm::vec3(0.0f, 0.0f, 0.0f);
              mensBscale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();
            ImGui::Text("girlB");
            ImGui::DragFloat3("girlB Pos", &girlBpos.x, 0.05f);
            ImGui::DragFloat3("girlB Rot", &girlBrot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("girlB Scale", &girlBscale.x, 0.01f, 0.05f, 2.0f);
            if (ImGui::Button("Traer girlB frente a camara")) {
              girlBpos = cameraPos + cameraFront * 0.8f;
              girlBpos.y = cameraPos.y;
              girlBrot = glm::vec3(0.0f, 0.0f, 0.0f);
              girlBscale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();
            ImGui::Text("mirrorBG");
            ImGui::DragFloat3("mirrorBG Pos", &mirrorBGpos.x, 0.05f);
            ImGui::DragFloat3("mirrorBG Rot", &mirrorBGRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("mirrorBG Scale", &mirrorBGScale.x, 0.01f, 0.05f,
                              2.0f);
            if (ImGui::Button("Traer mirrorBG frente a camara")) {
              mirrorBGpos = cameraPos + cameraFront * 0.8f;
              mirrorBGpos.y = cameraPos.y;
              mirrorBGRot = glm::vec3(0.0f, 0.0f, 0.0f);
              mirrorBGScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();
            ImGui::Text("Sillas");
            ImGui::DragFloat3("Sillas Pos", &sillasPos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Sillas Rot", &sillasRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sillas Scale", &sillasScale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::Text("Sillas - 2");
            ImGui::DragFloat3("Sillas 2 Pos", &sillas2Pos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Sillas 2 Rot", &sillas2Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sillas 2 Scale", &sillas2Scale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::Text("Sillas - Fila Ext. Derecha (3)");
            ImGui::DragFloat3("Sillas 3 Pos", &sillas3Pos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Sillas 3 Rot", &sillas3Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sillas 3 Scale", &sillas3Scale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::Text("Sillas - Fila Ext. Izquierda (4)");
            ImGui::DragFloat3("Sillas 4 Pos", &sillas4Pos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Sillas 4 Rot", &sillas4Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sillas 4 Scale", &sillas4Scale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::Text("Sillas - Fila Ext. Izquierda 2 (5)");
            ImGui::DragFloat3("Sillas 5 Pos", &sillas5Pos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Sillas 5 Rot", &sillas5Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sillas 5 Scale", &sillas5Scale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::Text("Sofa");
            ImGui::DragFloat3("Sofa Pos", &sofaPos.x, 0.05f, -100.0f, 100.0f);
            ImGui::DragFloat3("Sofa Rot", &sofaRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Sofa Scale", &sofaScale.x, 0.01f, 0.001f, 3.0f);
            ImGui::Separator();

            ImGui::Text("Monitor de Pared");
            ImGui::DragFloat3("Monitor Pos", &monitorPos.x, 0.05f, -100.0f,
                              100.0f);
            ImGui::DragFloat3("Monitor Rot", &monitorRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Monitor Scale", &monitorScale.x, 0.01f, 0.001f,
                              3.0f);
            ImGui::Separator();

            ImGui::End();
            }

            if (false) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::Begin("Editor Contencion", NULL);
            ImGui::Text("Tesla Model");
            ImGui::DragFloat3("Tesla Pos", &teslaPos.x, 0.05f);
            ImGui::DragFloat3("Tesla Rot", &teslaRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Tesla Scale", &teslaScale.x, 0.01f, 0.01f,
                              10.0f);
            if (ImGui::Button("Traer Tesla frente a camara")) {
              teslaPos = cameraPos + cameraFront * 2.0f;
              teslaPos.y = -0.5f;
              teslaRot = glm::vec3(0.0f, 0.0f, 0.0f);
              teslaScale = glm::vec3(0.15f, 0.15f, 0.15f);
            }
            ImGui::Separator();

            ImGui::Text("Sarcofago Model");
            ImGui::DragFloat3("Sarcofago Pos", &sarcofagoPos.x, 0.05f);
            ImGui::DragFloat3("Sarcofago Rot", &sarcofagoRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Sarcofago Scale", &sarcofagoScale.x, 0.01f,
                              0.01f, 10.0f);
            if (ImGui::Button("Traer frente a camara##sarcofago")) {
              sarcofagoPos = cameraPos + cameraFront * 2.0f;
              sarcofagoPos.y = -0.5f;
              sarcofagoRot = glm::vec3(0.0f, 0.0f, 0.0f);
              sarcofagoScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Cable Piso Models");
            if (ImGui::Button("Add Cable Piso")) {
              cablePisoPos.push_back(cameraPos + cameraFront * 2.0f);
              cablePisoRot.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
              cablePisoScale.push_back(glm::vec3(0.01918f, 0.01918f, 0.01918f));
            }
            for (size_t i = 0; i < cablePisoPos.size(); i++) {
              ImGui::PushID(static_cast<int>(i) + 5000);
              ImGui::Text("Cable Piso %zu", i);
              ImGui::DragFloat3("Pos", &cablePisoPos[i].x, 0.05f);
              ImGui::DragFloat3("Rot", &cablePisoRot[i].x, 0.5f, -180.0f,
                                180.0f);
              ImGui::DragFloat3("Scale", &cablePisoScale[i].x, 0.001f, 0.001f,
                                10.0f);
              if (ImGui::Button("Traer frente a camara")) {
                cablePisoPos[i] = cameraPos + cameraFront * 2.0f;
                cablePisoPos[i].y = -0.5f;
              }
              ImGui::SameLine();
              if (ImGui::Button("Eliminar")) {
                cablePisoPos.erase(cablePisoPos.begin() + i);
                cablePisoRot.erase(cablePisoRot.begin() + i);
                cablePisoScale.erase(cablePisoScale.begin() + i);
                ImGui::PopID();
                break;
              }
              ImGui::PopID();
            }
            ImGui::Separator();

            ImGui::Text("Cable Techo Models");
            if (ImGui::Button("Add Cable Techo")) {
              cableTechoPos.push_back(cameraPos + cameraFront * 2.0f);
              cableTechoRot.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
              cableTechoScale.push_back(
                  glm::vec3(-0.001557f, -0.001557f, -0.001557f));
            }
            for (size_t i = 0; i < cableTechoPos.size(); i++) {
              ImGui::PushID(static_cast<int>(i) + 6000);
              ImGui::Text("Cable Techo %zu", i);
              ImGui::DragFloat3("Pos", &cableTechoPos[i].x, 0.05f);
              ImGui::DragFloat3("Rot", &cableTechoRot[i].x, 0.5f, -180.0f,
                                180.0f);
              ImGui::DragFloat3("Scale", &cableTechoScale[i].x, 0.001f, 0.001f,
                                10.0f);
              if (ImGui::Button("Traer frente a camara")) {
                cableTechoPos[i] = cameraPos + cameraFront * 2.0f;
                cableTechoPos[i].y = -0.5f;
              }
              ImGui::SameLine();
              if (ImGui::Button("Eliminar")) {
                cableTechoPos.erase(cableTechoPos.begin() + i);
                cableTechoRot.erase(cableTechoRot.begin() + i);
                cableTechoScale.erase(cableTechoScale.begin() + i);
                ImGui::PopID();
                break;
              }
              ImGui::PopID();
            }
            ImGui::Separator();

            ImGui::Text("Warning Model");
            ImGui::DragFloat3("Warning Pos", &warningPos.x, 0.05f);
            ImGui::DragFloat3("Warning Rot", &warningRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Warning Scale", &warningScale.x, 0.01f, 0.01f,
                              10.0f);
            if (ImGui::Button("Traer frente a camara##warning")) {
              warningPos = cameraPos + cameraFront * 2.0f;
              warningPos.y = -0.5f;
              warningRot = glm::vec3(0.0f, 0.0f, 0.0f);
              warningScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Cajones Oficina (cajonesOF) Model");
            ImGui::DragFloat3("CajonesOF Pos", &cajonesOFPos.x, 0.05f);
            ImGui::DragFloat3("CajonesOF Rot", &cajonesOFRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("CajonesOF Scale", &cajonesOFScale.x, 0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer frente a camara##cajonesOF")) {
              cajonesOFPos = cameraPos + cameraFront * 2.0f;
              cajonesOFPos.y = -0.5f;
              cajonesOFRot = glm::vec3(0.0f, 0.0f, 0.0f);
              cajonesOFScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lampara Contencion Model");
            ImGui::DragFloat3("Lamp Pos", &lamparaContencionPos.x, 0.05f);
            ImGui::DragFloat3("Lamp Rot", &lamparaContencionRot.x, 0.5f,
                              -180.0f, 180.0f);
            ImGui::DragFloat3("Lamp Scale", &lamparaContencionScale.x, 0.01f,
                              0.01f, 10.0f);
            if (ImGui::Button("Traer frente a camara##lampara")) {
              lamparaContencionPos = cameraPos + cameraFront * 2.0f;
              lamparaContencionPos.y = 2.0f;
              lamparaContencionRot = glm::vec3(0.0f, 0.0f, 0.0f);
              lamparaContencionScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();
            ImGui::Text("Lampara 2 Model");
            ImGui::DragFloat3("Lamp2 Pos", &lampara2Pos.x, 0.05f);
            ImGui::DragFloat3("Lamp2 Rot", &lampara2Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lamp2 Scale", &lampara2Scale.x, 0.01f, 0.01f,
                              10.0f);
            if (ImGui::Button("Traer frente a camara##lampara2")) {
              lampara2Pos = cameraPos + cameraFront * 2.0f;
              lampara2Pos.y = 2.0f;
              lampara2Rot = glm::vec3(0.0f, 0.0f, 0.0f);
              lampara2Scale = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();
            ImGui::Text("Lampara 3 Model");
            ImGui::DragFloat3("Lamp3 Pos", &lampara3Pos.x, 0.05f);
            ImGui::DragFloat3("Lamp3 Rot", &lampara3Rot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Lamp3 Scale", &lampara3Scale.x, 0.01f, 0.01f,
                              10.0f);
            if (ImGui::Button("Traer frente a camara##lampara3")) {
              lampara3Pos = cameraPos + cameraFront * 2.0f;
              lampara3Pos.y = 2.0f;
              lampara3Rot = glm::vec3(0.0f, 0.0f, 0.0f);
              lampara3Scale = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();
            ImGui::Text("Emergency Lights");
            if (ImGui::Button("Agregar Luz de Emergencia")) {
              emergencyPos.push_back(cameraPos + cameraFront * 2.0f);
              emergencyRot.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
              emergencyScale.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
            }
            for (size_t i = 0; i < emergencyPos.size(); i++) {
              ImGui::PushID(i);
              ImGui::Text("Luz %d", (int)i);
              ImGui::DragFloat3("Pos", &emergencyPos[i].x, 0.05f);
              ImGui::DragFloat3("Rot", &emergencyRot[i].x, 0.5f, -180.0f,
                                180.0f);
              ImGui::DragFloat3("Scale", &emergencyScale[i].x, 0.01f, 0.01f,
                                10.0f);
              if (ImGui::Button("Traer frente a camara")) {
                emergencyPos[i] = cameraPos + cameraFront * 2.0f;
              }
              ImGui::SameLine();
              if (ImGui::Button("Eliminar")) {
                emergencyPos.erase(emergencyPos.begin() + i);
                emergencyRot.erase(emergencyRot.begin() + i);
                emergencyScale.erase(emergencyScale.begin() + i);
                ImGui::PopID();
                break; // Break the loop to avoid invalid iterator
              }
              ImGui::PopID();
            }

            ImGui::Separator();
            ImGui::Text("Reactor Model");
            ImGui::DragFloat3("Reactor Pos", &reactorPos.x, 0.05f);
            ImGui::DragFloat3("Reactor Rot", &reactorRot.x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("Reactor Scale", &reactorScale.x, 0.01f, 0.01f,
                              10.0f);
            if (ImGui::Button("Traer frente a camara##reactor")) {
              reactorPos = cameraPos + cameraFront * 2.0f;
              reactorPos.y = 2.0f;
              reactorRot = glm::vec3(0.0f, 0.0f, 0.0f);
              reactorScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();

            ImGui::Text("Machine Lab Model (Escenario Grande) - 3 Instancias");
            static int selectedMachineLab = 0;
            const char *machineLabItems[] = {"Machine Lab 1", "Machine Lab 2",
                                             "Machine Lab 3"};
            ImGui::Combo("Seleccionar Machine Lab", &selectedMachineLab,
                         machineLabItems, IM_ARRAYSIZE(machineLabItems));
            ImGui::DragFloat3("MachineLab Pos",
                              &machineLabPos[selectedMachineLab].x, 0.05f);
            ImGui::DragFloat3("MachineLab Rot",
                              &machineLabRot[selectedMachineLab].x, 0.5f,
                              -180.0f, 180.0f);
            ImGui::DragFloat3("MachineLab Scale",
                              &machineLabScale[selectedMachineLab].x, 0.01f,
                              0.01f, 10.0f);
            if (ImGui::Button("Traer frente a camara##machinelab")) {
              machineLabPos[selectedMachineLab] =
                  cameraPos + cameraFront * 2.0f;
              machineLabPos[selectedMachineLab].y = -0.5f;
              machineLabRot[selectedMachineLab] = glm::vec3(0.0f, 0.0f, 0.0f);
              machineLabScale[selectedMachineLab] = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();

            ImGui::Text("Metal Desk Model (Escenario Grande) - 8 Instancias");
            static int selectedMetalDesk = 0;
            const char *metalDeskItems[] = {
                "Metal Desk 1", "Metal Desk 2", "Metal Desk 3", "Metal Desk 4",
                "Metal Desk 5", "Metal Desk 6", "Metal Desk 7", "Metal Desk 8"};
            ImGui::Combo("Seleccionar Metal Desk", &selectedMetalDesk,
                         metalDeskItems, IM_ARRAYSIZE(metalDeskItems));
            ImGui::DragFloat3("MetalDesk Pos",
                              &metalDeskPos[selectedMetalDesk].x, 0.05f);
            ImGui::DragFloat3("MetalDesk Rot",
                              &metalDeskRot[selectedMetalDesk].x, 0.5f, -180.0f,
                              180.0f);
            ImGui::DragFloat3("MetalDesk Scale",
                              &metalDeskScale[selectedMetalDesk].x, 0.01f,
                              0.01f, 10.0f);
            if (ImGui::Button("Traer frente a camara##metaldesk")) {
              metalDeskPos[selectedMetalDesk] = cameraPos + cameraFront * 2.0f;
              metalDeskPos[selectedMetalDesk].y = -0.5f;
              metalDeskRot[selectedMetalDesk] = glm::vec3(0.0f, 0.0f, 0.0f);
              metalDeskScale[selectedMetalDesk] = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            ImGui::Separator();

            ImGui::Text("Panel de Control");
            ImGui::DragFloat3("PanelCtrl Pos", &panelControlPos.x, 0.05f);
            ImGui::DragFloat3("PanelCtrl Rot", &panelControlRot.x, 0.5f,
                              -180.0f, 180.0f);
            ImGui::DragFloat3("PanelCtrl Scale", &panelControlScale.x, 0.01f,
                              0.01f, 10.0f);
            if (ImGui::Button("Traer Panel Ctrl frente a camara")) {
              panelControlPos = cameraPos + cameraFront * 2.0f;
              panelControlPos.y = -0.5f;
              panelControlRot = glm::vec3(0.0f, 0.0f, 0.0f);
              panelControlScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Reactor Control");
            ImGui::DragFloat3("ReactorCtrl Pos", &reactorControlPos.x, 0.05f);
            ImGui::DragFloat3("ReactorCtrl Rot", &reactorControlRot.x, 0.5f,
                              -180.0f, 180.0f);
            ImGui::DragFloat3("ReactorCtrl Scale", &reactorControlScale.x,
                              0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Reactor Ctrl frente a camara")) {
              reactorControlPos = cameraPos + cameraFront * 2.0f;
              reactorControlPos.y = -0.5f;
              reactorControlRot = glm::vec3(0.0f, 0.0f, 0.0f);
              reactorControlScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lampara Reactor Model");
            ImGui::DragFloat3("LampReactor Pos", &lamparaReactorPos.x, 0.05f);
            ImGui::DragFloat3("LampReactor Rot", &lamparaReactorRot.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("LampReactor Scale", &lamparaReactorScale.x, 0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Lampara Reactor frente a camara")) {
              lamparaReactorPos = cameraPos + cameraFront * 2.0f;
              lamparaReactorPos.y = 2.0f;
              lamparaReactorRot = glm::vec3(0.0f, 0.0f, 0.0f);
              lamparaReactorScale = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lampara Reactor Model 2");
            ImGui::DragFloat3("LampReactor Pos 2", &lamparaReactorPos2.x, 0.05f);
            ImGui::DragFloat3("LampReactor Rot 2", &lamparaReactorRot2.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("LampReactor Scale 2", &lamparaReactorScale2.x, 0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Lampara Reactor 2 frente a camara")) {
              lamparaReactorPos2 = cameraPos + cameraFront * 2.0f;
              lamparaReactorPos2.y = 2.0f;
              lamparaReactorRot2 = glm::vec3(0.0f, 0.0f, 0.0f);
              lamparaReactorScale2 = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lampara Reactor Model 3");
            ImGui::DragFloat3("LampReactor Pos 3", &lamparaReactorPos3.x, 0.05f);
            ImGui::DragFloat3("LampReactor Rot 3", &lamparaReactorRot3.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("LampReactor Scale 3", &lamparaReactorScale3.x, 0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Lampara Reactor 3 frente a camara")) {
              lamparaReactorPos3 = cameraPos + cameraFront * 2.0f;
              lamparaReactorPos3.y = 2.0f;
              lamparaReactorRot3 = glm::vec3(0.0f, 0.0f, 0.0f);
              lamparaReactorScale3 = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Lampara Reactor Model 4");
            ImGui::DragFloat3("LampReactor Pos 4", &lamparaReactorPos4.x, 0.05f);
            ImGui::DragFloat3("LampReactor Rot 4", &lamparaReactorRot4.x, 0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("LampReactor Scale 4", &lamparaReactorScale4.x, 0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Lampara Reactor 4 frente a camara")) {
              lamparaReactorPos4 = cameraPos + cameraFront * 2.0f;
              lamparaReactorPos4.y = 2.0f;
              lamparaReactorRot4 = glm::vec3(0.0f, 0.0f, 0.0f);
              lamparaReactorScale4 = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::Text("Generador Model (3 Instancias)");
            static int selectedGenerador = 0;
            const char *generadorItems[] = {"Generador 1", "Generador 2",
                                            "Generador 3"};
            ImGui::Combo("Seleccionar Generador", &selectedGenerador,
                         generadorItems, IM_ARRAYSIZE(generadorItems));

            ImGui::DragFloat3("Gen Pos", &generadorPos[selectedGenerador].x,
                              0.05f);
            ImGui::DragFloat3("Gen Rot", &generadorRot[selectedGenerador].x,
                              0.5f, -180.0f, 180.0f);
            ImGui::DragFloat3("Gen Scale", &generadorScale[selectedGenerador].x,
                              0.01f, 0.01f, 10.0f);
            if (ImGui::Button("Traer Generador frente a la camara")) {
              generadorPos[selectedGenerador] = cameraPos + cameraFront * 2.0f;
              generadorPos[selectedGenerador].y = -0.5f;
              generadorRot[selectedGenerador] = glm::vec3(0.0f, 0.0f, 0.0f);
              generadorScale[selectedGenerador] = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            ImGui::Separator();

            ImGui::End(); // End 'Editor Contencion'
            }

            // --- UNIFIED LEVEL EDITOR ---
            if (activeEditorPanel == 1) {
            ImGui::SetNextWindowPos(ImVec2(leftPanelX, leftLevelY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(leftPanelW, availablePanelH), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.84f);
            ImGui::Begin("Editor de Niveles 🛠️", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            
            ImGui::TextColored(ImVec4(0.2f, 0.75f, 1.0f, 1.0f), "Editor de Mapa 3D");
            ImGui::Separator();

            static int selectedPropIdx = -1;
            if (placedProps.empty()) {
                selectedPropIdx = -1;
            } else if (selectedPropIdx < 0 || selectedPropIdx >= (int)placedProps.size()) {
                selectedPropIdx = 0;
            }

            // List of placed props
            if (ImGui::TreeNode("Objetos en Escena")) {
                for (int i = 0; i < (int)placedProps.size(); ++i) {
                    std::string label = std::to_string(i) + ": " + placedProps[i].modelName;
                    if (ImGui::Selectable(label.c_str(), selectedPropIdx == i)) {
                        selectedPropIdx = i;
                    }
                }
                ImGui::TreePop();
            }
            ImGui::Separator();

            if (selectedPropIdx >= 0 && selectedPropIdx < (int)placedProps.size()) {
                auto& prop = placedProps[selectedPropIdx];
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.1f, 1.0f), "Prop Seleccionado: %s", prop.modelName.c_str());
                
                ImGui::DragFloat3("Posición", &prop.pos.x, 0.05f);
                ImGui::DragFloat3("Rotación", &prop.rot.x, 0.5f, -180.0f, 180.0f);
                ImGui::DragFloat3("Escala", &prop.scale.x, 0.01f, 0.01f, 10.0f);
                ImGui::Checkbox("Activar Física/Colisión", &prop.collisionActive);

                if (ImGui::Button("Traer frente a camara")) {
                    prop.pos = cameraPos + cameraFront * 2.0f;
                    prop.pos.y = -0.5f;
                    prop.rot = glm::vec3(0.0f, 0.0f, 0.0f);
                    prop.scale = glm::vec3(1.0f, 1.0f, 1.0f);
                }

                ImGui::Spacing();
                
                if (ImGui::Button("📑 Duplicar Objeto", ImVec2(-1, 0))) {
                    PlacedProp dup = prop;
                    dup.pos += glm::vec3(0.5f, 0.0f, 0.5f); // Un pequeño offset para ver que se duplicó
                    placedProps.push_back(dup);
                    selectedPropIdx = (int)placedProps.size() - 1;
                }

                if (ImGui::Button("❌ Eliminar Objeto", ImVec2(-1, 0))) {
                    placedProps.erase(placedProps.begin() + selectedPropIdx);
                    if (selectedPropIdx >= (int)placedProps.size()) {
                        selectedPropIdx = (int)placedProps.size() - 1;
                    }
                }
            } else {
                ImGui::Text("Ningún objeto seleccionado");
            }
            ImGui::Separator();

            // Adding a new prop
            ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.2f, 1.0f), "Agregar Nuevo Objeto:");
            static const char* availableModels[] = { 
                "cajonesOF", "sarcofago", "tesla", "reactor", "reactorControl", "panelControl", "warning", "esquineros" 
            };
            static int selectedModelToAddIdx = 0;
            ImGui::Combo("Modelo", &selectedModelToAddIdx, availableModels, IM_ARRAYSIZE(availableModels));

            if (ImGui::Button("➕ Agregar a Escena", ImVec2(-1, 0))) {
                PlacedProp newProp;
                newProp.modelName = availableModels[selectedModelToAddIdx];
                newProp.pos = cameraPos + cameraFront * 2.0f;
                newProp.pos.y = -0.5f;
                newProp.rot = glm::vec3(0.0f, 0.0f, 0.0f);
                
                // Escalas base/por defecto para cada uno
                if (newProp.modelName == "tesla") newProp.scale = glm::vec3(0.150f, 0.120f, 0.090f);
                else if (newProp.modelName == "sarcofago") newProp.scale = glm::vec3(1.260f, 1.060f, 0.930f);
                else if (newProp.modelName == "warning") newProp.scale = glm::vec3(1.410f, 0.950f, 1.570f);
                else if (newProp.modelName == "reactorControl") newProp.scale = glm::vec3(0.890f, 0.730f, 0.280f);
                else if (newProp.modelName == "panelControl") newProp.scale = glm::vec3(0.450f, 0.490f, 0.180f);
                else if (newProp.modelName == "esquineros") newProp.scale = glm::vec3(0.890f, 0.750f, 0.810f);
                else newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);

                placedProps.push_back(newProp);
                selectedPropIdx = (int)placedProps.size() - 1;
            }
            ImGui::Separator();

            // Permanent Save Button!
            if (ImGui::Button("💾 GUARDAR CAMBIOS MAPA", ImVec2(-1, 40))) {
                saveLevelProps("assets/config_posiciones.txt");
            }
            ImGui::End();
            }

            if (activeEditorPanel == 2) {
            static int selectedEntityIndex = 0;
            static bool entityOnlyCollectibles = false;
            if (!gameEntities.empty()) {
              if (selectedEntityIndex < 0)
                selectedEntityIndex = 0;
              if (selectedEntityIndex >= (int)gameEntities.size())
                selectedEntityIndex = (int)gameEntities.size() - 1;
            }

            ImGui::SetNextWindowPos(ImVec2(leftPanelX, panelTopY),
                                    ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(leftPanelW, availablePanelH),
                                     ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.82f);
            ImGui::Begin("Spawn Inspector", NULL,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("Spawn guardado");
            ImGui::Text("Pos: %.1f %.1f %.1f", debugSpawnPos.x, debugSpawnPos.y,
                        debugSpawnPos.z);
            ImGui::Text("Yaw/Pitch: %.1f / %.1f", debugSpawnYaw,
                        debugSpawnPitch);
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
            auto teleportNear = [&](const glm::vec3& target) {
              cameraPos = target + glm::vec3(1.5f, 0.0f, 1.5f);
              cameraPos.y = baseCameraY;
              updateZone();
            };

            ImGui::SameLine();
            if (ImGui::Button("Gnomo")) {
              teleportNear(gnomePos);
            }

            auto findEntityByType = [&](int type) -> int {
              for (int i = 0; i < (int)gameEntities.size(); i++) {
                if (gameEntities[i].type == type) return i;
              }
              return -1;
            };
            int yellowKeycardIndex = findEntityByType(8);
            int redKeycardIndex = findEntityByType(9);
            int portalIndex = findEntityByType(7);

            if (ImGui::Button("Tarjeta amarilla") && yellowKeycardIndex >= 0)
              teleportNear(gameEntities[yellowKeycardIndex].pos);
            ImGui::SameLine();
            if (ImGui::Button("Tarjeta roja") && redKeycardIndex >= 0)
              teleportNear(gameEntities[redKeycardIndex].pos);

            if (ImGui::Button("Portal") && portalIndex >= 0)
              teleportNear(gameEntities[portalIndex].pos);
            ImGui::SameLine();
            if (!gameEntities.empty() &&
                ImGui::Button("Entidad seleccionada")) {
              teleportNear(gameEntities[selectedEntityIndex].pos);
            }
            ImGui::End();
            }

          if (showCollisionViewer) {
            ImGui::SetNextWindowPos(ImVec2(leftPanelX + leftPanelW + panelGap,
                                           panelTopY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(260.0f, 118.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.80f);
            ImGui::Begin("Collision Viewer", NULL,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Paredes", &collisionShowWalls);
            ImGui::Checkbox("Props bloqueantes", &collisionShowProps);
            ImGui::SliderFloat("Radio", &collisionViewerRadius, 2.0f, 20.0f,
                               "%.1f");
            ImGui::End();
          }
        } else {
          ImGui::SetNextWindowPos(ImVec2(0.0f, currentHeight * 0.42f),
                                  ImGuiCond_Always);
          ImGui::SetNextWindowSize(ImVec2(34.0f, 148.0f), ImGuiCond_Always);
          ImGui::SetNextWindowBgAlpha(0.72f);
          ImGui::Begin("ShowEditorBtn", NULL,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove);
          if (ImGui::Button(">\nE\nD\nI\nT\nO\nR", ImVec2(-1, -1))) {
            showDebugGUI = true;
          }
          ImGui::End();
        }

        if (hudMessageTimer > 0.0f) {
          ImGui::SetNextWindowPos(
              ImVec2(currentWidth * 0.1f, currentHeight * 0.8f));
          ImGui::SetNextWindowSize(
              ImVec2(currentWidth * 0.8f, currentHeight * 0.2f));
          ImGui::SetNextWindowBgAlpha(0.0f); // Transparente
          ImGui::Begin("HUD", NULL,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoInputs);
          ImGui::SetWindowFontScale(1.2f);

          // Centrar el texto
          float textWidth = ImGui::CalcTextSize(currentHUDMessage.c_str()).x;
          ImGui::SetCursorPosX((currentWidth * 0.8f - textWidth) * 0.5f);
          ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
                             currentHUDMessage.c_str());
          ImGui::End();
        }

        if (isReadingDocument) {
          ImGui::SetNextWindowPos(
              ImVec2(currentWidth * 0.18f, currentHeight * 0.16f));
          ImGui::SetNextWindowSize(
              ImVec2(currentWidth * 0.64f, currentHeight * 0.5f));
          ImGui::SetNextWindowBgAlpha(0.94f);
          ImGui::Begin("Documento", NULL,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse);
          ImGui::SetWindowFontScale(1.15f);
          ImGui::TextWrapped("%s", currentDocumentTitle.c_str());
          ImGui::Separator();
          ImGui::Spacing();
          ImGui::TextWrapped("%s", currentDocumentBody.c_str());
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                             "E o ESC para cerrar");
          ImGui::End();
        }
        // --- HOTBAR INVENTARIO (estilo Minecraft) ---
        if (gameState == PLAYING) {
          const float slotSize = 52.0f;
          const float slotPadding = 4.0f;
          const float iconPadding = 4.0f;
          const int maxSlots = 6; // Linterna, Llave1, Llave2, Bat1, Bat2, Bat3
          float hotbarWidth = maxSlots * (slotSize + slotPadding) + slotPadding;
          float hotbarHeight = slotSize + slotPadding * 2.0f + 16.0f; // extra para label
          float hotbarX = (currentWidth - hotbarWidth) * 0.5f;
          float hotbarY = currentHeight - hotbarHeight - 12.0f;

          ImGui::SetNextWindowPos(ImVec2(hotbarX, hotbarY));
          ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight));
          ImGui::SetNextWindowBgAlpha(0.55f);
          ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(slotPadding, slotPadding));
          ImGui::Begin("Hotbar", NULL,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);

          // Helper lambda para dibujar un slot
          auto drawSlot = [&](const char* label, ImTextureID texID, bool hasItem, bool isActive) {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Fondo del slot
            ImU32 bgColor = hasItem
              ? (isActive ? IM_COL32(80, 200, 80, 180) : IM_COL32(60, 60, 60, 200))
              : IM_COL32(30, 30, 30, 140);
            ImU32 borderColor = isActive ? IM_COL32(255, 255, 100, 255) : IM_COL32(100, 100, 100, 180);

            drawList->AddRectFilled(cursorPos,
              ImVec2(cursorPos.x + slotSize, cursorPos.y + slotSize),
              bgColor, 4.0f);
            drawList->AddRect(cursorPos,
              ImVec2(cursorPos.x + slotSize, cursorPos.y + slotSize),
              borderColor, 4.0f, 0, isActive ? 2.5f : 1.0f);

            // Icono de textura
            if (hasItem && texID != 0) {
              drawList->AddImage(texID,
                ImVec2(cursorPos.x + iconPadding, cursorPos.y + iconPadding),
                ImVec2(cursorPos.x + slotSize - iconPadding, cursorPos.y + slotSize - iconPadding));
            }

            // Label debajo
            ImVec2 textSize = ImGui::CalcTextSize(label);
            float textX = cursorPos.x + (slotSize - textSize.x) * 0.5f;
            drawList->AddText(ImVec2(textX, cursorPos.y + slotSize + 1.0f),
              hasItem ? IM_COL32(255, 255, 255, 255) : IM_COL32(100, 100, 100, 150), label);

            ImGui::Dummy(ImVec2(slotSize, slotSize));
            ImGui::SameLine(0.0f, slotPadding);
          };

          // Slot 0: Linterna (siempre la tiene)
          drawSlot(isFlashlightOn ? "[F] ON" : "[F] OFF",
                   (ImTextureID)(intptr_t)clueTexture, true, selectedHotbarSlot == 0);

          // Slot 1: Tarjeta Lvl1
          drawSlot("Llave 1",
                   (ImTextureID)(intptr_t)keycardTex, hasKeycardLvl1, selectedHotbarSlot == 1);

          // Slot 2: Tarjeta Lvl2
          drawSlot("Llave 2",
                   (ImTextureID)(intptr_t)keycardTex, hasKeycardLvl2, selectedHotbarSlot == 2);

          // Slots 3-5: Baterías
          for (int i = 0; i < 3; i++) {
            char batLabel[16];
            snprintf(batLabel, sizeof(batLabel), "Bat %d", i + 1);
            drawSlot(batLabel,
                     (ImTextureID)(intptr_t)batteryTex, bateriasRecolectadas > i, selectedHotbarSlot == (3 + i));
          }

          ImGui::End();
          ImGui::PopStyleVar(2);
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
