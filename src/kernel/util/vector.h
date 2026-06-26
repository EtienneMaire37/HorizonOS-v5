#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define VEC_MIN_ALLOC   8

#define DEFINE_VECTOR(name, type) \
typedef struct \
{ \
    size_t size; \
    size_t alloc; \
    uint8_t* data; \
} vector_##name##_t; \
static const vector_##name##_t vector_##name##_init = {.size = 0, .alloc = 0, .data = NULL}; \
static inline __attribute__((always_inline)) void vector_##name##_clear(vector_##name##_t* vec) \
{ \
    assert(vec); \
    vec->size = vec->alloc = 0; \
    free(vec->data); \
} \
static inline __attribute__((always_inline)) void vector_##name##_push_back(vector_##name##_t* vec, type new) \
{ \
    assert(vec); \
    if (vec->size + sizeof(type) > vec->alloc) \
    { \
        vec->alloc = (vec->alloc == 0) ? (VEC_MIN_ALLOC * sizeof(type)) : (vec->alloc * 2); \
        vec->data = realloc(vec->data, vec->alloc); \
    } \
    *(type*)&vec->data[vec->size] = new; \
    vec->size += sizeof(type); \
} \
static inline __attribute__((always_inline)) void vector_##name##_pop_back(vector_##name##_t* vec) \
{ \
    assert(vec); \
    if ((vec->size - sizeof(type)) * 2 <= vec->alloc) \
    { \
        vec->alloc = vec->alloc / 2; \
        if (vec->alloc < (VEC_MIN_ALLOC * sizeof(type))) \
            vec->alloc = (VEC_MIN_ALLOC * sizeof(type)); \
        vec->data = realloc(vec->data, vec->alloc); \
    } \
    vec->size -= sizeof(type); \
} \
static inline __attribute__((always_inline)) type* vector_##name##_at(vector_##name##_t* vec, size_t index) \
{ \
    assert (index * sizeof(type) < vec->size); \
    return (type*)&vec->data[sizeof(type) * index]; \
} \
static inline __attribute__((always_inline)) size_t vector_##name##_size(vector_##name##_t* vec) \
{ \
    return vec->size / sizeof(type); \
}
