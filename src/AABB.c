#include "AABB.h"
#include "utils.h"

AABB* createAABB(vec4 min, vec4 max) {
    AABB* this = malloc(sizeof(AABB));
    if(this == NULL) {
        throwException("Couldn't allocate memory for AABB");
    }

    glm_vec4_copy(min, this->min);
    glm_vec4_copy(max, this->max);

    return this;
}

void destroyAABB(AABB* this) {
    free(this);
}