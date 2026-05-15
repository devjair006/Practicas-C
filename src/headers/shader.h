#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexSource = loadSource(vertexPath);
        std::string fragmentSource = loadSource(fragmentPath);

        unsigned int vertexShader = compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
        unsigned int fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);

        programId = glCreateProgram();
        glAttachShader(programId, vertexShader);
        glAttachShader(programId, fragmentShader);
        glLinkProgram(programId);

        int linkSuccess;
        glGetProgramiv(programId, GL_LINK_STATUS, &linkSuccess);
        if (!linkSuccess) {
            char infoLog[1024];
            glGetProgramInfoLog(programId, 1024, NULL, infoLog);
            std::cerr << "ERROR: Falla al enlazar shader program:\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    ~Shader() {
        if (programId != 0) {
            glDeleteProgram(programId);
        }
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const { glUseProgram(programId); }
    unsigned int id() const { return programId; }

private:
    unsigned int programId = 0;

    static std::string loadSource(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "ERROR: No se pudo abrir el archivo de shader: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static unsigned int compile(GLenum type, const std::string& source, const std::string& name) {
        unsigned int shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR: Falla al compilar shader (" << name << "):\n" << infoLog << std::endl;
        }
        return shader;
    }
};

#endif
