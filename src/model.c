#include <cglm/cglm.h>
#include <glad/glad.h>
#include "material.h"
#include "model.h"
#include "string.h"
#include "utils.h"

ModelInstanceTracker* initModelInstanceTracker() {
    ModelInstanceTracker* this = malloc(sizeof(ModelInstanceTracker));
    if(this == NULL) {
        throwException("Couldn't allocate memory for ModelInstanceTracker");
    }

    this->instanceCount = 0;
    this->totalVertexCount = 0;

    return this;
}

void deinitModelInstanceTracker(ModelInstanceTracker* this) {
    free(this);
}

Model* createModel(ModelInstanceTracker* modelInstanceTracker, vec4* vertices, int vertexCount, Material* material, vec3 position, vec3 scale) {

    Model* this = malloc(sizeof(Model));
    if(this == NULL) {
        throwException("Couldn't allocate memory for model");
    }

    this->index = modelInstanceTracker->instanceCount;
    this->vertices = malloc(vertexCount * sizeof(vec4));
    if(this->vertices == NULL) {
        throwException("Couldn't allocate memory for vertices");
    }
    memcpy(this->vertices, vertices, vertexCount * sizeof(vec4));
    this->vertexCount = vertexCount;
    this->vertexOffset = modelInstanceTracker->totalVertexCount;
    this->material = createMaterial(GLM_VEC3_ZERO, GLM_VEC3_ZERO, 0.0f, 0.0f, material);
    glm_vec3_copy(position, this->position);
    glm_vec3_copy(scale, this->scale);

    modelInstanceTracker->instanceCount++;
    modelInstanceTracker->totalVertexCount += vertexCount;

    return this;
}

void destroyModel(Model* this) {
    destroyMaterial(this->material);
    free(this->vertices);
    free(this);
}

unsigned int prepareSSBO(ModelInstanceTracker* modelInstanceTracker) {
    unsigned int modelSSBO;
    glGenBuffers(1, &modelSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelInstanceTracker->instanceCount * (sizeof(ivec4) + sizeof(mat4) + sizeof(Material)) + modelInstanceTracker->totalVertexCount * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, modelSSBO);
    return modelSSBO;
}

void uploadModel(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    int instanceCount = modelInstanceTracker->instanceCount;
    int baseOffset = 0;

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(ivec4), sizeof(ivec4), (ivec4) {this->vertexOffset, this->vertexCount, 0, 0});
    baseOffset += instanceCount * sizeof(ivec4);

    mat4 modelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(modelMatrix, this->position);
    glm_scale(modelMatrix, this->scale);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(mat4), sizeof(mat4), modelMatrix);
    baseOffset += instanceCount * sizeof(mat4);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(Material), sizeof(Material), this->material);
    baseOffset += instanceCount * sizeof(Material);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->vertexOffset * sizeof(vec4), this->vertexCount * sizeof(vec4), this->vertices);
}

void uploadModelMatrix(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    mat4 modelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(modelMatrix, this->position);
    glm_scale(modelMatrix, this->scale);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, modelInstanceTracker->instanceCount * sizeof(ivec4) + this->index * sizeof(mat4), sizeof(mat4), modelMatrix);
}