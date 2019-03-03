#ifndef _BALLOC_H
#define _BALLOC_H

#include <stddef.h>
#include <stdint.h>
#include "list.h"

struct balloc_cb {
	size_t	pool_size;	/* total memory pool size in bytes, should be power of 2 */
	size_t	min_block_size;	/* minimum block size in bytes, should be power of 2 */
	size_t	level_count;	/* log2(pool_size/min_block_size) */

	struct list_entry *free_blocks;	/* array of per-level free block lists */
	uint8_t	*block_state_map;	/* bitmap with state of block pairs */
	uint8_t *mem_pool;
};

int balloc_cb_init(struct balloc_cb *cb, size_t pool_size, size_t min_block_size);
void balloc_cb_free(struct balloc_cb *cb);

void *balloc_alloc(struct balloc_cb *cb, size_t size);
void balloc_free(struct balloc_cb *cb, void *ptr);

#endif /* _BALLOC_H */
