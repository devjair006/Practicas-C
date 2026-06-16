#include <glad/glad.h>

#include "headers/animated_entity.h"
#include "headers/gltf_model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace {

const char *kTriggerNames[] = {"Automatico", "Distancia", "Mirada",
                               "Interactuar E", "Manual", "Zona 3D"};
const char *kActionNames[] = {"Esperar", "Reproducir animacion",
                              "Mover a punto", "Mostrar mensaje",
                              "Desaparecer", "Perseguir jugador",
                              "Huir del jugador"};

int ClampAnimationIndex(const AnimatedEntity &entity, int index) {
  int count = entity.model ? entity.model->GetAnimationCount() : 0;
  if (count <= 0)
    return 0;
  return (std::max)(0, (std::min)(index, count - 1));
}

void AutoConfigureCombatAnimations(AnimatedEntity &entity) {
  if (!entity.model)
    return;
  int damage = entity.model->FindAnimationIndexContains("damage");
  if (damage < 0)
    damage = entity.model->FindAnimationIndexContains("hurt");
  if (damage < 0)
    damage = entity.model->FindAnimationIndexContains("hit");
  int death = entity.model->FindAnimationIndexContains("death");
  if (death < 0)
    death = entity.model->FindAnimationIndexContains("die");
  if (damage >= 0)
    entity.damageAnimation = damage;
  if (death >= 0)
    entity.deathAnimation = death;
}

float FlatDistance(const glm::vec3 &a, const glm::vec3 &b) {
  return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
}

glm::mat4 BuildModelMatrix(const AnimatedEntity &entity) {
  glm::vec3 finalScale = entity.scale;
  if (entity.autoScale && entity.model) {
    glm::vec3 size = entity.model->localAABB.max - entity.model->localAABB.min;
    float largest = (std::max)(size.x, (std::max)(size.y, size.z));
    if (largest > 0.001f)
      finalScale *= entity.targetSize / largest;
  }

  glm::mat4 modelMatrix(1.0f);
  modelMatrix = glm::translate(modelMatrix, entity.position);
  modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.rotation.x),
                            glm::vec3(1.0f, 0.0f, 0.0f));
  modelMatrix = glm::rotate(
      modelMatrix,
      glm::radians(entity.rotation.y + entity.runtimeYawDegrees +
                   entity.facingOffsetDegrees),
      glm::vec3(0.0f, 1.0f, 0.0f));
  modelMatrix = glm::rotate(modelMatrix, glm::radians(entity.rotation.z),
                            glm::vec3(0.0f, 0.0f, 1.0f));
  return glm::scale(modelMatrix, finalScale);
}

bool RayIntersectsAABB(const glm::vec3 &origin, const glm::vec3 &direction,
                       const AABB &box, float maxDistance,
                       float &distanceOut) {
  float nearDistance = 0.0f;
  float farDistance = maxDistance;
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(direction[axis]) < 0.00001f) {
      if (origin[axis] < box.min[axis] || origin[axis] > box.max[axis])
        return false;
      continue;
    }
    float inverse = 1.0f / direction[axis];
    float first = (box.min[axis] - origin[axis]) * inverse;
    float second = (box.max[axis] - origin[axis]) * inverse;
    if (first > second)
      std::swap(first, second);
    nearDistance = (std::max)(nearDistance, first);
    farDistance = (std::min)(farDistance, second);
    if (nearDistance > farDistance)
      return false;
  }
  distanceOut = nearDistance;
  return nearDistance <= maxDistance;
}

} // namespace

AnimatedEntitySystem *activeAnimatedEntitySystem = nullptr;

AnimatedEntitySystem::AnimatedEntitySystem() { RefreshDiscoveredModels(); }
AnimatedEntitySystem::~AnimatedEntitySystem() = default;

void AnimatedEntitySystem::RefreshDiscoveredModels() {
  discoveredModelPaths_.clear();
  for (const std::string &root : {"assets/entidadesanimadas", "assets", "src"}) {
    std::error_code error;
    if (!std::filesystem::exists(root, error))
      continue;
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(root, error)) {
      if (error)
        break;
      if (!entry.is_regular_file())
        continue;
      std::string extension = entry.path().extension().string();
      std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (extension == ".glb" || extension == ".gltf") {
        std::string path = entry.path().generic_string();
        if (std::find(discoveredModelPaths_.begin(),
                      discoveredModelPaths_.end(),
                      path) == discoveredModelPaths_.end()) {
          discoveredModelPaths_.push_back(path);
        }
      }
    }
  }
  std::stable_sort(discoveredModelPaths_.begin(), discoveredModelPaths_.end(),
                   [](const std::string &a, const std::string &b) {
                     const bool aDedicated =
                         a.rfind("assets/entidadesanimadas/", 0) == 0;
                     const bool bDedicated =
                         b.rfind("assets/entidadesanimadas/", 0) == 0;
                     if (aDedicated != bDedicated)
                       return aDedicated;
                     return a < b;
                   });
}

GLTFModel *AnimatedEntitySystem::GetOrLoadModel(const std::string &path) {
  auto existing = models_.find(path);
  if (existing != models_.end())
    return existing->second.get();

  auto model = std::make_unique<GLTFModel>(path);
  GLTFModel *result = model.get();
  models_[path] = std::move(model);
  std::cout << "[ENTIDADES ANIMADAS] Modelo " << path << " | meshes: "
            << result->meshes.size()
            << " | animaciones: " << result->GetAnimationCount() << std::endl;
  return result;
}

