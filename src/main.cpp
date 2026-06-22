#define GLM_ENABLE_EXPERIMENTAL
// GLAD debe incluirse SIEMPRE antes que GLFW (y antes de cualquier header
// OpenGL)
#include <glad/glad.h>
// GLFW_INCLUDE_NONE evita que GLFW incluya <GL/gl.h> por su cuenta en Windows
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
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

#include "headers/animated_entity.h"
#include "headers/game_state.h"
#include "headers/gameplay.h"
#include "headers/obj_mesh.h"
#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/localization.h"


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
  case 10:
    return "Sangre";
  default:
    return "Desconocido";
  }
}

static bool isCollectibleEntityType(int type) {
  return type == 0 || type == 1 || type == 8 || type == 9;
}

static bool isInspectableEntityType(int type) {
  return type == 3 || type == 4 || type == 5 || type == 6 || type == 7 ||
         type == 10;
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
  return blockType == 7 || blockType == -7 || blockType == 8 ||
         blockType == 9 || blockType == -8 || blockType == -9;
}

static const char *getDoorDebugLabel(int blockType) {
  switch (blockType) {
  case 7:
    return "Puerta cerrada";
  case -7:
    return "Puerta abierta";
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
  glfwSwapInterval(1);
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

  ma_sound shotgunFireSound;
  bool shotgunFireSoundReady =
      ma_sound_init_from_file(
          &audioEngine,
          "assets/dragon-studio-cinematic-shotgun-with-reload-467480.mp3", 0,
          NULL, NULL, &shotgunFireSound) == MA_SUCCESS;

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

  GLTFModel *pistolViewmodel =
      new GLTFModel("assets/armas/FP_Arms_Pistol_01_Anims.glb");
  GLTFModel *rifleViewmodel =
      new GLTFModel("assets/armas/FP_Arms_rifle_01_Anims.glb");
  GLTFModel *shotgunViewmodel =
      new GLTFModel("assets/armas/FP_Arms_Shotgun_01_Anims.glb");

  //----------------------------------------------------------------------------AGREGAR
  // LOS ARCHIVOS GLTF/OBJ
  // AQUI--------------------------------------------------------------------------------

  GLTFModel *azulejoGLTF = new GLTFModel("assets/bano/azule.glb");
  GLTFModel *mirrorGLTF = new GLTFModel("assets/bano/mirror.glb");
  GLTFModel *mirrorBGGLTF = new GLTFModel("assets/bano/MirrorBG.glb");
  GLTFModel *ligthbathroomGLTF = new GLTFModel("assets/bano/ligthbathroom.glb");
  GLTFModel *ligthbathroom2GLTF = ligthbathroomGLTF;
  mensBGLTF = new GLTFModel("assets/bano/mensB.glb");
  girlBGLTF = new GLTFModel("assets/bano/girlB.glb");
  banoGLTF = new GLTFModel("assets/bano/Bano.glb");
  lavamanosGLTF = new GLTFModel("assets/bano/lavamanos.glb");
  urinarioGLTF = new GLTFModel("assets/bano/urinario.glb");

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
  sangrePisoGLTF = new GLTFModel("assets/sangre-piso.glb");
  sangrePiso2GLTF = new GLTFModel("assets/sangre-piso2.glb");
  sangreParedesGLTF = new GLTFModel("assets/help.glb");
  sangrePared2GLTF = new GLTFModel("assets/it-sees-you.glb");
  cajonesOFGLTF = new GLTFModel("assets/oficinas/cajonesOF.glb");
  behindYouGLTF = new GLTFModel("assets/behind-you.glb");
  barraGLTF = new GLTFModel("assets/contencion/barra.glb");
  logoGLTF = new GLTFModel("assets/contencion/logo.glb");
  logo2GLTF = new GLTFModel("assets/contencion/logo2.glb");
  consolaGLTF = new GLTFModel("assets/contencion/consola.glb");
  // Modelos de archivo restringido
  gabineteGLTF = new GLTFModel("assets/archivo/gabinete.glb");
  camaraGLTF = new GLTFModel("assets/archivo/camara.glb");
  serversGLTF = new GLTFModel("assets/archivo/servers.glb");
  terminalGLTF = new GLTFModel("assets/archivo/terminal.glb");
  boxCloseGLTF = new GLTFModel("assets/archivo/box-close.glb");
  boxOpenGLTF = new GLTFModel("assets/archivo/box-open.glb");
  vaultDoorGLTF = new GLTFModel("assets/archivo/vault-door.glb");
  escritorioGLTF = new GLTFModel("assets/archivo/escritorio.glb");
  mesaGLTF = new GLTFModel("assets/archivo/mesa.glb");
  miniLamparaGLTF = new GLTFModel("assets/archivo/mini-lampara.glb");
  computerGLTF = new GLTFModel("assets/archivo/computer.glb");
  sillaGLTF = new GLTFModel("assets/archivo/silla.glb");
  monitorGLTF = new GLTFModel("assets/monitor.glb");
  deskGLTF = new GLTFModel("assets/desk.glb");
  estanteGLTF = new GLTFModel("assets/estante.glb");
  sillitaGLTF = new GLTFModel("assets/sillita.glb");
  maquinaGLTF = new GLTFModel("assets/maquina.glb");
  paredGLTF = new GLTFModel("assets/pared.glb");

  // --- SALA DE MUESTRAS: Modelos (assets/muestras/) ---
  machineLabGLTF = new GLTFModel("assets/machine_lab.glb");
  estantesGLTF = new GLTFModel("assets/muestras/estantes.glb");
  morguefridgeGLTF = new GLTFModel("assets/muestras/morguefridge.glb");
  monitoringGLTF = new GLTFModel("assets/muestras/monitoring.glb");
  refrigeradorGLTF = new GLTFModel("assets/muestras/refrigerador.glb");
  camillaGLTF = new GLTFModel("assets/muestras/camilla.glb");
  muralGLTF = new GLTFModel("assets/muestras/mural.glb");
  terminalesGLTF = new GLTFModel("assets/muestras/terminales.glb");
  esferaGLTF = new GLTFModel("assets/muestras/esfera.glb");
  bodybagGLTF = new GLTFModel("assets/muestras/bodybag.glb");
  coffinGLTF = new GLTFModel("assets/muestras/coffin.glb");
  bloodyboxGLTF = new GLTFModel("assets/muestras/bloodybox.glb");
  labtableGLTF = new GLTFModel("assets/muestras/labtable.glb");
  shelfGLTF = new GLTFModel("assets/muestras/shelf.glb");
  safetyGLTF = new GLTFModel("assets/muestras/safety.glb");

    // --- SALA DE MUESTRAS: Modelos (assets/bodega/) ---
    neveraGLTF = new GLTFModel("assets/bodega/nevera.glb");
    estantebodGLTF = new GLTFModel("assets/bodega/estantebod.glb");
    boxesGLTF = new GLTFModel("assets/bodega/boxes.glb");
    barrilGLTF = new GLTFModel("assets/bodega/barril.glb");

  // --- SALA DE DESCANSO: modelos reutilizados (ya existian en assets/) ---
  GLTFModel *sillasGLTF = new GLTFModel("assets/sillas.glb");
  GLTFModel *sofaGLTF = new GLTFModel("assets/sofa.glb");
  metalDeskGLTF =
      new GLTFModel("assets/metal_desk.glb"); // global declarado en game_state

  // --- SALA DE DESCANSO: modelos nuevos (assets/descanso/) ---
  GLTFModel *lockerGLTF = new GLTFModel("assets/descanso/locker.glb");
  GLTFModel *bunkBedGLTF = new GLTFModel("assets/descanso/bunk_bed.glb");
  GLTFModel *lockersGLTF = new GLTFModel("assets/descanso/lockers.glb");
  GLTFModel *taxophoneGLTF =
      new GLTFModel("assets/descanso/old_soviet_taxophone.glb");
  GLTFModel *estanteCajasGLTF =
      new GLTFModel("assets/descanso/estante_cajas.glb");
  GLTFModel *expendedoraGLTF = new GLTFModel("assets/descanso/expendedora.glb");
  GLTFModel *extintorViejoGLTF =
      new GLTFModel("assets/descanso/extintor_viejo.glb");
  GLTFModel *jaulaGLTF = new GLTFModel("assets/descanso/jaula.glb");
  GLTFModel *compuDestruidaGLTF =
      new GLTFModel("assets/descanso/compu_destruida.glb");
  GLTFModel *oldSofaGLTF =
      new GLTFModel("assets/descanso/old_sofa_free.glb", true);
  GLTFModel *papelViejoGLTF = new GLTFModel("assets/descanso/papel_viejo.glb");
  GLTFModel *plantaElectricaGLTF =
      new GLTFModel("assets/descanso/planta_electrica.glb");
  GLTFModel *botasGLTF = new GLTFModel("assets/descanso/botas.glb");
  GLTFModel *comedorGLTF = new GLTFModel("assets/descanso/comedor.glb");
  GLTFModel *trashGLTF = new GLTFModel("assets/descanso/trash.glb");
  GLTFModel *trashBagGLTF = new GLTFModel("assets/descanso/trash_bag.glb");

  // --- ASCENSOR ---
  ascensorGLTF = new GLTFModel("assets/ascensor/ascensor.glb");
  cajaElectricaGLTF = new GLTFModel("assets/ascensor/caja-electrica.glb");
  plataformaGLTF = new GLTFModel("assets/ascensor/plataforma.glb");
  ductoGLTF = new GLTFModel("assets/ascensor/ducto.glb");
  ghostGLTF = new GLTFModel("assets/ascensor/ghost.glb");
  headGLTF = new GLTFModel("assets/ascensor/head.glb");

  // Registrar en modelRegistry
  modelRegistry["cajonesOF"] = cajonesOFGLTF;
  modelRegistry["gabinete"] = gabineteGLTF;
  modelRegistry["camara"] = camaraGLTF;
  modelRegistry["servers"] = serversGLTF;
  modelRegistry["terminal"] = terminalGLTF;
  modelRegistry["box-close"] = boxCloseGLTF;
  modelRegistry["box-open"] = boxOpenGLTF;
  modelRegistry["vault-door"] = vaultDoorGLTF;
  modelRegistry["escritorio"] = escritorioGLTF;
  modelRegistry["mesa"] = mesaGLTF;
  modelRegistry["mini-lampara"] = miniLamparaGLTF;
  modelRegistry["computer"] = computerGLTF;
  modelRegistry["silla"] = sillaGLTF;
  modelRegistry["barra"] = barraGLTF;
  modelRegistry["logo"] = logoGLTF;
  modelRegistry["logo2"] = logo2GLTF;
  modelRegistry["sarcofago"] = sarcofagoGLTF;
  modelRegistry["tesla"] = teslaGLTF;
  modelRegistry["reactor"] = reactorGLTF;
  modelRegistry["consola"] = consolaGLTF;
  modelRegistry["panelControl"] = panelControlGLTF;
  modelRegistry["warning"] = warningGLTF;
  modelRegistry["esquineros"] = esquinerosGLTF;
  modelRegistry["esquineros2"] = esquinerosGLTF;
  modelRegistry["esquineros3"] = esquinerosGLTF;
  modelRegistry["esquineros4"] = esquinerosGLTF;
  modelRegistry["cables_piso"] = cablePisoGLTF;
  modelRegistry["cables_techo"] = cableTechoGLTF;
  modelRegistry["lampara-reactor"] = lamparaReactorGLTF;
  modelRegistry["emergency"] = emergencyGLTF;
  modelRegistry["generador"] = generadorGLTF;
  modelRegistry["lampara"] = lamparaContencionGLTF;
  modelRegistry["lampara2"] = lampara2GLTF;
  modelRegistry["sangre-piso"] = sangrePisoGLTF;
  modelRegistry["sangre-piso2"] = sangrePiso2GLTF;
  modelRegistry["help"] = sangreParedesGLTF;
  modelRegistry["it-sees-you"] = sangrePared2GLTF;
  modelRegistry["behind-you"] = behindYouGLTF;

  // Sala de Descanso
  modelRegistry["sillas"] = sillasGLTF;
  modelRegistry["sillita"] = sillitaGLTF;
  modelRegistry["desk"] = deskGLTF;
  modelRegistry["monitor"] = monitorGLTF;
  modelRegistry["metal_desk"] = metalDeskGLTF;
  modelRegistry["sofa"] = sofaGLTF;
  modelRegistry["maquina"] = maquinaGLTF;
  modelRegistry["pared"] = paredGLTF;
  modelRegistry["locker"] = lockerGLTF;
  modelRegistry["bunk_bed"] = bunkBedGLTF;
  modelRegistry["lockers"] = lockersGLTF;
  modelRegistry["old_soviet_taxophone"] = taxophoneGLTF;
  modelRegistry["estante_cajas"] = estanteCajasGLTF;
  modelRegistry["expendedora"] = expendedoraGLTF;
  modelRegistry["extintor_viejo"] = extintorViejoGLTF;
  modelRegistry["jaula"] = jaulaGLTF;
  modelRegistry["compu_destruida"] = compuDestruidaGLTF;
  modelRegistry["old_sofa_free"] = oldSofaGLTF;
  modelRegistry["papel_viejo"] = papelViejoGLTF;
  modelRegistry["planta_electrica"] = plantaElectricaGLTF;
  modelRegistry["botas"] = botasGLTF;
  modelRegistry["comedor"] = comedorGLTF;
  modelRegistry["trash"] = trashGLTF;
  modelRegistry["trash_bag"] = trashBagGLTF;
  modelRegistry["estante"] = estanteGLTF;
  modelRegistry["estantes"] = estantesGLTF;
  modelRegistry["morguefridge"] = morguefridgeGLTF;
  modelRegistry["monitoring"] = monitoringGLTF;
  modelRegistry["refrigerador"] = refrigeradorGLTF;
  modelRegistry["camilla"] = camillaGLTF;
  modelRegistry["mural"] = muralGLTF;
  modelRegistry["terminales"] = terminalesGLTF;
  modelRegistry["esfera"] = esferaGLTF;
  modelRegistry["bodybag"] = bodybagGLTF;
  modelRegistry["coffin"] = coffinGLTF;
  modelRegistry["bloodybox"] = bloodyboxGLTF;
  modelRegistry["labtable"] = labtableGLTF;
  modelRegistry["shelf"] = shelfGLTF;
  modelRegistry["safety"] = safetyGLTF;
  modelRegistry["nevera"] = neveraGLTF;
  modelRegistry["estantebod"] = estantebodGLTF;
  modelRegistry["boxes"] = boxesGLTF;
  modelRegistry["barril"] = barrilGLTF;

  // Modelos generales/baño
  modelRegistry["Bano"] = banoGLTF;
  modelRegistry["azule"] = azulejoGLTF;
  modelRegistry["girlB"] = girlBGLTF;
  modelRegistry["gnome"] = gnomeGLTF;
  modelRegistry["lavamanos"] = lavamanosGLTF;
  modelRegistry["mensB"] = mensBGLTF;
  modelRegistry["mirror"] = mirrorGLTF;
  modelRegistry["MirrorBG"] = mirrorBGGLTF;
  modelRegistry["urinario"] = urinarioGLTF;

  // Modelo de luz (lampara estilo baño) disponible en el editor de niveles
  modelRegistry["ligthbathroom"] = ligthbathroomGLTF;

  // Ascensor
  modelRegistry["ascensor"] = ascensorGLTF;
  modelRegistry["caja-electrica"] = cajaElectricaGLTF;
  modelRegistry["plataforma"] = plataformaGLTF;
  modelRegistry["ducto"] = ductoGLTF;
  modelRegistry["ghost"] = ghostGLTF;
  modelRegistry["head"] = headGLTF;

  // Cargar propiedades desde archivo
  loadLevelProps("assets/config_posiciones.txt");
  if (std::none_of(
          placedProps.begin(), placedProps.end(),
          [](const PlacedProp &prop) { return prop.modelName == "jaula"; })) {
    placedProps.push_back({"jaula", glm::vec3(6.25f, -0.50f, 2.35f),
                           glm::vec3(0.0f, 90.0f, 0.0f),
                           glm::vec3(0.50f, 0.50f, 0.50f), false, "Descanso"});
    saveLevelProps("assets/config_posiciones.txt");
  }
  if (std::none_of(placedProps.begin(), placedProps.end(),
                   [](const PlacedProp &prop) {
                     return prop.modelName == "compu_destruida";
                   })) {
    placedProps.push_back({"compu_destruida", glm::vec3(18.35f, -0.30f, 4.15f),
                           glm::vec3(0.0f, -90.0f, 0.0f),
                           glm::vec3(0.55f, 0.55f, 0.55f), false, "Descanso"});
    saveLevelProps("assets/config_posiciones.txt");
  }
  AnimatedEntitySystem animatedEntities;
  activeAnimatedEntitySystem = &animatedEntities;
  animatedEntities.Load("assets/animated_entities.txt");
  std::cout << "[SISTEMA] Props oficinas cargados: CajonesOF("
            << cajonesOFGLTF->meshes.size() << ")" << std::endl;
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
  std::cout << "[SISTEMA] Props archivo cargados: "
            << "Gabinete(" << gabineteGLTF->meshes.size() << "), "
            << "Camara(" << camaraGLTF->meshes.size() << "), "
            << "Servers(" << serversGLTF->meshes.size() << "), "
            << "Terminal(" << terminalGLTF->meshes.size() << "), "
            << "VaultDoor(" << vaultDoorGLTF->meshes.size() << "), "
            << "Escritorio(" << escritorioGLTF->meshes.size() << "), "
            << "Computer(" << computerGLTF->meshes.size() << ")" << std::endl;
  std::cout << "[SISTEMA] Props muestras cargados: "
            << "MachineLab(" << machineLabGLTF->meshes.size() << "), "
            << "MorgueFridge(" << morguefridgeGLTF->meshes.size() << "), "
            << "Monitoring(" << monitoringGLTF->meshes.size() << "), "
            << "Refrigerador(" << refrigeradorGLTF->meshes.size() << "), "
            << "Camilla(" << camillaGLTF->meshes.size() << "), "
            << "Terminales(" << terminalesGLTF->meshes.size() << ")"
            << std::endl;
  std::cout << "[SISTEMA] Props descanso cargados: "
            << "Locker(" << lockerGLTF->meshes.size() << "), "
            << "BunkBed(" << bunkBedGLTF->meshes.size() << "), "
            << "Taxophone(" << taxophoneGLTF->meshes.size() << "), "
            << "Expendedora(" << expendedoraGLTF->meshes.size() << "), "
            << "Extintor(" << extintorViejoGLTF->meshes.size() << "), "
            << "PlantaElectrica(" << plantaElectricaGLTF->meshes.size() << ")"
            << std::endl;
  unsigned int wallTex1 = loadTexture("assets/paredesLAB.png");
  unsigned int wallTex2 = loadTexture("assets/bano/paredbanosT.png");
  unsigned int wallTex3 = loadTexture("assets/wall.png");
  unsigned int wallTex4 = loadTexture("assets/bano/paredbanosGT.png");
  unsigned int wallTex5 = loadTexture("assets/bano/pisosBanos.png");

  // Texturas de área de contención
  unsigned int wallContencionTex =
      loadTexture("assets/contencion/paredes-contencion.png");
  unsigned int floorContencionTex =
      loadTexture("assets/contencion/piso-contencion.png");
  unsigned int roofContencionTex =
      loadTexture("assets/contencion/techo-contencion.png");

  // Texturas del Archivo Restringido
  unsigned int wallArchivoTex = loadTexture("assets/archivo/paredes.png");
  unsigned int floorArchivoTex = loadTexture("assets/archivo/piso.png");
  unsigned int roofArchivoTex = loadTexture("assets/archivo/techo.png");

  // Texturas de Ascensor
  unsigned int wallAscensorTex = loadTexture("assets/ascensor/pared.png");
  unsigned int roofAscensorTex = loadTexture("assets/ascensor/techo.png");

  // Texturas de Sala de Generadores (Seccion 1)
  unsigned int wallGeneradoresTex = loadTexture("assets/sala-generadores/paredes.png");
  unsigned int floorGeneradoresTex = loadTexture("assets/sala-generadores/piso.png");
  unsigned int roofGeneradoresTex = loadTexture("assets/sala-generadores/techo.png");

  // Texturas de Sala de Pruebas (Seccion 2)
  unsigned int wallPruebasTex = loadTexture("assets/sala-pruebas/pared.png");
  unsigned int floorPruebasTex = loadTexture("assets/sala-pruebas/piso.png");
  unsigned int roofPruebasTex = loadTexture("assets/sala-pruebas/techo.png");

  // Textura de metal generada para las puertas
  unsigned int doorTex = loadTextureWithFallback("assets/puerta_metal.png", 0);

  unsigned int portalTex = loadTexture("assets/clue.png");

  unsigned int floorTexture = loadTexture("assets/pisoH.jpg");
  unsigned int logoTexture = loadTexture("assets/logo.png");
  unsigned int clueTexture = loadTexture("assets/clue.png");
  unsigned int enemyTexture = loadTexture("assets/enemy.png");

  // Texturas de la Sala de Descanso.
  // Si aún no existen los PNG en assets/descanso/, se usa una textura existente
  // como fallback para que el build no falle. Coloca tus imágenes ahí para
  // reemplazarlas (paredes.png, piso.png, techo.png).
  unsigned int wallDescansoTex =
      loadTextureWithFallback("assets/descanso/paredes.png", wallTex1);
  unsigned int floorDescansoTex =
      loadTextureWithFallback("assets/descanso/piso.png", floorTexture);
  unsigned int roofDescansoTex =
      loadTextureWithFallback("assets/descanso/techo.png", floorTexture);

  // Piso del área de Oficinas. Si aún no existe assets/oficinas/piso.png se usa
  // la textura por defecto como fallback. Coloca tu imagen ahí para cambiarla.
  unsigned int floorOficinasTex =
      loadTextureWithFallback("assets/oficinas/piso.png", floorTexture);

  // Texturas especÃ­ficas con fallback
  unsigned int batteryTex =
      loadTextureWithFallback("assets/battery.png", clueTexture);
  unsigned int keycardBlueTex =
      loadTextureWithFallback("assets/keycard-azul.png", clueTexture);
  unsigned int keycardRedTex =
      loadTextureWithFallback("assets/keycard-roja.png", clueTexture);
  unsigned int keycardYellowTex =
      loadTextureWithFallback("assets/keycard-amarilla.png", clueTexture);
  unsigned int keycardBlueInvTex =
      loadTextureWithFallback("assets/inv-keycard-azul.png", keycardBlueTex);
  unsigned int keycardRedInvTex =
      loadTextureWithFallback("assets/inv-keycard-roja.png", keycardRedTex);
  unsigned int keycardYellowInvTex = loadTextureWithFallback(
      "assets/inv-keycard-amarilla.png", keycardYellowTex);
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
  int pointLightPosLoc[32];
  int pointLightColLoc[32];
  int pointLightRadLoc[32];
  for (int i = 0; i < 32; i++) {
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
  int spotLightPosLoc[16];
  int spotLightDirLoc[16];
  int spotLightColLoc[16];
  int spotLightCutOffLoc[16];
  int spotLightOuterCutOffLoc[16];
  int spotLightRadLoc[16];
  for (int i = 0; i < 16; i++) {
    std::string base = "spotLights[" + std::to_string(i) + "]";
    spotLightPosLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".position").c_str());
    spotLightDirLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".direction").c_str());
    spotLightColLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".color").c_str());
    spotLightCutOffLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".cutOff").c_str());
    spotLightOuterCutOffLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".outerCutOff").c_str());
    spotLightRadLoc[i] =
        glGetUniformLocation(shaderProgram, (base + ".radius").c_str());
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
  bool weaponEnabled = true;
  bool weaponAutomatic = false;
  bool weaponTriggerWasDown = false;
  float weaponDamage = 35.0f;
  float weaponRange = 35.0f;
  float weaponFireInterval = 0.28f;
  float weaponCooldown = 0.0f;
  float weaponMuzzleFlashTimer = 0.0f;
  float weaponHitMarkerTimer = 0.0f;
  struct WeaponViewmodel {
    const char *name;
    GLTFModel *model;
    int idleAnimation;
    int walkAnimation;
    int runAnimation;
    int fireAnimation;
    int reloadAnimation;
    int armsIdleAnimation;
    int armsWalkAnimation;
    int armsRunAnimation;
    int armsFireAnimation;
    int armsReloadAnimation;
    glm::vec3 position;
    glm::vec3 rotation;
    float targetSize;
    float damage;
    float fireInterval;
    bool automatic;
    bool holdArmsAnimationAtEnd;
    std::vector<glm::mat4> bones;
  };
  auto findWeaponAnimation = [](GLTFModel *model,
                                std::initializer_list<const char *> names) {
    if (!model)
      return 0;
    for (const char *name : names) {
      int index = model->FindAnimationIndexContains(name);
      if (index >= 0)
        return index;
    }
    return 0;
  };
  std::vector<WeaponViewmodel> weapons = {
      {"Pistola", pistolViewmodel,
       findWeaponAnimation(pistolViewmodel, {"IdlePose", "BasePose"}),
       findWeaponAnimation(pistolViewmodel, {"Pistol_Walk"}),
       findWeaponAnimation(pistolViewmodel, {"Pistol_Walk"}),
       findWeaponAnimation(pistolViewmodel, {"Pistol_Aiming_Fire"}),
       findWeaponAnimation(pistolViewmodel, {"Pistol_IdlePose"}),
       findWeaponAnimation(pistolViewmodel, {"Arms_Draw"}),
       findWeaponAnimation(pistolViewmodel, {"Arms_Draw"}),
       findWeaponAnimation(pistolViewmodel, {"Arms_Draw"}),
       findWeaponAnimation(pistolViewmodel, {"Arms_Draw"}),
       findWeaponAnimation(pistolViewmodel, {"Arms_Draw"}),
       glm::vec3(0.18f, -0.38f, -0.72f), glm::vec3(0.0f), 0.9f, 35.0f, 0.28f,
       false, true},
      {"Rifle", rifleViewmodel,
       findWeaponAnimation(rifleViewmodel, {"Rifle_Breathing"}),
       findWeaponAnimation(rifleViewmodel, {"Rifle_Walk"}),
       findWeaponAnimation(rifleViewmodel, {"Rifle_Run"}),
       findWeaponAnimation(rifleViewmodel, {"Rifle_Breathing"}),
       findWeaponAnimation(rifleViewmodel, {"Rifle_BasePose"}),
       findWeaponAnimation(rifleViewmodel, {"Arms_BasePose"}),
       findWeaponAnimation(rifleViewmodel, {"Arms_BasePose"}),
       findWeaponAnimation(rifleViewmodel, {"Arms_BasePose"}),
       findWeaponAnimation(rifleViewmodel, {"Arms_BasePose"}),
       findWeaponAnimation(rifleViewmodel, {"Arms_Reload"}),
       glm::vec3(0.12f, -0.40f, -0.82f), glm::vec3(0.0f), 1.0f, 22.0f, 0.11f,
       true, false},
      {"Escopeta", shotgunViewmodel,
       findWeaponAnimation(shotgunViewmodel, {"shotgun01_BasePose"}),
       findWeaponAnimation(shotgunViewmodel, {"shotgun01_BasePose"}),
       findWeaponAnimation(shotgunViewmodel, {"shotgun01_BasePose"}),
       findWeaponAnimation(shotgunViewmodel, {"shotgun01_fire"}),
       findWeaponAnimation(shotgunViewmodel, {"shotgun01_ReloadStart"}),
       findWeaponAnimation(shotgunViewmodel, {"arms_basePose"}),
       findWeaponAnimation(shotgunViewmodel, {"arms_basePose"}),
       findWeaponAnimation(shotgunViewmodel, {"arms_basePose"}),
       findWeaponAnimation(shotgunViewmodel, {"arms_basePose"}),
       findWeaponAnimation(shotgunViewmodel, {"arms_ReloadStart"}),
       glm::vec3(0.10f, -0.42f, -0.88f), glm::vec3(0.0f), 1.05f, 70.0f, 0.75f,
       false, false}};
  int currentWeaponIndex = 0;
  int weaponAnimationIndex = weapons[0].idleAnimation;
  int weaponArmsAnimationIndex = weapons[0].armsIdleAnimation;
  float weaponAnimationTime = 0.0f;
  float weaponActionDuration = 0.0f;
  bool weaponActionPlaying = false;
  bool weaponCycleWasPressed = false;
  bool weaponReloadWasPressed = false;
  auto loadWeaponConfig = [&]() {
    std::ifstream config("assets/weapon_config.txt");
    int enabled = 1;
    int automatic = 0;
    if (config >> enabled >> automatic >> weaponDamage >> weaponRange >>
        weaponFireInterval) {
      weaponEnabled = enabled != 0;
      weaponAutomatic = automatic != 0;
    }
    int savedWeaponIndex = 0;
    if (config >> savedWeaponIndex) {
      currentWeaponIndex =
          glm::clamp(savedWeaponIndex, 0, static_cast<int>(weapons.size()) - 1);
      for (WeaponViewmodel &weapon : weapons) {
        config >> weapon.position.x >> weapon.position.y >> weapon.position.z >>
            weapon.rotation.x >> weapon.rotation.y >> weapon.rotation.z >>
            weapon.targetSize;
      }
      weaponDamage = weapons[currentWeaponIndex].damage;
      weaponFireInterval = weapons[currentWeaponIndex].fireInterval;
      weaponAutomatic = weapons[currentWeaponIndex].automatic;
    }
  };
  auto saveWeaponConfig = [&]() {
    std::ofstream config("assets/weapon_config.txt");
    config << (weaponEnabled ? 1 : 0) << " " << (weaponAutomatic ? 1 : 0) << " "
           << weaponDamage << " " << weaponRange << " " << weaponFireInterval
           << "\n";
    config << currentWeaponIndex << "\n";
    for (const WeaponViewmodel &weapon : weapons) {
      config << weapon.position.x << " " << weapon.position.y << " "
             << weapon.position.z << " " << weapon.rotation.x << " "
             << weapon.rotation.y << " " << weapon.rotation.z << " "
             << weapon.targetSize << "\n";
    }
  };
  loadWeaponConfig();

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
       36,
       8,
       wallTex2,
       wallTex5,
       floorTexture,
       {0.15f, 0.15f, 0.2f},
       true,
       true,
       false}, // Baños (cambia a tu textura de azulejo)
      {37,
       0,
       40,
       8,
       wallTex4,
       wallTex5,
       floorTexture,
       {0.15f, 0.15f, 0.2f},
       true,
       true,
       false}, // banos de girls

      {34,
       12,
       49,
       21,
       wallContencionTex,
       floorContencionTex,
       roofContencionTex,
       {1.0f, 1.0f, 1.0f},
       false,
       true,
       true}, // Area contencion

      {42,
       0,
       49,
       8,
       wallArchivoTex,
       floorArchivoTex,
       roofArchivoTex,
       {0.8f, 0.8f, 0.8f},
       true,
       true,
       true}, // Archivo Restringido

      {41,
       25,
       48,
       33,
       wallAscensorTex,
       floorTexture,
       roofAscensorTex,
       {0.8f, 0.8f, 0.8f},
       true,
       false,
       true}, // Ascensor

      {19,
       25,
       40,
       33,
       wallGeneradoresTex,
       floorGeneradoresTex,
       roofGeneradoresTex,
       {1.0f, 1.0f, 1.0f},
       true,
       true,
       true}, // Sala de Generadores (Seccion 1)

      {1,
       25,
       17,
       33,
       wallPruebasTex,
       floorPruebasTex,
       roofPruebasTex,
       {1.0f, 1.0f, 1.0f},
       true,
       true,
       true}, // Sala de Pruebas (Seccion 2)
      {0,
       0,
       13,
       9,
       wallDescansoTex,
       floorDescansoTex,
       roofDescansoTex,
       {0.25f, 0.25f, 0.25f},
       true,
       true,
       true}, // Sala de Descanso (cuarto superior izquierdo)

      {14,
       2,
       30,
       8,
       wallTex1,       // pared (sin usar: overrideWall=false)
       floorOficinasTex,
       floorTexture,   // techo (sin usar: overrideCeil=false)
       {0.3f, 0.3f, 0.3f},
       false,          // overrideWall: conserva las paredes actuales
       true,           // overrideFloor: aplica el piso de oficinas
       false},         // overrideCeil: conserva el techo actual
                       // Oficinas (sala central-superior, solo piso)

      {}, // pisos
  };

  auto getZone = [&](int gx, int gz) -> const RoomZone * {
    for (const auto &rz : roomZones)
      if (gx >= rz.x1 && gx <= rz.x2 && gz >= rz.z1 && gz <= rz.z2)
        return &rz;
    return nullptr;
  };

  // --- PASILLOS ---
  // Celdas de los corredores que conectan las distintas áreas. A estos se les
  // aplica el piso y el techo de la Sala de Descanso (floorDescansoTex /
  // roofDescansoTex). NO incluye la sala General (centro/arriba), que son
  // celdas abiertas pero pertenecen a una habitación, no a un pasillo.
  auto isPasillo = [&](int gx, int gz) -> bool {
    if (gz == 10 || gz == 11)
      return true; // pasillo horizontal superior (conecta áreas de arriba)
    if (gz == 22 || gz == 23)
      return true; // pasillo horizontal inferior (conecta salas de abajo)
    if ((gz == 0 || gz == 1) && gx >= 23 && gx <= 26)
      return true; // entrada superior
    if ((gx == 1 || gx == 2) && gz >= 12 && gz <= 24)
      return true; // corredor vertical izquierdo
    if (gx == 31 && gz >= 2 && gz <= 9)
      return true; // conector vertical (zona de baños)
    return false;
  };

  // --- STATIC MAP BATCHING ---
  struct MapBatch {
    unsigned int textureID;
    glm::vec3 baseColor;
    std::vector<float> vertices;
    unsigned int VAO, VBO;
  };
  std::vector<MapBatch> mapBatches;

  auto getBatch = [&](unsigned int tex, glm::vec3 color) -> MapBatch & {
    for (auto &b : mapBatches) {
      if (b.textureID == tex && abs(b.baseColor.x - color.x) < 0.01f &&
          abs(b.baseColor.y - color.y) < 0.01f &&
          abs(b.baseColor.z - color.z) < 0.01f) {
        return b;
      }
    }
    mapBatches.push_back({tex, color, {}, 0, 0});
    return mapBatches.back();
  };

  auto addFaceToBatch = [&](int faceIndex, unsigned int tex, glm::vec3 color,
                            glm::mat4 model) {
    MapBatch &batch = getBatch(tex, color);
    for (int i = 0; i < 6; i++) {
      int vIdx = faceIndex * 6 + i;
      glm::vec4 pos(vertices[vIdx * 8 + 0], vertices[vIdx * 8 + 1],
                    vertices[vIdx * 8 + 2], 1.0f);
      pos = model * pos;

      glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
      glm::vec3 normal(vertices[vIdx * 8 + 3], vertices[vIdx * 8 + 4],
                       vertices[vIdx * 8 + 5]);
      normal = glm::normalize(normalMatrix * normal);

      batch.vertices.push_back(pos.x);
      batch.vertices.push_back(pos.y);
      batch.vertices.push_back(pos.z);
      batch.vertices.push_back(normal.x);
      batch.vertices.push_back(normal.y);
      batch.vertices.push_back(normal.z);
      batch.vertices.push_back(vertices[vIdx * 8 + 6]); // u
      batch.vertices.push_back(vertices[vIdx * 8 + 7]); // v
    }
  };

  auto buildStaticMapBatches = [&]() {
    for (int z = 0; z < MAP_HEIGHT; z++) {
      for (int x = 0; x < MAP_WIDTH; x++) {
        int blockType = worldMap[z][x];
        const RoomZone *zone = getZone(x, z);

        // Paredes
        if (blockType > 0 && blockType < 8 && blockType != 7) {
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

          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(
              model, glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f, (float)z));
          model = glm::scale(model, glm::vec3(scaleX, wallHeight, scaleZ));

          unsigned int baseTex = wallTex1;
          if (blockType == 1)
            baseTex = wallTex1;
          else if (blockType == 2)
            baseTex = wallTex2;
          else if (blockType == 3)
            baseTex = wallTex3;
          else if (blockType == 4)
            baseTex = wallContencionTex;

          if (zone && zone->overrideWall)
            baseTex = zone->wallTex;

          // Dibujar cada cara individualmente para permitir texturas mixtas en
          // paredes compartidas
          for (int f = 0; f < 6; f++) {
            unsigned int faceTex = baseTex;

            // Si es una cara lateral, preguntar al vecino qué textura quiere
            // "ver"
            if (f < 4) {
              int nx = x, nz = z;
              if (f == 0)
                nz--; // Back (-Z)
              else if (f == 1)
                nz++; // Front (+Z)
              else if (f == 2)
                nx--; // Left (-X)
              else if (f == 3)
                nx++; // Right (+X)

              if (nx >= 0 && nx < MAP_WIDTH && nz >= 0 && nz < MAP_HEIGHT) {
                // Solo cambiamos la textura si el vecino es aire (espacio
                // visible)
                if (worldMap[nz][nx] <= 0) {
                  const RoomZone *neighborZone = getZone(nx, nz);
                  if (neighborZone && neighborZone->overrideWall) {
                    faceTex = neighborZone->wallTex;
                  }
                }
              }
            }
            addFaceToBatch(f, faceTex, glm::vec3(1.0f, 1.0f, 1.0f), model);
          }
        }

        // Piso y Techo

        // Piso
        // Piso por defecto: la textura del área de descanso. Las áreas con
        // piso propio (overrideFloor=true) lo conservan y no se ven afectadas.
        unsigned int fTex = floorDescansoTex;
        if (zone && zone->overrideFloor)
          fTex = zone->floorTex;

        glm::vec3 fCol = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::mat4 floorModel = glm::mat4(1.0f);
        floorModel =
            glm::translate(floorModel, glm::vec3((float)x, -1.0f, (float)z));
        // El piso es la cara superior de un cubo enterrado, o simplemente
        // usamos addFaceToBatch con f=5
        addFaceToBatch(5, fTex, fCol, floorModel);

        // Techo
        unsigned int cTex = floorTexture;
        glm::vec3 cCol = glm::vec3(0.3f, 0.3f, 0.3f);
        if (zone && zone->overrideCeil) {
          cCol = zone->ceilColor;
          cTex = zone->ceilTex;
        } else if (!zone && isPasillo(x, z)) {
          cTex = roofDescansoTex; // techo de descanso en los pasillos
          cCol = glm::vec3(0.25f, 0.25f, 0.25f);
        }

        glm::mat4 roofModel = glm::mat4(1.0f);
        roofModel = glm::translate(roofModel,
                                   glm::vec3((float)x, wallHeight, (float)z));
        // El techo es la cara inferior (f=4)
        addFaceToBatch(4, cTex, cCol, roofModel);
      }
    }

    // Bind batches
    for (auto &b : mapBatches) {
      glGenVertexArrays(1, &b.VAO);
      glGenBuffers(1, &b.VBO);
      glBindVertexArray(b.VAO);
      glBindBuffer(GL_ARRAY_BUFFER, b.VBO);
      glBufferData(GL_ARRAY_BUFFER, b.vertices.size() * sizeof(float),
                   b.vertices.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void *)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void *)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void *)(6 * sizeof(float)));
      glEnableVertexAttribArray(2);
    }
    glBindVertexArray(0);
  };
  buildStaticMapBatches();
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    deltaTime = (std::min)(currentFrame - lastFrame, 0.05f);
    lastFrame = currentFrame;

    if (hudMessageTimer > 0.0f) {
      hudMessageTimer -= deltaTime;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    processInput(window);
    weaponCooldown = (std::max)(0.0f, weaponCooldown - deltaTime);
    weaponMuzzleFlashTimer =
        (std::max)(0.0f, weaponMuzzleFlashTimer - deltaTime);
    weaponHitMarkerTimer = (std::max)(0.0f, weaponHitMarkerTimer - deltaTime);

    bool cycleWeaponDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    if (cycleWeaponDown && !weaponCycleWasPressed && gameState == PLAYING) {
      currentWeaponIndex =
          (currentWeaponIndex + 1) % static_cast<int>(weapons.size());
      WeaponViewmodel &weapon = weapons[currentWeaponIndex];
      weaponDamage = weapon.damage;
      weaponFireInterval = weapon.fireInterval;
      weaponAutomatic = weapon.automatic;
      weaponAnimationIndex = weapon.idleAnimation;
      weaponArmsAnimationIndex = weapon.armsIdleAnimation;
      weaponAnimationTime = 0.0f;
      weaponActionPlaying = false;
    }
    weaponCycleWasPressed = cycleWeaponDown;

    bool reloadWeaponDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (reloadWeaponDown && !weaponReloadWasPressed && gameState == PLAYING &&
        weaponEnabled) {
      WeaponViewmodel &weapon = weapons[currentWeaponIndex];
      weaponAnimationIndex = weapon.reloadAnimation;
      weaponArmsAnimationIndex = weapon.armsReloadAnimation;
      weaponAnimationTime = 0.0f;
      weaponActionDuration =
          weapon.model ? (std::max)(weapon.model->GetAnimationLengthSeconds(
                                        weaponAnimationIndex),
                                    weapon.model->GetAnimationLengthSeconds(
                                        weaponArmsAnimationIndex))
                       : 0.0f;
      weaponActionPlaying =
          weaponActionDuration > 0.02f &&
          (weapon.reloadAnimation != weapon.idleAnimation ||
           weapon.armsReloadAnimation != weapon.armsIdleAnimation);
    }
    weaponReloadWasPressed = reloadWeaponDown;

    bool weaponTriggerDown =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool weaponCanFire =
        weaponEnabled && gameState == PLAYING && !isReadingDocument &&
        isCursorLocked && !ImGui::GetIO().WantCaptureMouse &&
        weaponCooldown <= 0.0f &&
        (weaponAutomatic ? weaponTriggerDown
                         : weaponTriggerDown && !weaponTriggerWasDown);
    if (weaponCanFire) {
      WeaponViewmodel &weapon = weapons[currentWeaponIndex];
      weaponCooldown = weaponFireInterval;
      weaponMuzzleFlashTimer = 0.08f;
      if (currentWeaponIndex == 2 && shotgunFireSoundReady) {
        ma_sound_stop(&shotgunFireSound);
        ma_sound_seek_to_pcm_frame(&shotgunFireSound, 0);
        ma_sound_start(&shotgunFireSound);
      }
      weaponAnimationIndex = weapon.fireAnimation;
      weaponArmsAnimationIndex = weapon.armsFireAnimation;
      weaponAnimationTime = 0.0f;
      weaponActionDuration =
          weapon.model ? (std::max)(weapon.model->GetAnimationLengthSeconds(
                                        weaponAnimationIndex),
                                    weapon.model->GetAnimationLengthSeconds(
                                        weaponArmsAnimationIndex))
                       : 0.0f;
      weaponActionPlaying =
          weaponActionDuration > 0.02f &&
          (weapon.fireAnimation != weapon.idleAnimation ||
           weapon.armsFireAnimation != weapon.armsIdleAnimation);
      if (animatedEntities.ShootRay(cameraPos, cameraFront, weaponRange,
                                    weaponDamage)) {
        weaponHitMarkerTimer = 0.16f;
      }
    }
    weaponTriggerWasDown = weaponTriggerDown;
    WeaponViewmodel &activeWeapon = weapons[currentWeaponIndex];
    if (weaponActionPlaying) {
      weaponAnimationTime += deltaTime;
      if (weaponAnimationTime >= weaponActionDuration) {
        weaponActionPlaying = false;
        weaponAnimationTime = 0.0f;
      }
    } else {
      weaponAnimationIndex = isSprinting
                                 ? activeWeapon.runAnimation
                                 : (isMoving ? activeWeapon.walkAnimation
                                             : activeWeapon.idleAnimation);
      weaponArmsAnimationIndex =
          isSprinting ? activeWeapon.armsRunAnimation
                      : (isMoving ? activeWeapon.armsWalkAnimation
                                  : activeWeapon.armsIdleAnimation);
      weaponAnimationTime += deltaTime;
    }

    animatedEntities.Update(deltaTime, cameraPos, cameraFront,
                            interactionPressedThisFrame,
                            gameState == PLAYING && !isReadingDocument);
    for (const std::string &message : animatedEntities.ConsumeMessages()) {
      printTypewriter(getText(message));
    }

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
    glUniform1i(flashlightOnLoc,
                (isFlashlightOn && selectedHotbarSlot == 0) ? 1 : 0);

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
    glUniform3f(pointLightColLoc[0], 0.6f * flicker1, 0.45f * flicker1,
                0.15f * flicker1);
    glUniform1f(pointLightRadLoc[0], 3.5f);

    // Lámpara 2
    glUniform3fv(pointLightPosLoc[1], 1, glm::value_ptr(ligthbathroom2Pos));
    glUniform3f(pointLightColLoc[1], 0.6f * flicker2, 0.45f * flicker2,
                0.15f * flicker2);
    glUniform1f(pointLightRadLoc[1], 3.5f);

    // Lámpara 3 (baño)
    glUniform3fv(pointLightPosLoc[2], 1, glm::value_ptr(lamp3Pos));
    glUniform3f(pointLightColLoc[2], 0.6f * flicker3, 0.45f * flicker3,
                0.15f * flicker3);
    glUniform1f(pointLightRadLoc[2], 3.5f);

    // Lámpara 4 (baño)
    glUniform3fv(pointLightPosLoc[3], 1, glm::value_ptr(lamp4Pos));
    glUniform3f(pointLightColLoc[3], 0.6f * flicker4, 0.45f * flicker4,
                0.15f * flicker4);
    glUniform1f(pointLightRadLoc[3], 3.5f);

    // lampara contencion 1
    glUniform3fv(pointLightPosLoc[4], 1, glm::value_ptr(lamparaContencionPos));
    glUniform3f(pointLightColLoc[4], 0.6f * lampFlicker, 0.7f * lampFlicker,
                0.8f * lampFlicker);
    glUniform1f(pointLightRadLoc[4], 4.0f);

    // lampara contencion 2 (tenue)
    glUniform3fv(pointLightPosLoc[5], 1, glm::value_ptr(lampara2Pos));
    glUniform3f(pointLightColLoc[5], 0.3f * lampFlicker, 0.35f * lampFlicker,
                0.4f * lampFlicker);
    glUniform1f(pointLightRadLoc[5], 3.0f);

    // lampara contencion 3 (tenue)
    glUniform3fv(pointLightPosLoc[6], 1, glm::value_ptr(lampara3Pos));
    glUniform3f(pointLightColLoc[6], 0.3f * lampFlicker, 0.35f * lampFlicker,
                0.4f * lampFlicker);
    glUniform1f(pointLightRadLoc[6], 3.0f);

    // --- LUCES DINAMICAS PARA PROPS DEL EDITOR (Indices 7-10 y 14-31) ---
    std::vector<int> dynamicSlots = {7, 8, 9, 10, 14, 15};

    struct LightProp {
      const PlacedProp *prop;
      float distSq;
    };
    std::vector<LightProp> emittingProps;
    for (const auto &prop : placedProps) {
      if (prop.modelName == "emergency" || prop.modelName == "ligthbathroom") {
        float d2 = glm::distance2(cameraPos, prop.pos);
        if (d2 <= 18.0f * 18.0f)
          emittingProps.push_back({&prop, d2});
      }
    }
    std::sort(emittingProps.begin(), emittingProps.end(),
              [](const LightProp &a, const LightProp &b) {
                return a.distSq < b.distSq;
              });

    std::map<const PlacedProp *, int> assignedPointSlots;
    int currentSlotIdx = 0;
    for (const auto &lp : emittingProps) {
      if (currentSlotIdx >= (int)dynamicSlots.size())
        break;

      const auto &prop = *lp.prop;
      int slot = dynamicSlots[currentSlotIdx];
      assignedPointSlots[&prop] = slot;
      bool isEmergency = (prop.modelName == "emergency");
      bool isBano = (prop.modelName == "ligthbathroom");

      if (isEmergency) {
        glUniform3fv(pointLightPosLoc[slot], 1, glm::value_ptr(prop.pos));
        float intensity = 0.25f + 0.35f * emergencyPulse;
        glUniform3f(pointLightColLoc[slot], intensity, 0.0f, 0.0f);
        glUniform1f(pointLightRadLoc[slot], 3.5f);
        currentSlotIdx++;
      } else if (isBano) {
        float f = getFlicker(currentFrame, slot * 7.13f);
        glUniform3fv(pointLightPosLoc[slot], 1, glm::value_ptr(prop.pos));
        glUniform3f(pointLightColLoc[slot], 0.6f * f, 0.45f * f, 0.15f * f);
        glUniform1f(pointLightRadLoc[slot], 3.5f);
        currentSlotIdx++;
      }
    }
    for (int i = currentSlotIdx; i < (int)dynamicSlots.size(); i++) {
      int slot = dynamicSlots[i];
      glUniform3f(pointLightColLoc[slot], 0.0f, 0.0f, 0.0f);
      glUniform1f(pointLightRadLoc[slot], 0.0f);
    }

    // --- LUZ DE LA TESLA (Indice 11) ---
    float teslaPulse = 0.5f + 0.5f * sin(currentFrame * 20.0f);
    glUniform3fv(pointLightPosLoc[11], 1, glm::value_ptr(teslaPos));
    glUniform3f(pointLightColLoc[11], 0.2f * teslaPulse, 0.4f * teslaPulse,
                1.0f * teslaPulse);
    glUniform1f(pointLightRadLoc[11], 4.0f);

    // --- LUCES PARPADEANTES SALA DE DESCANSO (estilo baño, indices 12 y 13)
    // ---
    float flickerDescanso1 = getFlicker(currentFrame, 7.3f);
    float flickerDescanso2 = getFlicker(currentFrame, 33.9f);

    glUniform3fv(pointLightPosLoc[12], 1, glm::value_ptr(luzDescanso1Pos));
    glUniform3f(pointLightColLoc[12], 0.6f * flickerDescanso1,
                0.45f * flickerDescanso1, 0.15f * flickerDescanso1);
    glUniform1f(pointLightRadLoc[12], 4.0f);

    glUniform3fv(pointLightPosLoc[13], 1, glm::value_ptr(luzDescanso2Pos));
    glUniform3f(pointLightColLoc[13], 0.6f * flickerDescanso2,
                0.45f * flickerDescanso2, 0.15f * flickerDescanso2);
    glUniform1f(pointLightRadLoc[13], 4.0f);

    // Los indices fijos llegan hasta 13; solo enviamos los dinamicos cercanos.
    int highestPointLightSlot =
        currentSlotIdx > 0 ? dynamicSlots[currentSlotIdx - 1] : 13;
    glUniform1i(numPointLightsLoc, (std::max)(14, highestPointLightSlot + 1));

    // --- SPOTLIGHTS (Max 16) ---
    int spotIdx = 0;

    // 1. LÁMPARAS DE REACTOR (4 fijas - slots 0-3)
    glm::vec3 lp[4] = {lamparaReactorPos, lamparaReactorPos2,
                       lamparaReactorPos3, lamparaReactorPos4};
    glm::vec3 lr[4] = {lamparaReactorRot, lamparaReactorRot2,
                       lamparaReactorRot3, lamparaReactorRot4};
    for (int i = 0; i < 4; i++) {
      glUniform3fv(spotLightPosLoc[spotIdx], 1, glm::value_ptr(lp[i]));

      glm::mat4 rotMat = glm::mat4(1.0f);
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].x),
                           glm::vec3(1.0f, 0.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].y),
                           glm::vec3(0.0f, 1.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(lr[i].z),
                           glm::vec3(0.0f, 0.0f, 1.0f));
      glm::vec3 dir = glm::normalize(
          glm::vec3(rotMat * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));

      glUniform3fv(spotLightDirLoc[spotIdx], 1, glm::value_ptr(dir));
      glUniform3f(spotLightColLoc[spotIdx], 1.0f, 0.9f, 0.6f); // Warm yellow
      glUniform1f(spotLightCutOffLoc[spotIdx], glm::cos(glm::radians(25.0f)));
      glUniform1f(spotLightOuterCutOffLoc[spotIdx],
                  glm::cos(glm::radians(35.0f)));
      glUniform1f(spotLightRadLoc[spotIdx], 6.0f);
      spotIdx++;
    }

    // 2. SPOTLIGHTS DINAMICOS (Mini-lamparas y Emergencia - slots 4-15)
    struct SpotProp {
      const PlacedProp *prop;
      float distSq;
      bool isMini;
    };
    std::vector<SpotProp> dynamicSpots;
    for (const auto &prop : placedProps) {
      if (prop.modelName == "mini-lampara" || prop.modelName == "emergency") {
        float d2 = glm::distance2(cameraPos, prop.pos);
        if (d2 <= 18.0f * 18.0f)
          dynamicSpots.push_back(
              {&prop, d2, (prop.modelName == "mini-lampara")});
      }
    }
    std::sort(dynamicSpots.begin(), dynamicSpots.end(),
              [](const SpotProp &a, const SpotProp &b) {
                return a.distSq < b.distSq;
              });

    std::map<const PlacedProp *, float> miniLampFlickers;
    int miniLampCount = 0;
    for (const auto &sp : dynamicSpots) {
      if (spotIdx >= 10)
        break;
      const auto &prop = *sp.prop;

      glm::mat4 rotMat = glm::mat4(1.0f);
      rotMat = glm::rotate(rotMat, glm::radians(prop.rot.x),
                           glm::vec3(1.0f, 0.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(prop.rot.y),
                           glm::vec3(0.0f, 1.0f, 0.0f));
      rotMat = glm::rotate(rotMat, glm::radians(prop.rot.z),
                           glm::vec3(0.0f, 0.0f, 1.0f));

      if (sp.isMini) {
        glUniform3fv(spotLightPosLoc[spotIdx], 1,
                     glm::value_ptr(prop.pos + glm::vec3(0.0f, 0.35f, 0.0f)));
        glm::vec3 dir = glm::normalize(
            glm::vec3(rotMat * glm::vec4(0.0f, -0.7f, -1.0f, 0.0f)));
        glUniform3fv(spotLightDirLoc[spotIdx], 1, glm::value_ptr(dir));

        float f = 1.0f;
        if (miniLampCount < 2)
          f = getFlicker(currentFrame, miniLampCount * 23.4f);
        miniLampFlickers[&prop] = f;
        glUniform3f(spotLightColLoc[spotIdx], 0.7f * f, 0.65f * f, 0.5f * f);
        glUniform1f(spotLightCutOffLoc[spotIdx], glm::cos(glm::radians(35.0f)));
        glUniform1f(spotLightOuterCutOffLoc[spotIdx],
                    glm::cos(glm::radians(50.0f)));
        glUniform1f(spotLightRadLoc[spotIdx], 2.5f);
        miniLampCount++;
      } else {
        // Emergency
        glm::vec3 dir = glm::normalize(
            glm::vec3(rotMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        glm::vec3 offsetPos = prop.pos + dir * 0.15f;
        glUniform3fv(spotLightPosLoc[spotIdx], 1, glm::value_ptr(offsetPos));
        glUniform3fv(spotLightDirLoc[spotIdx], 1, glm::value_ptr(dir));
        float spotInt = 0.25f + 0.35f * emergencyPulse;
        glUniform3f(spotLightColLoc[spotIdx], spotInt, 0.0f, 0.0f);
        glUniform1f(spotLightCutOffLoc[spotIdx], glm::cos(glm::radians(35.0f)));
        glUniform1f(spotLightOuterCutOffLoc[spotIdx],
                    glm::cos(glm::radians(50.0f)));
        glUniform1f(spotLightRadLoc[spotIdx], 3.5f);
      }
      spotIdx++;
    }

    glUniform1i(numSpotLightsLoc, spotIdx);

    // Limpiar uniformes no usados
    for (int i = spotIdx; i < 16; i++) {
      glUniform3f(spotLightColLoc[i], 0.0f, 0.0f, 0.0f);
      glUniform1f(spotLightRadLoc[i], 0.0f);
    }

    // --- CULLING HELPER ---
    glm::vec2 camDir2D =
        glm::length(glm::vec2(cameraFront.x, cameraFront.z)) > 0.001f
            ? glm::normalize(glm::vec2(cameraFront.x, cameraFront.z))
            : glm::vec2(1, 0);
    auto shouldRender = [&](float x, float z, float radius = 2.0f) -> bool {
      glm::vec2 objPos(x, z);
      glm::vec2 dir = objPos - glm::vec2(cameraPos.x, cameraPos.z);
      float dist = glm::length(dir);
      constexpr float kPropRenderDistance = 18.0f;
      if (dist > kPropRenderDistance + radius)
        return false;
      if (dist > radius + 3.0f) {
        dir /= dist;
        if (glm::dot(camDir2D, dir) < -0.4f)
          return false;
      }
      return true;
    };

    // --- DIBUJAR MAPA BATCHEADO (Optimizado) ---
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(solidColorLoc, 0);
    glUniform1f(emissiveStrengthLoc, 0.0f);
    for (auto &b : mapBatches) {
      glBindVertexArray(b.VAO);
      glBindTexture(GL_TEXTURE_2D, b.textureID);
      if (dimensionAlterna)
        glUniform3f(colorLoc, 0.4f, 0.1f, 0.1f);
      else
        glUniform3f(colorLoc, b.baseColor.x, b.baseColor.y, b.baseColor.z);

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(glm::mat4(1.0f)));
      glDrawArrays(GL_TRIANGLES, 0, (GLsizei)b.vertices.size() / 8);
    }

    // --- MAPA (Solo objetos dinamicos/puertas) ---
    for (int z = 0; z < MAP_HEIGHT; z++) {
      for (int x = 0; x < MAP_WIDTH; x++) {
        if (!shouldRender((float)x, (float)z, 1.5f))
          continue;
        int blockType = worldMap[z][x];
        const RoomZone *zone = getZone(x, z);

        // Solo procesamos puertas en el bucle dinámico
        if (blockType != 7 && blockType != 8 && blockType != 9 &&
            blockType != 10 && blockType != 11 &&
            blockType != -7 && blockType != -8 && blockType != -9 &&
            blockType != -10 && blockType != -11)
          continue;

        // Consideramos la puerta visible tanto si esta cerrada (>0) como
        // abierta (<0)
        int renderBlock = worldMap[z][x];
        if (renderBlock != 0 && (blockType > 0 || renderBlock == -7 ||
                                 renderBlock == -8 || renderBlock == -9 ||
                                 renderBlock == -10 || renderBlock == -11)) {
          bool is3DDoor =
              (renderBlock == 7 || renderBlock == 8 || renderBlock == 9 ||
               renderBlock == 10 || renderBlock == 11 ||
               renderBlock == -7 || renderBlock == -8 || renderBlock == -9 ||
               renderBlock == -10 || renderBlock == -11);

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
            if (renderBlock == 11 || renderBlock == -11) {
              baseModel = glm::translate(
                  baseModel, glm::vec3(40.901f, -0.500f, 29.467f));
            } else {
              baseModel = glm::translate(
                  baseModel, glm::vec3((float)x + 0.5f, -0.5f, (float)z));
            }

            // 3. Escalar para encajar en el juego.
            // Ancho de ensamble: 1.81 -> Juego: 2.0 (Escala 1.1)
            // Alto Blender: 1.535 -> Juego: 1.0 (Escala 0.651)
            baseModel = glm::scale(baseModel, glm::vec3(1.1f, 0.651f, 1.1f));

            // 2. Rotar (dejaremos 0 grados asumiendo que los nuevos estan de
            // frente) Si se ven las texturas por detras, cambiaremos este valor
            // a 180 despues.
            if (renderBlock == 11 || renderBlock == -11) {
              baseModel = glm::rotate(baseModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }

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
              } else if (renderBlock == 10 || renderBlock == -10) {
                glUniform3f(colorLoc, 0.1f, 0.1f, 0.9f); // Metal Azul
              } else if (renderBlock == 11 || renderBlock == -11) {
                glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f); // Metal Negro/Gris Oscuro
              } else {
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
              }
            } else {
              glUniform1i(solidColorLoc, 1);
              if (renderBlock == 8 || renderBlock == -8) {
                glUniform3f(colorLoc, 1.0f, 0.8f, 0.2f); // Amarillo SÃ³lido
              } else if (renderBlock == 9 || renderBlock == -9) {
                glUniform3f(colorLoc, 0.9f, 0.1f, 0.1f); // Rojo SÃ³lido
              } else if (renderBlock == 10 || renderBlock == -10) {
                glUniform3f(colorLoc, 0.1f, 0.1f, 0.9f); // Azul SÃ³lido
              } else if (renderBlock == 11 || renderBlock == -11) {
                glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f); // Negro SÃ³lido
              } else {
                glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
              }
            }

            // Obtener Ã¡ngulo actual de la animaciÃ³n
            float currentAnim = 0.0f;
            if (renderBlock == 8 || renderBlock == -8) {
              currentAnim = door1Anim;
            } else if (renderBlock == 9 || renderBlock == -9) {
              currentAnim = door2Anim;
            } else if (renderBlock == 7 || renderBlock == -7 || renderBlock == 10 || renderBlock == -10 || renderBlock == 11 || renderBlock == -11) {
              int key = z * MAP_WIDTH + x;
              auto it = activeDoorsAnim.find(key);
              if (it != activeDoorsAnim.end()) {
                currentAnim = it->second;
              } else if (renderBlock < 0) {
                currentAnim = 90.0f; // Abierta si es negativo
              } else {
                currentAnim = 0.0f;
              }
            }

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
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                               glm::value_ptr(gnomeModel));
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
    animatedEntities.Render(shaderProgram, modelLoc, solidColorLoc, colorLoc,
                            isAnimatedLoc, finalBonesLoc, cameraPos);

    glActiveTexture(GL_TEXTURE0);
    if (isAnimatedLoc >= 0)
      glUniform1i(isAnimatedLoc, 0);
    glUniform1i(solidColorLoc, 0);
    glUniform3f(colorLoc, 1.0f, 1.0f,
                1.0f); // MUY IMPORTANTE: Resetear color que pudo dejar el techo
                       // oscuro del mapa
    glBindVertexArray(VAO);

    /*
    if (banoGLTF && !banoGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;
      glm::vec3 positions[8] = {banoPos,  banoPos2, banoPos3, banoPos4,
                                banoPos5, banoPos6, banoPos7, banoPos8};
      glm::vec3 rotations[8] = {banoRot,  banoRot2, banoRot3, banoRot4,
                                banoRot5, banoRot6, banoRot7, banoRot8};
      glm::vec3 scales[8] = {banoScale,  banoScale2, banoScale3, banoScale4,
                             banoScale5, banoScale6, banoScale7, banoScale8};
      for (int i = 0; i < 8; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }
      if (!instanceModels.empty())
        banoGLTF->DrawInstanced(shaderProgram, solidColorLoc, instanceModels);
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
      if (shouldRender(mensBpos.x, mensBpos.z, 3.0f))
        mensBGLTF->Draw(shaderProgram, solidColorLoc);
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
      if (shouldRender(azulejoPos.x, azulejoPos.z, 3.0f))
        azulejoGLTF->Draw(shaderProgram, solidColorLoc);
    }

    if (lavamanosGLTF && !lavamanosGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;
      glm::vec3 positions[8] = {lavamanosPos,  lavamanosPos2, lavamanosPos3,
                                lavamanosPos4, lavamanosPos5, lavamanosPos6,
                                lavamanosPos7, lavamanosPos8};
      glm::vec3 rotations[8] = {lavamanosRot,  lavamanosRot2, lavamanosRot3,
                                lavamanosRot4, lavamanosRot5, lavamanosRot6,
                                lavamanosRot7, lavamanosRot8};
      glm::vec3 scales[8] = {lavamanosScale,  lavamanosScale2, lavamanosScale3,
                             lavamanosScale4, lavamanosScale5, lavamanosScale6,
                             lavamanosScale7, lavamanosScale8};
      for (int i = 0; i < 8; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }
      if (!instanceModels.empty())
        lavamanosGLTF->DrawInstanced(shaderProgram, solidColorLoc,
                                     instanceModels);
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
      if (shouldRender(mirrorPos.x, mirrorPos.z, 3.0f))
        mirrorGLTF->Draw(shaderProgram, solidColorLoc);

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
      if (shouldRender(mirrorPos2.x, mirrorPos2.z, 3.0f))
        mirrorGLTF->Draw(shaderProgram, solidColorLoc);

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
      if (shouldRender(mirrorPos3.x, mirrorPos3.z, 3.0f))
        mirrorGLTF->Draw(shaderProgram, solidColorLoc);

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
      if (shouldRender(mirrorPos4.x, mirrorPos4.z, 3.0f))
        mirrorGLTF->Draw(shaderProgram, solidColorLoc);
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
      if (shouldRender(girlBpos.x, girlBpos.z, 3.0f))
        girlBGLTF->Draw(shaderProgram, solidColorLoc);
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
                  1.0f * flicker1); // Hacer que brille la lampara
      if (shouldRender(ligthbathroomPos.x, ligthbathroomPos.z, 3.0f))
        ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
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
                  1.0f * flicker3); // Hacer que brille la lampara
      if (shouldRender(lamp3Pos.x, lamp3Pos.z, 3.0f))
        ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
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
                  1.0f * flicker4); // Hacer que brille la lampara
      if (shouldRender(lamp4Pos.x, lamp4Pos.z, 3.0f))
        ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

      // Luz de descanso 1 (sala de sofas)
      glm::mat4 luzDescanso1Model = glm::mat4(1.0f);
      luzDescanso1Model = glm::translate(luzDescanso1Model, luzDescanso1Pos);
      luzDescanso1Model =
          glm::rotate(luzDescanso1Model, glm::radians(luzDescanso1Rot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      luzDescanso1Model =
          glm::rotate(luzDescanso1Model, glm::radians(luzDescanso1Rot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      luzDescanso1Model =
          glm::rotate(luzDescanso1Model, glm::radians(luzDescanso1Rot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      luzDescanso1Model = glm::scale(luzDescanso1Model, luzDescanso1Scale);
      luzDescanso1Model = glm::translate(
          luzDescanso1Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(luzDescanso1Model));
      glUniform1f(emissiveStrengthLoc, 1.0f * flickerDescanso1);
      if (shouldRender(luzDescanso1Pos.x, luzDescanso1Pos.z, 3.0f))
    ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f); // Resetear

      // Luz de descanso 2 (sala de sofas)
      glm::mat4 luzDescanso2Model = glm::mat4(1.0f);
      luzDescanso2Model = glm::translate(luzDescanso2Model, luzDescanso2Pos);
      luzDescanso2Model =
          glm::rotate(luzDescanso2Model, glm::radians(luzDescanso2Rot.x),
                      glm::vec3(1.0f, 0.0f, 0.0f));
      luzDescanso2Model =
          glm::rotate(luzDescanso2Model, glm::radians(luzDescanso2Rot.y),
                      glm::vec3(0.0f, 1.0f, 0.0f));
      luzDescanso2Model =
          glm::rotate(luzDescanso2Model, glm::radians(luzDescanso2Rot.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));
      luzDescanso2Model = glm::scale(luzDescanso2Model, luzDescanso2Scale);
      luzDescanso2Model = glm::translate(
          luzDescanso2Model, glm::vec3(-0.423f, -2.7725f, 2.622f));
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                         glm::value_ptr(luzDescanso2Model));
      glUniform1f(emissiveStrengthLoc, 1.0f * flickerDescanso2);
      if (shouldRender(luzDescanso2Pos.x, luzDescanso2Pos.z, 3.0f))
    ligthbathroomGLTF->Draw(shaderProgram, solidColorLoc);
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
                  1.0f * flicker2); // Hacer que brille la lampara
      if (shouldRender(ligthbathroom2Pos.x, ligthbathroom2Pos.z, 3.0f))
        ligthbathroom2GLTF->Draw(shaderProgram, solidColorLoc);
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
      if (shouldRender(mirrorBGpos.x, mirrorBGpos.z, 3.0f))
        mirrorBGGLTF->Draw(shaderProgram, solidColorLoc);
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
      if (shouldRender(urinarioPos.x, urinarioPos.z, 3.0f))
        urinarioGLTF->Draw(shaderProgram, solidColorLoc);
    }
    */

    // --- BUCLE DINAMICO DE RENDERING DE PROPS ---
    int renderSlotIdx = 0;
    int miniLampDrawCount = 0;
    int cameraAnimCount = 0;
    int ghostDrawCount = 0;
    std::map<GLTFModel *, std::vector<glm::mat4>> propInstanceBatches;
    for (const auto &prop : placedProps) {
      GLTFModel *model = modelRegistry[prop.modelName];
      if (!model || model->meshes.empty())
        continue;

      // --- aparicion y desaparicion de fantasmas
      if (prop.modelName == "ghost") {
        ghostDrawCount++;
        float hiddenDuration = 2.5f + (ghostDrawCount % 3) * 1.5f; // 2.5s, 4.0s, 5.5s
        float visibleDuration = 0.6f + (ghostDrawCount % 3) * 0.4f; // 0.6s, 1.0s, 1.4s
        float totalCycle = hiddenDuration + visibleDuration;

        float offset = ghostDrawCount * 11.23f; // Desfase para desincronizarlos
        float cycleTime = fmod(currentFrame + offset, totalCycle);

        if (cycleTime < hiddenDuration) {
          continue; // Salta el renderizado de este fantasma en este frame
        }
      }

      bool esLuzBano = (prop.modelName == "ligthbathroom");
      bool esEmergency = (prop.modelName == "emergency");

      int associatedSlot = -1;
      if (esLuzBano || esEmergency) {
        if (renderSlotIdx < (int)dynamicSlots.size()) {
          associatedSlot = dynamicSlots[renderSlotIdx];
          renderSlotIdx++;
        }
      }

      if (!shouldRender(prop.pos.x, prop.pos.z, 3.0f))
        continue;

      glm::mat4 pModel = glm::mat4(1.0f);
      pModel = glm::translate(pModel, prop.pos);
      pModel = glm::rotate(pModel, glm::radians(prop.rot.x),
                           glm::vec3(1.0f, 0.0f, 0.0f));

      float finalRotY = prop.rot.y;
      if (prop.modelName == "camara") {
        if (cameraAnimCount < 3) {
          // Paneo suave de +-40 grados desincronizado por posicion X
          finalRotY += 40.0f * sin(currentFrame * 0.75f + prop.pos.x);
        }
        cameraAnimCount++;
      }

      pModel = glm::rotate(pModel, glm::radians(finalRotY),
                           glm::vec3(0.0f, 1.0f, 0.0f));
      pModel = glm::rotate(pModel, glm::radians(prop.rot.z),
                           glm::vec3(0.0f, 0.0f, 1.0f));
      pModel = glm::scale(pModel, prop.scale);

      // El modelo de luz (lampara baño) trae un offset de pivote de Blender y
      // debe brillar con el parpadeo, igual que las lamparas fijas.
      bool esLamparaReactor = (prop.modelName == "lampara-reactor");
      bool esMiniLampara = (prop.modelName == "mini-lampara");
      float lampGlow = 0.0f;
      float miniLampFlicker = 1.0f;

      if (esLuzBano) {
        pModel = glm::translate(pModel, glm::vec3(-0.423f, -2.7725f, 2.622f));
        // Mismo parpadeo que la luz puntual asociada (mismo offset por indice)
        if (associatedSlot >= 0 && associatedSlot < 32)
          lampGlow = getFlicker(currentFrame, associatedSlot * 7.13f);
      }

      if (esMiniLampara) {
        if (miniLampDrawCount < 2) {
          miniLampFlicker = getFlicker(currentFrame, miniLampDrawCount * 23.4f);
        }
        miniLampDrawCount++;
      }

      bool hasAnims = (model->m_Scene && model->m_Scene->HasAnimations());
      bool needsIndividualDraw = esLuzBano || esLamparaReactor ||
                                 esMiniLampara || esEmergency || hasAnims;
      if (!needsIndividualDraw) {
        propInstanceBatches[model].push_back(pModel);
        continue;
      }

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pModel));
      if (esLuzBano)
        glUniform1f(emissiveStrengthLoc, 1.0f * lampGlow);
      if (esLamparaReactor)
        glUniform1f(emissiveStrengthLoc, 0.9f * lampFlicker);
      if (esMiniLampara)
        glUniform1f(emissiveStrengthLoc, 0.8f * miniLampFlicker);
      if (esEmergency)
        glUniform1f(emissiveStrengthLoc, 0.40f + 0.80f * emergencyPulse);

      bool esAscensor = (prop.modelName == "ascensor");
      if (esAscensor)
        glDisable(GL_CULL_FACE);

      float elevatorAnimTime = currentFrame;
      if (esAscensor) {
        float animDuration = model->GetAnimationLengthSeconds(0);
        if (animDuration > 0.001f) {
          float totalCycle =
              animDuration + 5.0f; // 5 segundos de espera cerradas
          float cycleTime = fmod(currentFrame, totalCycle);

          if (cycleTime > animDuration) {
            // Durante la espera, fijamos el tiempo al final de la animación
            // (puertas cerradas)
            elevatorAnimTime = animDuration - 0.01f;
          } else {
            elevatorAnimTime = cycleTime;
          }
        }
      }

      if (hasAnims) {
        model->DrawAnimated(elevatorAnimTime, 0, shaderProgram, modelLoc,
                            solidColorLoc, pModel);
      } else {
        model->Draw(shaderProgram, solidColorLoc);
      }

      if (esAscensor)
        glEnable(GL_CULL_FACE);

      glUniform1f(emissiveStrengthLoc, 0.0f);
    }

    glUniform1f(emissiveStrengthLoc, 0.0f);
    for (auto &[model, instanceModels] : propInstanceBatches) {
      model->DrawInstanced(shaderProgram, solidColorLoc, instanceModels);
    }
    //*------------------

    if (monitorGLTF && !monitorGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;

      glm::vec3 positions[2] = {monitorPos, monitor2Pos};
      glm::vec3 rotations[2] = {monitorRot, monitor2Rot};
      glm::vec3 scales[2] = {monitorScale, monitor2Scale};

      for (int i = 0; i < 2; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }

      if (!instanceModels.empty())
        monitorGLTF->DrawInstanced(shaderProgram, solidColorLoc,
                                   instanceModels);
    }

    if (deskGLTF && !deskGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;

      glm::vec3 positions[6] = {deskPos,  desk2Pos, desk3Pos,
                                desk4Pos, desk5Pos, desk6Pos};
      glm::vec3 rotations[6] = {deskRot,  desk2Rot, desk3Rot,
                                desk4Rot, desk5Rot, desk6Rot};
      glm::vec3 scales[6] = {deskScale,  desk2Scale, desk3Scale,
                             desk4Scale, desk5Scale, desk6Scale};

      for (int i = 0; i < 6; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }

      if (!instanceModels.empty())
        deskGLTF->DrawInstanced(shaderProgram, solidColorLoc, instanceModels);
    }

    if (estanteGLTF && !estanteGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;

      glm::vec3 positions[3] = {estantePos, estante2Pos, estante3Pos};
      glm::vec3 rotations[3] = {estanteRot, estante2Rot, estante3Rot};
      glm::vec3 scales[3] = {estanteScale, estante2Scale, estante3Scale};

      for (int i = 0; i < 3; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }

      if (!instanceModels.empty())
        estanteGLTF->DrawInstanced(shaderProgram, solidColorLoc,
                                   instanceModels);
    }

    if (sillitaGLTF && !sillitaGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;

      glm::vec3 positions[6] = {sillita1Pos, sillita2Pos, sillita3Pos,
                                sillita4Pos, sillita5Pos, sillita6Pos};
      glm::vec3 rotations[6] = {sillita1Rot, sillita2Rot, sillita3Rot,
                                sillita4Rot, sillita5Rot, sillita6Rot};
      glm::vec3 scales[6] = {sillita1Scale, sillita2Scale, sillita3Scale,
                             sillita4Scale, sillita5Scale, sillita6Scale};

      for (int i = 0; i < 6; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }

      if (!instanceModels.empty())
        sillitaGLTF->DrawInstanced(shaderProgram, solidColorLoc,
                                   instanceModels);
    }

    if (maquinaGLTF && !maquinaGLTF->meshes.empty()) {
      std::vector<glm::mat4> instanceModels;

      glm::vec3 positions[1] = {maquinaPos};
      glm::vec3 rotations[1] = {maquinaRot};
      glm::vec3 scales[1] = {maquinaScale};

      for (int i = 0; i < 1; i++) {
        if (shouldRender(positions[i].x, positions[i].z, 3.0f)) {
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, positions[i]);
          model = glm::rotate(model, glm::radians(rotations[i].x),
                              glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].y),
                              glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(rotations[i].z),
                              glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, scales[i]);
          instanceModels.push_back(model);
        }
      }

      if (!instanceModels.empty())
        maquinaGLTF->DrawInstanced(shaderProgram, solidColorLoc,
                                   instanceModels);
    }

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
        if (shouldRender(cablePisoPos[i].x, cablePisoPos[i].z, 3.0f))
          cablePisoGLTF->Draw(shaderProgram, solidColorLoc);
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
        if (shouldRender(cableTechoPos[i].x, cableTechoPos[i].z, 3.0f))
          cableTechoGLTF->Draw(shaderProgram, solidColorLoc);
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
      glUniform1f(emissiveStrengthLoc, 1.0f * lampFlicker);
      if (shouldRender(lamparaContencionPos.x, lamparaContencionPos.z, 3.0f))
        lamparaContencionGLTF->Draw(shaderProgram, solidColorLoc);
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
      glUniform1f(emissiveStrengthLoc, 0.6f * lampFlicker);
      if (shouldRender(lampara2Pos.x, lampara2Pos.z, 3.0f))
        lampara2GLTF->Draw(shaderProgram, solidColorLoc);
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
      glUniform1f(emissiveStrengthLoc, 0.6f * lampFlicker);
      if (shouldRender(lampara3Pos.x, lampara3Pos.z, 3.0f))
        lampara3GLTF->Draw(shaderProgram, solidColorLoc);
      glUniform1f(emissiveStrengthLoc, 0.0f);
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
        if (shouldRender(generadorPos[i].x, generadorPos[i].z, 3.0f))
          generadorGLTF->Draw(shaderProgram, solidColorLoc);
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
            entityModel, glm::vec3(0.0f, -0.06f, 0.0f)); // Subirla para verla
        entityModel =
            glm::rotate(entityModel, entity.seed, glm::vec3(0.0f, 1.0f, 0.0f));
        entityModel =
            glm::scale(entityModel, glm::vec3(1.0f, 1.0f, 1.0f)); // Agrandarla
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
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

        entityModel = glm::translate(
            entityModel, glm::vec3(0.0f, -0.1f, 0.0f)); // Bajar a la mesa
        entityModel = glm::rotate(entityModel, glm::radians(0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f)); // EstÃ¡tico
        entityModel = glm::scale(entityModel, glm::vec3(0.6f, 0.6f, 0.6f));
        if (dimensionAlterna)
          glUniform3f(colorLoc, 1.0f, 0.4f, 0.4f);
        else
          glUniform3f(colorLoc, 1.0f, 1.0f,
                      1.0f); // Usar colores 100% reales de Blender
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
        glDrawArrays(GL_TRIANGLES, 0, objVertexCount);

        glUniform1i(solidColorLoc, 0); // Restaurar texturas normales

        glBindVertexArray(VAO);      // Restaurar VAO
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
          entityModel = glm::rotate(entityModel, currentFrame * 2.0f,
                                    glm::vec3(1.0f, 1.0f, 1.0f));
          float pulse = 0.8f + 0.2f * sin(currentFrame * 15.0f);
          glUniform3f(colorLoc, pulse, 0.0f, 0.0f);
        } else {
          entityModel = glm::rotate(entityModel, currentFrame * 0.5f,
                                    glm::vec3(0.0f, 1.0f, 0.0f));
          glUniform3f(colorLoc, 0.2f, 0.5f, 1.0f);
        }
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));
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
            if (glm::length(cellCenter - glm::vec2(cameraPos.x, cameraPos.z)) >
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
                glm::vec3((float)x, (wallHeight - 1.0f) * 0.5f, (float)z));
            debugModel = glm::scale(
                debugModel,
                glm::vec3(scaleX + 0.02f, wallHeight + 0.02f, scaleZ + 0.02f));
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

          glm::vec3 scale = (entity.type == 4) ? glm::vec3(1.6f, 1.0f, 1.2f)
                                               : glm::vec3(1.2f, 2.2f, 1.2f);
          glm::mat4 debugModel = glm::mat4(1.0f);
          debugModel = glm::translate(
              debugModel,
              glm::vec3(entity.pos.x, entity.pos.y + 0.2f, entity.pos.z));
          debugModel = glm::scale(debugModel, scale);
          glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
          glUniform3f(colorLoc, 1.0f, 0.9f, 0.2f); // Amarillo para entidades
          glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 2. Helper lambda: dibuja las mismas cajas por submesh que usa la
        // colision real.
        auto drawModelHitbox = [&](GLTFModel *model, const glm::vec3 &pos,
                                   const glm::vec3 &rot, const glm::vec3 &scl,
                                   glm::vec3 hitboxColor =
                                       glm::vec3(0.1f, 0.75f, 1.0f)) {
          if (!model || model->meshes.empty())
            return;
          if (glm::length(glm::vec2(pos.x - cameraPos.x, pos.z - cameraPos.z)) >
              collisionViewerRadius)
            return;

          glm::mat4 mat = glm::mat4(1.0f);
          mat = glm::translate(mat, pos);
          mat = glm::rotate(mat, glm::radians(rot.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
          mat = glm::rotate(mat, glm::radians(rot.y),
                            glm::vec3(0.0f, 1.0f, 0.0f));
          mat = glm::rotate(mat, glm::radians(rot.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
          mat = glm::scale(mat, scl);

          // Dibujar como OBB: trasladamos y escalamos la matriz del modelo
          // usando el centro y tamaño local AABB!
          for (const auto &mesh : model->meshes) {
            glm::vec3 localCenter =
                (mesh.localAABB.min + mesh.localAABB.max) * 0.5f;
            glm::vec3 localSize = mesh.localAABB.max - mesh.localAABB.min;

            glm::mat4 debugModel = glm::translate(mat, localCenter);
            debugModel = glm::scale(debugModel, localSize);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                               glm::value_ptr(debugModel));
            glUniform3f(colorLoc, hitboxColor.r, hitboxColor.g,
                        hitboxColor.b); // Color personalizado para props GLTF
            glDrawArrays(GL_TRIANGLES, 0, 36);
          }
        };

        auto drawManualOBBHitbox = [&](const glm::vec3 &pos, float yawDegrees,
                                       const glm::vec3 &halfSize) {
          if (glm::length(glm::vec2(pos.x - cameraPos.x, pos.z - cameraPos.z)) >
              collisionViewerRadius)
            return;

          glm::mat4 debugModel = glm::mat4(1.0f);
          debugModel = glm::translate(debugModel, pos);
          debugModel = glm::rotate(debugModel, glm::radians(yawDegrees),
                                   glm::vec3(0.0f, 1.0f, 0.0f));
          debugModel = glm::scale(debugModel, halfSize * 2.0f);

          glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
          glUniform3f(colorLoc, 1.0f, 0.55f, 0.05f);
          glDrawArrays(GL_TRIANGLES, 0, 36);
        };

        // --- Props del Baño ---
        glm::vec3 banoPositions[8] = {banoPos,  banoPos2, banoPos3, banoPos4,
                                      banoPos5, banoPos6, banoPos7, banoPos8};
        glm::vec3 banoRotations[8] = {banoRot,  banoRot2, banoRot3, banoRot4,
                                      banoRot5, banoRot6, banoRot7, banoRot8};
        glm::vec3 banoScales[8] = {banoScale,  banoScale2, banoScale3,
                                   banoScale4, banoScale5, banoScale6,
                                   banoScale7, banoScale8};
        for (int i = 0; i < 8; i++) {
          drawModelHitbox(banoGLTF, banoPositions[i], banoRotations[i],
                          banoScales[i]);
        }

        glm::vec3 lavaPositions[8] = {
            lavamanosPos,  lavamanosPos2, lavamanosPos3, lavamanosPos4,
            lavamanosPos5, lavamanosPos6, lavamanosPos7, lavamanosPos8};
        glm::vec3 lavaRotations[8] = {
            lavamanosRot,  lavamanosRot2, lavamanosRot3, lavamanosRot4,
            lavamanosRot5, lavamanosRot6, lavamanosRot7, lavamanosRot8};
        glm::vec3 lavaScales[8] = {
            lavamanosScale,  lavamanosScale2, lavamanosScale3, lavamanosScale4,
            lavamanosScale5, lavamanosScale6, lavamanosScale7, lavamanosScale8};
        for (int i = 0; i < 8; i++) {
          drawModelHitbox(lavamanosGLTF, lavaPositions[i], lavaRotations[i],
                          lavaScales[i]);
        }

        drawModelHitbox(urinarioGLTF, urinarioPos, urinarioRot, urinarioScale);
        drawModelHitbox(mensBGLTF, mensBpos, mensBrot, mensBscale);
        drawModelHitbox(girlBGLTF, girlBpos, girlBrot, girlBscale);

        // --- Props Dinámicos (placedProps) ------
        for (const auto &prop : placedProps) {
          GLTFModel *model = modelRegistry[prop.modelName];
          if (model) {
            glm::vec3 color = prop.collisionActive
                                  ? glm::vec3(0.1f, 0.75f, 1.0f)
                                  : glm::vec3(1.0f, 0.25f, 0.25f);
            drawModelHitbox(model, prop.pos, prop.rot, prop.scale, color);
          }
        }

        for (int i = 0; i < 3; i++) {
          drawModelHitbox(generadorGLTF, generadorPos[i], generadorRot[i],
                          generadorScale[i]);
        }
      }

      glLineWidth(1.0f);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // --- DIBUJAR ENTIDADES 2D ---
    glBindVertexArray(quadVAO);
    glDisable(GL_CULL_FACE);

    for (auto &entity : gameEntities) {
      if (!entity.active || (entity.type >= 3 && entity.type != 8 &&
                             entity.type != 9 && entity.type != 11))
        continue;
      if (entity.type == 0 || entity.type == 3)
        continue; // Ya se dibujaron en 3D
      if (entity.type == 2 && !portalActivado)
        continue;

      if (entity.type == 1) {
        glBindTexture(GL_TEXTURE_2D, batteryTex);
      } else if (entity.type == 8) {
        glBindTexture(GL_TEXTURE_2D, keycardYellowTex);
      } else if (entity.type == 9) {
        glBindTexture(GL_TEXTURE_2D, keycardRedTex);
      } else if (entity.type == 11) {
        glBindTexture(GL_TEXTURE_2D, keycardBlueTex);
      } else if (entity.type == 0 || entity.type == 3) {
        glBindTexture(GL_TEXTURE_2D, clueTexture);
      } else {
        glBindTexture(GL_TEXTURE_2D, enemyTexture);
      }

      glm::mat4 entityModel = glm::mat4(1.0f);

      // FlotaciÃ³n sutil de objetos clave para visibilidad---
      float bounce = 0.0f;
      if (entity.type == 8 || entity.type == 9 || entity.type == 11)
        bounce = sin(currentFrame * 3.0f) * 0.1f;

      float targetY = entity.pos.y + bounce;

      entityModel = glm::translate(
          entityModel, glm::vec3(entity.pos.x, targetY, entity.pos.z));

      float anguloHaciaCamara =
          atan2(cameraPos.x - entity.pos.x, cameraPos.z - entity.pos.z);
      entityModel = glm::rotate(entityModel, anguloHaciaCamara,
                                glm::vec3(0.0f, 1.0f, 0.0f));

      float escala = (entity.type != 2) ? 0.3f : 0.9f;

      entityModel = glm::scale(entityModel, glm::vec3(escala, escala, escala));

      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(entityModel));

      if (entity.type == 2 && dimensionAlterna) {
        float pulse = 0.5f + 0.5f * sin(currentFrame * 10.0f);
        glUniform3f(colorLoc, pulse, 0.1f, 0.1f);
        glUniform1f(emissiveStrengthLoc, 0.0f);
      } else if (entity.type == 8) {
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Tarjeta amarilla
        glUniform1f(emissiveStrengthLoc, 1.2f);  // Hacer que brille
        glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f);
      } else if (entity.type == 9) {
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Tarjeta roja
        glUniform1f(emissiveStrengthLoc, 1.2f);  // Hacer que brille
        glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f);
      } else if (entity.type == 11) {
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // Tarjeta azul
        glUniform1f(emissiveStrengthLoc, 1.2f);  // Hacer que brille
        glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f);
      } else {
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        glUniform1f(emissiveStrengthLoc, 0.0f);
        glVertexAttrib3f(3, 1.0f, 1.0f, 1.0f);
      }

      glDrawArrays(GL_TRIANGLES, 0, 6);
      glUniform1f(emissiveStrengthLoc,
                  0.0f); // Resetear para siguientes objetos
    }

    // Viewmodel FPS: se dibuja con profundidad limpia para no atravesar
    // paredes.
    if (gameState == PLAYING && weaponEnabled && !isReadingDocument) {
      WeaponViewmodel &weapon = weapons[currentWeaponIndex];
      if (weapon.model && !weapon.model->meshes.empty()) {
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glm::mat4 viewmodelView(1.0f);
        glm::mat4 viewmodelProjection =
            glm::perspective(glm::radians(58.0f),
                             static_cast<float>(currentWidth) /
                                 static_cast<float>(currentHeight),
                             0.01f, 20.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewmodelView));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE,
                           glm::value_ptr(viewmodelProjection));

        glm::vec3 boundsSize =
            weapon.model->localAABB.max - weapon.model->localAABB.min;
        float largest =
            (std::max)(boundsSize.x, (std::max)(boundsSize.y, boundsSize.z));
        float viewmodelScale =
            largest > 0.001f ? weapon.targetSize / largest : 1.0f;
        float recoil = weaponMuzzleFlashTimer > 0.0f ? 0.035f : 0.0f;

        glm::mat4 viewmodelMatrix(1.0f);
        viewmodelMatrix = glm::translate(
            viewmodelMatrix, weapon.position + glm::vec3(0.0f, 0.0f, recoil));
        viewmodelMatrix =
            glm::rotate(viewmodelMatrix, glm::radians(weapon.rotation.x),
                        glm::vec3(1.0f, 0.0f, 0.0f));
        viewmodelMatrix =
            glm::rotate(viewmodelMatrix, glm::radians(weapon.rotation.y),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        viewmodelMatrix =
            glm::rotate(viewmodelMatrix, glm::radians(weapon.rotation.z),
                        glm::vec3(0.0f, 0.0f, 1.0f));
        viewmodelMatrix =
            glm::scale(viewmodelMatrix, glm::vec3(viewmodelScale));

        bool hasBones = weapon.model->CountBonesInMeshes() > 0;
        if (hasBones) {
          float armsAnimationTime = weaponAnimationTime;
          if (weapon.holdArmsAnimationAtEnd) {
            float armsLength = weapon.model->GetAnimationLengthSeconds(
                weaponArmsAnimationIndex);
            if (armsLength > 0.001f)
              armsAnimationTime = armsLength * 0.999f;
          }
          weapon.model->UpdateAnimationLayers(
              armsAnimationTime, weaponArmsAnimationIndex, weaponAnimationTime,
              weaponAnimationIndex, weapon.bones);
          if (finalBonesLoc >= 0 && !weapon.bones.empty()) {
            glUniformMatrix4fv(finalBonesLoc,
                               static_cast<GLsizei>(weapon.bones.size()),
                               GL_FALSE, glm::value_ptr(weapon.bones[0]));
          }
        }
        if (isAnimatedLoc >= 0)
          glUniform1i(isAnimatedLoc, hasBones ? 1 : 0);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE,
                           glm::value_ptr(viewmodelMatrix));
        glUniform1i(solidColorLoc, 0);
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        glUniform1f(emissiveStrengthLoc, 0.85f);
        if (hasBones) {
          weapon.model->Draw(shaderProgram, solidColorLoc);
        } else {
          weapon.model->DrawAnimated(weaponAnimationTime, weaponAnimationIndex,
                                     shaderProgram, modelLoc, solidColorLoc,
                                     viewmodelMatrix);
        }
        if (isAnimatedLoc >= 0)
          glUniform1i(isAnimatedLoc, 0);
        glUniform1i(solidColorLoc, 0);
        glUniform1f(emissiveStrengthLoc, 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(0);
      }
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
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(orthoModel));

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
        printTypewriter(getText("TYPE_SCENE_1"));
      };

      ImDrawList *drawList = ImGui::GetBackgroundDrawList();
      ImVec2 screenMin(0.0f, 0.0f);
      ImVec2 screenMax((float)currentWidth, (float)currentHeight);
      float t = (float)glfwGetTime();

      drawList->AddRectFilledMultiColor(
          screenMin, screenMax, IM_COL32(3, 7, 11, 238),
          IM_COL32(8, 16, 22, 238), IM_COL32(1, 2, 5, 248),
          IM_COL32(2, 4, 7, 248));
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
        float y =
            fmodf(seed * 11.3f + t * (6.0f + (i % 5)), (float)currentHeight);
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
      const char *title = getText("MENU_TITLE");
      const char *subtitle = menuOpcionesActivo ? getText("MENU_SUBTITLE_AUDIO") : getText("MENU_SUBTITLE");
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
      drawList->AddCircle(titleCenter, 17.0f, IM_COL32(220, 245, 255, 180), 32,
                          1.3f);

      ImVec2 subSize = ImGui::CalcTextSize(subtitle);
      ImGui::SetCursorPos(
          ImVec2((currentWidth - subSize.x) * 0.5f, titleCenter.y + 24.0f));
      ImGui::TextColored(ImVec4(0.62f, 0.78f, 0.84f, 1.0f), "%s", subtitle);

      float menuWidth = 260.0f;
      float menuX = (currentWidth - menuWidth) * 0.5f;
      float menuY = currentHeight * 0.48f;
      ImGui::SetCursorPos(ImVec2(menuX, menuY));
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.02f, 0.05f, 0.07f, 0.28f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.55f, 0.75f, 0.82f, 0.22f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.75f, 0.92f, 1.0f, 0.35f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.96f, 1.0f, 1.0f));

      auto menuButton = [&](const char *label) {
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

      if (menuOpcionesActivo) {
        if (menuButton(juegoMuteado ? getText("MENU_MUTE_ACTIVE") : getText("MENU_MUTE"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          juegoMuteado = true;
          ma_engine_set_volume(&audioEngine, 0.0f);
        }
        if (menuButton(!juegoMuteado ? getText("MENU_UNMUTE_ACTIVE") : getText("MENU_UNMUTE"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          juegoMuteado = false;
          ma_engine_set_volume(&audioEngine, 1.0f);
        }
        if (menuButton(currentLanguage == LANG_ES ? getText("MENU_LANG_ES") : getText("MENU_LANG_ES_INACTIVE"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          currentLanguage = LANG_ES;
        }
        if (menuButton(currentLanguage == LANG_EN ? getText("MENU_LANG_EN") : getText("MENU_LANG_EN_INACTIVE"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          currentLanguage = LANG_EN;
        }
        if (menuButton(getText("MENU_BACK"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          menuOpcionesActivo = false;
        }
      } else {
        if (menuButton(getText("MENU_START")))
          startGameFromMenu();
        if (menuButton(getText("MENU_OPTIONS"))) {
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          menuOpcionesActivo = true;
        }
        menuButton(getText("MENU_FILES"));
        menuButton(getText("MENU_CREDITS"));
        if (menuButton(getText("MENU_EXIT")))
          glfwSetWindowShouldClose(window, true);
      }

      ImGui::PopStyleColor(4);
      ImGui::PopStyleVar(2);

      ImGui::SetWindowFontScale(0.85f);
      const char *hint = menuOpcionesActivo ? getText("MENU_HINT_BACK") : getText("MENU_HINT_START");
      ImVec2 hintSize = ImGui::CalcTextSize(hint);
      ImGui::SetCursorPos(
          ImVec2((currentWidth - hintSize.x) * 0.5f, currentHeight * 0.86f));
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
      const float leftSpawnH =
          (glm::max)(120.0f, availablePanelH - leftStatusH - leftLevelH -
                                 panelGap * 2.0f);

      ImGui::SetNextWindowPos(ImVec2(0.0f, panelTopY), ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(sideTabW, 430.0f), ImGuiCond_Always);
      ImGui::SetNextWindowBgAlpha(0.78f);
      ImGui::Begin("EditorDockTabs", NULL,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
      auto sideTab = [&](const char *label, int panel) {
        if (activeEditorPanel == panel) {
          ImGui::PushStyleColor(ImGuiCol_Button,
                                ImVec4(0.20f, 0.38f, 0.58f, 1.0f));
        }
        bool clicked = ImGui::Button(label, ImVec2(-1.0f, 58.0f));
        if (activeEditorPanel == panel) {
          ImGui::PopStyleColor();
        }
        if (clicked)
          activeEditorPanel = panel;
      };
      sideTab("I", 0);
      sideTab("M", 1);
      sideTab("S", 2);
      sideTab("A", 3);
      if (ImGui::Button("<", ImVec2(-1.0f, 34.0f))) {
        showDebugGUI = false;
      }
      ImGui::End();

      // Display coordinates top-left
      if (activeEditorPanel == 0) {
        ImGui::SetNextWindowPos(ImVec2(leftPanelX, panelTopY),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(leftPanelW, 182.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.78f);
        ImGui::Begin("Coords", NULL,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "POSICION ACTUAL:");
        ImGui::Text("X: %.1f  |  Z: %.1f", cameraPos.x, cameraPos.z);
        ImGui::Text("FPS: %.0f  |  Frame: %.2f ms", ImGui::GetIO().Framerate,
                    1000.0f / (std::max)(1.0f, ImGui::GetIO().Framerate));
        ImGui::Spacing();
        ImGui::Checkbox("Ver Hitboxes (H)", &showCollisionViewer);
        ImGui::Spacing();
        if (ImGui::Button("Ocultar Editor (G)", ImVec2(-1, 0))) {
          showDebugGUI = false;
        }
        ImGui::End();
      }

      if (activeEditorPanel == 1) {
        ImGui::SetNextWindowPos(ImVec2(leftPanelX, leftLevelY),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(leftPanelW, availablePanelH),
                                 ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.84f);
        ImGui::Begin("Editor de Niveles 🛠️", NULL,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.2f, 0.75f, 1.0f, 1.0f),
                           "Editor de Mapa 3D");
        ImGui::Separator();

        static int selectedPropIdx = -1;
        if (placedProps.empty()) {
          selectedPropIdx = -1;
        } else if (selectedPropIdx < 0 ||
                   selectedPropIdx >= (int)placedProps.size()) {
          selectedPropIdx = 0;
        }

        // --- FILTRO POR ÁREA ---
        // Areas basadas en carpetas reales de assets/ que contienen .glb
        static const char *kAreaNames[] = {"Todas",   "General",  "Contencion",
                                           "Archivo", "Oficinas", "Descanso",
                                           "Baño",    "Ascensor", "sala-pruebas",
                                           "sala-generadores"};
        static int areaFilterIdx = 0; // 0 = Todas
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##AreaFiltro", &areaFilterIdx, kAreaNames,
                     IM_ARRAYSIZE(kAreaNames));
        ImGui::SameLine(0, 4);
        ImGui::TextDisabled("Filtro de Área");
        ImGui::Spacing();

        // List of placed props (filtrada por área)
        if (ImGui::TreeNode("Objetos en Escena")) {
          bool anyVisible = false;
          for (int i = 0; i < (int)placedProps.size(); ++i) {
            // Aplicar filtro: índice 0 = "Todas" muestra todo
            if (areaFilterIdx != 0 &&
                placedProps[i].area != kAreaNames[areaFilterIdx]) {
              continue;
            }
            anyVisible = true;
            std::string label = std::to_string(i) + ": [" +
                                placedProps[i].area + "] " +
                                placedProps[i].modelName;
            if (ImGui::Selectable(label.c_str(), selectedPropIdx == i)) {
              selectedPropIdx = i;
            }
          }
          if (!anyVisible) {
            ImGui::TextDisabled("(Sin objetos en esta area)");
          }
          ImGui::TreePop();
        }
        ImGui::Separator();

        if (selectedPropIdx >= 0 && selectedPropIdx < (int)placedProps.size()) {
          auto &prop = placedProps[selectedPropIdx];
          ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.1f, 1.0f),
                             "Prop Seleccionado: %s", prop.modelName.c_str());

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
            dup.pos +=
                glm::vec3(0.5f, 0.0f,
                          0.5f); // Un pequeño offset para ver que se duplicó===
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
        ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.2f, 1.0f),
                           "Agregar Nuevo Objeto:");
        static const char *availableModels[] = {
            // -- Contencion --
            "barra", "cables_piso", "cables_techo", "consola", "emergency",
            "esquineros", "generador", "lampara-reactor", "lampara", "lampara2",
            "logo", "logo2", "panelControl", "reactor", "sangre-piso",
            "sangre-piso2", "help", "it-sees-you", "sarcofago", "tesla",
            "warning", "behind-you",
            // -- Archivo --
            "box-close", "box-open", "camara", "computer", "escritorio",
            "gabinete", "mesa", "mini-lampara", "servers", "silla", "terminal",
            "vault-door",
            // -- Oficinas --
            "cajonesOF",
            // -- Descanso --
            "botas", "bunk_bed", "comedor", "estante_cajas", "expendedora",
            "extintor_viejo", "locker", "lockers", "old_sofa_free", "jaula",
            "compu_destruida", "old_soviet_taxophone", "papel_viejo",
            "planta_electrica", "trash", "trash_bag",
            // -- General (raiz assets/) --
            "gnome", "machine_lab", "metal_desk", "monitor", "pared", "sillas",
            "sofa",
            // -- Ascensor --
            "ascensor", "caja-electrica", "plataforma",
            "ducto", "ghost", "head",
            // -- Baño --
            "Bano", "azule", "girlB", "lavamanos", "ligthbathroom", "mensB",
            "mirror", "MirrorBG", "urinario"};
        static int selectedModelToAddIdx = 0;
        ImGui::Combo("Modelo", &selectedModelToAddIdx, availableModels,
                     IM_ARRAYSIZE(availableModels));

        static int lastAreaFilterIdx = -1;
        static int newPropAreaIdx = 0;
        if (areaFilterIdx != lastAreaFilterIdx) {
          if (areaFilterIdx > 0) {
            newPropAreaIdx = areaFilterIdx - 1;
          } else {
            newPropAreaIdx = 0; // "General"
          }
          lastAreaFilterIdx = areaFilterIdx;
        }

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##AreaNueva", &newPropAreaIdx, kAreaNames + 1,
                     IM_ARRAYSIZE(kAreaNames) - 1);
        ImGui::SameLine(0, 4);
        ImGui::TextDisabled("Área Destino");

        if (ImGui::Button("➕ Agregar a Escena", ImVec2(-1, 0))) {
          PlacedProp newProp;
          newProp.modelName = availableModels[selectedModelToAddIdx];
          newProp.pos = cameraPos + cameraFront * 2.0f;
          newProp.pos.y = -0.5f;
          newProp.rot = glm::vec3(0.0f, 0.0f, 0.0f);

          // Escalas base/por defecto para cada uno
          if (newProp.modelName == "tesla")
            newProp.scale = glm::vec3(0.150f, 0.120f, 0.090f);
          else if (newProp.modelName == "sarcofago")
            newProp.scale = glm::vec3(1.260f, 1.060f, 0.930f);
          else if (newProp.modelName == "warning")
            newProp.scale = glm::vec3(1.410f, 0.950f, 1.570f);
          else if (newProp.modelName == "consola")
            newProp.scale = glm::vec3(0.890f, 0.730f, 0.280f);
          else if (newProp.modelName == "panelControl")
            newProp.scale = glm::vec3(0.450f, 0.490f, 0.180f);
          else if (newProp.modelName == "esquineros")
            newProp.scale = glm::vec3(0.890f, 0.750f, 0.810f);
          else if (newProp.modelName == "cables_piso")
            newProp.scale = glm::vec3(0.01918f, 0.01918f, 0.01918f);
          else if (newProp.modelName == "cables_techo")
            newProp.scale = glm::vec3(-0.001557f, -0.001557f, -0.001557f);
          else if (newProp.modelName == "lampara-reactor")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "gabinete")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "camara")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "servers")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "terminal")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "box-close")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "box-open")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "vault-door")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "escritorio")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "mesa")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "mini-lampara")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "computer")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "ascensor" ||
                   newProp.modelName == "caja-electrica" ||
                   newProp.modelName == "plataforma" ||
                   newProp.modelName == "ducto") {
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          } else if (newProp.modelName == "compu_destruida") {
            newProp.scale = glm::vec3(0.55f, 0.55f, 0.55f);
            newProp.collisionActive = false;
          } else if (newProp.modelName == "silla")
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
          else if (newProp.modelName == "sangre-piso" ||
                   newProp.modelName == "sangre-piso2" ||
                   newProp.modelName == "help" ||
                   newProp.modelName == "it-sees-you") {
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            newProp.collisionActive = false;
          } else
            newProp.scale = glm::vec3(1.0f, 1.0f, 1.0f);

          // Asignar área: usa el combo de Área Destino
          newProp.area = kAreaNames[newPropAreaIdx + 1];

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
        ImGui::Text("Yaw/Pitch: %.1f / %.1f", debugSpawnYaw, debugSpawnPitch);
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
        auto teleportNear = [&](const glm::vec3 &target) {
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
            if (gameEntities[i].type == type)
              return i;
          }
          return -1;
        };
        int yellowKeycardIndex = findEntityByType(8);
        int redKeycardIndex = findEntityByType(9);
        int blueKeycardIndex = findEntityByType(11);
        int portalIndex = findEntityByType(7);

        if (ImGui::Button("Tarjeta amarilla") && yellowKeycardIndex >= 0)
          teleportNear(gameEntities[yellowKeycardIndex].pos);
        ImGui::SameLine();
        if (ImGui::Button("Tarjeta roja") && redKeycardIndex >= 0)
          teleportNear(gameEntities[redKeycardIndex].pos);
        ImGui::SameLine();
        if (ImGui::Button("Tarjeta azul") && blueKeycardIndex >= 0)
          teleportNear(gameEntities[blueKeycardIndex].pos);

        if (ImGui::Button("Portal") && portalIndex >= 0)
          teleportNear(gameEntities[portalIndex].pos);
        ImGui::SameLine();
        if (!gameEntities.empty() && ImGui::Button("Entidad seleccionada")) {
          teleportNear(gameEntities[selectedEntityIndex].pos);
        }
        ImGui::End();
      }

      if (activeEditorPanel == 3) {
        ImGui::SetNextWindowPos(ImVec2(leftPanelX, panelTopY),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(leftPanelW, availablePanelH),
                                 ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.86f);
        ImGui::Begin("Editor de Entidades Animadas", NULL,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        animatedEntities.DrawEditor(cameraPos, cameraFront,
                                    "assets/animated_entities.txt");
        ImGui::SeparatorText("Arma del jugador");
        ImGui::Checkbox("Arma equipada", &weaponEnabled);
        const char *weaponNames[] = {"Pistola", "Rifle", "Escopeta"};
        if (ImGui::Combo("Viewmodel activo", &currentWeaponIndex, weaponNames,
                         IM_ARRAYSIZE(weaponNames))) {
          WeaponViewmodel &weapon = weapons[currentWeaponIndex];
          weaponDamage = weapon.damage;
          weaponFireInterval = weapon.fireInterval;
          weaponAutomatic = weapon.automatic;
          weaponAnimationIndex = weapon.idleAnimation;
          weaponArmsAnimationIndex = weapon.armsIdleAnimation;
          weaponAnimationTime = 0.0f;
          weaponActionPlaying = false;
        }
        WeaponViewmodel &weaponEditor = weapons[currentWeaponIndex];
        ImGui::DragFloat3("Posicion viewmodel", &weaponEditor.position.x,
                          0.005f);
        ImGui::DragFloat3("Rotacion viewmodel", &weaponEditor.rotation.x, 0.5f);
        ImGui::DragFloat("Tamano viewmodel", &weaponEditor.targetSize, 0.01f,
                         0.05f, 5.0f);
        ImGui::Checkbox("Disparo automatico", &weaponAutomatic);
        ImGui::DragFloat("Dano del arma", &weaponDamage, 1.0f, 1.0f, 1000.0f);
        ImGui::DragFloat("Alcance del arma", &weaponRange, 0.5f, 1.0f, 100.0f);
        ImGui::DragFloat("Intervalo disparo", &weaponFireInterval, 0.01f, 0.05f,
                         3.0f);
        if (ImGui::Button("Guardar configuracion arma", ImVec2(-1.0f, 0.0f)))
          saveWeaponConfig();
        if (ImGui::Button("Recargar configuracion arma", ImVec2(-1.0f, 0.0f)))
          loadWeaponConfig();
        ImGui::TextDisabled(
            "Disparar: clic izquierdo | Recargar: R | Cambiar: Q");
        ImGui::End();
      }

      if (showCollisionViewer) {
        ImGui::SetNextWindowPos(
            ImVec2(leftPanelX + leftPanelW + panelGap, panelTopY),
            ImGuiCond_Always);
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
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
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
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
      ImGui::SetWindowFontScale(1.15f);
      ImGui::TextWrapped("%s", currentDocumentTitle.c_str());
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::TextWrapped("%s", currentDocumentBody.c_str());
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", getText("DOC_CLOSE_HINT"));
      ImGui::End();
    }
    // --- HOTBAR INVENTARIO (estilo Minecraft) ---
    if (gameState == PLAYING) {
      const float slotSize = 52.0f;
      const float slotPadding = 4.0f;
      const float iconPadding = 4.0f;
      const int maxSlots = 6; // Linterna, Llave1, Llave2, Bat1, Bat2, Bat3
      float hotbarWidth = maxSlots * (slotSize + slotPadding) + slotPadding;
      float hotbarHeight =
          slotSize + slotPadding * 2.0f + 16.0f; // extra para label
      float hotbarX = (currentWidth - hotbarWidth) * 0.5f;
      float hotbarY = currentHeight - hotbarHeight - 12.0f;

      ImGui::SetNextWindowPos(ImVec2(hotbarX, hotbarY));
      ImGui::SetNextWindowSize(ImVec2(hotbarWidth, hotbarHeight));
      ImGui::SetNextWindowBgAlpha(0.55f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(slotPadding, slotPadding));
      ImGui::Begin("Hotbar", NULL,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);

      // Helper lambda para dibujar un slot
      auto drawSlot = [&](const char *label, ImTextureID texID, bool hasItem,
                          bool isActive) {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        // Fondo del slot
        ImU32 bgColor = hasItem ? (isActive ? IM_COL32(80, 200, 80, 180)
                                            : IM_COL32(60, 60, 60, 200))
                                : IM_COL32(30, 30, 30, 140);
        ImU32 borderColor = isActive ? IM_COL32(255, 255, 100, 255)
                                     : IM_COL32(100, 100, 100, 180);

        drawList->AddRectFilled(
            cursorPos, ImVec2(cursorPos.x + slotSize, cursorPos.y + slotSize),
            bgColor, 4.0f);
        drawList->AddRect(
            cursorPos, ImVec2(cursorPos.x + slotSize, cursorPos.y + slotSize),
            borderColor, 4.0f, 0, isActive ? 2.5f : 1.0f);

        // Icono de textura
        if (hasItem && texID != 0) {
          drawList->AddImage(
              texID,
              ImVec2(cursorPos.x + iconPadding, cursorPos.y + iconPadding),
              ImVec2(cursorPos.x + slotSize - iconPadding,
                     cursorPos.y + slotSize - iconPadding));
        }

        // Label debajo
        ImVec2 textSize = ImGui::CalcTextSize(label);
        float textX = cursorPos.x + (slotSize - textSize.x) * 0.5f;
        drawList->AddText(ImVec2(textX, cursorPos.y + slotSize + 1.0f),
                          hasItem ? IM_COL32(255, 255, 255, 255)
                                  : IM_COL32(100, 100, 100, 150),
                          label);

        ImGui::Dummy(ImVec2(slotSize, slotSize));
        ImGui::SameLine(0.0f, slotPadding);
      };

      // Slot 0: Linterna (siempre la tiene)
      drawSlot(isFlashlightOn ? getText("INV_FLASHLIGHT_ON") : getText("INV_FLASHLIGHT_OFF"),
               (ImTextureID)(intptr_t)clueTexture, true,
               selectedHotbarSlot == 0);

      // Slot 1: Tarjeta Amarilla
      drawSlot(getText("INV_KEY_YELLOW"), (ImTextureID)(intptr_t)keycardYellowInvTex,
               hasKeycardYellow, selectedHotbarSlot == 1);

      // Slot 2: Tarjeta Roja
      drawSlot(getText("INV_KEY_RED"), (ImTextureID)(intptr_t)keycardRedInvTex, hasKeycardRed,
               selectedHotbarSlot == 2);

      // Slot 3: Tarjeta Azul
      drawSlot(getText("INV_KEY_BLUE"), (ImTextureID)(intptr_t)keycardBlueInvTex,
               hasKeycardBlue, selectedHotbarSlot == 3);

      // Slots 4-5: Baterías (sólo mostramos 2 o reducimos baterías, o ajustamos
      // índices) Ajustemos las baterías a la derecha. El máximo de slots es 6.
      // (0 a 5) Linterna(0), LlaveA(1), LlaveR(2), LlaveAz(3), Bat1(4), Bat2(5)
      // Wait, there are 3 batteries originally. We might need to add a slot or
      // group batteries. Grouping batteries:
      char batLabel[16];
      snprintf(batLabel, sizeof(batLabel), getText("INV_BATS"), bateriasRecolectadas);
      drawSlot(batLabel, (ImTextureID)(intptr_t)batteryTex,
               bateriasRecolectadas > 0, selectedHotbarSlot == 4);

      ImGui::End();
      ImGui::PopStyleVar(2);
    }

    if (gameState == PLAYING && weaponEnabled && !isReadingDocument) {
      ImDrawList *weaponHud = ImGui::GetForegroundDrawList();
      ImVec2 center(currentWidth * 0.5f, currentHeight * 0.5f);
      ImU32 crosshairColor = weaponHitMarkerTimer > 0.0f
                                 ? IM_COL32(255, 70, 70, 255)
                                 : IM_COL32(225, 235, 235, 210);
      float gap = 5.0f;
      float length = 8.0f;
      weaponHud->AddLine(ImVec2(center.x - gap - length, center.y),
                         ImVec2(center.x - gap, center.y), crosshairColor,
                         1.5f);
      weaponHud->AddLine(ImVec2(center.x + gap, center.y),
                         ImVec2(center.x + gap + length, center.y),
                         crosshairColor, 1.5f);
      weaponHud->AddLine(ImVec2(center.x, center.y - gap - length),
                         ImVec2(center.x, center.y - gap), crosshairColor,
                         1.5f);
      weaponHud->AddLine(ImVec2(center.x, center.y + gap),
                         ImVec2(center.x, center.y + gap + length),
                         crosshairColor, 1.5f);

      if (weaponMuzzleFlashTimer > 0.0f) {
        weaponHud->AddCircleFilled(ImVec2(center.x, center.y), 4.0f,
                                   IM_COL32(255, 190, 55, 215), 8);
      }
    }

    if (wirePuzzleActive) {
      ImGuiIO& io = ImGui::GetIO();
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);
      ImGui::Begin(getText("MINIGAME_WIRE_TITLE"), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

      static ImU32 leftColors[4] = { IM_COL32(255, 0, 0, 255), IM_COL32(0, 255, 0, 255), IM_COL32(0, 0, 255, 255), IM_COL32(255, 255, 0, 255) };
      static ImU32 rightColors[4] = { IM_COL32(0, 255, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(0, 0, 255, 255) }; // Shuffled

      static int selectedLeftNode = -1;
      static int nodeConnections[4] = { -1, -1, -1, -1 }; // Index is left node, value is right node index

      ImDrawList* drawList = ImGui::GetWindowDrawList();
      ImVec2 windowPos = ImGui::GetWindowPos();

      // Nodes positioning
      float nodeRadius = 15.0f;
      float startY = windowPos.y + 100.0f;
      float ySpacing = 60.0f;
      float leftX = windowPos.x + 80.0f;
      float rightX = windowPos.x + 420.0f;

      ImVec2 mousePos = ImGui::GetIO().MousePos;
      bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

      // Draw existing connections
      for (int i = 0; i < 4; ++i) {
        if (nodeConnections[i] != -1) {
          ImVec2 p1 = ImVec2(leftX, startY + i * ySpacing);
          ImVec2 p2 = ImVec2(rightX, startY + nodeConnections[i] * ySpacing);
          drawList->AddLine(p1, p2, leftColors[i], 5.0f);
        }
      }

      // Draw active dragging connection
      if (selectedLeftNode != -1) {
        ImVec2 p1 = ImVec2(leftX, startY + selectedLeftNode * ySpacing);
        drawList->AddLine(p1, mousePos, leftColors[selectedLeftNode], 5.0f);
      }

      bool allConnectedCorrectly = true;

      // Draw left nodes
      for (int i = 0; i < 4; ++i) {
        ImVec2 p = ImVec2(leftX, startY + i * ySpacing);
        drawList->AddCircleFilled(p, nodeRadius, leftColors[i]);
        drawList->AddCircle(p, nodeRadius + 2.0f, IM_COL32(200, 200, 200, 255), 0, 2.0f);

        // Interaction
        float distSq = (mousePos.x - p.x) * (mousePos.x - p.x) + (mousePos.y - p.y) * (mousePos.y - p.y);
        if (distSq < nodeRadius * nodeRadius) {
          drawList->AddCircle(p, nodeRadius + 4.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
          if (mouseClicked) {
            selectedLeftNode = i;
            nodeConnections[i] = -1; // Disconnect if reconnecting
          }
        }
      }

      // Draw right nodes
      for (int i = 0; i < 4; ++i) {
        ImVec2 p = ImVec2(rightX, startY + i * ySpacing);
        drawList->AddCircleFilled(p, nodeRadius, rightColors[i]);
        drawList->AddCircle(p, nodeRadius + 2.0f, IM_COL32(200, 200, 200, 255), 0, 2.0f);

        // Interaction (Drop)
        float distSq = (mousePos.x - p.x) * (mousePos.x - p.x) + (mousePos.y - p.y) * (mousePos.y - p.y);
        if (distSq < nodeRadius * nodeRadius) {
          drawList->AddCircle(p, nodeRadius + 4.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
          if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && selectedLeftNode != -1) {
            // Drop connection
            nodeConnections[selectedLeftNode] = i;
            selectedLeftNode = -1;
            ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          }
        }
      }

      if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        selectedLeftNode = -1; // Cancel drag if mouse released outside
      }

      // Check win condition
      for (int i = 0; i < 4; ++i) {
        if (nodeConnections[i] == -1 || leftColors[i] != rightColors[nodeConnections[i]]) {
          allConnectedCorrectly = false;
          break;
        }
      }

      ImGui::SetCursorPos(ImVec2(20, 40));
      if (allConnectedCorrectly) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", getText("MINIGAME_ACCESS_GRANTED"));
        // Open door logic
        if (blackDoorGridX != -1 && blackDoorGridZ != -1) {
          // Abrir la puerta principal y buscar adyacentes (doble puerta)
          int gx = blackDoorGridX;
          int gz = blackDoorGridZ;
          worldMap[gz][gx] = -11;
          
          if (gx > 0 && worldMap[gz][gx - 1] == 11) worldMap[gz][gx - 1] = -11;
          if (gx < MAP_WIDTH - 1 && worldMap[gz][gx + 1] == 11) worldMap[gz][gx + 1] = -11;
          if (gz > 0 && worldMap[gz - 1][gx] == 11) worldMap[gz - 1][gx] = -11;
          if (gz < MAP_HEIGHT - 1 && worldMap[gz + 1][gx] == 11) worldMap[gz + 1][gx] = -11;

          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          printTypewriter(getText("TYPE_BLACK_DOOR"));
          blackDoorGridX = -1;
          blackDoorGridZ = -1;
          wirePuzzleActive = false;
          isCursorLocked = true;
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          firstMouse = true;
        }
      } else {
        ImGui::Text("%s", getText("MINIGAME_WIRE_HINT"));
      }

      ImGui::End();
    }

    if (symbolPuzzleActive) {
      ImGuiIO& io = ImGui::GetIO();
      ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
      ImGui::Begin(getText("MINIGAME_SYMBOL_TITLE"), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

      const char* symbols[] = { " O ", " X ", "/\\", "[]", "<>", "||" };
      const int numSymbols = 6;

      ImGui::Text(getText("MINIGAME_SYMBOL_HINT"), symbols[symbolPuzzleTargetSymbol]);
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      bool allAligned = true;

      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20, 20));
      for (int i = 0; i < 3; ++i) {
        ImGui::PushID(i);
        
        ImGui::SetCursorPosX(100.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s %d:", getText("MINIGAME_SYMBOL_WHEEL"), i + 1);
        ImGui::SameLine();
        
        if (ImGui::Button("<", ImVec2(40, 40))) {
          symbolPuzzleWheelIndices[i] = (symbolPuzzleWheelIndices[i] - 1 + numSymbols) % numSymbols;
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
        }
        ImGui::SameLine();
        
        ImGui::Button(symbols[symbolPuzzleWheelIndices[i]], ImVec2(60, 40));
        
        ImGui::SameLine();
        if (ImGui::Button(">", ImVec2(40, 40))) {
          symbolPuzzleWheelIndices[i] = (symbolPuzzleWheelIndices[i] + 1) % numSymbols;
          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
        }
        ImGui::PopID();
        
        if (symbolPuzzleWheelIndices[i] != symbolPuzzleTargetSymbol) {
          allAligned = false;
        }
      }
      ImGui::PopStyleVar();

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      if (allAligned) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", getText("MINIGAME_ACCESS_GRANTED"));
        
        if (whiteDoorGridX != -1 && whiteDoorGridZ != -1) {
          int gx = whiteDoorGridX;
          int gz = whiteDoorGridZ;
          
          std::vector<std::pair<int, int>> doorsToOpen;
          doorsToOpen.push_back({gx, gz});
          if (gx > 0 && worldMap[gz][gx - 1] == 7) doorsToOpen.push_back({gx - 1, gz});
          if (gx < MAP_WIDTH - 1 && worldMap[gz][gx + 1] == 7) doorsToOpen.push_back({gx + 1, gz});
          if (gz > 0 && worldMap[gz - 1][gx] == 7) doorsToOpen.push_back({gx, gz - 1});
          if (gz < MAP_HEIGHT - 1 && worldMap[gz + 1][gx] == 7) doorsToOpen.push_back({gx, gz + 1});

          for (auto& p : doorsToOpen) {
            worldMap[p.second][p.first] = -7;
            activeDoorsAnim[p.second * MAP_WIDTH + p.first] = 0.0f;
          }

          ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
          printTypewriter(getText("TYPE_DOOR_OPEN"));
          whiteDoorGridX = -1;
          whiteDoorGridZ = -1;
          symbolPuzzleActive = false;
          isCursorLocked = true;
          firstMouse = true;
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
      } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", getText("MINIGAME_SYSTEM_LOCKED"));
      }

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
  if (shotgunFireSoundReady)
    ma_sound_uninit(&shotgunFireSound);
  ma_engine_uninit(&audioEngine);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);

  glfwTerminate();
  return 0;
}
