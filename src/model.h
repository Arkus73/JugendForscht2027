#ifndef MODEL
#define MODEL

#include "AABB.h"
#include <cglm/cglm.h>
#include "dynamicArray.h"
#include "material.h"

typedef struct {
    int index;

    vec4* vertices;
    int vertexCount;
    int vertexOffset;
    AABB* boundingBox;
    
    Material* material;

    vec3 position;
    vec3 scale;
    bool dynamic;
} Model;

typedef struct {
    DynamicArray* instances;
    int totalVertexCount;
} ModelInstanceTracker;

ModelInstanceTracker* initModelInstanceTracker();
void deinitModelInstanceTracker(ModelInstanceTracker* this);
Model* createModel(ModelInstanceTracker* modelInstanceTracker, vec4* vertices, int vertexCount, Material* material, vec3 position, vec3 scale, bool dynamic);
void destroyModel(Model* this);
// Erst nach der Erstellung und vor dem Upload aller Models aufrufen
unsigned int prepareModelSSBO(ModelInstanceTracker* modelInstanceTracker);
void uploadModel(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO);
void uploadAllModels(ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO);
// CPU Daten der statischen, nicht änderbaren Modelle werden gelöscht, um unnötig allokierten Speicher freizugeben
void disposeStaticModels(ModelInstanceTracker* modelInstanceTracker);
void uploadModelMatrix(Model* this, ModelInstanceTracker* modelInstanceTracker, unsigned int modelSSBO);

#endif