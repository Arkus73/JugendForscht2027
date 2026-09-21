#include <cglm/cglm.h>
#include "material.h"
#include "string.h"
#include "utils.h"

Material* createMaterial(vec3 albedo, vec2 roughness, float metallic, float transmittance, float IOR, vec3 emissionColour, float emissionStrength, Material* material, DynamicArray* materialInstanceTracker) {

    Material* this = malloc(sizeof(Material));
    if(this == NULL) {
        throwException("Couldn't allocate memory for material");
    }

    // Wird ein Material als "Blueprint" angegeben, wird dieses kopiert und ausgegeben
    if(material == NULL) {
        glm_vec3_copy(albedo, this->albedo);
        glm_vec2_copy(roughness, this->roughness);
        this->metallic = metallic;
        this->transmittance = transmittance;
        this->IOR = IOR;
        glm_vec3_copy(emissionColour, this->emissionColour);
        this->emissionStrength = emissionStrength;
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