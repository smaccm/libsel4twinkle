/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef OBJECT_ALLOCATOR_H
#define OBJECT_ALLOCATOR_H

#include <sel4/sel4.h>

#include "allocator.h"

seL4_CPtr
allocator_alloc_kobject(struct allocator *allocator,
        seL4_Word item_type, seL4_Word item_size);

#endif /* OBJECT_ALLOCATOR_H */

