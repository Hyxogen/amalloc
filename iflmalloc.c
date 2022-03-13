#include "amalloc.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define INCR_SIZE 32

#define PACK(size, alloc) ((size) | (alloc))

#define GETSIZE(hdr) (*(unsigned int *)(hdr) & ~0x7)
#define GETALLOC(hdr) (*(unsigned int *)(hdr) & 0x1)
#define GETDATA(hdr) ((unsigned int *)(hdr) + 1)
#define GETHDR(data) ((unsigned int *)(data) - 1)

#define PUTW(addr, word) (*(unsigned int *)(addr) = (word))

#define NEXT(hdr) ((char *)(hdr) + GETSIZE(hdr) + 1)

static void *g_start;
static void *g_end;

int amalloc_init(void) {
	g_start = sbrk(INCR_SIZE + 8);
	g_end = (char *) g_start + INCR_SIZE + 8;
	if (g_start == (void *) -1)
		return -1;
	PUTW(g_start, PACK(INCR_SIZE, 0));
	PUTW(NEXT(g_start), PACK(0, 1));
	return 0;
}

void *_find_first_fit(size_t size) {
	void *cur;

	cur = g_start;
	while (GETALLOC(cur) || GETSIZE(cur) < size) {
		if (GETSIZE(cur) == 0 && GETALLOC(cur))
			return NULL;
		cur = NEXT(cur);
	}
	return cur;
}

size_t _split(void *addr, size_t size) {
	size_t old;

	old = GETSIZE(addr);
	if (GETSIZE(addr) - size < 8)
		size = GETSIZE(addr);
	PUTW(addr, PACK(size, 0));
	if (old - size)
		PUTW(NEXT(addr), PACK(old - size - 4, 0));
	return (GETSIZE(addr) - size);
}

void *amalloc(size_t size) {
	void *find;

	if (size < 8)
		size = 8;
	find = _find_first_fit(size);
	if (!find)
		return (NULL);
	_split(find, size);
	PUTW(find, PACK(GETSIZE(find), 1));
	fprintf(stdout, "final size:%u\n", GETSIZE(find));
	return GETDATA(find);
}

void afree(void *ptr) {
	if (GETALLOC(NEXT(ptr)) == 0)
		PUTW(ptr, PACK(GETSIZE(ptr) + GETSIZE(NEXT(ptr)) + 1, 0));
	else
		PUTW(ptr, PACK(GETSIZE(ptr), 0));
}

void adebug_print(void) {
	void *cur;

	cur = g_start;
	do {
		fprintf(stdout, "size:%d bytes, alloc:%s\n", GETSIZE(cur), GETALLOC(cur) ? "true" : "false");
		cur = NEXT(cur);
	} while (*(unsigned int *)cur != 1);
}

void hexdump(void) {
	char *cur;
	size_t index;

	index = 0;
	for (cur = g_start; cur != g_end; cur++) {
		if (index % 8 == 0)
			fprintf(stdout, "\n%p: ", cur);
		fprintf(stdout, "%2x ", (unsigned int) *cur);
		index++;
	}
	fprintf(stdout, "\n");
}
