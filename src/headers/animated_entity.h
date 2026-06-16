#pragma once

#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

class GLTFModel;

enum class AnimatedTriggerType {
  Auto,
  Distance,
  LookAt,
  Interact,
  Manual,
  Zone
};

enum class AnimatedActionType {
  Wait,
  PlayAnimation,
  MoveTo,
  Message,
  Disappear,
  ChasePlayer,
  FleeFromPlayer
};

struct AnimatedAction {
  AnimatedActionType type = AnimatedActionType::Wait;
  int animationIndex = 0;
  float duration = 1.0f;
  float speed = 1.0f;
  glm::vec3 target = glm::vec3(0.0f);
  bool loop = true;
  std::string text;
};

struct AnimatedEntity {
  std::string name = "Entidad";
  std::string modelPath;
  GLTFModel *model = nullptr;

  glm::vec3 spawnPosition = glm::vec3(0.0f);
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 rotation = glm::vec3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);

  bool enabled = true;
  bool visible = true;
  bool autoScale = true;
  bool faceMovement = true;
  bool collisionActive = false;
  float targetSize = 1.7f;
  float facingOffsetDegrees = 0.0f;

  AnimatedTriggerType trigger = AnimatedTriggerType::Distance;
  float triggerDistance = 4.0f;
  float lookThreshold = 0.92f;
  glm::vec3 triggerZoneMin = glm::vec3(0.0f);
  glm::vec3 triggerZoneMax = glm::vec3(1.0f);

  int idleAnimation = 0;
  float animationSpeed = 1.0f;
  bool animationLoop = true;
  bool sequenceLoop = false;
  std::vector<AnimatedAction> actions;

  bool canReceiveDamage = true;
  float maxHealth = 100.0f;
  float health = 100.0f;
  float hitboxScale = 1.0f;
  int damageAnimation = 0;
  int deathAnimation = 0;
  float damageAnimationDuration = 0.45f;
  float deathAnimationDuration = 1.5f;
  float knockbackDistance = 0.15f;
  bool disappearAfterDeath = false;
  float corpseLifetime = 3.0f;

  bool running = false;
  bool finished = false;
  bool takingDamage = false;
  bool dead = false;
  float damageTimer = 0.0f;
  float deathTimer = 0.0f;
  int actionIndex = 0;
  float actionTime = 0.0f;
  float animationTime = 0.0f;
  int currentAnimation = 0;
  float runtimeYawDegrees = 0.0f;
  glm::vec3 moveDirection = glm::vec3(0.0f, 0.0f, 1.0f);

  bool previewEnabled = false;
  int previewAnimation = 0;
  float previewTime = 0.0f;
  std::vector<glm::mat4> boneTransforms;
};

class AnimatedEntitySystem {
public:
  AnimatedEntitySystem();
  ~AnimatedEntitySystem();

  bool Load(const std::string &path);
  bool Save(const std::string &path) const;
  int Add(const std::string &name, const std::string &modelPath,
          const glm::vec3 &position);
  void Remove(int index);
  void Reset(int index);
  void Start(int index);
  void ResetAll();

  void Update(float deltaTime, const glm::vec3 &playerPosition,
              const glm::vec3 &cameraFront, bool interactPressed,
              bool gameplayActive);
  void Render(unsigned int shaderProgram, int modelLoc, int solidColorLoc,
              int colorLoc, int isAnimatedLoc, int finalBonesLoc,
              const glm::vec3 &cameraPosition);
  void DrawEditor(const glm::vec3 &cameraPosition,
                  const glm::vec3 &cameraFront, const std::string &savePath);
  bool CheckCollision(const glm::vec3 &playerPosition,
                      float playerRadius) const;
  bool ShootRay(const glm::vec3 &origin, const glm::vec3 &direction,
                float range, float damage, int *hitEntityIndex = nullptr,
                glm::vec3 *hitPoint = nullptr);
  void ApplyDamage(int index, float damage, const glm::vec3 &hitDirection);

  std::vector<AnimatedEntity> &Entities();
  const std::vector<AnimatedEntity> &Entities() const;
  std::vector<std::string> ConsumeMessages();

private:
  GLTFModel *GetOrLoadModel(const std::string &path);
  void RefreshDiscoveredModels();
  void AdvanceAction(AnimatedEntity &entity);
  bool Triggered(const AnimatedEntity &entity, const glm::vec3 &playerPosition,
                 const glm::vec3 &cameraFront, bool interactPressed) const;

  std::vector<AnimatedEntity> entities_;
  std::map<std::string, std::unique_ptr<GLTFModel>> models_;
  std::vector<std::string> pendingMessages_;
  std::vector<std::string> discoveredModelPaths_;
  bool debugVisualsEnabled_ = false;
};

extern AnimatedEntitySystem *activeAnimatedEntitySystem;
