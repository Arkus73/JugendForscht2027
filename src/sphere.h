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
    char attribPrefix[20];
} Sphere;

Sphere* createSphere(vec3 center, float radius, Material* material, bool dynamic, DynamicArray* sphereInstanceTracker);
void destroySphere(Sphere* this);
void destroyAllSpheres(DynamicArray* sphereInstanceTracker);
void disposeStaticSpheres(DynamicArray* sphereInstanceTracker);

void uploadSphere(Sphere* this, Shader raytracer);
void uploadAllSpheres(DynamicArray* sphereInstanceTracker, Shader raytracer);
void uploadSphereSpatialData(Sphere* this, Shader raytracer);
void uploadSphereMaterialData(Sphere* this, Shader raytracer);


#endif