#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 ObjColor;

uniform sampler2D texture1;
uniform vec3 objectColor;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform float cutOff;
uniform float outerCutOff;
uniform int flashlightOn;

uniform int dimensionAlterna;
uniform int currentZone;
uniform int allLightsOn;
uniform float time;
uniform vec2 resolution;
uniform int useSolidColor;
uniform float emissiveStrength;
uniform float globalDarkness;

struct PointLight {
    vec3 position;
    vec3 color;
    float radius;
};
#define MAX_POINT_LIGHTS 32
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float cutOff;
    float outerCutOff;
    float radius;
};
#define MAX_SPOT_LIGHTS 16
uniform int numSpotLights;
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

void main() {
    float ambientStrength = 0.05;
    vec3 ambientColor = vec3(1.0);
    vec3 flashColor = vec3(1.0);

    if (currentZone == 1) {
        ambientColor = vec3(0.6, 0.7, 0.8);
        flashColor = vec3(0.9, 0.9, 1.0);
        ambientStrength = 0.1 + (sin(time * 10.0) * 0.02);
    } else if (currentZone == 2) {
        ambientColor = vec3(0.4, 0.9, 0.5);
        flashColor = vec3(0.8, 1.0, 0.8);
        ambientStrength = 0.15;
    } else if (currentZone == 3) {
        ambientColor = vec3(0.3, 0.5, 1.0);
        flashColor = vec3(1.0, 1.0, 1.0);
        ambientStrength = 0.2;
    }

    if (allLightsOn == 1) {
        ambientColor = vec3(1.0, 1.0, 1.0);
        ambientStrength = 0.8;
    }

    if (dimensionAlterna == 1) {
        ambientColor = vec3(0.6, 0.0, 0.2);
        ambientStrength = 0.1 + (sin(time * 20.0) * 0.05) + (cos(time * 50.0) * 0.03);
        if(ambientStrength < 0.02) ambientStrength = 0.02;
        flashColor = vec3(1.0, 0.3, 0.3) * (0.7 + 0.3 * sin(time * 40.0));
    }

    vec3 ambient = ambientStrength * ambientColor;
    vec3 diffuse = vec3(0.0);

    if (flashlightOn == 1) {
        vec3 norm = normalize(Normal);
        vec3 lightDirVec = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDirVec), 0.0);
        diffuse = diff * flashColor;

        float theta = dot(lightDirVec, normalize(-lightDir));
        float epsilon = cutOff - outerCutOff;
        float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

        diffuse *= intensity * attenuation;
    }

    vec3 pointLightsDiffuse = vec3(0.0);
    vec3 norm = normalize(Normal);
    for(int i = 0; i < numPointLights; i++) {
        float radius = pointLights[i].radius;
        if (radius <= 0.0) continue;
        vec3 toLight = pointLights[i].position - FragPos;
        float distance = length(toLight);
        if (distance >= radius) continue;
        vec3 lightDirVec = toLight / max(distance, 0.0001);
        float diff = max(dot(norm, lightDirVec), 0.0);

        // Atenuación suave estilo Unreal: la luz muere exactamente en "radius" sin cortes feos
        float falloff = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
        float attenuation = falloff * falloff;

        pointLightsDiffuse += diff * pointLights[i].color * attenuation;
    }

    vec3 spotLightsDiffuse = vec3(0.0);
    for(int i = 0; i < numSpotLights; i++) {
        float radius = spotLights[i].radius;
        if (radius <= 0.0) continue;
        vec3 toLight = spotLights[i].position - FragPos;
        float distance = length(toLight);
        if (distance >= radius) continue;
        vec3 lightDirVec = toLight / max(distance, 0.0001);
        float diff = max(dot(norm, lightDirVec), 0.0);

        float theta = dot(lightDirVec, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);

        float falloff = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
        float attenuation = falloff * falloff;

        spotLightsDiffuse += diff * spotLights[i].color * intensity * attenuation;
    }

    diffuse += pointLightsDiffuse + spotLightsDiffuse;

    vec4 texColor = texture(texture1, TexCoord);
    if (useSolidColor == 1) {
        texColor = vec4(ObjColor, 1.0);
    } else if (texColor.a < 0.1) {
        discard;
    }

    vec3 result = (ambient + diffuse) * objectColor;

    if (emissiveStrength > 0.0) {
        result += ObjColor * emissiveStrength;
    }

    if (dimensionAlterna == 1) {
        vec2 uv = gl_FragCoord.xy / resolution;
        float distToCenter = distance(uv, vec2(0.5));
        result *= smoothstep(0.9, 0.2, distToCenter);
    }

    result *= globalDarkness;

    FragColor = texColor * vec4(result, 1.0);
}
