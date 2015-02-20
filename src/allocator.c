/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * Simple kernel resource object manager.
 *
 * This implements an allocator that is capable of two types of operations:
 *
 *     (i) Allocate a new kernel object of a given type;
 *
 *     (ii) Reset the allocator, destroying all items previously created.
 *
 * In particular, there is no method to free() individual kernel objects.
 *
 * An new allocator is created by providing a CNode to perform allocations
 * into, a single contiguous range of free cap slots in that CNode, and an
 * array of untyped memory items to use.
 */

#ifndef UNUSED_NDEBUG
# ifdef NDEBUG
#  define UNUSED_NDEBUG(x)  ((void)x)
# else
#  define UNUSED_NDEBUG()
# endif
#endif

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>

#include <twinkle/allocator.h>

static seL4_CPtr range_alloc(struct cap_range *range, int count);

/*
 * Initialise an allocator object at 'allocator'.
 *
 * The struct 'allocator' is memory where we should construct the allocator.
 * All state will be kept in this struct, allowing multiple independent
 * allocators to co-exist.
 *
 * 'root_cnode', 'root_cnode_depth', 'first_slot' and 'num_slots' specify
 * a CNode containing a contiguous range of free cap slots that we will use for
 * our allocations.
 *
 * 'items' and 'num_items' specify untyped memory items that we will allocate
 * from.
 */
void
allocator_create(struct allocator *allocator,
                 seL4_CPtr root_cnode, unsigned long root_cnode_depth,
                 unsigned long root_cnode_offset,
                 unsigned long first_slot, unsigned long num_slots,
                 struct untyped_item *items, int num_items)
{
    int i;

    /* Check parameters. */
    assert(num_items < MAX_UNTYPED_ITEMS);

    /* Setup CNode information. */
    allocator->root_cnode = root_cnode;
    allocator->root_cnode_depth = root_cnode_depth;
    allocator->root_cnode_offset = root_cnode_offset;
    allocator->cslots.first = first_slot;
    allocator->cslots.count = num_slots;
    allocator->num_slots_used = 0;
    allocator->num_init_untyped_items = 0;

    /* Setup all of our pools as empty. */
    for (i = MIN_UNTYPED_SIZE; i <= MAX_UNTYPED_SIZE; i++) {
        allocator->untyped_items[i - MIN_UNTYPED_SIZE].first = 0;
        allocator->untyped_items[i - MIN_UNTYPED_SIZE].count = 0;
    }

    /* Copy untyped items. */
    for (i = 0; i < num_items; i++)
        allocator_add_root_untyped_item(allocator,
                items[i].cap, items[i].size_bits);
}

/*
 * Create a new allocator from an existing allocator, stealing all resources
 * that the parent allocator has remaining.
 *
 * If this allocator is destroyed or reset, all items created by this allocator
 * will be revoked, but resources created by our parent will remain. If our
 * parent is destroyed or reset, all resources created by the child allocator
 * will also be revoked.
 */
void
allocator_create_child(struct allocator *parent, struct allocator *child,
        seL4_CPtr root_cnode, unsigned long root_cnode_depth,
        unsigned long root_cnode_offset,
        unsigned long first_slot, unsigned long num_slots)
{
    int i;

    /* Setup allocator. */
    allocator_create(child, root_cnode, root_cnode_depth,
                     root_cnode_offset, first_slot, num_slots, NULL, 0);

    /* Steal resources from our parent. */
    for (i = MAX_UNTYPED_SIZE; i >= MIN_UNTYPED_SIZE; i--) {
        while (1) {
            seL4_CPtr r = allocator_alloc_untyped(parent, i);
            if (!r) {
                break;
            }
            allocator_add_root_untyped_item(child, r, i);
        }
    }
}

/*
 * Permanently add additional untyped memory to the allocator.
 *
 * The allocator will permanently hold on to this memory and continue using it
 * until allocator_destroy() is called, even if the allocator is reset.
 */
void
allocator_add_root_untyped_item(struct allocator *allocator,
        seL4_CPtr cap, unsigned long size_bits)
{
    int n;

    assert(cap != 0);
    assert(size_bits >= MIN_UNTYPED_SIZE);
    assert(size_bits <= MAX_UNTYPED_SIZE);
    assert(allocator->num_init_untyped_items < MAX_UNTYPED_ITEMS);

    n = allocator->num_init_untyped_items;
    allocator->init_untyped_items[n].cap = cap;
    allocator->init_untyped_items[n].size_bits = size_bits;
    allocator->init_untyped_items[n].is_free = 1;
    allocator->num_init_untyped_items++;
}

/*
 * Allocate an empty cslot.
 */
seL4_CPtr
allocator_alloc_cslot(struct allocator *allocator)
{
    seL4_CPtr result;

    /* Determine whether we have any free slots. */
    if (!(allocator->cslots.count - allocator->num_slots_used)) {
        return 0;
    }

    /* Pick the first one. */
    result = allocator->cslots.first
             + allocator->num_slots_used + allocator->root_cnode_offset;

    /* Record this slot as used. */
    allocator->num_slots_used += 1;

    return result;
}

/*
 * Allocate empty cslots, those cslots are contiguous.
 *
 * Return the first cslot.
 */
seL4_CPtr
allocator_alloc_cslots(struct allocator *allocator, int num_slots)
{
    seL4_CPtr result;

    /* Check if we have enough slots left. */
    if ((allocator->cslots.count - allocator->num_slots_used) < num_slots) {
        return 0;
    }

    /* Pick the first one. */
    result = allocator->cslots.first + allocator->num_slots_used + allocator->root_cnode_offset;

    /* Record this slot as used. */
    allocator->num_slots_used += num_slots;

    return result;
}

