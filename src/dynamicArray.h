#include <stdlib.h>
#include <stdbool.h>

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

typedef struct {
    void* ptr;
    int len;
    int capacity;
    size_t elementSize;
} DynamicArray;

#define TO_VALUE(type) *(type*)

void destroyDynamicArray(DynamicArray* this);
DynamicArray* createDynamicArray(size_t elementSize, int startCapacity);

void addToDynamicArray(DynamicArray* this, void* item);
void addFloatToDynamicArray(DynamicArray* this, float item);
void addIntToDynamicArray(DynamicArray* this, int item);
void addUnsignedIntToDynamicArray(DynamicArray* this, unsigned int item);

void removeByIndexFromDynamicArray(DynamicArray* this, int index);
void removeByValueFromDynamicArray(DynamicArray* this, void* value);

void* getFromDynamicArray(DynamicArray* this, int index);

bool doesDynamicArrayContain(DynamicArray* this, void* value);

#endif