#ifndef SPHERE
#define SPHERE

#include <cglm/cglm.h>
#include "dynamicArray.h"
#include "material.h"
#include "shader.h"

typedef struct {
    int index;
    vec3 center;
    float radius;
    bool dynamic;
    Material* material;
} Sphere;

Sphere* createSphere(vec3 center, float radius, Material* material, bool dynamic, DynamicArray* sphereInstanceTracker);
void destroySphere(Sphere* this);
void destroyAllSpheres(DynamicArray* sphereInstanceTracker);
void disposeStaticSpheres(DynamicArray* sphereInstanceTracker);

unsigned int prepareSphereSSBO(DynamicArray* sphereInstanceTracker);
void uploadSphereSpatialData(Sphere* this, unsigned int sphereSSBO);
void uploadSphereMaterialData(Sphere* this, DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO);
void uploadSphere(Sphere* this, DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO);
void uploadAllSpheres(DynamicArray* sphereInstanceTracker, unsigned int sphereSSBO);


#endif