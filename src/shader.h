#ifndef SHADER_H
#define SHADER_H

#include <cglm/cglm.h>

typedef struct {
    unsigned int ID;
} Shader;

Shader createShader(const char* vertexDir, const char* fragmentDir);
Shader createComputeShader(const char* dir);
void useShader(Shader shader);
void setInt(Shader shader, const char* name, int value);
void setFloat(Shader shader, const char* name, float value);
void setVec2(Shader shader, const char* name, vec2 vec);
void setVec3(Shader shader, const char* name, vec3 vec);
void setVec4(Shader shader, const char* name, vec4 vec);
void setMatrix(Shader shader, const char* name, const mat4 mat);

#endif