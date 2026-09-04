#include "ii42_weighted_space_saving.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static size_t
ii42_weighted_space_saving_hash(uint32_t item)
{
    uint32_t value = item;

    value ^= value >> 16;
    value *= UINT32_C(0x7feb352d);
    value ^= value >> 15;
    value *= UINT32_C(0x846ca68b);
    value ^= value >> 16;
    return (size_t) value;
}

static bool
ii42_weighted_space_saving_counter_less(
    const ii42_weighted_space_saving *summary,
    size_t left_index,
    size_t right_index
)
{
    const ii42_weighted_space_saving_counter *left =
        &summary->counters[left_index];
    const ii42_weighted_space_saving_counter *right =
        &summary->counters[right_index];

    if (left->estimate != right->estimate)
    {
        return left->estimate < right->estimate;
    }
    return left->item > right->item;
}

static void
ii42_weighted_space_saving_heap_swap(
    ii42_weighted_space_saving *summary,
    size_t left,
    size_t right
)
{
    size_t left_index = summary->heap[left];
    size_t right_index = summary->heap[right];

    summary->heap[left] = right_index;
    summary->heap[right] = left_index;
    summary->counters[left_index].heap_position = right;
    summary->counters[right_index].heap_position = left;
}

static void
ii42_weighted_space_saving_heap_up(
    ii42_weighted_space_saving *summary,
    size_t position
)
{
    while (position > 0)
    {
        size_t parent = (position - 1U) / 2U;

        if (!ii42_weighted_space_saving_counter_less(
                summary,
                summary->heap[position],
                summary->heap[parent]))
        {
            break;
        }
        ii42_weighted_space_saving_heap_swap(summary, position, parent);
        position = parent;
    }
}

static void
ii42_weighted_space_saving_heap_down(
    ii42_weighted_space_saving *summary,
    size_t position
)
{
    for (;;)
    {
        size_t left = position * 2U + 1U;
        size_t right = left + 1U;
        size_t smallest = position;

        if (left < summary->len &&
            ii42_weighted_space_saving_counter_less(
                summary,
                summary->heap[left],
                summary->heap[smallest]))
        {
            smallest = left;
        }
        if (right < summary->len &&
            ii42_weighted_space_saving_counter_less(
                summary,
                summary->heap[right],
                summary->heap[smallest]))
        {
            smallest = right;
        }
        if (smallest == position)
        {
            break;
        }
        ii42_weighted_space_saving_heap_swap(summary, position, smallest);
        position = smallest;
    }
}

static bool
ii42_weighted_space_saving_hash_lookup(
    const ii42_weighted_space_saving *summary,
    uint32_t item,
    size_t *slot_out,
    size_t *counter_index_out
)
{
    size_t mask = summary->hash_capacity - 1U;
    size_t slot = ii42_weighted_space_saving_hash(item) & mask;

    for (size_t probe = 0; probe < summary->hash_capacity; probe++)
    {
        const ii42_weighted_space_saving_hash_slot *entry =
            &summary->hash_slots[slot];

        if (entry->counter_index_plus_one == 0)
        {
            *slot_out = slot;
            return false;
        }
        if (entry->item == item)
        {
            *slot_out = slot;
            *counter_index_out = entry->counter_index_plus_one - 1U;
            return true;
        }
        slot = (slot + 1U) & mask;
    }
    *slot_out = SIZE_MAX;
    return false;
}

static ii42_status
ii42_weighted_space_saving_hash_insert(
    ii42_weighted_space_saving *summary,
    uint32_t item,
    size_t counter_index
)
{
    size_t slot;
    size_t ignored_index = 0;

    if (ii42_weighted_space_saving_hash_lookup(
            summary,
            item,
            &slot,
            &ignored_index) ||
        slot == SIZE_MAX)
    {
        return II42_ERR_FORMAT;
    }
    summary->hash_slots[slot].item = item;
    summary->hash_slots[slot].counter_index_plus_one = counter_index + 1U;
    return II42_OK;
}

static ii42_status
ii42_weighted_space_saving_hash_remove(
    ii42_weighted_space_saving *summary,
    uint32_t item
)
{
    size_t mask = summary->hash_capacity - 1U;
    size_t hole;
    size_t ignored_index = 0;
    size_t scan;

    if (!ii42_weighted_space_saving_hash_lookup(
            summary,
            item,
            &hole,
            &ignored_index))
    {
        return II42_ERR_FORMAT;
    }
    scan = (hole + 1U) & mask;
    while (summary->hash_slots[scan].counter_index_plus_one != 0)
    {
        size_t home = ii42_weighted_space_saving_hash(
            summary->hash_slots[scan].item
        ) & mask;
        size_t home_distance = (scan - home) & mask;
        size_t hole_distance = (scan - hole) & mask;

        if (home_distance >= hole_distance)
        {
            summary->hash_slots[hole] = summary->hash_slots[scan];
            hole = scan;
        }
        scan = (scan + 1U) & mask;
    }
    memset(&summary->hash_slots[hole], 0, sizeof(summary->hash_slots[hole]));
    return II42_OK;
}

