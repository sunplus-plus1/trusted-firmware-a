/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>

#include <sp_def.h>
#include <sp_mmap.h>
#include <sp_private.h>

static const mmap_region_t sp_mmap[PLATFORM_MMAP_REGIONS + 1] = {
	MAP_REGION_FLAT(SP_DRAM_SEC_ADDR, SP_DRAM_SEC_SIZE,    MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(SP_DRAM_NS_ADDR,  SP_DRAM_NS_SIZE,     MT_MEMORY | MT_RW | MT_NS),
	MAP_REGION_FLAT(SP_RGST_BASE,     SP_RGST_SIZE,        MT_DEVICE | MT_RW | MT_SECURE | MT_EXECUTE_NEVER),
	MAP_REGION_FLAT(SP_CBSRAM_BASE,   SP_CBSRAM_SIZE,      MT_MEMORY | MT_RW | MT_NS),
	{},
};

unsigned int plat_get_syscnt_freq2(void)
{
	return SP_ARM_COUNTER_CLK_IN;
}

void sp_configure_mmu_el3(int flags)
{
	mmap_add_region(BL31_BASE, BL31_BASE, (BL31_LIMIT - BL31_BASE),
			MT_MEMORY | MT_RW | MT_SECURE);
	mmap_add_region(BL_CODE_BASE, BL_CODE_BASE, (BL_CODE_END - BL_CODE_BASE),
			MT_CODE | MT_SECURE);
	mmap_add_region(BL_RO_DATA_BASE, BL_RO_DATA_BASE, BL_RO_DATA_END - BL_RO_DATA_BASE,
			MT_RO_DATA | MT_SECURE);

	mmap_add(sp_mmap);
	init_xlat_tables();

	enable_mmu_el3(0);
}

uint16_t sp_read_soc_id(void)
{
	uint32_t reg = mmio_read_32(SP_RGST_BASE);
	return reg;
}

uint16_t sp_read_soc_id2(void)
{
	uint32_t reg = mmio_read_32(SP_AO_RGST_BASE);
	return reg;
}

#include <lib/smccc.h>
#include <services/arm_arch_svc.h>
/* https://www.jedec.org/system/files/docs/JEP106BJ.01.pdf */
#define JEDEC_SUNPLUS_BKID U(13)
#define JEDEC_SUNPLUS_MFID U(0xc8)

/*****************************************************************************
 * plat_is_smccc_feature_available() - This function checks whether SMCCC
 *                                     feature is availabile for platform.
 * @fid: SMCCC function id
 *
 * Return SMC_OK if SMCCC feature is available and SMC_ARCH_CALL_NOT_SUPPORTED
 * otherwise.
 *****************************************************************************/
int32_t plat_is_smccc_feature_available(u_register_t fid)
{
	switch (fid) {
	case SMCCC_ARCH_SOC_ID:
		return SMC_ARCH_CALL_SUCCESS;
	default:
		return SMC_ARCH_CALL_NOT_SUPPORTED;
	}
}

int32_t plat_get_soc_version(void)
{
	uint32_t manfid = (JEDEC_SUNPLUS_BKID << 24U) | (JEDEC_SUNPLUS_MFID << 16U);

	return (int32_t)(manfid | (sp_read_soc_id2() & 0xFFFFU));
}

int32_t plat_get_soc_revision(void)
{
	return 0;
}
