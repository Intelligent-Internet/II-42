#ifndef II42_BLOCK_RANGES_H
#define II42_BLOCK_RANGES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ii42_core.h"

typedef struct ii42_block_range
{
    uint32_t start_block;
    uint32_t block_count;
} ii42_block_range;

typedef struct ii42_block_range_inventory
{
    ii42_block_range *ranges;
    size_t range_count;
    size_t range_capacity;
    size_t observed_range_count;
    uint64_t reachable_block_count;
    uint64_t interior_unreachable_block_count;
    uint32_t highest_reachable_block_exclusive;
    uint32_t published_block_high_watermark;
    bool finalized;
} ii42_block_range_inventory;

typedef struct ii42_block_range_allocator
{
    ii42_block_range *ranges;
    size_t range_count;
    uint64_t available_block_count;
    uint64_t allocated_block_count;
    bool initialized;
} ii42_block_range_allocator;

void ii42_block_range_inventory_init(
    ii42_block_range_inventory *inventory
);

void ii42_block_range_inventory_free(
    ii42_block_range_inventory *inventory
);

ii42_status ii42_block_range_inventory_add(
    ii42_block_range_inventory *inventory,
    uint32_t start_block,
    uint32_t block_count
);

/*
 * Sort, deduplicate, and merge the exact live ranges below one checked root
 * high-water mark. Exact duplicate references are legal; partial overlap is
 * corruption because two distinct immutable objects may not own the same
 * physical page.
 */
ii42_status ii42_block_range_inventory_finalize(
    ii42_block_range_inventory *inventory,
    uint32_t published_block_high_watermark
);

void ii42_block_range_allocator_init(
    ii42_block_range_allocator *allocator
);

void ii42_block_range_allocator_free(
    ii42_block_range_allocator *allocator
);

/*
 * Build the exact complement of one finalized reachability inventory below
 * its checked high-water mark. Block zero must be reachable and is never
 * allocatable.
 */
ii42_status ii42_block_range_allocator_build(
    const ii42_block_range_inventory *inventory,
    ii42_block_range_allocator *allocator
);

/*
 * Build directly from canonical free ranges authenticated by one manifest.
 * The ranges must be sorted, non-adjacent, and lie below the checked root
 * high-water mark. Block zero is never reusable.
 */
ii42_status ii42_block_range_allocator_build_free(
    const ii42_block_range *ranges,
    size_t range_count,
    uint32_t published_block_high_watermark,
    ii42_block_range_allocator *allocator
);

/*
 * Consume the smallest free run that can hold block_count contiguous pages.
 * Equal-sized runs are selected in physical order.
 */
bool ii42_block_range_allocator_allocate_best_fit(
    ii42_block_range_allocator *allocator,
    uint32_t block_count,
    uint32_t *start_block_out
);

#endif
