#ifndef MATERIAL
#define MATERIAL

#include <cglm/cglm.h>

typedef struct {
    vec3 colour;
    vec3 emissionColour;
    float emissionStrength;
    float smoothness;
} Material;

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness);
void destroyMaterial(Material* this);

#endif