#include <cglm/cglm.h>
#include "material.h"
#include "shader.h"
#include "sphere.h"
#include "string.h"
#include "utils.h"

#define NUM_ATTRIBS 5
char attribSuffixes[NUM_ATTRIBS][50] = {"center", "radius", "material.colour", "material.emissionColour", "material.emissionStrength"};

Sphere* createSphere(int index, vec3 center, float radius, vec3 colour, vec3 emissionColour, float emissionStrength) {

    Sphere* this = malloc(sizeof(Sphere));
    if(this == NULL) {
        throwException("Couldn't allocate memory for sphere");
    }

    this->index = index;
    glm_vec3_copy(center, this->center);
    this->radius = radius;
    this->material = createMaterial(colour, emissionColour, emissionStrength);
    snprintf(this->attribPrefix, 20, "spheres[%d].", this->index);

    return this;
}

void uploadSphere(Sphere* this, Shader raytracer) {

    char attribs[NUM_ATTRIBS][50] = {"", "", "", "", ""};
    for(int i = 0 ; i < NUM_ATTRIBS; i++) {
        strcat(attribs[i], this->attribPrefix);
        strcat(attribs[i], attribSuffixes[i]);
    }

    setVec3(raytracer, attribs[0], this->center);
    setFloat(raytracer, attribs[1], this->radius);
    setVec3(raytracer, attribs[2], this->material->colour);
    setVec3(raytracer, attribs[3], this->material->emissionColour);
    setFloat(raytracer, attribs[4], this->material->emissionStrength);
}

void uploadSphereSpatialData(Sphere* this, Shader raytracer) {

    char attribs[2][50] = {"", ""};
    for(int i = 0 ; i < 2; i++) {
        strcat(attribs[i], this->attribPrefix);
        strcat(attribs[i], attribSuffixes[i]);
    }

    setVec3(raytracer, attribs[0], this->center);
    setFloat(raytracer, attribs[1], this->radius);
}

void destroySphere(Sphere* this) {
    destroyMaterial(this->material);
    free(this);
}