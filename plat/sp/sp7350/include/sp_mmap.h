/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SP_MMAP_H
#define SP_MMAP_H

/* Memory regions */
#define SP_DRAM_BASE			0x00000000
#define SP_DRAM_VIRT_BASE		0x00000000
#define SP_DRAM_SIZE			0x80000000

#define SP_CBSRAM_BASE			0xfa200000
#define SP_CBSRAM_SIZE			0x00040000 // to save page table?

/* Memory-mapped devices */

#define SP_RGST_BASE			0xf8000000
#define SP_RGST_SIZE			0x03000000 /* to cover GIC 0xfaxx_xxxx */

#define SP_AO_RGST_BASE			0xf8800000


#define SP_UART0_BASE			0xf8801900

#define SP_GICD_BASE			0xfa001000
#define SP_GICC_BASE			0xfa002000

#define SP_NPU_BASE				0xf8200000

/* macro */
#define RF_GRP(_grp, _reg)           ((((_grp) * 32 + (_reg)) * 4) + SP_RGST_BASE)
#define RF_GRP_AO(_grp, _reg)         ((((_grp) * 32 + (_reg)) * 4) + SP_AO_RGST_BASE)

#define RF_MASK_V(_mask, _val)       (((_mask) << 16) | (_val))
#define RF_MASK_V_SET(_mask)         (((_mask) << 16) | (_mask))
#define RF_MASK_V_CLR(_mask)         (((_mask) << 16) | 0)

#define SP_MAILBOX_CA55_TO_M4_IRQ0   RF_GRP(258,24)


#endif /* SP_MMAP_H */
