/* cp15-armv7.h

   written by Marc Singer
   24 Feb 2007

   Copyright (C) 2007 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   *** FIXME: these macros have not been edited for the platform

   We need:

    CLEANALL_DCACHE;
    INVALIDATE_ICACHE;
    INVALIDATE_DCACHE;
    INVALIDATE_TLB;
    DRAIN_WRITE_BUFFER;
    CP15_WAIT;


   Kernel Snippets
   ---------------

ENTRY(cpu_v7_dcache_clean_area)
#ifndef TLB_CAN_READ_FROM_L1_CACHE
        dcache_line_size r2, r3
1:      mcr     p15, 0, r0, c7, c10, 1          @ clean D entry
        add     r0, r0, r2
        subs    r1, r1, r2
        bhi     1b
        dsb
#endif

#ifdef CONFIG_ARM_ERRATA_430973
        mcr     p15, 0, r2, c7, c5, 6           @ flush BTAC/BTB
#endif
#ifdef CONFIG_ARM_ERRATA_754322
        dsb
#endif
        mcr     p15, 0, r2, c13, c0, 1          @ set reserved context ID
        isb
1:      mcr     p15, 0, r0, c2, c0, 0           @ set TTB 0
        isb
#ifdef CONFIG_ARM_ERRATA_754322
        dsb
#endif
        mcr     p15, 0, r1, c13, c0, 1          @ set context ID
        isb
#endif

        mcr     p15, 0, ip, c8, c7, 0   @ invalidate TLBs
        mcr     p15, 0, ip, c7, c5, 0   @ invalidate I cache

__v7_setup:
        adr     r12, __v7_setup_stack           @ the local stack
        stmia   r12, {r0-r5, r7, r9, r11, lr}
        bl      v7_flush_dcache_all
        ldmia   r12, {r0-r5, r7, r9, r11, lr}

        mrc     p15, 0, r0, c0, c0, 0           @ read main ID register
        and     r10, r0, #0xff000000            @ ARM?
        teq     r10, #0x41000000
        bne     3f
        and     r5, r0, #0x00f00000             @ variant
        and     r6, r0, #0x0000000f             @ revision
        orr     r6, r6, r5, lsr #20-4           @ combine variant and revision
        ubfx    r0, r0, #4, #12                 @ primary part number

        / * Cortex-A8 Errata * /
        ldr     r10, =0x00000c08                @ Cortex-A8 primary part number
        teq     r0, r10
        bne     2f
#ifdef CONFIG_ARM_ERRATA_430973
        teq     r5, #0x00100000                 @ only present in r1p*
        mrceq   p15, 0, r10, c1, c0, 1          @ read aux control register
        orreq   r10, r10, #(1 << 6)             @ set IBE to 1
        mcreq   p15, 0, r10, c1, c0, 1          @ write aux control register
#endif
#ifdef CONFIG_ARM_ERRATA_458693
        teq     r6, #0x20                       @ only present in r2p0
        mrceq   p15, 0, r10, c1, c0, 1          @ read aux control register
        orreq   r10, r10, #(1 << 5)             @ set L1NEON to 1
        orreq   r10, r10, #(1 << 9)             @ set PLDNOP to 1
        mcreq   p15, 0, r10, c1, c0, 1          @ write aux control register
#endif
#ifdef CONFIG_ARM_ERRATA_460075
        teq     r6, #0x20                       @ only present in r2p0
        mrceq   p15, 1, r10, c9, c0, 2          @ read L2 cache aux ctrl register
        tsteq   r10, #1 << 22
        orreq   r10, r10, #(1 << 22)            @ set the Write Allocate disable bit
        mcreq   p15, 1, r10, c9, c0, 2          @ write the L2 cache aux ctrl register
#endif

3:      mov     r10, #0
#ifdef HARVARD_CACHE
        mcr     p15, 0, r10, c7, c5, 0          @ I+BTB cache invalidate
#endif
        dsb
#ifdef CONFIG_MMU
        mcr     p15, 0, r10, c8, c7, 0          @ invalidate I + D TLBs
        mcr     p15, 0, r10, c2, c0, 2          @ TTB control register
        ALT_SMP(orr     r4, r4, #TTB_FLAGS_SMP)
        ALT_UP(orr      r4, r4, #TTB_FLAGS_UP)
        ALT_SMP(orr     r8, r8, #TTB_FLAGS_SMP)
        ALT_UP(orr      r8, r8, #TTB_FLAGS_UP)
        mcr     p15, 0, r8, c2, c0, 1           @ load TTB1
        ldr     r5, =PRRR                       @ PRRR
        ldr     r6, =NMRR                       @ NMRR
        mcr     p15, 0, r5, c10, c2, 0          @ write PRRR
        mcr     p15, 0, r6, c10, c2, 1          @ write NMRR



/ *
 *      v7_flush_dcache_all()
 *
 *      Flush the whole D-cache.
 *
 *      Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
 *
 *      - mm    - mm_struct describing address space
 * /
ENTRY(v7_flush_dcache_all)
        dmb                                     @ ensure ordering with previous memory accesses
        mrc     p15, 1, r0, c0, c0, 1           @ read clidr
        ands    r3, r0, #0x7000000              @ extract loc from clidr
        mov     r3, r3, lsr #23                 @ left align loc bit field
        beq     finished                        @ if loc is 0, then no need to clean
        mov     r10, #0                         @ start clean at cache level 0
loop1:
        add     r2, r10, r10, lsr #1            @ work out 3x current cache level
        mov     r1, r0, lsr r2                  @ extract cache type bits from clidr
        and     r1, r1, #7                      @ mask of the bits for current cache only
        cmp     r1, #2                          @ see what cache we have at this level
        blt     skip                            @ skip if no cache, or just i-cache
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        isb                                     @ isb to sych the new cssr&csidr
        mrc     p15, 1, r1, c0, c0, 0           @ read the new csidr
        and     r2, r1, #7                      @ extract the length of the cache lines
        add     r2, r2, #4                      @ add 4 (line length offset)
        ldr     r4, =0x3ff
        ands    r4, r4, r1, lsr #3              @ find maximum number on the way size
        clz     r5, r4                          @ find bit position of way size increment
        ldr     r7, =0x7fff
        ands    r7, r7, r1, lsr #13             @ extract max number of the index size
loop2:
        mov     r9, r4                          @ create working copy of max way size
loop3:
 ARM(   orr     r11, r10, r9, lsl r5    )       @ factor way and cache number into r11
 THUMB( lsl     r6, r9, r5              )
 THUMB( orr     r11, r10, r6            )       @ factor way and cache number into r11
 ARM(   orr     r11, r11, r7, lsl r2    )       @ factor index number into r11
 THUMB( lsl     r6, r7, r2              )
 THUMB( orr     r11, r11, r6            )       @ factor index number into r11
        mcr     p15, 0, r11, c7, c14, 2         @ clean & invalidate by set/way
        subs    r9, r9, #1                      @ decrement the way
        bge     loop3
        subs    r7, r7, #1                      @ decrement the index
        bge     loop2
skip:
        add     r10, r10, #2                    @ increment cache number
        cmp     r3, r10
        bgt     loop1
finished:
        mov     r10, #0                         @ swith back to cache level 0
        mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
        dsb
        isb
        mov     pc, lr
ENDPROC(v7_flush_dcache_all)




*/

