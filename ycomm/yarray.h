#ifndef __YARRAY__
#define __YARRAY__

//  yarray is the linked list of small pieces of arrays designed for flexible insertion and deletion and fast index access.

#ifdef __cplusplus
extern "C" {
#endif

#include "ylist.h"

typedef struct _yarray yarray;
yarray *yarray_create(int fragment_size);
void yarray_destroy(yarray *array);
// destroy the array and the data is also free by ufree()
void yarray_destroy_custom(yarray *array, user_free ufree);
int yarray_push_back(yarray *array, void *data);
int yarray_push_front(yarray *array, void *data);
void *yarray_pop_back(yarray *array);
void *yarray_pop_front(yarray *array);
void *yarray_data(yarray *array, int index);
int yarray_empty(yarray *array);
int yarray_insert(yarray *array, int index, void *data);
void *yarray_delete(yarray *array, int index);
void yarray_delete_custom(yarray *array, int index, user_free ufree);
void yarray_fprintf(FILE *fp, yarray *array);

typedef int(*yarray_callback)(int index, void *data, void *addition);
int yarray_traverse(yarray *array, yarray_callback cb, void *addition);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // __YARRAY__