#ifndef II42_WEIGHTED_SPACE_SAVING_H
#define II42_WEIGHTED_SPACE_SAVING_H

#include "ii42_core.h"

#include <stddef.h>
#include <stdint.h>

typedef struct ii42_weighted_space_saving_counter
{
    uint32_t item;
    double estimate;
    double error;
    size_t heap_position;
} ii42_weighted_space_saving_counter;

typedef struct ii42_weighted_space_saving_hash_slot
{
    uint32_t item;
    size_t counter_index_plus_one;
} ii42_weighted_space_saving_hash_slot;

typedef struct ii42_weighted_space_saving
{
    ii42_weighted_space_saving_counter *counters;
    ii42_weighted_space_saving_hash_slot *hash_slots;
    size_t *heap;
    size_t capacity;
    size_t len;
    size_t hash_capacity;
    uint64_t replacements;
    double total_weight;
    double maximum_error;
} ii42_weighted_space_saving;

void ii42_weighted_space_saving_init_empty(
    ii42_weighted_space_saving *summary
);

ii42_status ii42_weighted_space_saving_init(
    ii42_weighted_space_saving *summary,
    size_t capacity
);

void ii42_weighted_space_saving_free(
    ii42_weighted_space_saving *summary
);

ii42_status ii42_weighted_space_saving_offer(
    ii42_weighted_space_saving *summary,
    uint32_t item,
    double weight
);

#endif