int AnimatedEntitySystem::Add(const std::string &name,
                              const std::string &modelPath,
                              const glm::vec3 &position) {
  AnimatedEntity entity;
  entity.name = name.empty() ? "Entidad" : name;
  entity.modelPath = modelPath;
  entity.model = GetOrLoadModel(modelPath);
  AutoConfigureCombatAnimations(entity);
  entity.spawnPosition = position;
  entity.position = position;
  entities_.push_back(std::move(entity));
  return static_cast<int>(entities_.size()) - 1;
}

void AnimatedEntitySystem::Remove(int index) {
  if (index < 0 || index >= static_cast<int>(entities_.size()))
    return;
  entities_.erase(entities_.begin() + index);
}

void AnimatedEntitySystem::Reset(int index) {
  if (index < 0 || index >= static_cast<int>(entities_.size()))
    return;
  AnimatedEntity &entity = entities_[index];
  entity.position = entity.spawnPosition;
  entity.visible = true;
  entity.running = false;
  entity.finished = false;
  entity.actionIndex = 0;
  entity.actionTime = 0.0f;
  entity.animationTime = 0.0f;
  entity.currentAnimation = entity.idleAnimation;
  entity.health = entity.maxHealth;
  entity.takingDamage = false;
  entity.dead = false;
  entity.damageTimer = 0.0f;
  entity.deathTimer = 0.0f;
}

void AnimatedEntitySystem::Start(int index) {
  if (index < 0 || index >= static_cast<int>(entities_.size()))
    return;
  AnimatedEntity &entity = entities_[index];
  if (!entity.enabled)
    return;
  entity.running = true;
  entity.finished = false;
  entity.visible = true;
  entity.actionIndex = 0;
  entity.actionTime = 0.0f;
  entity.animationTime = 0.0f;
  entity.currentAnimation = entity.idleAnimation;
}

void AnimatedEntitySystem::ResetAll() {
  for (int i = 0; i < static_cast<int>(entities_.size()); ++i)
    Reset(i);
}

bool AnimatedEntitySystem::Triggered(const AnimatedEntity &entity,
                                     const glm::vec3 &playerPosition,
                                     const glm::vec3 &cameraFront,
                                     bool interactPressed) const {
  if (entity.trigger == AnimatedTriggerType::Manual)
    return false;
  if (entity.trigger == AnimatedTriggerType::Auto)
    return true;
  if (entity.trigger == AnimatedTriggerType::Zone) {
    glm::vec3 zoneMin = glm::min(entity.triggerZoneMin, entity.triggerZoneMax);
    glm::vec3 zoneMax = glm::max(entity.triggerZoneMin, entity.triggerZoneMax);
    return playerPosition.x >= zoneMin.x && playerPosition.x <= zoneMax.x &&
           playerPosition.y >= zoneMin.y && playerPosition.y <= zoneMax.y &&
           playerPosition.z >= zoneMin.z && playerPosition.z <= zoneMax.z;
  }

  glm::vec3 toEntity = entity.position - playerPosition;
  toEntity.y = 0.0f;
  float distance = glm::length(toEntity);
  if (distance > entity.triggerDistance)
    return false;
  if (entity.trigger == AnimatedTriggerType::Distance)
    return true;

  glm::vec3 flatFront(cameraFront.x, 0.0f, cameraFront.z);
  if (glm::length2(toEntity) < 0.0001f || glm::length2(flatFront) < 0.0001f)
    return false;
  float look = glm::dot(glm::normalize(toEntity), glm::normalize(flatFront));
  if (look < entity.lookThreshold)
    return false;
  if (entity.trigger == AnimatedTriggerType::LookAt)
    return true;
  return interactPressed;
}

void AnimatedEntitySystem::AdvanceAction(AnimatedEntity &entity) {
  entity.actionIndex++;
  entity.actionTime = 0.0f;
  entity.animationTime = 0.0f;
  if (entity.actionIndex >= static_cast<int>(entity.actions.size())) {
    if (entity.sequenceLoop && !entity.actions.empty()) {
      entity.actionIndex = 0;
    } else {
      entity.running = false;
      entity.finished = true;
      entity.currentAnimation = entity.idleAnimation;
    }
  }
}

