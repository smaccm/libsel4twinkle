/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


/**
 * A vka implementation for twinkle
 */
#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_VKA

#include <sel4/sel4.h>

#include <twinkle/allocator.h>
#include <twinkle/vka.h>

#include <vka/vka.h>
#include <vka/object.h>


static inline int twinkle_vka_cspace_alloc(void *self, seL4_CPtr *res)
{
    *res = allocator_alloc_cslot((struct allocator *) self);
    return *res == 0;
}

static inline void twinkle_vka_cspace_free(void *self, seL4_CPtr slot)
{
    allocator_free_cslot((struct allocator *)self, slot);
}

static inline void twinkle_vka_cspace_make_path(void *self, seL4_CPtr slot, cspacepath_t *res)
{
    struct allocator *allocator = (struct allocator *) self;

    res->capPtr = slot;
    res->capDepth = 32;
    res->root = allocator->root_cnode;
    res->dest = allocator->root_cnode;
    res->destDepth = allocator->root_cnode_depth;
    res->offset = slot;
    res->window = 1;
}


static inline int twinkle_vka_utspace_alloc(void *self, const cspacepath_t *dest, seL4_Word type,
                                            seL4_Word size_bits, uint32_t *res)
{

    struct allocator *allocator = (struct allocator *) self;
    uint32_t ut_size_bits = vka_get_object_size(type, size_bits);
    /* allocate untyped memory the size we want */
    seL4_CPtr untyped_memory = allocator_alloc_untyped(allocator, ut_size_bits);
    if (!untyped_memory) {
        return -1;
    }

#ifdef CONFIG_KERNEL_STABLE
    /* retype into the type we want */
    return seL4_Untyped_RetypeAtOffset(untyped_memory, type, 0, size_bits, seL4_CapInitThreadCNode,
                                       allocator->root_cnode, allocator->root_cnode_depth,
                                       dest->capPtr, 1);
#else
    return seL4_Untyped_Retype(untyped_memory, type, size_bits, seL4_CapInitThreadCNode,
                               allocator->root_cnode, allocator->root_cnode_depth,
                               dest->capPtr, 1);
#endif

}


void
twinkle_init_vka(vka_t* vka, struct allocator *allocator)
{
    vka->data =  (void *) allocator;
    vka->cspace_alloc = twinkle_vka_cspace_alloc;
    vka->cspace_make_path = twinkle_vka_cspace_make_path;
    vka->utspace_alloc = twinkle_vka_utspace_alloc;
    vka->cspace_free = twinkle_vka_cspace_free;

    /* twinkle does not implement these functions */
    vka->utspace_free = dummy_vka_utspace_free;
    vka->utspace_free = dummy_vka_utspace_free;
    vka->utspace_paddr = dummy_vka_utspace_paddr;
}

#endif /* CONFIG_LIB_SEL4_VKA */
