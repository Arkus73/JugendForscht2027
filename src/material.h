#ifndef MATERIAL
#define MATERIAL

#include <cglm/cglm.h>
#include "dynamicArray.h"
#include "utils.h"

typedef struct {
    vec3 albedoSpectrumCoeffs;
    float metallic;
    vec2 roughness;
    float transmittance;
    float IOR;
    float emissionSpectrum[SPECTRAL_RESOLUTION];
    float padding[(SPECTRAL_RESOLUTION % 4 == 0) ? 0 : (4 - (SPECTRAL_RESOLUTION % 4))];    // Füllt das struct auf ein vielfaches von 16 Byte auf, damit das Alignment passt
} Material;

Material* createMaterial(vec3 albedoSpectrumCoeffs, vec2 roughness, float metallic, float transmittance, float IOR, float* emissionSpectrum, Material* material, DynamicArray* materialInstanceTracker);
void destroyMaterial(Material* this);
void disposeMaterialBlueprints(DynamicArray* materialInstanceTracker);

#endif