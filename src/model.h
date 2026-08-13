#ifndef MODEL
#define MODEL

#include <cglm/cglm.h>
#include "dynamicArray.h"
#include "material.h"

typedef struct {
    int index;

    vec4* vertices;
    int vertexCount;
    int vertexOffset;
    Material* material;

    vec3 position;
    vec3 scale;
} Model;

typedef struct {
    int instanceCount;
    int totalVertexCount;
} ModelInstanceTracker;

ModelInstanceTracker* initModelInstanceTracker();
void deinitModelInstanceTracker(ModelInstanceTracker* this);
Model* createModel(ModelInstanceTracker* modelInstanceTracker, vec4* vertices, int vertexCount, Material* material, vec3 position, vec3 scale);
void destroyModel(Model* this);
// Erst nach der Erstellung und vor dem Upload aller Models aufrufen
unsigned int prepareSSBO(ModelInstanceTracker* modelInstanceTracker);
void uploadModel(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO);
void uploadModelMatrix(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO);

#endif