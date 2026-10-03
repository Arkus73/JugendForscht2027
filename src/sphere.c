#include <cglm/cglm.h>
#include <glad/glad.h>
#include "material.h"
#include "shader.h"
#include "sphere.h"
#include "string.h"
#include "utils.h"

Sphere* createSphere(vec3 center, float radius, Material* material, bool dynamic, DynamicArray* sphereInstanceTracker) {

    Sphere* this = malloc(sizeof(Sphere));
    if(this == NULL) {
        throwException("Couldn't allocate memory for sphere");
    }

    this->index = sphereInstanceTracker->len;
    glm_vec3_copy(center, this->center);
    this->radius = radius;
    this->dynamic = dynamic;
    this->material = createMaterial(GLM_VEC3_ZERO, GLM_VEC2_ZERO, 0.0f, 0.0f, 0.0f, NULL, material, NULL);

    addToDynamicArray(sphereInstanceTracker, &this);

    return this;
}

void destroySphere(Sphere* this) {
    destroyMaterial(this->material);
    free(this);
}

void destroyAllSpheres(DynamicArray* sphereInstanceTracker) {
    for(int i = 0; i < sphereInstanceTracker->len; i++) {
        Sphere* sphere = TO_VALUE(Sphere*) getFromDynamicArray(sphereInstanceTracker, i);
        if(sphere != NULL) {
            destroySphere(sphere);
        }
    }
    destroyDynamicArray(sphereInstanceTracker);
}

void disposeStaticSpheres(DynamicArray* sphereInstanceTracker) {
    int index = 0;
    for(int i = 0; i < sphereInstanceTracker->len; i++) {
        Sphere* sphere = TO_VALUE(Sphere*) getFromDynamicArray(sphereInstanceTracker, index);
        if(!sphere->dynamic) {
            // Ist die Sphere statisch wird sie aus der CPU-RAM gelöscht und mit einem NULL ersetzt
            destroySphere(sphere);
            removeByIndexFromDynamicArray(sphereInstanceTracker, index);
            void* null = NULL;
            addToDynamicArray(sphereInstanceTracker, &null);
        } else {
            index++;
        }
    }
}

unsigned int prepareSphereSSBO(DynamicArray* sphereInstanceTracker) {
    unsigned int sphereSSBO;
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sphereInstanceTracker->len * (sizeof(vec4) + sizeof(Material)), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSSBO);
    return sphereSSBO;
}

void uploadSphereSpatialData(Sphere* this, unsigned int sphereSSBO) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, this->index * sizeof(vec4), sizeof(vec4), (vec4) {this->center[0], this->center[1], this->center[2], this->radius});
}

void uploadSphereMaterialData(Sphere* this, DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, sphereInstanceTracker->len * sizeof(vec4) + this->index * sizeof(Material), sizeof(Material), this->material);
}

void uploadSphere(Sphere* this, DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO) {
    uploadSphereSpatialData(this, sphereSSBO);
    uploadSphereMaterialData(this, sphereInstanceTracker, sphereSSBO);
}

void uploadAllSpheres(DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO) {
    for(int i = 0; i < sphereInstanceTracker->len; i++) {
        uploadSphere(TO_VALUE(Sphere*) getFromDynamicArray(sphereInstanceTracker, i), sphereInstanceTracker, sphereSSBO);
    }
}