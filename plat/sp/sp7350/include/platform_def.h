/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <common/tbbr/tbbr_img_def.h>
#include <lib/utils_def.h>
#include <plat/common/common_def.h>

#include <sp_mmap.h>

#define CPU_WAIT_INIT_VAL		0xffffffff
#define CORE_CPU_START_POS(core_id)	(CORE0_CPU_START_POS - ((core_id) * 8))
#define CORE3_CPU_START_POS     	(0xfa23fc00 - 0x28)  // core3 wait fa23_fbd8
#define CORE2_CPU_START_POS    	 	(0xfa23fc00 - 0x20)  // core2 wait fa23_fbe0
#define CORE1_CPU_START_POS    	 	(0xfa23fc00 - 0x18)  // core1 wait fa23_fbe8
#define CORE0_CPU_START_POS     	(0xfa23fc00 - 0x10)  // core0 wait fa23_fbf0

#define PLAT_SP_HOLD_BASE		    (CORE0_CPU_START_POS) //from CPU0~CPU3
#define PLAT_SP_HOLD_SIZE		    (PLATFORM_CORE_COUNT * 8)
#define PLAT_SP_HOLD_STATE_WAIT		(0)
#define PLAT_SP_HOLD_STATE_GO		(1)
/*
 * DRAM
 *   Secure :   0 ~   4MB
 *   NS     :   4 ~  64MB
 *
 * Software :
 *   BL31   @ 2MB
 *   uboot  @ 3MB
 */
#define SP_DRAM_SEC_ADDR		(SP_DRAM_BASE)
#define SP_DRAM_SEC_SIZE		(5U << 20)
#define SP_DRAM_NS_ADDR			(SP_DRAM_SEC_ADDR + SP_DRAM_SEC_SIZE)
#define SP_DRAM_NS_SIZE			(SP_DRAM_SIZE - SP_DRAM_NS_ADDR)

#define BL31_BASE			0x200000 /* @ 2MB */
#define BL31_LIMIT			(BL31_BASE + 0x100000)

#define BL32_BASE				BL31_LIMIT
#define BL32_LIMIT				(BL32_BASE + 0x200000)
#define SP_LINUX_DTB_OFFSET		(SP_DRAM_BASE + 0x1F80000)  /* dtb @ 31M+512k */
#define PLAT_SP_NS_IMAGE_OFFSET	(SP_DRAM_BASE + 0x500040) /* uboot @ 5MB+64 */


/* stack */
#define PLATFORM_STACK_SIZE		(0x2000 / PLATFORM_CORE_COUNT)  /* in case that verbose string is long */



/* max cacheline size = 64 bytes */
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/* map regions */
#define PLATFORM_MMAP_REGIONS		4
#define MAX_MMAP_REGIONS		(3 + PLATFORM_MMAP_REGIONS)
#define MAX_XLAT_TABLES			8

#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)


/* PWR */
#define PLAT_MAX_PWR_LVL_STATES		U(2)
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)
#define PLAT_MAX_PWR_LVL		U(2) /* See plat/sp/common/sp_topology.c */
#define PLAT_NUM_PWR_DOMAINS		(U(1) + PLATFORM_CLUSTER_COUNT + PLATFORM_CORE_COUNT)

/* Local power state for power domains in Run state. */
#define PLAT_LOCAL_STATE_RUN		0
/* Local power state for retention. Valid only for CPU power domains */
#define PLAT_LOCAL_STATE_RET		1
/*
 * Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains.
 */
#define PLAT_LOCAL_STATE_OFF		2

/* cores */
#define PLATFORM_CLUSTER_COUNT		U(1)
#define PLATFORM_MAX_CPUS_PER_CLUSTER	U(4)
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * PLATFORM_MAX_CPUS_PER_CLUSTER)

#endif /* PLATFORM_DEF_H */
