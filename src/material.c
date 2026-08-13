#include <cglm/cglm.h>
#include "material.h"
#include "string.h"
#include "utils.h"

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness, Material* material) {

    Material* this = malloc(sizeof(Material));
    if(this == NULL) {
        throwException("Couldn't allocate memory for material");
    }

    if(material == NULL) {
        glm_vec3_copy(colour, this->colour);
        glm_vec3_copy(emissionColour, this->emissionColour);
        this->emissionStrength = emissionStrength;
        this->smoothness = smoothness;
    } else {
        memcpy(this, material, sizeof(Material));
    }

    return this;
}

void destroyMaterial(Material* this) {
    free(this);
}