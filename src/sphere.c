#include <cglm/cglm.h>
#include "material.h"
#include "shader.h"
#include "sphere.h"
#include "string.h"
#include "utils.h"

#define NUM_ATTRIBS 6
char attribSuffixes[NUM_ATTRIBS][50] = {"center", "radius", "material.colour", "material.emissionColour", "material.emissionStrength", "material.smoothness"};

Sphere* createSphere(vec3 center, float radius, Material* material, bool dynamic, DynamicArray* sphereInstanceTracker) {

    Sphere* this = malloc(sizeof(Sphere));
    if(this == NULL) {
        throwException("Couldn't allocate memory for sphere");
    }

    this->index = sphereInstanceTracker->len;
    glm_vec3_copy(center, this->center);
    this->radius = radius;
    this->dynamic = dynamic;
    this->material = createMaterial(GLM_VEC3_ZERO, GLM_VEC3_ZERO, 0.0f, 0.0f, material, NULL);
    snprintf(this->attribPrefix, 20, "spheres[%d].", this->index);

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

void uploadSphere(Sphere* this, Shader raytracer) {

    char attribs[NUM_ATTRIBS][60] = {"", "", "", "", ""};
    for(int i = 0; i < NUM_ATTRIBS; i++) {
        strcat(attribs[i], this->attribPrefix);
        strcat(attribs[i], attribSuffixes[i]);
    }

    setVec3(raytracer, attribs[0], this->center);
    setFloat(raytracer, attribs[1], this->radius);
    setVec3(raytracer, attribs[2], this->material->colour);
    setVec3(raytracer, attribs[3], this->material->emissionColour);
    setFloat(raytracer, attribs[4], this->material->emissionStrength);
    setFloat(raytracer, attribs[5], this->material->smoothness);
}

void uploadAllSpheres(DynamicArray* sphereInstanceTracker, Shader raytracer) {
    for(int i = 0; i < sphereInstanceTracker->len; i++) {
        uploadSphere(TO_VALUE(Sphere*) getFromDynamicArray(sphereInstanceTracker, i), raytracer);
    }
}

void uploadSphereSpatialData(Sphere* this, Shader raytracer) {

    char attribs[2][60] = {"", ""};
    for(int i = 0; i < 2; i++) {
        strcat(attribs[i], this->attribPrefix);
        strcat(attribs[i], attribSuffixes[i]);
    }

    setVec3(raytracer, attribs[0], this->center);
    setFloat(raytracer, attribs[1], this->radius);
}

void uploadSphereMaterialData(Sphere* this, Shader raytracer) {

    char attribs[NUM_ATTRIBS - 2][60] = {"", "", "", ""};
    for(int i = 0; i < NUM_ATTRIBS - 2; i++) {
        strcat(attribs[i], this->attribPrefix);
        strcat(attribs[i], attribSuffixes[i + 2]);
    }

    setVec3(raytracer, attribs[0], this->material->colour);
    setVec3(raytracer, attribs[1], this->material->emissionColour);
    setFloat(raytracer, attribs[2], this->material->emissionStrength);
    setFloat(raytracer, attribs[3], this->material->smoothness);
}