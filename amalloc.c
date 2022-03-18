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
#define GET_FTR_ADDR(hdr) ((void *)((char *)(hdr) + GET_SIZE((hdr))))
#define GET_HDR_VAL(hdr) (*(unsigned int *)(hdr))
#define GET_HDR_ADDR(data) ((void *)((char *)(data) - 4))
#define GET_DAT_ADDR(hdr) ((void *)((char *)(hdr) + 4))

#define PUTW(addr, word) (*(unsigned int *)(addr) = (unsigned int)(word))

#define NEXT(hdr) ((void *)((char *)(hdr) + GET_SIZE((hdr)) + 4))
#define PREV(hdr) ((void *)((char *)(hdr) - GET_SIZE(GET_PRV_FTR((hdr))) + 4))

static void *g_start;

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
		cur++;
	}
	fprintf(stdout, "\n");
}

void hexdump() {
	_print_hex(g_start, INCR_SIZE + 8);
}

void adebug_print() {
	void *cur;

	cur = NEXT(g_start);
	while (1) {
		fprintf(stdout, "<-%s- %s(%u)\n", GET_PRV_ALLOC(cur) ? "true" : "false ",
				GET_CUR_ALLOC(cur) ? "true" : "false", GET_SIZE(cur));
		if (GET_SIZE(cur) == 0 && GET_CUR_ALLOC(cur))
			return;
		cur = NEXT(cur);
	}
}
	

int _merge(void *hdr, void **out) {
	*out = hdr;
	if (GET_CUR_ALLOC(hdr))
		return 0;
	if (!GET_PRV_ALLOC(hdr)) {
		_merge(PREV(hdr), out);
		return 1;
	} else if (!GET_CUR_ALLOC(NEXT(hdr))) {
		PUTW(hdr, PACK(GET_SIZE(hdr) + GET_SIZE(NEXT(hdr)) + 4, 0, 1));
		PUTW(GET_FTR_ADDR(hdr), GET_HDR_VAL(hdr));
		_merge(hdr, out);
		return 1;
	}
	return 0;
}


void *_find_first_fit(size_t size) {
	void *cur;

	cur = NEXT(g_start);
	while (GET_CUR_ALLOC(cur) || GET_SIZE(cur) < size) {
		if (GET_SIZE(cur) == 0 && GET_CUR_ALLOC(cur))
			return NULL;
		if (_merge(cur, &cur))
			continue;
		cur = NEXT(cur);
	}
	return cur;
}

void _split(void *hdr, size_t size) {
	size_t old_size;

	old_size = GET_SIZE(hdr);
	if ((old_size - size) < 8)
		size = GET_SIZE(hdr);
	PUTW(hdr, PACK(size, GET_CUR_ALLOC(hdr), GET_PRV_ALLOC(hdr)));
	if (old_size - size) {
		PUTW(NEXT(hdr), PACK(old_size - size - 4, 0, GET_CUR_ALLOC(hdr)));
		PUTW(GET_FTR_ADDR(NEXT(hdr)), GET_HDR_VAL(NEXT(hdr)));
	}
}

int amalloc_init(void) {
	g_start = sbrk(INCR_SIZE + 8);

	if (g_start == (void *) -1)
		return -1;
	PUTW(g_start, PACK(0, 1, 1));
	PUTW(NEXT(g_start), PACK(INCR_SIZE - 4, 0, 1));
	PUTW(GET_FTR_ADDR(NEXT(g_start)), GET_HDR_VAL(NEXT(g_start)));
	PUTW(NEXT(NEXT(g_start)), PACK(0, 1, 0));
	return 0;
}

void *amalloc(size_t size) {
	void *loc;

	if (!size)
		return NULL;
	if (size % 4)
		size += 4 - (size % 4);
	loc = _find_first_fit(size + 4);
	if (!loc)
		return NULL;
	_split(loc, size);
	PUTW(loc, PACK(size, 1, GET_PRV_ALLOC(loc)));
	PUTW(NEXT(loc), PACK(GET_SIZE(NEXT(loc)), GET_CUR_ALLOC(NEXT(loc)), 1));
	return GET_DAT_ADDR(loc);
}

void afree(void *ptr) {
	void *hdr;

	hdr = GET_HDR_ADDR(ptr);
	PUTW(hdr, PACK(GET_SIZE(hdr), 0, GET_PRV_ALLOC(hdr)));
	PUTW(GET_FTR_ADDR(hdr), GET_HDR_VAL(hdr));
	PUTW(NEXT(hdr), PACK(GET_SIZE(NEXT(hdr)), GET_CUR_ALLOC(NEXT(hdr)), 0));
	if (!GET_CUR_ALLOC(NEXT(hdr)))
		PUTW(GET_FTR_ADDR(NEXT(hdr)), GET_HDR_VAL(NEXT(hdr)));
}
