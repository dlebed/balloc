#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "balloc.h"


int main(void)
{
	struct balloc_cb cb;
	uint8_t *buf;
	int res;

	res = balloc_cb_init(&cb, 1024 * 1024, 1024);
	printf("balloc_cb_init: %d\n", res);

	if (res)
		return res;

	buf = balloc_alloc(&cb, 3000);

	if (!buf)
		printf("NULL\n");

	balloc_free(&cb, buf);

	balloc_cb_free(&cb);

	return 0;
}
