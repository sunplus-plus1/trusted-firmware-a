/*
 * Copyright (c) 2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <common/bl_common.h>
#include <platform_def.h>
	.globl	sp_sec_entry_point

	.globl  plat_my_core_pos
	.globl	plat_get_my_entrypoint
	.globl	plat_secondary_cold_boot_setup
	.globl	plat_is_my_cpu_primary

func plat_my_core_pos /* MPIDR : 8100_0x00, x=0~3 */
	mrs     x0, mpidr_el1
	lsr	x0, x0, #MPIDR_AFF1_SHIFT
	and	x0, x0, #MPIDR_AFFLVL_MASK
	ret
endfunc plat_my_core_pos

/* -----------------------------------------------------------------------
 * Indicate a cold boot for every CPU
 * -----------------------------------------------------------------------
 */
func plat_get_my_entrypoint

	mov	x1, x30
	bl	plat_is_my_cpu_primary
	/*
	 * Secondaries always cold boot.
	*/
	cbz	w0, 1f

	adr	x0, sp_sec_entry_point
	ldr	x0, [x0]

	ret x1

1:
	mov	x0, #0
	ret	x1


endfunc plat_get_my_entrypoint

/* -----------------------------------------------------------------------
 * void plat_secondary_cold_boot_setup (void);
 * -----------------------------------------------------------------------
 */

func plat_secondary_cold_boot_setup

	bl	plat_my_core_pos

	mov_imm	x2, PLAT_SP_HOLD_BASE
	sub	x2, x2, x0, LSL #3	/* core wait address = x2 - (core * 8) */
	/*
	 * This code runs way before requesting the warmboot of this core,
	 * so it is possible to clear the mailbox before getting a request
	 * to boot.
	 */
	mov	x1, PLAT_SP_HOLD_STATE_WAIT  /* set 0 to core1~core3 pos*/
	str	x1,[x2]

poll_mailbox:
	wfe
	ldr	x1, [x2]
	cmp	x1, PLAT_SP_HOLD_STATE_GO  /* wait sp_pwr_on to set 1 to core pos */
	bne	poll_mailbox

	/* Jump to the provided entrypoint */
	adr	x0, sp_sec_entry_point
	ldr	x1, [x0]

	br	x1

endfunc	plat_secondary_cold_boot_setup

/* -----------------------------------------------------------------------
 * unsigned int plat_is_my_cpu_primary (void);
 *
 * Find out whether the current cpu is the primary cpu
 * -----------------------------------------------------------------------
 */
func plat_is_my_cpu_primary
	mov	x9, x30
	bl	plat_my_core_pos
	cmp	x0, #0
	cset	x0, eq		/* set 1 for primary, and set 0 for others */
	ret	x9
endfunc plat_is_my_cpu_primary

	.data
	.align 3

	/* --------------------------------------------------
	 * CPU Secure entry point - resume from suspend
	 * --------------------------------------------------
	 */
sp_sec_entry_point:
	.quad	0

