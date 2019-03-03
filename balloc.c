#include "balloc.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>

#define BIT(x) (1u << (x))
#define POW2(x) (1u << (x))

static bool balloc_is_pow2(size_t val)
{
	return val && !(val & (val - 1));
}

/* naive log2 implementation for pow2 numbers */
static size_t balloc_log2(size_t val)
{
	size_t res = 0;

	if (!val)
		return 0;

	while (!(val & 1)) {
		val >>= 1;
		res++;
	}

	return res;
}

int balloc_cb_init(struct balloc_cb *cb, size_t pool_size, size_t min_block_size)
{
	size_t state_map_size;
	size_t i;

	memset(cb, 0, sizeof(*cb));

	cb->pool_size = pool_size;
	cb->min_block_size = min_block_size;

	if (!balloc_is_pow2(pool_size))
		return 1;

	if (!balloc_is_pow2(min_block_size))
		return 2;

	if (min_block_size >= pool_size)
		return 3;

	if (min_block_size < sizeof(*cb->free_blocks))
		return 4;

	cb->level_count = balloc_log2(pool_size / min_block_size) + 1;

	/* Allocate memory for all internal structures */
	cb->free_blocks = malloc(cb->level_count * sizeof(*cb->free_blocks));

	if (!cb->free_blocks)
		goto out_err;

	state_map_size = POW2(cb->level_count) - 1; /* 2 ^ (level_count) - 1, in bits */
	state_map_size = (state_map_size + CHAR_BIT - 1) / CHAR_BIT; /* In bytes */
	cb->block_state_map = malloc(state_map_size);

	cb->mem_pool = aligned_alloc(min_block_size, pool_size);

	/* All internal structures allocated */
	for (i = 0; i < cb->level_count; i++)
		list_head_init(&cb->free_blocks[i]);

	memset(cb->block_state_map, 0, state_map_size);

	/* Add whole memory pool as one big free block */
	list_add(&cb->free_blocks[0], (struct list_entry *)cb->mem_pool);

	return 0;
out_err:
	balloc_cb_free(cb);
	return 5;
}

void balloc_cb_free(struct balloc_cb *cb)
{
	free(cb->free_blocks);
	free(cb->block_state_map);
	free(cb->mem_pool);
}

static size_t balloc_block_level(const struct balloc_cb *cb, size_t size)
{
	size_t level = cb->level_count - 1;
	size_t level_block_size = cb->min_block_size;

	while (level_block_size <= cb->pool_size) {
		if (size <= level_block_size)
			return level;

		level_block_size *= 2;
		level--;
	}

	return level;
}

static size_t balloc_block_parent_idx(size_t idx)
{
	return (idx - 1) / 2;
}

static size_t balloc_block_buddy_idx(size_t idx)
{
	return ((idx - 1) ^ 1) + 1;
}

static size_t balloc_block_child_left_idx(size_t idx)
{
	return idx * 2 + 1;
}

static size_t balloc_block_child_right_idx(size_t idx)
{
	return idx * 2 + 2;
}

static void balloc_block_state_used_set(struct balloc_cb *cb, size_t idx)
{
	cb->block_state_map[idx / CHAR_BIT] |= BIT(idx % CHAR_BIT);
}

static void balloc_block_state_used_clear(struct balloc_cb *cb, size_t idx)
{
	cb->block_state_map[idx / CHAR_BIT] &= ~BIT(idx % CHAR_BIT);
}

static bool balloc_block_is_used(struct balloc_cb *cb, size_t idx)
{
	return cb->block_state_map[idx / CHAR_BIT] & BIT(idx % CHAR_BIT);
}

static size_t balloc_block_level_size(const struct balloc_cb *cb, size_t level)
{
	return cb->min_block_size * POW2(cb->level_count - level - 1);
}

