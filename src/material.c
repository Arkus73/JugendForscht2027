#include <cglm/cglm.h>
#include "material.h"
#include "utils.h"

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness) {

    Material* this = malloc(sizeof(Material));
    if(this == NULL) {
        throwException("Couldn't allocate memory for material");
    }

    glm_vec3_copy(colour, this->colour);
    glm_vec3_copy(emissionColour, this->emissionColour);
    this->emissionStrength = emissionStrength;
    this->smoothness = smoothness;

    return this;
}

void destroyMaterial(Material* this) {
    free(this);
}