#ifndef MATERIAL
#define MATERIAL

#include <cglm/cglm.h>

typedef struct {
    vec3 colour;
    vec3 emissionColour;
    float emissionStrength;
} Material;

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength);
void destroyMaterial(Material* this);

#endif