void
ii42_weighted_space_saving_init_empty(
    ii42_weighted_space_saving *summary
)
{
    if (summary != NULL)
    {
        memset(summary, 0, sizeof(*summary));
    }
}

ii42_status
ii42_weighted_space_saving_init(
    ii42_weighted_space_saving *summary,
    size_t capacity
)
{
    size_t hash_capacity = 8;

    if (summary == NULL || capacity == 0 || capacity > SIZE_MAX / 2U)
    {
        return II42_ERR_INVALID;
    }
    ii42_weighted_space_saving_free(summary);
    while (hash_capacity < capacity * 2U)
    {
        if (hash_capacity > SIZE_MAX / 2U)
        {
            return II42_ERR_RANGE;
        }
        hash_capacity *= 2U;
    }
    summary->counters = calloc(capacity, sizeof(*summary->counters));
    summary->hash_slots = calloc(
        hash_capacity,
        sizeof(*summary->hash_slots)
    );
    summary->heap = malloc(capacity * sizeof(*summary->heap));
    if (summary->counters == NULL || summary->hash_slots == NULL ||
        summary->heap == NULL)
    {
        ii42_weighted_space_saving_free(summary);
        return II42_ERR_NOMEM;
    }
    summary->capacity = capacity;
    summary->hash_capacity = hash_capacity;
    return II42_OK;
}

void
ii42_weighted_space_saving_free(ii42_weighted_space_saving *summary)
{
    if (summary == NULL)
    {
        return;
    }
    free(summary->heap);
    free(summary->hash_slots);
    free(summary->counters);
    memset(summary, 0, sizeof(*summary));
}

ii42_status
ii42_weighted_space_saving_offer(
    ii42_weighted_space_saving *summary,
    uint32_t item,
    double weight
)
{
    size_t slot;
    size_t counter_index = 0;
    double total_weight;

    if (summary == NULL || summary->capacity == 0 || !isfinite(weight) ||
        weight <= 0.0)
    {
        return II42_ERR_INVALID;
    }
    total_weight = summary->total_weight + weight;
    if (!isfinite(total_weight))
    {
        return II42_ERR_RANGE;
    }
    summary->total_weight = total_weight;
    if (ii42_weighted_space_saving_hash_lookup(
            summary,
            item,
            &slot,
            &counter_index))
    {
        ii42_weighted_space_saving_counter *counter =
            &summary->counters[counter_index];

        counter->estimate += weight;
        if (!isfinite(counter->estimate))
        {
            return II42_ERR_RANGE;
        }
        ii42_weighted_space_saving_heap_down(
            summary,
            counter->heap_position
        );
        return II42_OK;
    }
    if (summary->len < summary->capacity)
    {
        ii42_weighted_space_saving_counter *counter;

        counter_index = summary->len;
        counter = &summary->counters[counter_index];
        counter->item = item;
        counter->estimate = weight;
        counter->error = 0.0;
        counter->heap_position = summary->len;
        summary->heap[summary->len] = counter_index;
        summary->len++;
        if (ii42_weighted_space_saving_hash_insert(
                summary,
                item,
                counter_index) != II42_OK)
        {
            return II42_ERR_FORMAT;
        }
        ii42_weighted_space_saving_heap_up(
            summary,
            counter->heap_position
        );
        return II42_OK;
    }
    counter_index = summary->heap[0];
    {
        ii42_weighted_space_saving_counter *counter =
            &summary->counters[counter_index];
        double minimum = counter->estimate;
        ii42_status status = ii42_weighted_space_saving_hash_remove(
            summary,
            counter->item
        );

        if (status != II42_OK)
        {
            return status;
        }
        counter->item = item;
        counter->estimate = minimum + weight;
        counter->error = minimum;
        if (!isfinite(counter->estimate))
        {
            return II42_ERR_RANGE;
        }
        if (counter->error > summary->maximum_error)
        {
            summary->maximum_error = counter->error;
        }
        summary->replacements++;
        status = ii42_weighted_space_saving_hash_insert(
            summary,
            item,
            counter_index
        );
        if (status != II42_OK)
        {
            return status;
        }
    }
    ii42_weighted_space_saving_heap_down(summary, 0);
    return II42_OK;
}
