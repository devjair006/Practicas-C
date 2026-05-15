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
    uniform float time;
    uniform vec2 resolution;
    uniform int useSolidColor;

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

        vec4 texColor = texture(texture1, TexCoord);
        if (useSolidColor == 1) {
            texColor = vec4(ObjColor, 1.0); // Usar el color empaquetado del OBJ
        } else if (texColor.a < 0.1) {
            discard; 
        }
        
        vec3 result = (ambient + diffuse) * objectColor;
        
        if (dimensionAlterna == 1) {
            vec2 uv = gl_FragCoord.xy / resolution;
            float distToCenter = distance(uv, vec2(0.5));
            result *= smoothstep(0.9, 0.2, distToCenter);
        }
        
        FragColor = texColor * vec4(result, 1.0);
    }