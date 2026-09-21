#ifndef MATERIAL
#define MATERIAL

#include <cglm/cglm.h>
#include "dynamicArray.h"

typedef struct {
    vec3 albedo;
    float metallic;
    vec3 emissionColour;
    float emissionStrength;
    vec2 roughness;
    float transmittance;
    float IOR;
} Material;

Material* createMaterial(vec3 albedo, vec2 roughness, float metallic, float transmittance, float IOR, vec3 emissionColour, float emissionStrength, Material* material, DynamicArray* materialInstanceTracker);
void destroyMaterial(Material* this);
void disposeMaterialBlueprints(DynamicArray* materialInstanceTracker);

#endif