void AnimatedEntitySystem::Update(float deltaTime,
                                  const glm::vec3 &playerPosition,
                                  const glm::vec3 &cameraFront,
                                  bool interactPressed,
                                  bool gameplayActive) {
  for (AnimatedEntity &entity : entities_) {
    if (!entity.enabled || !entity.model)
      continue;

    if (entity.previewEnabled) {
      entity.previewTime += deltaTime * entity.animationSpeed;
      continue;
    }
    if (entity.dead) {
      entity.currentAnimation =
          ClampAnimationIndex(entity, entity.deathAnimation);
      entity.deathTimer += deltaTime;
      entity.animationTime += deltaTime * entity.animationSpeed;
      if (entity.disappearAfterDeath &&
          entity.deathTimer >=
              entity.deathAnimationDuration + entity.corpseLifetime) {
        entity.visible = false;
      }
      continue;
    }
    if (entity.takingDamage) {
      entity.currentAnimation =
          ClampAnimationIndex(entity, entity.damageAnimation);
      entity.damageTimer += deltaTime;
      entity.animationTime += deltaTime * entity.animationSpeed;
      if (entity.damageTimer >= entity.damageAnimationDuration) {
        entity.takingDamage = false;
        entity.damageTimer = 0.0f;
        entity.animationTime = 0.0f;
      }
      continue;
    }
    if (!gameplayActive)
      continue;

    if (!entity.running && !entity.finished &&
        Triggered(entity, playerPosition, cameraFront, interactPressed)) {
      entity.running = true;
      entity.actionIndex = 0;
      entity.actionTime = 0.0f;
      entity.animationTime = 0.0f;
    }
    if (!entity.running)
      continue;
    if (entity.actions.empty()) {
      entity.running = false;
      entity.finished = true;
      continue;
    }

    bool processNext = true;
    int safety = 0;
    while (processNext && entity.running && safety++ < 8) {
      processNext = false;
      AnimatedAction &action = entity.actions[entity.actionIndex];

      switch (action.type) {
      case AnimatedActionType::Wait:
        entity.currentAnimation = entity.idleAnimation;
        entity.actionTime += deltaTime;
        entity.animationTime += deltaTime * entity.animationSpeed;
        if (entity.actionTime >= (std::max)(0.0f, action.duration)) {
          AdvanceAction(entity);
          processNext = true;
        }
        break;

      case AnimatedActionType::PlayAnimation: {
        entity.currentAnimation =
            ClampAnimationIndex(entity, action.animationIndex);
        entity.actionTime += deltaTime;
        entity.animationTime += deltaTime * action.speed;
        float duration = action.duration;
        if (duration <= 0.0f && entity.model) {
          duration =
              entity.model->GetAnimationLengthSeconds(entity.currentAnimation);
        }
        if (duration <= 0.0f)
          duration = 1.0f;
        if (entity.actionTime >= duration) {
          AdvanceAction(entity);
          processNext = true;
        }
        break;
      }

      case AnimatedActionType::MoveTo: {
        entity.currentAnimation =
            ClampAnimationIndex(entity, action.animationIndex);
        glm::vec3 toTarget = action.target - entity.position;
        float distance = glm::length(toTarget);
        float step = (std::max)(0.0f, action.speed) * deltaTime;
        if (distance <= step || distance < 0.001f) {
          entity.position = action.target;
          AdvanceAction(entity);
          processNext = true;
        } else {
          entity.moveDirection = toTarget / distance;
          entity.position += entity.moveDirection * step;
          if (entity.faceMovement) {
            entity.runtimeYawDegrees =
                glm::degrees(atan2(entity.moveDirection.x,
                                   entity.moveDirection.z));
          }
          entity.actionTime += deltaTime;
          entity.animationTime += deltaTime * entity.animationSpeed;
        }
        break;
      }

      case AnimatedActionType::Message:
        if (!action.text.empty())
          pendingMessages_.push_back(action.text);
        AdvanceAction(entity);
        processNext = true;
        break;

      case AnimatedActionType::Disappear:
        entity.visible = false;
        entity.running = false;
        entity.finished = true;
        break;

      case AnimatedActionType::ChasePlayer:
      case AnimatedActionType::FleeFromPlayer: {
        entity.currentAnimation =
            ClampAnimationIndex(entity, action.animationIndex);
        glm::vec3 direction =
            action.type == AnimatedActionType::ChasePlayer
                ? playerPosition - entity.position
                : entity.position - playerPosition;
        direction.y = 0.0f;
        if (glm::length2(direction) > 0.0001f) {
          entity.moveDirection = glm::normalize(direction);
          entity.position +=
              entity.moveDirection * (std::max)(0.0f, action.speed) * deltaTime;
          if (entity.faceMovement) {
            entity.runtimeYawDegrees =
                glm::degrees(atan2(entity.moveDirection.x,
                                   entity.moveDirection.z));
          }
        }
        entity.actionTime += deltaTime;
        entity.animationTime += deltaTime * entity.animationSpeed;
        if (action.duration > 0.0f && entity.actionTime >= action.duration) {
          AdvanceAction(entity);
          processNext = true;
        }
        break;
      }
      }
    }
  }
}