static size_t balloc_block_to_idx(const struct balloc_cb *cb, const void *block, size_t level)
{
	const uintptr_t base_addr = (uintptr_t)cb->mem_pool;
	const uintptr_t block_addr = (uintptr_t)block;
	const size_t block_offset = block_addr - base_addr;
	size_t idx;

	assert(block_addr >= base_addr);

	idx = POW2(level) - 1; /* base level offset */
	idx += block_offset / balloc_block_level_size(cb, level);

	return idx;
}

static struct list_entry *balloc_alloc_split(struct balloc_cb *cb, size_t target_level)
{
	struct list_entry *free_blocks_head;
	struct list_entry *free_block;
	size_t level = target_level;
	size_t block_idx;
	size_t size;

	while (true) {
		free_blocks_head = &cb->free_blocks[level];

		if (!list_is_empty(free_blocks_head) || !level)
			break;

		level--;
	}

	if (level == target_level || list_is_empty(free_blocks_head))
		return NULL;

	free_block = list_pop_head(free_blocks_head);
	block_idx = balloc_block_to_idx(cb, free_block, level);
	size = balloc_block_level_size(cb, level);

	while (level < target_level) {
		struct list_entry *buddy;

		balloc_block_state_used_set(cb, block_idx);
		level++;
		size /= 2;
		free_blocks_head = &cb->free_blocks[level];

		buddy = (struct list_entry *)((uintptr_t)free_block + size);
		list_add_head(free_blocks_head, (struct list_entry *)buddy);

		block_idx = balloc_block_child_left_idx(block_idx);
	}

	printf("balloc_alloc_split %zu %zu %zu\n", level, block_idx, size);

	balloc_block_state_used_set(cb, block_idx);
	return free_block;
}

void *balloc_alloc(struct balloc_cb *cb, size_t size)
{
	const size_t level = balloc_block_level(cb, size);
	struct list_entry *free_blocks_head;
	struct list_entry *free_block;

	if (size > cb->pool_size)
		return NULL;

	printf("size %zu level %zu\n", size, level);

	free_blocks_head = &cb->free_blocks[level];
	free_block = list_pop_head(free_blocks_head);

	if (free_block) {
		size_t block_idx = balloc_block_to_idx(cb, free_block, level);

		balloc_block_state_used_set(cb, block_idx);
		return free_block;
	}

	return balloc_alloc_split(cb, level);
}

static size_t balloc_block_get_level(struct balloc_cb *cb, const void *block, size_t *idx_res)
{
	size_t level = cb->level_count - 1;
	size_t idx = balloc_block_to_idx(cb, block, level);

	while (!balloc_block_is_used(cb, idx) && level > 0) {
		idx = balloc_block_parent_idx(idx);
		level--;
	}

	*idx_res = idx;

	return level;
}

static struct list_entry *balloc_block_buddy(struct list_entry *block, size_t idx, size_t size)
{
	if (idx % 2)
		return (struct list_entry *)((uintptr_t)block + size);
	else
		return (struct list_entry *)((uintptr_t)block - size);
}

void balloc_free(struct balloc_cb *cb, void *ptr)
{
	struct list_entry *block = (struct list_entry *)ptr;
	size_t idx = 0;
	size_t level;
	size_t size;

	if (!block)
		return;

	level = balloc_block_get_level(cb, block, &idx);
	size = balloc_block_level_size(cb, level);

	while (true) {
		size_t buddy_idx = balloc_block_buddy_idx(idx);
		struct list_entry *buddy_block;

		balloc_block_state_used_clear(cb, idx);

		printf("balloc_free %zu %zu %zu\n", level, size, idx);

		if (!level || balloc_block_is_used(cb, buddy_idx)) {
			struct list_entry *free_blocks_head = &cb->free_blocks[level];

			list_add_head(free_blocks_head, block);
			break;
		}

		buddy_block = balloc_block_buddy(block, idx, size);
		list_del(buddy_block);

		level--;
		size *= 2;
		idx = balloc_block_parent_idx(idx);
	}
}
