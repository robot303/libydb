#ifndef __YALLOC__
#define __YALLOC__
#include <stdlib.h>

const char *ystrdup(char *src);
const char *ystrndup(char *src, int srclen);
const void *ydatadup(void *src, int srclen);
const void *ysearch(void *src, int srclen);
void yfree(const void *src);
void yfree_all();

#endif // __YALLOC__