void AnimatedEntitySystem::Render(unsigned int shaderProgram, int modelLoc,
                                  int solidColorLoc, int colorLoc,
                                  int isAnimatedLoc, int finalBonesLoc,
                                  const glm::vec3 &cameraPosition) {
  for (AnimatedEntity &entity : entities_) {
    if (!entity.enabled || !entity.visible || !entity.model ||
        entity.model->meshes.empty() ||
        FlatDistance(entity.position, cameraPosition) > 35.0f) {
      continue;
    }

    int animationIndex =
        entity.previewEnabled ? entity.previewAnimation : entity.currentAnimation;
    animationIndex = ClampAnimationIndex(entity, animationIndex);
    float animationTime =
        entity.previewEnabled ? entity.previewTime : entity.animationTime;
    bool animationLoops = entity.previewEnabled ? entity.animationLoop : true;
    if (!entity.previewEnabled && entity.running && entity.actionIndex >= 0 &&
        entity.actionIndex < static_cast<int>(entity.actions.size())) {
      animationLoops = entity.actions[entity.actionIndex].loop;
    }
    if (entity.dead || entity.takingDamage)
      animationLoops = false;
    if (!animationLoops && entity.model) {
      float length = entity.model->GetAnimationLengthSeconds(animationIndex);
      if (length > 0.0f)
        animationTime = (std::min)(animationTime, length * 0.999f);
    }

    glm::mat4 modelMatrix = BuildModelMatrix(entity);

    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glUniform1i(solidColorLoc, 0);

    bool hasBones = entity.model->CountBonesInMeshes() > 0;
    if (hasBones) {
      entity.model->UpdateAnimation(animationTime, entity.boneTransforms,
                                    animationIndex);
      if (finalBonesLoc >= 0 && !entity.boneTransforms.empty()) {
        glUniformMatrix4fv(finalBonesLoc,
                           static_cast<GLsizei>(entity.boneTransforms.size()),
                           GL_FALSE, glm::value_ptr(entity.boneTransforms[0]));
      }
      if (isAnimatedLoc >= 0)
        glUniform1i(isAnimatedLoc, 1);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
      entity.model->Draw(shaderProgram, solidColorLoc);
    } else {
      if (isAnimatedLoc >= 0)
        glUniform1i(isAnimatedLoc, 0);
      entity.model->DrawAnimated(animationTime, animationIndex, shaderProgram,
                                 modelLoc, solidColorLoc, modelMatrix);
    }
  }

  if (isAnimatedLoc >= 0)
    glUniform1i(isAnimatedLoc, 0);
  glUniform1i(solidColorLoc, 0);

  if (debugVisualsEnabled_) {
    std::vector<float> lineVertices;
    auto addLine = [&](const glm::vec3 &a, const glm::vec3 &b) {
      for (const glm::vec3 &point : {a, b}) {
        lineVertices.insert(lineVertices.end(),
                            {point.x, point.y, point.z, 0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f});
      }
    };

    for (const AnimatedEntity &entity : entities_) {
      if (!entity.enabled)
        continue;
      if (entity.trigger == AnimatedTriggerType::Distance ||
          entity.trigger == AnimatedTriggerType::LookAt ||
          entity.trigger == AnimatedTriggerType::Interact) {
        constexpr int segments = 32;
        for (int i = 0; i < segments; ++i) {
          float a0 = glm::two_pi<float>() * static_cast<float>(i) / segments;
          float a1 =
              glm::two_pi<float>() * static_cast<float>(i + 1) / segments;
          glm::vec3 p0 =
              entity.spawnPosition +
              glm::vec3(cos(a0) * entity.triggerDistance, 0.05f,
                        sin(a0) * entity.triggerDistance);
          glm::vec3 p1 =
              entity.spawnPosition +
              glm::vec3(cos(a1) * entity.triggerDistance, 0.05f,
                        sin(a1) * entity.triggerDistance);
          addLine(p0, p1);
        }
      } else if (entity.trigger == AnimatedTriggerType::Zone) {
        glm::vec3 min = glm::min(entity.triggerZoneMin, entity.triggerZoneMax);
        glm::vec3 max = glm::max(entity.triggerZoneMin, entity.triggerZoneMax);
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {max.x, min.y, max.z}, {min.x, min.y, max.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z}};
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                  {4, 5}, {5, 6}, {6, 7}, {7, 4},
                                  {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto &edge : edges)
          addLine(corners[edge[0]], corners[edge[1]]);
      }

      glm::vec3 routePoint = entity.spawnPosition;
      for (const AnimatedAction &action : entity.actions) {
        if (action.type == AnimatedActionType::MoveTo) {
          addLine(routePoint + glm::vec3(0.0f, 0.08f, 0.0f),
                  action.target + glm::vec3(0.0f, 0.08f, 0.0f));
          routePoint = action.target;
        }
      }

      if (entity.canReceiveDamage && entity.model &&
          !entity.model->meshes.empty()) {
        AABB hitbox = entity.model->GetWorldAABB(BuildModelMatrix(entity));
        glm::vec3 center = (hitbox.min + hitbox.max) * 0.5f;
        glm::vec3 halfSize =
            (hitbox.max - hitbox.min) * 0.5f * entity.hitboxScale;
        glm::vec3 min = center - halfSize;
        glm::vec3 max = center + halfSize;
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {max.x, min.y, max.z}, {min.x, min.y, max.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z}};
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0},
                                  {4, 5}, {5, 6}, {6, 7}, {7, 4},
                                  {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto &edge : edges)
          addLine(corners[edge[0]], corners[edge[1]]);
      }
    }

    if (!lineVertices.empty()) {
      static unsigned int lineVAO = 0;
      static unsigned int lineVBO = 0;
      if (lineVAO == 0) {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
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
      glBindVertexArray(lineVAO);
      glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
      glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float),
                   lineVertices.data(), GL_DYNAMIC_DRAW);
      glm::mat4 identity(1.0f);
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
      glUniform1i(solidColorLoc, 1);
      glUniform3f(colorLoc, 0.1f, 0.9f, 1.0f);
      glLineWidth(2.0f);
      glDrawArrays(GL_LINES, 0,
                   static_cast<GLsizei>(lineVertices.size() / 8));
      glLineWidth(1.0f);
      glBindVertexArray(0);
      glUniform1i(solidColorLoc, 0);
    }
  }
}

bool AnimatedEntitySystem::Save(const std::string &path) const {
  std::ofstream out(path);
  if (!out.is_open())
    return false;

  out << "# Animated entities v1\n";
  for (const AnimatedEntity &entity : entities_) {
    out << "ENTITY " << std::quoted(entity.name) << " "
        << std::quoted(entity.modelPath) << " " << entity.spawnPosition.x << " "
        << entity.spawnPosition.y << " " << entity.spawnPosition.z << " "
        << entity.rotation.x << " " << entity.rotation.y << " "
        << entity.rotation.z << " " << entity.scale.x << " " << entity.scale.y
        << " " << entity.scale.z << " " << (entity.enabled ? 1 : 0) << " "
        << (entity.autoScale ? 1 : 0) << " " << entity.targetSize << " "
        << (entity.faceMovement ? 1 : 0) << " " << entity.facingOffsetDegrees
        << " " << static_cast<int>(entity.trigger) << " "
        << entity.triggerDistance << " " << entity.lookThreshold << " "
        << entity.idleAnimation << " " << entity.animationSpeed << " "
        << (entity.animationLoop ? 1 : 0) << " "
        << (entity.sequenceLoop ? 1 : 0) << " "
        << (entity.collisionActive ? 1 : 0) << " "
        << entity.triggerZoneMin.x << " " << entity.triggerZoneMin.y << " "
        << entity.triggerZoneMin.z << " " << entity.triggerZoneMax.x << " "
        << entity.triggerZoneMax.y << " " << entity.triggerZoneMax.z << " "
        << (entity.canReceiveDamage ? 1 : 0) << " " << entity.maxHealth << " "
        << entity.hitboxScale << " " << entity.damageAnimation << " "
        << entity.deathAnimation << " " << entity.damageAnimationDuration << " "
        << entity.deathAnimationDuration << " " << entity.knockbackDistance
        << " " << (entity.disappearAfterDeath ? 1 : 0) << " "
        << entity.corpseLifetime << "\n";
    for (const AnimatedAction &action : entity.actions) {
      out << "ACTION " << static_cast<int>(action.type) << " "
          << action.animationIndex << " " << action.duration << " "
          << action.speed << " " << action.target.x << " " << action.target.y
          << " " << action.target.z << " " << (action.loop ? 1 : 0) << " "
          << std::quoted(action.text) << "\n";
    }
    out << "END\n";
  }
  return true;
}

