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
 * Setup a memory allocator for the first-stage bootloader.
 *
 * When the system first boots into userspace, we have a bunch of untyped memory.
 *
 * This code will:
 *    (1) Parse the seL4 bootinfo to find untyped memory available to us;
 *    (2) Return an object allocator to the caller, setup to use the memory
 *        found in (1) and the cap slots found in the root CNode.
 */

#include <stdio.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <sel4/bootinfo.h>
#include <sel4/messages.h>

#include <twinkle/allocator.h>
#include <twinkle/object_allocator.h>
#include <twinkle/bootstrap.h>

/*
 * Fill the given allocator with resources from the given
 * bootinfo structure.
 */
static void
fill_allocator_with_resources(struct allocator *allocator,
        seL4_BootInfo *bootinfo)
{
    int i;

    for (i = 0; i < bootinfo->untyped.end - bootinfo->untyped.start; i++) {
        allocator_add_root_untyped_item(
            allocator,
            bootinfo->untyped.start + i,
            bootinfo->untypedSizeBitsList[i]
        );
    }
}

/*
 * Create an object allocator managing the root CNode's free slots
 */
static void
create_bootstrap_allocator(struct allocator *allocator)
{
    /* Fetch seL4 bootinfo. */
    seL4_BootInfo *bootinfo = seL4_GetBootInfo();

    /* Create the allocator. */
    allocator_create(
        allocator,
        seL4_CapInitThreadCNode,
        seL4_WordBits,
        0,
        bootinfo->empty.start,
        bootinfo->empty.end - bootinfo->empty.start,
        NULL,
        0
    );

    /* Give the allocator all of our free memory. */
    fill_allocator_with_resources(allocator, bootinfo);

#ifdef CONFIG_DEBUG_BUILD
    /* Test out everything. */
    allocator_self_test(allocator);
#endif
}

/*
 * Create first-stage allocator (directly use bootstrap allocator).
 */
struct allocator *
create_first_stage_allocator(void) {
    static struct allocator allocator;

    create_bootstrap_allocator(&allocator);
    allocator_self_test(&allocator);

    return &allocator;
}
