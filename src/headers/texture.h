#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <stb_image.h>

#include <iostream>

// Desactivar las macros molestas de Windows para que std::max y std::min funcionen bien
#define NOMINMAX

inline unsigned int loadTexture(char const * path) {
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 4);
    if (data) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        GLenum format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        stbi_image_free(data);
        return textureID;
    } else {
        std::cout << "Textura fallo: " << path << std::endl;
        return 0;
    }
}

inline unsigned int loadTextureWithFallback(char const * path, unsigned int fallback) {
    unsigned int tex = loadTexture(path);
    return tex == 0 ? fallback : tex;
}

#endif // TEXTURE_H