bool AnimatedEntitySystem::Load(const std::string &path) {
  std::ifstream in(path);
  if (!in.is_open())
    return false;

  entities_.clear();
  AnimatedEntity current;
  bool readingEntity = false;
  bool currentHasCombatData = false;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    std::stringstream ss(line);
    std::string command;
    ss >> command;
    if (command == "ENTITY") {
      current = AnimatedEntity{};
      currentHasCombatData = false;
      int enabled = 1;
      int autoScale = 1;
      int faceMovement = 1;
      int trigger = 1;
      int loop = 1;
      int sequenceLoop = 0;
      int collisionActive = 0;
      int canReceiveDamage = 1;
      int disappearAfterDeath = 0;
      ss >> std::quoted(current.name) >> std::quoted(current.modelPath) >>
          current.spawnPosition.x >> current.spawnPosition.y >>
          current.spawnPosition.z >> current.rotation.x >> current.rotation.y >>
          current.rotation.z >> current.scale.x >> current.scale.y >>
          current.scale.z >> enabled >> autoScale >> current.targetSize >>
          faceMovement >> current.facingOffsetDegrees >> trigger >>
          current.triggerDistance >> current.lookThreshold >>
          current.idleAnimation >> current.animationSpeed >> loop;
      ss >> sequenceLoop;
      ss >> collisionActive;
      ss >> current.triggerZoneMin.x >> current.triggerZoneMin.y >>
          current.triggerZoneMin.z >> current.triggerZoneMax.x >>
          current.triggerZoneMax.y >> current.triggerZoneMax.z;
      currentHasCombatData = static_cast<bool>(
          ss >> canReceiveDamage >> current.maxHealth >> current.hitboxScale >>
          current.damageAnimation >> current.deathAnimation >>
          current.damageAnimationDuration >> current.deathAnimationDuration >>
          current.knockbackDistance >> disappearAfterDeath >>
          current.corpseLifetime);
      current.enabled = enabled != 0;
      current.autoScale = autoScale != 0;
      current.faceMovement = faceMovement != 0;
      current.trigger = static_cast<AnimatedTriggerType>(trigger);
      current.animationLoop = loop != 0;
      current.sequenceLoop = sequenceLoop != 0;
      current.collisionActive = collisionActive != 0;
      current.canReceiveDamage = canReceiveDamage != 0;
      current.disappearAfterDeath = disappearAfterDeath != 0;
      current.health = current.maxHealth;
      current.position = current.spawnPosition;
      readingEntity = true;
    } else if (command == "ACTION" && readingEntity) {
      AnimatedAction action;
      int type = 0;
      int loop = 1;
      ss >> type >> action.animationIndex >> action.duration >> action.speed >>
          action.target.x >> action.target.y >> action.target.z >> loop >>
          std::quoted(action.text);
      action.type = static_cast<AnimatedActionType>(type);
      action.loop = loop != 0;
      current.actions.push_back(action);
    } else if (command == "END" && readingEntity) {
      current.model = GetOrLoadModel(current.modelPath);
      if (!currentHasCombatData)
        AutoConfigureCombatAnimations(current);
      current.currentAnimation = current.idleAnimation;
      entities_.push_back(std::move(current));
      readingEntity = false;
    }
  }
  ResetAll();
  return true;
}

