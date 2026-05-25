#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aObjColor;
layout (location = 4) in ivec4 boneIds;
layout (location = 5) in vec4 weights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ObjColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 finalBonesMatrices[100];
uniform int isAnimated;

uniform int dimensionAlterna;
uniform float time;

void main() {
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    bool hasBoneInfluence = false;

    if (isAnimated == 1) {
        for(int i = 0 ; i < 4 ; i++) {
            if(boneIds[i] == -1) continue;
            if(boneIds[i] >= 100) break;

            hasBoneInfluence = true;
            vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos, 1.0f);
            totalPosition += localPosition * weights[i];
            vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
            totalNormal += localNormal * weights[i];
        }
    }

    if (!hasBoneInfluence) {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }

    vec3 finalPos = totalPosition.xyz;
    if (dimensionAlterna == 1) {
        finalPos.x += sin(time * 50.0 + totalPosition.y) * 0.05;
        finalPos.y += cos(time * 30.0 + totalPosition.z) * 0.02;
    }

    FragPos = vec3(model * vec4(finalPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    TexCoord = aTexCoord;
    ObjColor = aObjColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
