#include "amalloc.h"
#include <stdio.h>

int main(void) {
	int *data, *extra;
	long *large;

	if (amalloc_init() < 0) {
		fprintf(stderr, "Failed to init amalloc\n");
		return 1;
	}
	adebug_print();
	hexdump();
	fprintf(stdout, "Initialized\n");
	data = amalloc(sizeof(int));
	adebug_print();
	fprintf(stdout, "first amalloc\n");
	hexdump();
	extra = amalloc(sizeof(int));
	fprintf(stdout, "second amalloc\n");
	if (data == NULL || extra == NULL) {
		fprintf(stderr, "Failed to amalloc\n");
		return 1;
	}
	adebug_print();
	fprintf(stdout, "Done amallocing\n");
	*data = 42;
	*extra = 0x1234;
	printf("data (%p):%d\n", (void *) data, *data);
	printf("extra (%p):%d\n", (void *) extra, *extra);
	printf("data (%p):%d\n", (void *) data, *data);
	hexdump();
	afree(data);
	adebug_print();
	fprintf(stdout, "Second free\n");
	afree(extra);
	adebug_print();
	fprintf(stdout, "\n\n");
	large = amalloc(sizeof(*large));
	large = amalloc(sizeof(*large));
	adebug_print();
	fprintf(stdout, "%p\n", (void*) large);
	return (0);
}