/*
 * Retype an untyped item.
 */
int
allocator_retype_untyped_memory(struct allocator *allocator,
        seL4_CPtr untyped_item, seL4_Word item_type, seL4_Word item_size,
        int num_items, struct cap_range *result)
{
    int max_objects;
    int error;
    UNUSED_NDEBUG(error);

    /* Determine the maximum number of items we have space in our CNode for. */
    max_objects = allocator->cslots.count - allocator->num_slots_used;
    if (num_items > max_objects) {
        result->count = 0;
        result->first = 0;
        return 0;
    }

    /* Do the allocation. We expect at least one item will be created. */
    #ifdef CONFIG_KERNEL_STABLE
    error = seL4_Untyped_RetypeAtOffset(
                untyped_item,
                item_type, 0, item_size,
                seL4_CapInitThreadCNode,
                allocator->root_cnode, allocator->root_cnode_depth, allocator->cslots.first + allocator->num_slots_used,
                num_items);
    #else
    error = seL4_Untyped_Retype(
                untyped_item,
                item_type, item_size,
                seL4_CapInitThreadCNode,
                allocator->root_cnode, allocator->root_cnode_depth, allocator->cslots.first + allocator->num_slots_used,
                num_items);
    #endif
    assert(!error);

    /* Save the allocation. */
    result->count = num_items;
    result->first = allocator->cslots.first
                    + allocator->num_slots_used + allocator->root_cnode_offset;

    /* Record these slots as used. */
    allocator->num_slots_used += num_items;

    return num_items;
}

/*
 * Allocate untyped item of size 'size_bits' bits.
 */
seL4_CPtr
allocator_alloc_untyped(struct allocator *allocator, unsigned long size_bits)
{
    seL4_CPtr result;
    seL4_CPtr big_untyped_item;
    int i;
    int created_objects;
    struct cap_range *pool;

    /* If it is too small or too big, not much we can do. */
    if (size_bits < MIN_UNTYPED_SIZE) {
        return 0;
    }
    if (size_bits > MAX_UNTYPED_SIZE) {
        return 0;
    }

    /* Grab our pool. */
    pool = &allocator->untyped_items[size_bits - MIN_UNTYPED_SIZE];

    /* Do we have something of the correct size in one of our pools? */
    result = range_alloc(pool, 1);
    if (result) {
        return result;
    }

    /* Do we have something of the correct size in initial memory regions? */
    for (i = 0; i < allocator->num_init_untyped_items; i++) {
        if (allocator->init_untyped_items[i].is_free &&
                allocator->init_untyped_items[i].size_bits == size_bits) {
            allocator->init_untyped_items[i].is_free = 0;
            return allocator->init_untyped_items[i].cap;
        }
    }

    /* Otherwise, try splitting something of a bigger size. */
    big_untyped_item = allocator_alloc_untyped(allocator, size_bits + 1);
    if (!big_untyped_item) {
        return 0;
    }
    created_objects = allocator_retype_untyped_memory(allocator,
                      big_untyped_item, seL4_UntypedObject, size_bits, 2, pool);
    if (!created_objects) {
        return 0;
    }

    /* Allocate out of our split. */
    result = range_alloc(pool, 1);
    assert(result);
    return result;
}

/*
 * Reset the allocator back to its initial state.
 */
void
allocator_reset(struct allocator *allocator)
{
    int i;
    int error;
    UNUSED_NDEBUG(error);
    /* Recycle all of our untyped memory back into its original form. */
    for (i = 0; i < allocator->num_init_untyped_items; i++) {
        /* If it hasn't been used, ignore it. */
        if (allocator->init_untyped_items[i].is_free) {
            continue;
        }

        /* Otherwise, tear down any child objects created from it. */
        error = seL4_CNode_Recycle(
                    seL4_CapInitThreadCNode,
                    allocator->init_untyped_items[i].cap,
                    seL4_WordBits);
        assert(!error);
        allocator->init_untyped_items[i].is_free = 1;
    }

    /* All of our free CNodes should be free again. */
    allocator->num_slots_used = 0;

    /* Reset all of our pools to empty. */
    for (i = MIN_UNTYPED_SIZE; i <= MAX_UNTYPED_SIZE; i++) {
        allocator->untyped_items[i - MIN_UNTYPED_SIZE].first = 0;
        allocator->untyped_items[i - MIN_UNTYPED_SIZE].count = 0;
    }
}

/*
 * Destroy an allocator, releasing any resources held by it.
 */
void
allocator_destroy(struct allocator *allocator)
{
    allocator_reset(allocator);
}

/*
 * Allocate an object of the given type.
 */

/*
 * Perform a simple test of the allocator. This test will reset the
 * allocator's state.
 */
void
allocator_self_test(struct allocator *allocator)
{
    seL4_CPtr x, y;
    x = allocator_alloc_untyped(allocator, MIN_UNTYPED_SIZE);
    allocator_reset(allocator);
    y = allocator_alloc_untyped(allocator, MIN_UNTYPED_SIZE);
    allocator_reset(allocator);
    UNUSED_NDEBUG(x);
    UNUSED_NDEBUG(y);
    assert(x == y);
}

/*
 * Allocate 'count' items out of the given range.
 */
static seL4_CPtr
range_alloc(struct cap_range *range, int count)
{
    /* If there are not enough items in the range, abort. */
    if (range->count < count) {
        return 0;
    }

    /* Allocate from the range. */
    assert(range->first);
    range->count -= count;
    return range->first + range->count;
}
