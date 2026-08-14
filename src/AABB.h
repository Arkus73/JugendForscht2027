#ifndef AABB_H
#define AABB_H

#include <cglm/cglm.h>

typedef struct  {
    vec4 min, max;
} AABB;

AABB* createAABB(vec4 min, vec4 max);
void destroyAABB(AABB* this);

#endif