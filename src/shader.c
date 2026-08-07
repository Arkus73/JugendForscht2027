#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "utils.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

char* loadShaderSource(const char* dir) {
    FILE* file = fopen(dir, "rb");  // Datei wird geöffnet
    if (file == NULL) {
        throwException("ERROR: Could not open file");
    }

    fseek(file, 0, SEEK_END);   // Größe der Datei wird ermittelt
    long size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(size + 1); // Buffer wird allokiert
    if (buffer == NULL) {
        fclose(file);
        throwException("ERROR: Memory allocation failed");
    }

    fread(buffer, 1, size, file);   // Buffer wird aus der Datei beschrieben
    buffer[size] = '\0';

    fclose(file);   // Datei wird geschlossen und der Buffer returned
    return buffer;
}

unsigned int createPartShader(const char* dir, int type) {
    const char* shaderSource = loadShaderSource(dir);     // Shader wird aus ShaderSource erstellt 
    unsigned int shader; 
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    int success;       // Mögliche Kompilierungsfehler werden geprintet
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if(type == GL_VERTEX_SHADER) {
            printf("ERROR::VERTEX::SHADER::COMPILATION_FAILED\n%s", infoLog);
        } else if(type == GL_FRAGMENT_SHADER) {
            printf("ERROR::FRAGMENT::SHADER::COMPILATION_FAILED\n%s", infoLog);
        } else {
            printf("ERROR::COMPUTE::SHADER::COMPILATION_FAILED\n%s", infoLog);
        }
    }
    free((void*)shaderSource);
    return shader;
}

unsigned int createShaderProgram(unsigned int vertexOrComputeShader, unsigned int fragmentShader, bool computeShader) {
    unsigned int shaderProgram;     // Shader werden in ShaderProgram zusammengefasst und angewendet
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexOrComputeShader);
    if(!computeShader) {
        glAttachShader(shaderProgram, fragmentShader);
    }
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);    // Mögliche Linkingfehler werden geprintet
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::LINKING::FAILED\n%s", infoLog);
    }
    return shaderProgram;
}

Shader createShader(const char* vertexDir, const char* fragmentDir) {
    Shader shader;
    unsigned int vertexShader = createPartShader(vertexDir, GL_VERTEX_SHADER);
    unsigned int fragmentShader = createPartShader(fragmentDir, GL_FRAGMENT_SHADER);
    shader.ID = createShaderProgram(vertexShader, fragmentShader, false);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shader;
}

Shader createComputeShader(const char* dir) {
    Shader shader;
    unsigned int computeShader = createPartShader(dir, GL_COMPUTE_SHADER);
    shader.ID = createShaderProgram(computeShader, 0, true);
    glDeleteShader(computeShader);
    return shader;
}

void useShader(Shader shader) {
    glUseProgram(shader.ID);
}

void setInt(Shader shader, const char* name, int value) {   // Kann auch genutzt werden, um uniform bool zu setzen
    glUniform1i(glGetUniformLocation(shader.ID, name), value);
}

void setFloat(Shader shader, const char* name, float value) {
    glUniform1f(glGetUniformLocation(shader.ID, name), value);
}

void setVec2(Shader shader, const char* name, vec2 vec) {
    glUniform2fv(glGetUniformLocation(shader.ID, name), 1, vec);
}

void setVec3(Shader shader, const char* name, vec3 vec) {
    glUniform3fv(glGetUniformLocation(shader.ID, name), 1, vec);
}

void setVec4(Shader shader, const char* name, vec4 vec) {
    glUniform4fv(glGetUniformLocation(shader.ID, name), 1, vec);
}

void setMatrix(Shader shader, const char* name, const mat4 mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, name), 1, GL_FALSE, (const GLfloat*) mat);
}


