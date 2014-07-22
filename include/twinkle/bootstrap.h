/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include <sel4/sel4.h>

#include <twinkle/allocator.h>
#include <twinkle/object_allocator.h>

struct allocator *
create_first_stage_allocator(void);

#endif /* BOOTSTRAP_H */