void AnimatedEntitySystem::DrawEditor(const glm::vec3 &cameraPosition,
                                      const glm::vec3 &cameraFront,
                                      const std::string &savePath) {
  static int selected = -1;
  static int selectedAction = -1;
  static int lastSelected = -2;
  static char selectedName[128] = {};
  static char selectedPath[256] = {};
  static char newName[64] = "Nueva entidad";
  static char newPath[256] = "assets/entidadesanimadas/modelo.glb";

  if (entities_.empty()) {
    selected = -1;
  } else if (selected < 0 || selected >= static_cast<int>(entities_.size())) {
    selected = 0;
  }

  if (ImGui::Button("Guardar entidades", ImVec2(-1.0f, 0.0f)))
    Save(savePath);
  if (ImGui::Button("Recargar desde TXT", ImVec2(-1.0f, 0.0f))) {
    Load(savePath);
    selected = entities_.empty() ? -1 : 0;
    lastSelected = -2;
  }
  ImGui::Checkbox("Ver radios y rutas", &debugVisualsEnabled_);
  ImGui::Separator();

  if (ImGui::BeginListBox("##AnimatedEntities", ImVec2(-1.0f, 115.0f))) {
    for (int i = 0; i < static_cast<int>(entities_.size()); ++i) {
      std::string label = std::to_string(i) + ": " + entities_[i].name;
      if (ImGui::Selectable(label.c_str(), selected == i)) {
        selected = i;
        selectedAction = -1;
      }
    }
    ImGui::EndListBox();
  }

  if (selected >= 0 && selected < static_cast<int>(entities_.size())) {
    AnimatedEntity &entity = entities_[selected];
    int animationCount = entity.model ? entity.model->GetAnimationCount() : 0;
    if (lastSelected != selected) {
      std::snprintf(selectedName, sizeof(selectedName), "%s",
                    entity.name.c_str());
      std::snprintf(selectedPath, sizeof(selectedPath), "%s",
                    entity.modelPath.c_str());
      lastSelected = selected;
    }
    ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.15f, 1.0f), "%s",
                       entity.name.c_str());
    ImGui::Text("Estado: %s",
                entity.dead
                    ? "Muerta"
                    : (entity.takingDamage
                           ? "Recibiendo dano"
                           : (entity.running
                                  ? "Ejecutando"
                                  : (entity.finished ? "Finalizada"
                                                     : "Esperando"))));
    if (ImGui::InputText("Nombre entidad", selectedName,
                         sizeof(selectedName))) {
      entity.name = selectedName;
    }
    ImGui::InputText("Modelo GLB", selectedPath, sizeof(selectedPath));
    if (ImGui::Button("Aplicar modelo", ImVec2(-1.0f, 0.0f))) {
      entity.modelPath = selectedPath;
      entity.model = GetOrLoadModel(entity.modelPath);
      AutoConfigureCombatAnimations(entity);
      entity.boneTransforms.clear();
    }

    if (ImGui::DragFloat3("Posicion", &entity.spawnPosition.x, 0.05f))
      entity.position = entity.spawnPosition;
    ImGui::DragFloat3("Rotacion", &entity.rotation.x, 0.5f);
    ImGui::DragFloat3("Escala", &entity.scale.x, 0.01f, 0.001f, 20.0f);
    ImGui::Checkbox("Auto escala", &entity.autoScale);
    if (entity.autoScale)
      ImGui::DragFloat("Tamano objetivo", &entity.targetSize, 0.01f, 0.05f,
                       20.0f);
    ImGui::DragFloat("Offset frente", &entity.facingOffsetDegrees, 1.0f,
                     -360.0f, 360.0f);
    ImGui::Checkbox("Orientar al moverse", &entity.faceMovement);
    ImGui::Checkbox("Activa", &entity.enabled);
    ImGui::Checkbox("Visible", &entity.visible);
    ImGui::Checkbox("Colision con jugador", &entity.collisionActive);
    ImGui::Checkbox("Repetir secuencia", &entity.sequenceLoop);

    int trigger = static_cast<int>(entity.trigger);
    if (ImGui::Combo("Activador", &trigger, kTriggerNames,
                     IM_ARRAYSIZE(kTriggerNames))) {
      entity.trigger = static_cast<AnimatedTriggerType>(trigger);
    }
    if (entity.trigger != AnimatedTriggerType::Auto &&
        entity.trigger != AnimatedTriggerType::Manual &&
        entity.trigger != AnimatedTriggerType::Zone) {
      ImGui::DragFloat("Radio activacion", &entity.triggerDistance, 0.1f, 0.1f,
                       50.0f);
    }
    if (entity.trigger == AnimatedTriggerType::LookAt ||
        entity.trigger == AnimatedTriggerType::Interact) {
      ImGui::SliderFloat("Precision mirada", &entity.lookThreshold, -1.0f,
                         1.0f);
    }
    if (entity.trigger == AnimatedTriggerType::Zone) {
      ImGui::DragFloat3("Zona minima", &entity.triggerZoneMin.x, 0.05f);
      ImGui::DragFloat3("Zona maxima", &entity.triggerZoneMax.x, 0.05f);
      if (ImGui::Button("Min = posicion actual"))
        entity.triggerZoneMin = cameraPosition;
      ImGui::SameLine();
      if (ImGui::Button("Max = posicion actual"))
        entity.triggerZoneMax = cameraPosition;
    }

    if (ImGui::Button("Traer frente a camara")) {
      entity.spawnPosition = cameraPosition + cameraFront * 2.0f;
      entity.position = entity.spawnPosition;
    }
    if (ImGui::Button("Reiniciar"))
      Reset(selected);
    ImGui::SameLine();
    if (ImGui::Button("Probar secuencia"))
      Start(selected);

    ImGui::SeparatorText("Combate");
    ImGui::Checkbox("Puede recibir dano", &entity.canReceiveDamage);
    ImGui::DragFloat("Vida maxima", &entity.maxHealth, 1.0f, 1.0f, 10000.0f);
    entity.health = (std::min)(entity.health, entity.maxHealth);
    float healthFraction =
        entity.maxHealth > 0.0f ? entity.health / entity.maxHealth : 0.0f;
    char healthLabel[64];
    std::snprintf(healthLabel, sizeof(healthLabel), "%.0f / %.0f",
                  entity.health, entity.maxHealth);
    ImGui::ProgressBar(healthFraction, ImVec2(-1.0f, 0.0f), healthLabel);
    ImGui::DragFloat("Escala hitbox disparo", &entity.hitboxScale, 0.01f,
                     0.1f, 5.0f);
    ImGui::DragInt("Animacion dano", &entity.damageAnimation, 0.1f, 0,
                   (std::max)(0, animationCount - 1));
    ImGui::DragInt("Animacion muerte", &entity.deathAnimation, 0.1f, 0,
                   (std::max)(0, animationCount - 1));
    ImGui::DragFloat("Duracion dano", &entity.damageAnimationDuration, 0.05f,
                     0.05f, 10.0f);
    ImGui::DragFloat("Duracion muerte", &entity.deathAnimationDuration, 0.05f,
                     0.05f, 30.0f);
    ImGui::DragFloat("Retroceso por impacto", &entity.knockbackDistance, 0.01f,
                     0.0f, 5.0f);
    ImGui::Checkbox("Desaparecer al morir", &entity.disappearAfterDeath);
    if (entity.disappearAfterDeath) {
      ImGui::DragFloat("Tiempo del cadaver", &entity.corpseLifetime, 0.1f,
                       0.0f, 60.0f);
    }
    if (ImGui::Button("Aplicar 25 dano"))
      ApplyDamage(selected, 25.0f, cameraFront);
    ImGui::SameLine();
    if (ImGui::Button("Matar"))
      ApplyDamage(selected, entity.health, cameraFront);

    ImGui::SeparatorText("Probador de animaciones");
    ImGui::Text("Animaciones detectadas: %d", animationCount);
    if (animationCount > 0) {
      ImGui::SliderInt("Animacion preview", &entity.previewAnimation, 0,
                       animationCount - 1);
      entity.previewAnimation =
          ClampAnimationIndex(entity, entity.previewAnimation);
      ImGui::TextDisabled("%s",
                          entity.model
                              ->GetAnimationName(entity.previewAnimation)
                              .c_str());
      ImGui::DragFloat("Velocidad animacion", &entity.animationSpeed, 0.05f,
                       0.01f, 5.0f);
      ImGui::Checkbox("Loop preview", &entity.animationLoop);
      if (ImGui::Checkbox("Preview", &entity.previewEnabled))
        entity.previewTime = 0.0f;
      ImGui::SameLine();
      if (ImGui::Button("Reiniciar preview"))
        entity.previewTime = 0.0f;
    }

    ImGui::SeparatorText("Secuencia");
    if (ImGui::BeginListBox("##Actions", ImVec2(-1.0f, 105.0f))) {
      for (int i = 0; i < static_cast<int>(entity.actions.size()); ++i) {
        std::string label =
            std::to_string(i + 1) + ". " +
            kActionNames[static_cast<int>(entity.actions[i].type)];
        if (ImGui::Selectable(label.c_str(), selectedAction == i))
          selectedAction = i;
      }
      ImGui::EndListBox();
    }

    static int actionToAdd = 0;
    ImGui::Combo("Nueva accion", &actionToAdd, kActionNames,
                 IM_ARRAYSIZE(kActionNames));
    if (ImGui::Button("Agregar accion", ImVec2(-1.0f, 0.0f))) {
      AnimatedAction action;
      action.type = static_cast<AnimatedActionType>(actionToAdd);
      action.target = cameraPosition;
      entity.actions.push_back(action);
      selectedAction = static_cast<int>(entity.actions.size()) - 1;
    }

    if (selectedAction >= 0 &&
        selectedAction < static_cast<int>(entity.actions.size())) {
      AnimatedAction &action = entity.actions[selectedAction];
      int type = static_cast<int>(action.type);
      if (ImGui::Combo("Tipo accion", &type, kActionNames,
                       IM_ARRAYSIZE(kActionNames))) {
        action.type = static_cast<AnimatedActionType>(type);
      }
      if (action.type == AnimatedActionType::Wait ||
          action.type == AnimatedActionType::PlayAnimation ||
          action.type == AnimatedActionType::ChasePlayer ||
          action.type == AnimatedActionType::FleeFromPlayer) {
        ImGui::DragFloat("Duracion", &action.duration, 0.05f, 0.0f, 60.0f);
      }
      if (action.type == AnimatedActionType::PlayAnimation ||
          action.type == AnimatedActionType::MoveTo ||
          action.type == AnimatedActionType::ChasePlayer ||
          action.type == AnimatedActionType::FleeFromPlayer) {
        ImGui::DragInt("Animacion", &action.animationIndex, 0.1f, 0,
                       (std::max)(0, animationCount - 1));
        ImGui::DragFloat("Velocidad", &action.speed, 0.05f, 0.01f, 20.0f);
        ImGui::Checkbox("Loop", &action.loop);
      }
      if (action.type == AnimatedActionType::MoveTo) {
        ImGui::DragFloat3("Destino", &action.target.x, 0.05f);
        if (ImGui::Button("Destino = posicion actual"))
          action.target = cameraPosition;
      }
      if (action.type == AnimatedActionType::Message) {
        char textBuffer[256] = {};
        std::snprintf(textBuffer, sizeof(textBuffer), "%s",
                      action.text.c_str());
        if (ImGui::InputTextMultiline("Mensaje", textBuffer,
                                      sizeof(textBuffer),
                                      ImVec2(-1.0f, 58.0f))) {
          action.text = textBuffer;
        }
      }
      if (ImGui::Button("Subir") && selectedAction > 0) {
        std::swap(entity.actions[selectedAction],
                  entity.actions[selectedAction - 1]);
        selectedAction--;
      }
      ImGui::SameLine();
      if (ImGui::Button("Bajar") &&
          selectedAction + 1 < static_cast<int>(entity.actions.size())) {
        std::swap(entity.actions[selectedAction],
                  entity.actions[selectedAction + 1]);
        selectedAction++;
      }
      ImGui::SameLine();
      if (ImGui::Button("Eliminar accion")) {
        entity.actions.erase(entity.actions.begin() + selectedAction);
        selectedAction = -1;
      }
    }

    ImGui::Separator();
    if (ImGui::Button("Duplicar entidad", ImVec2(-1.0f, 0.0f))) {
      AnimatedEntity duplicate = entity;
      duplicate.name += " copia";
      duplicate.spawnPosition += glm::vec3(0.5f, 0.0f, 0.5f);
      duplicate.position = duplicate.spawnPosition;
      duplicate.boneTransforms.clear();
      duplicate.health = duplicate.maxHealth;
      duplicate.dead = false;
      duplicate.takingDamage = false;
      duplicate.running = false;
      duplicate.finished = false;
      duplicate.visible = true;
      entities_.push_back(std::move(duplicate));
      selected = static_cast<int>(entities_.size()) - 1;
      lastSelected = -2;
    }
    if (ImGui::Button("Eliminar entidad", ImVec2(-1.0f, 0.0f))) {
      Remove(selected);
      selected = entities_.empty()
                     ? -1
                     : (std::min)(selected,
                                  static_cast<int>(entities_.size()) - 1);
      lastSelected = -2;
    }
  }

  ImGui::SeparatorText("Agregar GLB animado");
  ImGui::TextDisabled("Carpeta principal: assets/entidadesanimadas");
  ImGui::InputText("Nombre", newName, sizeof(newName));
  ImGui::InputText("Ruta GLB", newPath, sizeof(newPath));
  static int discoveredModel = -1;
  if (ImGui::BeginCombo(
          "Modelos encontrados",
          discoveredModel >= 0 &&
                  discoveredModel < static_cast<int>(discoveredModelPaths_.size())
              ? discoveredModelPaths_[discoveredModel].c_str()
              : "Seleccionar GLB...")) {
    for (int i = 0; i < static_cast<int>(discoveredModelPaths_.size()); ++i) {
      if (ImGui::Selectable(discoveredModelPaths_[i].c_str(),
                            discoveredModel == i)) {
        discoveredModel = i;
        std::snprintf(newPath, sizeof(newPath), "%s",
                      discoveredModelPaths_[i].c_str());
      }
    }
    ImGui::EndCombo();
  }
  if (ImGui::Button("Buscar modelos nuevos", ImVec2(-1.0f, 0.0f)))
    RefreshDiscoveredModels();
  if (ImGui::Button("Agregar frente a camara", ImVec2(-1.0f, 0.0f))) {
    selected = Add(newName, newPath, cameraPosition + cameraFront * 2.0f);
    selectedAction = -1;
    lastSelected = -2;
  }
}

