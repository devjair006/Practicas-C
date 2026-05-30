#pragma once

#include <string>

struct aiNode;
struct GLFWwindow;

bool checkCollision(float x, float z);
void updateZone();
void printTypewriter(std::string text);
void openDocument(const std::string& title, const std::string& body);
void closeDocument();
void tryOpenDoor(GLFWwindow* window);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void printNodeHierarchy(const aiNode* node, int depth);
