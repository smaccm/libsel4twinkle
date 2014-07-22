/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef TWINKLE_VKA_H
#define TWINKLE_VKA_H

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_VKA

#include <twinkle/allocator.h>
#include <vka/vka.h>

/**
 * Initialise a twinkle implementation of the vka interface
 *
 * @param vka uninitialised vka data structure
 * @param allocator initialised twinkle allocator
 */
void twinkle_init_vka(vka_t* vka, struct allocator *allocator);

#endif /* CONFIG_LIB_SEL4_VSPACE */
#endif /* TWINKLE_VKA_H */