#if !defined (__CP15_ARMV7_H__)
#    define   __CP15_ARMV7_H__

	/* ---- Cache control */

#define INVALIDATE_ICACHE \
  __asm volatile ( \
 /* "mcr p15, 0, %0, c7, c1, 0\n\t" / * inv I-cache inner sharable (SMP) */ \
    "mcr p15, 0, %0, c7, c5, 0\n\t" /* I+BTB cache invalidate */ \
    :: "r" (0))
/*
#define INVALIDATE_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (a))	// XXX
*/
/*
#define INVALIDATE_ICACHE_I(i)                                        \
  __asm volatile ("mcr p15, 0, %0, c7, c5, 2\n\t" :: "r" (i))	// XXX
*/
/*
#define FLUSH_PREFETCH                                                \
  __asm volatile ("mcr p15, 0, %0, c7, c5, 4\n\t" :: "r" (0))	// XXX
*/
/*
#define FLUSH_BRANCH_CACHE                                            \
  __asm volatile ("mcr p15, 0, %0, c7, c5, 6\n\t" :: "r" (0))	// XXX
*/
/*
#define FLUSH_BRANCH_CACHE_VA(a)                                      \
  __asm volatile ("mcr p15, 0, %0, c7, c5, 7\n\t" :: "r" (a))	// XXX
*/

/* *** FIXME: I think this may be dangerous if someone is expecting to really invalidate the cache.  This is OK as long as there is a flush_all preceeding the dcache invalidation. */
#define INVALIDATE_DCACHE DATA_MEMORY_BARRIER

/*#define INVALIDATE_DCACHE                                             \
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0\n\t" :: "r" (0))	// XXX
*/
/*
#define INVALIDATE_DCACHE_VA(a)                                       \
  __asm volatile ("mcr p15, 0, %0, c7, c6, 1\n\t" :: "r" (a))	// XXX
*/
/*
#define INVALIDATE_DCACHE_I(i)                                        \
  __asm volatile ("mcr p15, 0, %0, c7, c6, 2\n\t" :: "r" (i))	// XXX
*/

#define INVALIDATE_CACHE \
  do { INVALIDATE_DCACHE; INVALIDATE_ICACHE; } while (0)


/*
#define CLEAN_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 0\n\t" :: "r" (0))	// XXX
*/
/*
#define CLEAN_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t" :: "r" (a))	// XXX
*/
/*
#define CLEAN_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 2\n\t" :: "r" (i))	// XXX
*/

#define DATA_SYNCHRONIZATION_BARRIER                                    \
  __asm volatile ("dsb")

