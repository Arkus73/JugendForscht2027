#ifndef MATERIAL
#define MATERIAL

#include <cglm/cglm.h>
#include "dynamicArray.h"

typedef struct {
    vec3 colour;
    float smoothness;
    vec3 emissionColour;
    float emissionStrength;
} Material;

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness, Material* material, DynamicArray* materialInstanceTracker);
void destroyMaterial(Material* this);
void disposeMaterialBlueprints(DynamicArray* materialInstanceTracker);

#endif