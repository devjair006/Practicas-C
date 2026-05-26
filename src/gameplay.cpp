#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "headers/gameplay.h"

#include <cmath>
#include <iostream>

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "headers/game_state.h"
#include "gltf_model.h"

bool checkSphereAABBCollision(glm::vec3 sphereCenter, float radius, AABB box) {
    float closestX = glm::max(box.min.x, glm::min(sphereCenter.x, box.max.x));
    float closestY = glm::max(box.min.y, glm::min(sphereCenter.y, box.max.y));
    float closestZ = glm::max(box.min.z, glm::min(sphereCenter.z, box.max.z));

    float dx = sphereCenter.x - closestX;
    float dy = sphereCenter.y - closestY;
    float dz = sphereCenter.z - closestZ;
    float distanceSq = dx * dx + dy * dy + dz * dz;

    return distanceSq <= (radius * radius);
}

bool checkCollision(float x, float z) {
    float playerRadius = 0.25f;

    int startX = static_cast<int>(floor(x - 1.0f));
    int endX = static_cast<int>(ceil(x + 1.0f));
    int startZ = static_cast<int>(floor(z - 1.0f));
    int endZ = static_cast<int>(ceil(z + 1.0f));

    for (int cz = startZ; cz <= endZ; cz++) {
        for (int cx = startX; cx <= endX; cx++) {
            if (cx < 0 || cx >= MAP_WIDTH || cz < 0 || cz >= MAP_HEIGHT) {
                return true;
            }

            if (worldMap[cz][cx] > 0) {
                float halfX = wallWidth / 2.0f;
                float halfZ = wallWidth / 2.0f;
                bool leftW = (cx > 0 && worldMap[cz][cx - 1] > 0);
                bool rightW = (cx < MAP_WIDTH - 1 && worldMap[cz][cx + 1] > 0);
                bool upW = (cz > 0 && worldMap[cz - 1][cx] > 0);
                bool downW = (cz < MAP_HEIGHT - 1 && worldMap[cz + 1][cx] > 0);
                if (leftW || rightW) halfX = 0.5f;
                if (upW || downW) halfZ = 0.5f;

                float wallMinX = static_cast<float>(cx) - halfX;
                float wallMaxX = static_cast<float>(cx) + halfX;
                float wallMinZ = static_cast<float>(cz) - halfZ;
                float wallMaxZ = static_cast<float>(cz) + halfZ;

                float closestX = glm::clamp(x, wallMinX, wallMaxX);
                float closestZ = glm::clamp(z, wallMinZ, wallMaxZ);

                float dx = x - closestX;
                float dz = z - closestZ;
                if ((dx * dx + dz * dz) <= (playerRadius * playerRadius)) return true;
            }
        }
    }

    for (auto& entity : gameEntities) {
        if (!entity.active) continue;
        if (entity.type == 4 || entity.type == 6) {
            float dist = glm::length(glm::vec2(x - entity.pos.x, z - entity.pos.z));
            if (dist < 0.8f) return true;
        }
    }

    // --- COLISIÓN CON PROPS DE BAÑO GLB (AABB) ---
    glm::vec3 playerPos(x, cameraPos.y, z);

    if (banoGLTF && !banoGLTF->meshes.empty()) {
        glm::vec3 positions[4] = { banoPos, banoPos2, banoPos3, banoPos4 };
        glm::vec3 rotations[4] = { banoRot, banoRot2, banoRot3, banoRot4 };
        glm::vec3 scales[4] = { banoScale, banoScale2, banoScale3, banoScale4 };
        for (int i = 0; i < 4; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            model = glm::rotate(model, glm::radians(rotations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, scales[i]);
            AABB worldBox = banoGLTF->GetWorldAABB(model);
            if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) {
                return true;
            }
        }
    }

    if (lavamanosGLTF && !lavamanosGLTF->meshes.empty()) {
        glm::vec3 positions[4] = { lavamanosPos, lavamanosPos2, lavamanosPos3, lavamanosPos4 };
        glm::vec3 rotations[4] = { lavamanosRot, lavamanosRot2, lavamanosRot3, lavamanosRot4 };
        glm::vec3 scales[4] = { lavamanosScale, lavamanosScale2, lavamanosScale3, lavamanosScale4 };
        for (int i = 0; i < 4; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            model = glm::rotate(model, glm::radians(rotations[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotations[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotations[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, scales[i]);
            AABB worldBox = lavamanosGLTF->GetWorldAABB(model);
            if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) {
                return true;
            }
        }
    }

    if (urinarioGLTF && !urinarioGLTF->meshes.empty()) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, urinarioPos);
        model = glm::rotate(model, glm::radians(urinarioRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(urinarioRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(urinarioRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, urinarioScale);
        AABB worldBox = urinarioGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) {
            return true;
        }
    }

    // --- COLISION CON TESLA GLB ---
    if (teslaGLTF && !teslaGLTF->meshes.empty()) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, teslaPos);
        model = glm::rotate(model, glm::radians(teslaRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(teslaRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(teslaRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, teslaScale);
        AABB worldBox = teslaGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) {
            return true;
        }
    }

    // --- COLISION CON ESQUINEROS ---
    if (esquinerosGLTF && !esquinerosGLTF->meshes.empty()) {
        // Esquinero 1
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, esquinerosPos);
        model = glm::rotate(model, glm::radians(esquinerosRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquinerosRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquinerosRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, esquinerosScale);
        AABB worldBox = esquinerosGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) return true;

        // Esquinero 2
        model = glm::mat4(1.0f);
        model = glm::translate(model, esquineros2Pos);
        model = glm::rotate(model, glm::radians(esquineros2Rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros2Rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros2Rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, esquineros2Scale);
        worldBox = esquinerosGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) return true;

        // Esquinero 3
        model = glm::mat4(1.0f);
        model = glm::translate(model, esquineros3Pos);
        model = glm::rotate(model, glm::radians(esquineros3Rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros3Rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros3Rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, esquineros3Scale);
        worldBox = esquinerosGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) return true;

        // Esquinero 4
        model = glm::mat4(1.0f);
        model = glm::translate(model, esquineros4Pos);
        model = glm::rotate(model, glm::radians(esquineros4Rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros4Rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(esquineros4Rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, esquineros4Scale);
        worldBox = esquinerosGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) return true;
    }

    // --- COLISION CON GENERADOR ---
    if (generadorGLTF && !generadorGLTF->meshes.empty()) {
        for (int i = 0; i < 3; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, generadorPos[i]);
            model = glm::rotate(model, glm::radians(generadorRot[i].x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(generadorRot[i].y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(generadorRot[i].z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, generadorScale[i]);
            AABB worldBox = generadorGLTF->GetWorldAABB(model);
            if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) return true;
        }
    }

    // --- COLISION CON PANEL CONTROL ---
    if (panelControlGLTF && !panelControlGLTF->meshes.empty()) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, panelControlPos);
        model = glm::rotate(model, glm::radians(panelControlRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(panelControlRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(panelControlRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, panelControlScale);
        AABB worldBox = panelControlGLTF->GetWorldAABB(model);
        if (checkSphereAABBCollision(playerPos, playerRadius, worldBox)) {
            return true;
        }
    }



    return false;
}

void updateZone() {
    if (cameraPos.z >= 35.0f) currentZone = 1;
    else if (cameraPos.z >= 15.0f) currentZone = 2;
    else currentZone = 3;
}

void printTypewriter(std::string text) {
    currentHUDMessage = text;
    hudMessageTimer = 5.0f;
    std::cout << "\n> " << text << "\n" << std::endl;
}

void openDocument(const std::string& title, const std::string& body) {
    currentDocumentTitle = title;
    currentDocumentBody = body;
    isReadingDocument = true;
}

void closeDocument() {
    isReadingDocument = false;
    currentDocumentTitle.clear();
    currentDocumentBody.clear();
}

void tryOpenDoor(GLFWwindow* window) {
    (void)window;
    glm::vec3 checkPos = cameraPos + cameraFront * 1.5f;
    int gridX = static_cast<int>(round(checkPos.x));
    int gridZ = static_cast<int>(round(checkPos.z));

    if (gridX >= 0 && gridX < MAP_WIDTH && gridZ >= 0 && gridZ < MAP_HEIGHT) {
        int targetBlock = worldMap[gridZ][gridX];

        if (targetBlock == 8) {
            if (hasKeycardLvl1) {
                for (int cx = 0; cx < MAP_WIDTH; cx++) {
                    if (worldMap[gridZ][cx] == 8) worldMap[gridZ][cx] = -8;
                }
                door1Opening = true;
                printTypewriter("[PUERTA]: Tarjeta Nivel 1 Aceptada. Accediendo a Sala de Control.");
                ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
            } else {
                printTypewriter("[PUERTA BLOQUEADA]: Se requiere Tarjeta Amarilla (Nivel 1).");
            }
        } else if (targetBlock == 9) {
            if (hasKeycardLvl2) {
                for (int cx = 0; cx < MAP_WIDTH; cx++) {
                    if (worldMap[gridZ][cx] == 9) worldMap[gridZ][cx] = -9;
                }
                door2Opening = true;
                printTypewriter("[PUERTA]: Tarjeta Nivel 2 Aceptada. Peligro: Zona de Alta Radiacion.");
                ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
            } else {
                printTypewriter("[PUERTA BLOQUEADA]: Se requiere Tarjeta Roja (Nivel 2).");
            }
        }
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (isReadingDocument) closeDocument();
        else glfwSetWindowShouldClose(window, true);
    }

    if (gameState == GAMEOVER) return;

    if (isReadingDocument) {
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            if (!eKeyWasPressed) {
                closeDocument();
                eKeyWasPressed = true;
            }
        } else {
            eKeyWasPressed = false;
        }
        return;
    }

    if (gameState == MENU) {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            gameState = PLAYING;
            isCursorLocked = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
            ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
            std::cout << "=========================================================" << std::endl;
            std::cout << "               PROYECTO CONFIDENCIAL - REINICIO          " << std::endl;
            std::cout << "=========================================================\n" << std::endl;
            printTypewriter("ESCENA 1: PASILLO DE ACCESO");
            std::cout << "El entorno es silencioso y vacio." << std::endl;
            std::cout << "Moverte: W A S D  | Mirar: MOUSE | Sprint: SHIFT" << std::endl;
            std::cout << "Interactuar/Abrir Puertas: E | Linterna: F" << std::endl;
            std::cout << "Busca TARJETAS DE ACCESO para avanzar a las siguientes salas." << std::endl;
        }
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyWasPressed) {
            isCursorLocked = !isCursorLocked;
            if (isCursorLocked) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            tabKeyWasPressed = true;
        }
    } else {
        tabKeyWasPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyWasPressed) {
            isFlashlightOn = !isFlashlightOn;
            fKeyWasPressed = true;
            ma_engine_play_sound(&audioEngine, "assets/click.wav", NULL);
        }
    } else {
        fKeyWasPressed = false;
    }

    if (!isCursorLocked) return;

    float cameraSpeed = 3.5f;
    isSprinting = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && stamina > 0.0f && !isExhausted) {
        cameraSpeed = 6.0f;
        stamina -= 30.0f * deltaTime;
        isSprinting = true;
        if (stamina <= 0.0f) {
            isExhausted = true;
            std::cout << "\n[AGITADO]: Te has quedado sin aliento.\n" << std::endl;
        }
    } else {
        stamina += 15.0f * deltaTime;
        if (stamina > 100.0f) {
            stamina = 100.0f;
            isExhausted = false;
        }
    }

    cameraSpeed *= deltaTime;

    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += glm::normalize(glm::cross(cameraFront, cameraUp));

    moveDir.y = 0.0f;
    isMoving = false;

    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir) * cameraSpeed;
        if (!checkCollision(cameraPos.x + moveDir.x, cameraPos.z)) {
            cameraPos.x += moveDir.x;
            isMoving = true;
        }
        if (!checkCollision(cameraPos.x, cameraPos.z + moveDir.z)) {
            cameraPos.z += moveDir.z;
            isMoving = true;
        }
    }

    if (isMoving) {
        float bobSpeed = isSprinting ? 15.0f : 10.0f;
        headBobTimer += deltaTime * bobSpeed;
        cameraPos.y = baseCameraY + sin(headBobTimer) * 0.1f;
    } else {
        cameraPos.y = glm::mix(cameraPos.y, baseCameraY, deltaTime * 5.0f);
        headBobTimer = 0.0f;
    }

    int prevZone = currentZone;
    updateZone();
    if (prevZone != currentZone) {
        if (currentZone == 2 && !dimensionAlterna) printTypewriter("ESCENA 2: SALA DE CONTROL \nLuz verde tenue. Computadoras encendidas solas.");
        if (currentZone == 3 && !dimensionAlterna) printTypewriter("ESCENA 3: LABORATORIO PRINCIPAL\nEncuentras la esfera central del experimento. Necesitas baterias.");
    }

    bool justPressedE = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyWasPressed) {
            justPressedE = true;
            eKeyWasPressed = true;
            tryOpenDoor(window);

            if (!portalActivado && currentZone == 3) {
                float distA_Consola = glm::length(cameraPos - glm::vec3(25.0f, 0.0f, 7.0f));
                if (distA_Consola < 2.0f) {
                    if (bateriasRecolectadas >= 3) {
                        portalActivado = true;
                        dimensionAlterna = true;
                        std::cout << "\n=========================================================" << std::endl;
                        printTypewriter("ESCENA 4 & 5: ACTIVACION Y DISTORSION DE LA REALIDAD");
                        std::cout << "[SISTEMA REACTIVADO]... INICIANDO SECUENCIA DE COPIA." << std::endl;
                        std::cout << "[ADVERTENCIA]... ANOMALIA DETECTADA EN LA REPLICACION." << std::endl;
                        std::cout << "El entorno pierde estabilidad. Los objetos empiezan a flotar." << std::endl;
                        printTypewriter("NO ES UNA COPIA... ESTA APRENDIENDO. CORRE.");
                        std::cout << "=========================================================\n" << std::endl;
                        ma_engine_play_sound(&audioEngine, "assets/start.wav", NULL);
                    } else {
                        std::cout << "\n[CONSOLA]: Energia principal fuera de linea. Faltan " << 3 - bateriasRecolectadas << " Baterias.\n" << std::endl;
                    }
                }
            }
        }
    } else {
        eKeyWasPressed = false;
    }

    for (auto& entity : gameEntities) {
        if (!entity.active) continue;

        float distancia = glm::length(entity.pos - cameraPos);
        glm::vec3 realDirToEntity = glm::normalize(entity.pos - cameraPos);
        float lookAngle = glm::dot(cameraFront, realDirToEntity);

        if (entity.type == 0 || entity.type == 1 || entity.type == 8 || entity.type == 9) {
            if (distancia < 1.5f && justPressedE) {
                entity.active = false;
                ma_engine_play_sound(&audioEngine, "assets/collect.wav", NULL);

                if (entity.type == 0) {
                    printTypewriter(entity.text);
                } else if (entity.type == 1) {
                    bateriasRecolectadas++;
                    std::cout << "\n[BATERIA RECOLECTADA]: Tienes " << bateriasRecolectadas << " / 3\n" << std::endl;
                } else if (entity.type == 8) {
                    hasKeycardLvl1 = true;
                    std::cout << "\n[OBJETO CLAVE]: Has obtenido la TARJETA AMARILLA (Nivel 1).\n" << std::endl;
                    openDocument(
                        "TARJETA AMARILLA - NIVEL 1",
                        "Autorizacion: Sala de Control.\n\n"
                        "Personal permitido: mantenimiento y soporte.\n"
                        "Observacion manuscrita:\n"
                        "\"Si la puerta se abre sola, no entres.\""
                    );
                } else if (entity.type == 9) {
                    hasKeycardLvl2 = true;
                    std::cout << "\n[OBJETO CLAVE]: Has obtenido la TARJETA ROJA (Nivel 2).\n" << std::endl;
                    openDocument(
                        "TARJETA ROJA - NIVEL 2",
                        "Autorizacion: Laboratorio principal.\n\n"
                        "Acceso restringido a personal senior.\n"
                        "Nota de emergencia:\n"
                        "\"No activen el nucleo sin las baterias. La copia ya no obedece.\""
                    );
                }
            }
        } else if (entity.type == 3 || entity.type == 4 || entity.type == 5 || entity.type == 6 || entity.type == 7) {
            if (distancia < 3.0f && lookAngle > 0.92f && justPressedE) {
                if (!entity.text.empty()) {
                    printTypewriter(entity.text);
                } else if (entity.type == 4) {
                    printTypewriter("[CAJON]: Esta vacio o atascado.");
                }
            }
        } else if (entity.type == 2 && portalActivado) {
            float entityLookAngle = glm::dot(cameraFront, -realDirToEntity);

            if (entityLookAngle < 0.5f) {
                float speed = 4.5f * deltaTime;
                if (!checkCollision(entity.pos.x + realDirToEntity.x * speed, entity.pos.z)) entity.pos.x += realDirToEntity.x * speed;
                if (!checkCollision(entity.pos.x, entity.pos.z + realDirToEntity.z * speed)) entity.pos.z += realDirToEntity.z * speed;
                entity.pos.y = 0.0f;
            }

            if (distancia < 0.9f) {
                gameState = GAMEOVER;
                std::cout << "\n=========================================================" << std::endl;
                printTypewriter("ESCENA 9: FALLO TOTAL");
                std::cout << "La silueta humanoide se retuerce frente a ti." << std::endl;
                std::cout << "Sus facciones se asientan. Son... las tuyas." << std::endl;
                std::cout << "La entidad ha imitado perfectamente tu postura." << std::endl;
                printTypewriter("COPIA COMPLETA. HAS SIDO REEMPLAZADO.");
                std::cout << "=========================================================\n" << std::endl;
            }
        }
    }

    if (door1Opening && door1Anim < 90.0f) {
        door1Anim += 120.0f * deltaTime;
        if (door1Anim > 90.0f) door1Anim = 90.0f;
    }
    if (door2Opening && door2Anim < 90.0f) {
        door2Anim += 120.0f * deltaTime;
        if (door2Anim > 90.0f) door2Anim = 90.0f;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (gameState != PLAYING || !isCursorLocked) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.15f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void printNodeHierarchy(const aiNode* node, int depth) {
    if (!node || depth > 12) return;
    for (int i = 0; i < depth; i++) std::cout << "  ";
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);
    std::cout << "- Node: " << node->mName.C_Str() << " | Pos: (" << position.x << ", " << position.y << ", " << position.z
              << ") | Scale: (" << scaling.x << ", " << scaling.y << ", " << scaling.z << ")" << std::endl;
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        printNodeHierarchy(node->mChildren[i], depth + 1);
    }
}
