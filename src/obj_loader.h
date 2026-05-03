#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

// Función para leer un archivo .obj y devolver los vértices intercalados listos para OpenGL
// Formato de salida: PosX, PosY, PosZ, NormX, NormY, NormZ, TexU, TexV, ColR, ColG, ColB (11 floats por vértice)
inline bool loadOBJ(const char * path, std::vector<float> & out_vertices) {
    std::cout << "Cargando archivo OBJ: " << path << "...\n";

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> faceColors; // Color for each face vertex

    std::vector<float> temp_vertices;  // x, y, z
    std::vector<float> temp_uvs;       // u, v
    std::vector<float> temp_normals;   // nx, ny, nz

    std::map<std::string, glm::vec3> materials;
    glm::vec3 current_color(1.0f, 1.0f, 1.0f); // Blanco por defecto

    std::string directory = std::string(path);
    size_t last_slash = directory.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        directory = directory.substr(0, last_slash + 1);
    } else {
        directory = "";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Imposible abrir el archivo: " << path << "!\n";
        return false;
    }

    std::string lineHeader;
    while (file >> lineHeader) {
        if (lineHeader == "mtllib") {
            std::string mtlFilename;
            file >> mtlFilename;
            std::string mtlPath = directory + mtlFilename;
            std::ifstream mtlFile(mtlPath);
            if (mtlFile.is_open()) {
                std::string mtlHeader;
                std::string currentMtl = "";
                while (mtlFile >> mtlHeader) {
                    if (mtlHeader == "newmtl") {
                        mtlFile >> currentMtl;
                        materials[currentMtl] = glm::vec3(1.0f);
                    } else if (mtlHeader == "Kd") {
                        float r, g, b;
                        mtlFile >> r >> g >> b;
                        if (currentMtl != "") {
                            materials[currentMtl] = glm::vec3(r, g, b);
                        }
                    } else {
                        std::string discard;
                        std::getline(mtlFile, discard);
                    }
                }
                std::cout << "  Cargada libreria de materiales: " << mtlFilename << "\n";
            } else {
                std::cout << "  [ADVERTENCIA] No se encontro el archivo de materiales: " << mtlPath << "\n";
            }
        } else if (lineHeader == "usemtl") {
            std::string mtlName;
            file >> mtlName;
            if (materials.find(mtlName) != materials.end()) {
                current_color = materials[mtlName];
            } else {
                current_color = glm::vec3(1.0f, 1.0f, 1.0f);
            }
        } else if (lineHeader == "v") {
            float x, y, z;
            file >> x >> y >> z;
            temp_vertices.push_back(x);
            temp_vertices.push_back(y);
            temp_vertices.push_back(z);
        } else if (lineHeader == "vt") {
            float u, v;
            file >> u >> v;
            temp_uvs.push_back(u);
            temp_uvs.push_back(1.0f - v); // Invertir V para OpenGL
        } else if (lineHeader == "vn") {
            float nx, ny, nz;
            file >> nx >> ny >> nz;
            temp_normals.push_back(nx);
            temp_normals.push_back(ny);
            temp_normals.push_back(nz);
        } else if (lineHeader == "f") {
            std::string line;
            std::getline(file, line);
            std::stringstream ss(line);
            std::string vertexStr;
            
            std::vector<unsigned int> faceV, faceUV, faceN;
            while (ss >> vertexStr) {
                unsigned int vIdx = 0, uvIdx = 0, nIdx = 0;
                // Parse v/vt/vn or v//vn
                int matches = sscanf(vertexStr.c_str(), "%u/%u/%u", &vIdx, &uvIdx, &nIdx);
                if (matches != 3) {
                    matches = sscanf(vertexStr.c_str(), "%u//%u", &vIdx, &nIdx);
                    if (matches == 2) uvIdx = 1; // dummy uv if missing
                    else {
                        matches = sscanf(vertexStr.c_str(), "%u/%u", &vIdx, &uvIdx);
                        if (matches == 2) nIdx = 1;
                        else {
                            sscanf(vertexStr.c_str(), "%u", &vIdx);
                            uvIdx = 1; nIdx = 1;
                        }
                    }
                }
                faceV.push_back(vIdx);
                faceUV.push_back(uvIdx);
                faceN.push_back(nIdx);
            }
            
            // Triangulate any n-gon (triangle fan approach)
            for (size_t i = 1; i + 1 < faceV.size(); i++) {
                vertexIndices.push_back(faceV[0]);
                uvIndices.push_back(faceUV[0]);
                normalIndices.push_back(faceN[0]);
                faceColors.push_back(current_color);

                vertexIndices.push_back(faceV[i]);
                uvIndices.push_back(faceUV[i]);
                normalIndices.push_back(faceN[i]);
                faceColors.push_back(current_color);

                vertexIndices.push_back(faceV[i+1]);
                uvIndices.push_back(faceUV[i+1]);
                normalIndices.push_back(faceN[i+1]);
                faceColors.push_back(current_color);
            }
        } else {
            // Saltamos el resto de la línea
            std::string discard;
            std::getline(file, discard);
        }
    }

    // Intercalamos los datos para OpenGL (Pos, Normal, TexCoord, Color = 11 floats)
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];
        glm::vec3 color = faceColors[i];

        // Position
        if (vertexIndex > 0 && (vertexIndex - 1) * 3 + 2 < temp_vertices.size()) {
            out_vertices.push_back(temp_vertices[(vertexIndex - 1) * 3 + 0]);
            out_vertices.push_back(temp_vertices[(vertexIndex - 1) * 3 + 1]);
            out_vertices.push_back(temp_vertices[(vertexIndex - 1) * 3 + 2]);
        } else {
            out_vertices.push_back(0.0f); out_vertices.push_back(0.0f); out_vertices.push_back(0.0f);
        }

        // Normal
        if (normalIndex > 0 && (normalIndex - 1) * 3 + 2 < temp_normals.size()) {
            out_vertices.push_back(temp_normals[(normalIndex - 1) * 3 + 0]);
            out_vertices.push_back(temp_normals[(normalIndex - 1) * 3 + 1]);
            out_vertices.push_back(temp_normals[(normalIndex - 1) * 3 + 2]);
        } else {
            out_vertices.push_back(0.0f); out_vertices.push_back(1.0f); out_vertices.push_back(0.0f);
        }

        // TexCoord
        if (uvIndex > 0 && (uvIndex - 1) * 2 + 1 < temp_uvs.size()) {
            out_vertices.push_back(temp_uvs[(uvIndex - 1) * 2 + 0]);
            out_vertices.push_back(temp_uvs[(uvIndex - 1) * 2 + 1]);
        } else {
            out_vertices.push_back(0.0f); out_vertices.push_back(0.0f);
        }
        
        // Color
        out_vertices.push_back(color.r);
        out_vertices.push_back(color.g);
        out_vertices.push_back(color.b);
    }

    std::cout << "Modelo cargado con " << out_vertices.size() / 11 << " vertices.\n";
    return true;
}

#endif
