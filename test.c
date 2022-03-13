#include "amalloc.h"
#include <stdio.h>

int main(void) {
	int *data, *extra;

	if (amalloc_init() < 0) {
		fprintf(stderr, "Failed to init amalloc\n");
		return 1;
	}
	hexdump();
	fprintf(stdout, "Initialized\n");
	data = amalloc(sizeof(int));
	fprintf(stdout, "first amalloc\n");
	extra = amalloc(sizeof(int));
	fprintf(stdout, "second amalloc\n");
	if (data == NULL || extra == NULL) {
		fprintf(stderr, "Failed to amalloc\n");
		return 1;
	}
	fprintf(stdout, "Done amallocing\n");
	*data = 42;
	*extra = 0x1234;
	printf("data (%p):%d\n", (void *) data, *data);
	printf("extra (%p):%d\n", (void *) extra, *extra);
	printf("data (%p):%d\n", (void *) data, *data);
	hexdump();
	afree(data);
	return (0);
}
