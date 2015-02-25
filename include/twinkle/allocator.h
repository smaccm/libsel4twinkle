/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <sel4/sel4.h>

/* Minimum/maximum size of untyped objects we will support. */
#define MIN_UNTYPED_SIZE 4
#define MAX_UNTYPED_SIZE 32

/* Maximum number of untyped items we will support. */
#define MAX_UNTYPED_ITEMS 256

/* An untyped item. */
struct untyped_item {
    /* Cap to the untyped item. */
    seL4_CPtr cap;

    /* Size of the untyped item. */
    unsigned long size_bits;
};

/* A range of caps. */
struct cap_range {
    unsigned long first;
    unsigned long count;
};

/*
 * Allocator struct.
 *
 * All state used by the allocator is kept in this struct; this allows several
 * instances of the allocator to be used simultaneously.
 */
struct allocator {
    /* CNode we allocate from. */
    seL4_CPtr root_cnode;
    seL4_CPtr root_cnode_depth;
    seL4_CPtr root_cnode_offset;

    /* Range of free slots in the above CNode. */
    struct cap_range cslots;

    /* Number of slots we have used. */
    unsigned long num_slots_used;

    /* Initial memory items. */
    unsigned long num_init_untyped_items;
    struct {
        seL4_CPtr cap;
        unsigned long size_bits;
        unsigned long is_free;
    } init_untyped_items[MAX_UNTYPED_ITEMS];

    /* Untyped memory items we have created. */
    struct cap_range untyped_items[(MAX_UNTYPED_SIZE - MIN_UNTYPED_SIZE) + 1];
};

void
allocator_create(struct allocator *allocator,
                 seL4_CPtr root_cnode, unsigned long root_cnode_depth,
                 unsigned long root_cnode_offset,
                 unsigned long first_slot, unsigned long num_slots,
                 struct untyped_item *items, int num_items);

void
allocator_create_child(struct allocator *parent, struct allocator *child,
                       seL4_CPtr root_cnode, unsigned long root_cnode_depth,
                       unsigned long root_cnode_offset,
                       unsigned long first_slot, unsigned long num_slots);

void
allocator_add_root_untyped_item(struct allocator *allocator,
                                seL4_CPtr item, unsigned long size_bits);

seL4_CPtr
allocator_alloc_cslot(struct allocator *allocator);

void
allocator_free_cslot(struct allocator *allocator, seL4_CPtr slot);

seL4_CPtr
allocator_alloc_cslots(struct allocator *allocator, int num_slots);

seL4_CPtr
allocator_alloc_untyped(struct allocator *allocator, unsigned long size_bits);

int
allocator_retype_untyped_memory(struct allocator *allocator,
                                seL4_CPtr untyped_item, seL4_Word item_type, seL4_Word item_size,
                                int num_items, struct cap_range *result);

void
allocator_reset(struct allocator *allocator);

void
allocator_destroy(struct allocator *allocator);

void
allocator_self_test(struct allocator *allocator);

#endif /* ALLOCATOR_H */