#define DATA_MEMORY_BARRIER                                             \
  __asm volatile ("dmb")

/*
#define LOAD_CACHE_DIRTY_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c10, 6\n\t" :  "=r" (v))	// XXX
*/

/*
#define LOAD_BLOCK_TRANSFER_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c12, 4\n\t" :  "=r" (v))	// XXX
*/
/*
#define STOP_PREFETCH_RANGE(v)\
  __asm volatile ("mcr p15, 0, %0, c7, c12, 5\n\t" :: "r" (0))	// XXX
*/

/*
#define PREFETCH_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c13, 1\n\t" :: "r" (a))	// XXX
*/

#define CLEAN_INVALIDATE_DCACHE_ \
  do { u32 clidr; u32 csidr; u32 loc; u32 level;                           \
    __asm volatile ("dmb\n\t" "mrc p15, 1, %0, c0, c0, 1" : "=r" (clidr)); \
    loc = (clidr >> 24) & 7; \
    for (level = 0; level < loc; ++level) { \
      u32 r2; u32 r4; u32 r5; s32 r7;       \
      u32 ctype = (clidr >> (level*3)) & 7; \
      if ((ctype & 2) == 0) continue; \
      __asm volatile ("mcr p15, 2, %0, c0, c0, 0" :: "r" (level << 1)); /* select dcache in csselr */ \
      __asm volatile ("isb\n\t" "mrc p15, 1, %0, c0, c0, 0" : "=r" (csidr)); /* read csidr */ \
      r2 = (csidr & 7) + 4; \
      r4 = 0x3ff & (csidr >> 3);              /* ways */ \
      __asm volatile ("clz %0, %1" : "=r" (r5) : "r" (r4)); \
      r7 = 0x7fff & (csidr >> 13);            /* indexes */ \
      for (; r7 >= 0; --r7) { \
        s32 way; for (way = r4; way >= 0; --way) { \
          u32 v = (level<<1) | (way<<r5) | (r7<<r2); \
          __asm volatile ("mcr p15, 0, %0, c7, c14, 2" :: "r" (v)); \
    } } } \
    __asm volatile ("mcr p15, 2, %0, c0, c0, 0" :: "r" (0)); /* Restore level 0->csselr */ \
    __asm volatile ("dsb\n\t" "isb"); } while (0)


/*
#define CLEAN_INV_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 0\n\t" :: "r" (0))	// XXX
*/
/*
#define CLEAN_INV_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 1\n\t" :: "r" (a))	// XXX
*/
/*
#define CLEAN_INV_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 2\n\t" :: "r" (i))	// XXX
*/

	/* ---- Cache lockdown */

//*** FIXME: it isn't clear whether or not we need to lock the cache
//*** or if we can.  Documentation for CP15 on the MX51 is absent.
#define UNLOCK_CACHE
  //  __asm volatile ("mcr p15, 0, %0, c9, c1, 1\n\t"
  //		  "mcr p15, 0, %0, c9, c2, 1\n\t" :: "r" (0))

	/* ---- TLB control */

#define INVALIDATE_TLB                                                \
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))

#define INVALIDATE_ITLB\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 0\n\t" :: "r" (0))

#define INVALIDATE_ITLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 1\n\t" :: "r" (a))

#define INVALIDATE_DTLB\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 0\n\t" :: "r" (0))

#define INVALIDATE_DTLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 1\n\t" :: "r" (a))


/* --- ARMV7 */

#define CLEANALL_DCACHE FLUSH_DCACHE_ALL

#undef DRAIN_WRITE_BUFFER
#define DRAIN_WRITE_BUFFER DATA_SYNCHRONIZATION_BARRIER
#undef INVALIDATE_CACHE_VA
#undef INVALIDATE_CACHE_I
#undef CLEAN_CACHE_VA
#undef CLEAN_CACHE_I
#undef PREFETCH_ICACHE_VA

/*
#define STORE_REMAP_PERIPHERAL_PORT(v)\
  __asm volatile ("mcr p15, 0, %0, c15, c2, 4\n\t" :: "r" (v))	// XXX
*/
/*
#define LOAD_REMAP_PERIPHERAL_PORT(v)\
  __asm volatile ("mrc p15, 0, %0, c15, c2, 4\n\t" : "=r" (v))	// XXX
*/


#define FLUSH_ICACHE_ALL \
  INVALIDATE_ICACHE_
#define FLUSH_DCACHE_ALL \
  CLEAN_INVALIDATE_DCACHE_

#define TTB_FLAGS (0)
//                   | (1 << 0) 		/* inner cacheable */
//                   | (3 << 3))		/* write-back, no allocate on write */

#undef STORE_TTB
#define STORE_TTB(a) \
  __asm volatile ("mcr p15, 0, %0, c2, c0, 0\n\t"                       \
                  "mcr p15, 0, %1, c2, c0, 2" :: "r" (((u32)a) | TTB_FLAGS), "r" (0));

#endif  /* __CP15_ARMV7_H__ */
