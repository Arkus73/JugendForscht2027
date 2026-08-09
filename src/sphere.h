#ifndef SPHERE
#define SPHERE

#include <cglm/cglm.h>
#include "material.h"
#include "shader.h"

typedef struct {
    int index;
    vec3 center;
    float radius;
    Material* material;
    char attribPrefix[20];
} Sphere;

Sphere* createSphere(int index, vec3 center, float radius, vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness);
void destroySphere(Sphere* this);
void uploadSphere(Sphere* this, Shader raytracer);
void uploadSphereSpatialData(Sphere* this, Shader raytracer);
void uploadSphereMaterialData(Sphere* this, Shader raytracer);


#endif