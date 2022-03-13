#include "amalloc.h"

#include <stdio.h>
#include <unistd.h>

#ifndef INCR_SIZE
#define INCR_SIZE 32
#elif INCR_SIZE <= 0
#error INCR_SIZE must be a positive integer
#endif

#define PACK(size, cur_alloc, prv_alloc) ((size) | (cur_alloc) | ((prv_alloc) << 1))
#define GET_SIZE(hdr) (*(unsigned int *)(hdr) & ~0x3)
#define GET_CUR_ALLOC(hdr) (*(unsigned int *)(hdr) & 0x1)
#define GET_PRV_ALLOC(hdr) (!!(*(unsigned int *)(hdr) & 0x2))
#define GET_PRV_FTR(hdr) ((void *)((char *)(hdr) - 4))
#define GET_FTR_ADDR(hdr) ((void *)((char *)(hdr) + GET_SIZE((hdr)) - 4))
#define GET_HDR_VAL(hdr) (*(unsigned int *)(hdr))

#define PUTW(addr, word) (*(unsigned int *)(hdr) = (unsigned int)(word))

#define NEXT(hdr) ((void *)((char *)(hdr) + GET_SIZE((hdr)) + 1))
#define PREV(hdr) ((void *)((char *)(hdr) - GET_SIZE(GET_PRV_FTR((hdr))) + 4))

static void *g_start;
static void *g_last_alloc;

void _print_hex(const void *addr, size_t size) {
	const char *cur;
	size_t ndx;

	cur = addr;
	for (ndx = 0; ndx < size; ndx++) {
		if (ndx % 8 == 0)
			fprintf(stdout, "%p: ", (void *) cur);
		fprintf(stdout, "%2x ", (unsigned int) *cur);
		if ((ndx + 1) % 8 == 0)
			fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
}

int _merge(void *hdr, void **out) {
	*out = hdr;
	if (GET_CUR_ALLOC(hdr))
		return 0;
	if (!GET_PRV_ALLOC(hdr)) {
		_merge(PREV(hdr), out);
		return 1;
	} else if (!GET_CUR_ALLOC(NEXT(hdr))) {
		PUTW(hdr, PACK(GET_SIZE(hdr), 0, 1));
		PUTW(GET_FTR_ADDR(hdr), GET_HDR_VAL(hdr));
		_merge(hdr, out);
		return 1;
	}
	return 0;
}


void *_find_best_fit(size_t size) {
	void *cur;

	cur = g_start;
	while (GET_CUR_ALLOC(cur) || GET_SIZE(cur) < size) {
		if (GET_SIZE(cur) == 0 && GET_CUR_ALLOC(cur))
			return NULL;
		if (_merge(cur, &cur))
			continue;
		cur = NEXT(cur);
	}
	return cur;
}

int amalloc_init(void) {
	g_start = sbrk(INCR_SIZE + 8);

	if (g_start == (void *) -1)
		return -1;