std::vector<AnimatedEntity> &AnimatedEntitySystem::Entities() {
  return entities_;
}

const std::vector<AnimatedEntity> &AnimatedEntitySystem::Entities() const {
  return entities_;
}

std::vector<std::string> AnimatedEntitySystem::ConsumeMessages() {
  std::vector<std::string> result;
  result.swap(pendingMessages_);
  return result;
}

bool AnimatedEntitySystem::CheckCollision(const glm::vec3 &playerPosition,
                                          float playerRadius) const {
  for (const AnimatedEntity &entity : entities_) {
    if (!entity.enabled || !entity.visible || entity.dead ||
        !entity.collisionActive ||
        !entity.model || entity.model->meshes.empty()) {
      continue;
    }
    if (entity.model->CheckSphereCollision(playerPosition, playerRadius,
                                           BuildModelMatrix(entity))) {
      return true;
    }
  }
  return false;
}

bool AnimatedEntitySystem::ShootRay(const glm::vec3 &origin,
                                    const glm::vec3 &direction, float range,
                                    float damage, int *hitEntityIndex,
                                    glm::vec3 *hitPoint) {
  if (range <= 0.0f || glm::length2(direction) < 0.0001f)
    return false;

  glm::vec3 rayDirection = glm::normalize(direction);
  float closestDistance = range;
  int closestIndex = -1;

  for (int i = 0; i < static_cast<int>(entities_.size()); ++i) {
    const AnimatedEntity &entity = entities_[i];
    if (!entity.enabled || !entity.visible || !entity.canReceiveDamage ||
        entity.dead || !entity.model || entity.model->meshes.empty()) {
      continue;
    }

    AABB hitbox = entity.model->GetWorldAABB(BuildModelMatrix(entity));
    glm::vec3 center = (hitbox.min + hitbox.max) * 0.5f;
    glm::vec3 halfSize =
        (hitbox.max - hitbox.min) * 0.5f * entity.hitboxScale;
    hitbox.min = center - halfSize;
    hitbox.max = center + halfSize;

    float hitDistance = range;
    if (RayIntersectsAABB(origin, rayDirection, hitbox, range, hitDistance) &&
        hitDistance < closestDistance) {
      closestDistance = hitDistance;
      closestIndex = i;
    }
  }

  if (closestIndex < 0)
    return false;

  if (hitEntityIndex)
    *hitEntityIndex = closestIndex;
  if (hitPoint)
    *hitPoint = origin + rayDirection * closestDistance;
  ApplyDamage(closestIndex, damage, rayDirection);
  return true;
}

