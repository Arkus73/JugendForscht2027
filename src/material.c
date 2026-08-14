#include <cglm/cglm.h>
#include "material.h"
#include "string.h"
#include "utils.h"

Material* createMaterial(vec3 colour, vec3 emissionColour, float emissionStrength, float smoothness, Material* material, DynamicArray* materialInstanceTracker) {

    Material* this = malloc(sizeof(Material));
    if(this == NULL) {
        throwException("Couldn't allocate memory for material");
    }

    // Wird ein Material als "Blueprint" angegeben, wird dieses kopiert und ausgegeben
    if(material == NULL) {
        glm_vec3_copy(colour, this->colour);
        glm_vec3_copy(emissionColour, this->emissionColour);
        this->emissionStrength = emissionStrength;
        this->smoothness = smoothness;
        // Wird das Material eine "Blueprint" und wird nur zum kopieren durch Model-Instanzen verwendet, wird es registriert, um es nach der Szenenerstellung leicht aufräumen zu können
        addToDynamicArray(materialInstanceTracker, &this);
    } else {
        memcpy(this, material, sizeof(Material));
    }

    return this;
}

void destroyMaterial(Material* this) {
    free(this);
}

void disposeMaterialBlueprints(DynamicArray* materialInstanceTracker) {
    for(int i = 0; i < materialInstanceTracker->len; i++) {
        destroyMaterial(TO_VALUE(Material*) getFromDynamicArray(materialInstanceTracker, i));
    }
    destroyDynamicArray(materialInstanceTracker);
}