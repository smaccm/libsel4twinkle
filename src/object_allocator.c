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
 * Simple kernel object allocator.
 *
 * Given an untyped item allocator, allocates a kernel object
 * of the given type.
 *
 * This is a convenience wrapper around the seL4 API; nothing in here is
 * particularly deep.
 */

#ifndef UNUSED_NDEBUG
# ifdef NDEBUG
#  define UNUSED_NDEBUG(x)  ((void)x)
# else
#  define UNUSED_NDEBUG(x)
# endif
#endif

#include <assert.h>
#include <stdio.h>

#include <twinkle/allocator.h>
#include <twinkle/object_allocator.h>

#include <vka/object.h>

/*
 * Allocate a single object of the given type.
 */
seL4_CPtr
allocator_alloc_kobject(struct allocator *allocator,
                        seL4_Word item_type, seL4_Word item_size)
{
    unsigned long size_bits;
    seL4_CPtr untyped_memory;
    struct cap_range cap_range;
    int result;
    UNUSED_NDEBUG(result);
    /* Allocate an untyped memory item of the right size. */
    size_bits = vka_get_object_size(item_type, item_size);
    untyped_memory = allocator_alloc_untyped( allocator, size_bits);
    if (!untyped_memory) {
        return 0;
    }

    /* Allocate an object. */
    result = allocator_retype_untyped_memory(allocator, untyped_memory,
                                             item_type, item_size, 1, &cap_range);

    /* We should have gotten either zero items (if we ran out of caps), or one
     * item (if everything went well). If we get more than one, we
     * miscalculated our sizes. */
    assert((result == 0) || (result == 1));

    /* Return the first item (or zero if none were allocated. */
    return cap_range.first;
}