void AnimatedEntitySystem::ApplyDamage(int index, float damage,
                                       const glm::vec3 &hitDirection) {
  if (index < 0 || index >= static_cast<int>(entities_.size()))
    return;
  AnimatedEntity &entity = entities_[index];
  if (!entity.enabled || !entity.visible || !entity.canReceiveDamage ||
      entity.dead || damage <= 0.0f) {
    return;
  }

  entity.previewEnabled = false;
  entity.health = (std::max)(0.0f, entity.health - damage);

  glm::vec3 knockback(hitDirection.x, 0.0f, hitDirection.z);
  if (glm::length2(knockback) > 0.0001f) {
    entity.position +=
        glm::normalize(knockback) * entity.knockbackDistance;
  }

  entity.animationTime = 0.0f;
  if (entity.health <= 0.0f) {
    entity.dead = true;
    entity.takingDamage = false;
    entity.running = false;
    entity.finished = true;
    entity.deathTimer = 0.0f;
    entity.currentAnimation =
        ClampAnimationIndex(entity, entity.deathAnimation);
    pendingMessages_.push_back("[COMBATE]: " + entity.name + " eliminada.");
  } else {
    entity.takingDamage = true;
    entity.damageTimer = 0.0f;
    entity.currentAnimation =
        ClampAnimationIndex(entity, entity.damageAnimation);
  }
}
