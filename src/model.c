#include "AABB.h"
#include <cglm/cglm.h>
#include <float.h>
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

    this->instances = createDynamicArray(sizeof(Model*), 1);
    this->totalVertexCount = 0;

    return this;
}

void deinitModelInstanceTracker(ModelInstanceTracker* this) {
    for(int i = 0; i < this->instances->len; i++) {
        Model* model = TO_VALUE(Model*) getFromDynamicArray(this->instances, i);
        if(model != NULL) {
            destroyModel(model);
        }
    }
    destroyDynamicArray(this->instances);
    free(this);
}

void extractBoundingBox(vec4* vertices, int vertexCount, AABB** boundingBox) {

    vec4 boxMin = {FLT_MAX, FLT_MAX, FLT_MAX, 0.0f};
    vec4 boxMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f};

    for(int i = 0; i < vertexCount; i++) {
        glm_vec3_minv(boxMin, vertices[i], boxMin);
        glm_vec3_maxv(boxMax, vertices[i], boxMax);
    }

    *boundingBox = createAABB(boxMin, boxMax);
}

Model* createModel(ModelInstanceTracker* modelInstanceTracker, vec4* vertices, int vertexCount, Material* material, vec3 position, vec3 scale, bool dynamic) {

    Model* this = malloc(sizeof(Model));
    if(this == NULL) {
        throwException("Couldn't allocate memory for model");
    }

    this->index = modelInstanceTracker->instances->len;

    this->vertices = malloc(vertexCount * sizeof(vec4));
    if(this->vertices == NULL) {
        throwException("Couldn't allocate memory for vertices");
    }
    memcpy(this->vertices, vertices, vertexCount * sizeof(vec4));
    this->vertexCount = vertexCount;
    this->vertexOffset = modelInstanceTracker->totalVertexCount;
    extractBoundingBox(vertices, vertexCount, &this->boundingBox);

    this->material = createMaterial(GLM_VEC3_ZERO, GLM_VEC2_ZERO, 0.0f, GLM_VEC3_ZERO, 0.0f, material, NULL);;
    glm_vec3_copy(position, this->position);
    glm_vec3_copy(scale, this->scale);
    this->dynamic = dynamic;

    addToDynamicArray(modelInstanceTracker->instances, &this);
    modelInstanceTracker->totalVertexCount += vertexCount;

    return this;
}

void destroyModel(Model* this) {
    destroyMaterial(this->material);
    destroyAABB(this->boundingBox);
    free(this->vertices);
    free(this);
}

unsigned int prepareSSBO(ModelInstanceTracker* modelInstanceTracker) {
    unsigned int modelSSBO;
    glGenBuffers(1, &modelSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelInstanceTracker->instances->len * (sizeof(ivec4) + sizeof(mat4) + sizeof(mat4) + sizeof(AABB) + sizeof(Material)) + modelInstanceTracker->totalVertexCount * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, modelSSBO);
    return modelSSBO;
}

void uploadModel(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    int instanceCount = modelInstanceTracker->instances->len;
    int baseOffset = 0;

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(ivec4), sizeof(ivec4), (ivec4) {this->vertexOffset, this->vertexCount, 0, 0});
    baseOffset += instanceCount * sizeof(ivec4);

    mat4 modelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(modelMatrix, this->position);
    glm_scale(modelMatrix, this->scale);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(mat4), sizeof(mat4), modelMatrix);
    baseOffset += instanceCount * sizeof(mat4);

    mat4 invModelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(modelMatrix, invModelMatrix);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(mat4), sizeof(mat4), invModelMatrix);
    baseOffset += instanceCount * sizeof(mat4);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(AABB), sizeof(AABB), this->boundingBox);
    baseOffset += instanceCount * sizeof(AABB);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->index * sizeof(Material), sizeof(Material), this->material);
    baseOffset += instanceCount * sizeof(Material);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + this->vertexOffset * sizeof(vec4), this->vertexCount * sizeof(vec4), this->vertices);
}

void uploadAllModels(ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO) {
    for(int i = 0; i < modelInstanceTracker->instances->len; i++) {
        uploadModel(TO_VALUE(Model*) getFromDynamicArray(modelInstanceTracker->instances, i), modelInstanceTracker, modelSSBO);
    }
}

void disposeStaticModels(ModelInstanceTracker* modelInstanceTracker) {
    int index = 0;
    for(int i = 0; i < modelInstanceTracker->instances->len; i++) {
        Model* model = TO_VALUE(Model*) getFromDynamicArray(modelInstanceTracker->instances, index);
        if(!model->dynamic) {
            // Ist das Model statisch wird es aus der CPU-RAM gelöscht und mit einem NULL ersetzt
            destroyModel(model);
            removeByIndexFromDynamicArray(modelInstanceTracker->instances, index);
            void* null = NULL;
            addToDynamicArray(modelInstanceTracker->instances, &null);
        } else {
            index++;
        }
    }
}

void uploadModelMatrix(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO) {

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    mat4 modelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_translate(modelMatrix, this->position);
    glm_scale(modelMatrix, this->scale);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, modelInstanceTracker->instances->len * sizeof(ivec4) + this->index * sizeof(mat4), sizeof(mat4), modelMatrix);

    mat4 invModelMatrix = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_inv(modelMatrix, invModelMatrix);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, modelInstanceTracker->instances->len * (sizeof(ivec4) + sizeof(mat4)) + this->index * sizeof(mat4), sizeof(mat4), invModelMatrix);

}