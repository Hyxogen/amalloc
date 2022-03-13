#include "amalloc.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define INCR_SIZE 32

#define PACK(size, cur_alloc, prev_alloc, nex_alloc) ((size) | (cur_alloc) | ((prev_alloc) << 1) | ((nex_alloc) << 2))

#define GETSIZE(hdr) (*(unsigned int *)(hdr) & ~0x7)
#define GET_CUR_ALLOC(hdr) (*(unsigned int *)(hdr) & 0x1)
#define GET_PRV_ALLOC(hdr) ((!!(*(unsigned int *)(hdr) & 0x2)))
#define GET_NEX_ALLOC(hdr) ((!!(*(unsigned int *)(hdr) & 0x4)))
#define GETDATA(hdr) ((unsigned int *)(hdr) + 1)
#define GETHDR(ptr) ((unsigned int *)(ptr) - 1)
#define GETFTR(hdr) ((char *)(hdr) + GETSIZE((hdr)) - 4)
#define GETHDRVAL(hdr) (*(unsigned int *)(hdr))

#define PUTW(addr, word) (*(unsigned int *)(addr) = (word))

#define NEXT(hdr) ((void*)((char *)(hdr) + GETSIZE(hdr)))
#define PREV(hdr) ((void*)((char *)(hdr) - GETSIZE((unsigned int *)(hdr) - 1)))

static void *g_start;
static void *g_end;

void _print_link(unsigned int *hdr) {
		fprintf(stdout, "size:%d bytes, <-%s- %s -%s->\n", GETSIZE(hdr), GET_PRV_ALLOC(hdr) ? "true" : "false", GET_CUR_ALLOC(hdr) ? "true" : "false", GET_NEX_ALLOC(hdr) ? "true" : "false");
}

int amalloc_init(void) {
	g_start = sbrk(INCR_SIZE + 16);
	g_end = (char *) g_start + INCR_SIZE + 16;
	if (g_start == (void *) -1)
		return -1;
	PUTW(g_start, PACK(0, 1, 1, 0));
	g_start = ((unsigned int *) g_start) + 1;
	PUTW(g_start, PACK(INCR_SIZE, 0, 1, 1));
	PUTW(NEXT(g_start), PACK(0, 1, 0, 1));
	fprintf(stdout, "%x\n", *(unsigned int *) g_start);
	return 0;
}

int _merge(void *hdr, void **out) {
	size_t new_size;

	*out = hdr;
	if (GET_CUR_ALLOC(hdr))
		return 0;
	if (!GET_PRV_ALLOC(hdr)) {
		new_size = GETSIZE(hdr) + GETSIZE(PREV(hdr));
		PUTW(PREV(hdr), new_size); 
		PUTW(GETFTR(hdr), new_size); 
		_merge(PREV(hdr), out);
		return 1;
	} else if (!GET_NEX_ALLOC(hdr)) {
		new_size = GETSIZE(hdr) + GETSIZE(NEXT(hdr));
		PUTW(hdr, new_size);
		PUTW(GETFTR(NEXT(hdr)), new_size);
		_merge(hdr, out);
		return 1;
	}
	return 0; 
}

void *_find_first_fit(size_t size) {
	void *cur;

	cur = g_start;
	while (GET_CUR_ALLOC(cur) || GETSIZE(cur) < size) {
		if (GETSIZE(cur) == 0 && GET_CUR_ALLOC(cur))
			return NULL;
		if (_merge(cur, &cur))
			continue;
		cur = NEXT(cur);
	}
	return cur;
}

size_t _split(void *hdr, size_t size) {
	size_t old;

	old = GETSIZE(hdr);
	if (old == size)
		return size;
	if (old - size < 8)
		size = GETSIZE(hdr);
	fprintf(stdout, "hdr val:%x, prev_alloc:%d\n", *(unsigned int *) hdr, GET_PRV_ALLOC(hdr));
	PUTW(hdr, PACK(size, 0, GET_PRV_ALLOC(hdr), !(old - size) && GET_NEX_ALLOC(hdr))); /* het kan zijn dat 2x 8 bytes free naast elkaar staan en dat de volgende keer wanneer je 8 bytes allocated er niet gemerged wordt en de volgede link dus wel free is*/
	if (old - size) {
		PUTW(NEXT(hdr), PACK(old - size, 0, 0, 0));
		PUTW(NEXT(hdr), PACK(old - size, 0, 0, GET_CUR_ALLOC(NEXT(NEXT(hdr)))));
		PUTW(GETFTR(NEXT(hdr)), GETHDRVAL(NEXT(hdr)));
	}
	return (GETSIZE(hdr) - size);
}

void *amalloc(size_t size) {
	void *find;
	size_t block_size;

	block_size = size + 4;
	if (block_size % 8)
		block_size += 8 - block_size % 8;
	find = _find_first_fit(block_size);
	if (!find)
		return (NULL);
	_split(find, block_size);
	PUTW(find, PACK(GETSIZE(find), 1, GET_PRV_ALLOC(find), GET_NEX_ALLOC(find)));
	if (!GET_PRV_ALLOC(find))
		PUTW(PREV(find), PACK(GETSIZE(PREV(find)), GET_CUR_ALLOC(PREV(find)), GET_PRV_ALLOC(PREV(find)), 1));
	PUTW(NEXT(find), PACK(GETSIZE(NEXT(find)), GET_CUR_ALLOC(NEXT(find)), 1, GET_NEX_ALLOC(NEXT(find))));
	return GETDATA(find);
}

void afree(void *ptr) {
	void *hdr;

	hdr = GETHDR(ptr);
	PUTW(hdr, PACK(GETSIZE(hdr), 0, GET_PRV_ALLOC(hdr), GET_NEX_ALLOC(hdr)));
	PUTW(PREV(hdr), PACK(GETSIZE(PREV(hdr)), GET_CUR_ALLOC(PREV(hdr)), GET_PRV_ALLOC(PREV(hdr)), 0));
	PUTW(NEXT(hdr), PACK(GETSIZE(NEXT(hdr)), GET_CUR_ALLOC(NEXT(hdr)), 0, GET_NEX_ALLOC(PREV(hdr))));

}

void adebug_print(void) {
	void *cur;

	cur = g_start;
	while (1) {
		_print_link(cur);
		if (GETSIZE(cur) == 0 && GET_CUR_ALLOC(cur))
			break;
		if (!GETSIZE(cur) && !GET_CUR_ALLOC(cur) && !GET_PRV_ALLOC(cur) && !GET_NEX_ALLOC(cur))
			break;
		cur = NEXT(cur);
	}
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
