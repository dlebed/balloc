#include "balloc.h"

int balloc_cb_init(struct balloc_cb *cb, size_t pool_size, size_t min_block_size)
{
	cb->pool_size = pool_size;
	cb->min_block_size = min_block_size;


	return 0;
}

void balloc_cb_free(struct balloc_cb *cb)
{

}

void *balloc_alloc(struct balloc_cb *cb, size_t size)
{

	return NULL;
}

void balloc_free(struct balloc_cb *cb, void *ptr)
{

}
