#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aObjColor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ObjColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int dimensionAlterna;
uniform float time;

void main() {
    vec3 finalPos = aPos;
    if (dimensionAlterna == 1) {
        finalPos.x += sin(time * 50.0 + aPos.y) * 0.05;
        finalPos.y += cos(time * 30.0 + aPos.z) * 0.02;
    }

    FragPos = vec3(model * vec4(finalPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    ObjColor = aObjColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
