#ifndef AMALLOC_H
#define AMALLOC_H

#include <sys/types.h>

int amalloc_init(void);
void *amalloc(size_t size);
void afree(void *ptr);
void adebug_print(void);
void hexdump(void);

#endif
