/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include "sp_ethosn_getter.h"
#include "ethosn.h"

#define FCONF_GET_PROPERTY(a, b, c)	a##__##b##_getter(c)

/*
 * Number of Arm Ethos-N NPU (NPU) cores available for a
 * particular parent device
 */
#define ETHOSN_NUM_CORES \
	FCONF_GET_PROPERTY(hw_config, ethosn_config, num_cores)

/* Address to an NPU core  */
#define ETHOSN_CORE_ADDR(core_idx) \
	FCONF_GET_PROPERTY(hw_config, ethosn_core_addr, core_idx)

/* NPU core sec registry address */
#define ETHOSN_CORE_SEC_REG(core_addr, reg_offset) \
	(core_addr  +  reg_offset)

/* Reset timeout in us */
#define ETHOSN_RESET_TIMEOUT_US		U(10 * 1000 * 1000)
#define ETHOSN_RESET_WAIT_US		U(1)

#define SEC_DEL_REG			U(0x0004)
#define SEC_DEL_VAL			U(0x81C)
#define SEC_DEL_EXCC_MASK		U(0x20)

#define SEC_SECCTLR_REG			U(0x0010)
#define SEC_SECCTLR_VAL			U(0x3)

#define SEC_DEL_MMUSID_REG		U(0x2008)
#define SEC_DEL_MMUSID_VAL		U(0x3FFFF)

#define SEC_DEL_ADDR_EXT_REG		U(0x201C)
#define SEC_DEL_ADDR_EXT_VAL		U(0x15)

#define SEC_SYSCTRL0_REG		U(0x0018)
#define SEC_SYSCTRL0_SOFT_RESET		U(3U << 29)
#define SEC_SYSCTRL0_HARD_RESET		U(1U << 31)

static bool ethosn_is_core_addr_valid(uintptr_t core_addr)
{
	for (uint32_t core_idx = 0U; core_idx < ETHOSN_NUM_CORES; core_idx++) {
		if (ETHOSN_CORE_ADDR(core_idx) == core_addr) {
			return true;
		}
	}

	return false;
}

static void ethosn_delegate_to_ns(uintptr_t core_addr)
{
	mmio_setbits_32(ETHOSN_CORE_SEC_REG(core_addr, SEC_SECCTLR_REG),
			SEC_SECCTLR_VAL);

	mmio_setbits_32(ETHOSN_CORE_SEC_REG(core_addr, SEC_DEL_REG),
			SEC_DEL_VAL);

	mmio_setbits_32(ETHOSN_CORE_SEC_REG(core_addr, SEC_DEL_MMUSID_REG),
			SEC_DEL_MMUSID_VAL);

	mmio_setbits_32(ETHOSN_CORE_SEC_REG(core_addr, SEC_DEL_ADDR_EXT_REG),
			SEC_DEL_ADDR_EXT_VAL);
}

static int ethosn_is_sec(uintptr_t core_addr)
{
	if ((mmio_read_32(ETHOSN_CORE_SEC_REG(core_addr, SEC_DEL_REG))	
		& SEC_DEL_EXCC_MASK) != 0U) {
		return 0;
	}

	return 1;
}

static bool ethosn_reset(uintptr_t core_addr, int hard_reset)
{
	unsigned int timeout;
	const uintptr_t sysctrl0_reg =
		ETHOSN_CORE_SEC_REG(core_addr, SEC_SYSCTRL0_REG);
	const uint32_t reset_val = (hard_reset != 0) ? SEC_SYSCTRL0_HARD_RESET
						    : SEC_SYSCTRL0_SOFT_RESET;

    //dump wave form
	//mmio_write_32(0xF8000000, 0xabcd1234);

#if 0//debug 
	NOTICE("SEC_DEL_REG:f8200004:(%x)\n",mmio_read_32(ETHOSN_CORE_SEC_REG(ETHOSN_CORE_ADDR(0), SEC_DEL_REG)));
	NOTICE("sysctrl0_reg_adr:(%lx) reset_val: (%x)\n",sysctrl0_reg,reset_val);
	NOTICE("sysctrl0_reg:(%x)\n",mmio_read_32(sysctrl0_reg));
	//NOTICE("0xF821F000-NPUID:(%x)\n",mmio_read_32(0xF821F000));
	NOTICE("sysctrl0_DL1_reg:(%x)\n",mmio_read_32(0xF8210018));
	//NOTICE("0xF821F004-UNIT_COUNT:(%x)\n",mmio_read_32(0xF821F004));
	//NOTICE("0xF821F008-MCE-FEATURE:(%x)\n",mmio_read_32(0xF821F008));
	//mmio_write_32(0xF8210018, 0x60000000);
	//NOTICE("sysctrl0_DL1_reg:(%x)\n",mmio_read_32(0xF8210018));
	//NOTICE("0xF821F00C-DFC-FEATURE:(%x)\n",mmio_read_32(0xF821F00C));

    NOTICE("HW sec control(0xF80001E0):(%x)\n",mmio_read_32(0xF80001E0));//mon hw sec n78:bit29,bit13
    NOTICE("HW sec control(0xF80029E8):(%x)\n",mmio_read_32(0xF80029E8));//mon hw sec n78:bit9,bit1
#endif
#if 1//enable HW auto reset
    //NOTICE("HW reset control(0xF8000164):(%x)\n",mmio_read_32(0xF8000164));
    mmio_write_32(0xF8000164, 0x2000|(0x2000<<16));//HW auto reset enable (bit13)
    udelay(1);
	//NOTICE("HW reset control(0xF8000164):(%x)\n",mmio_read_32(0xF8000164));
#endif

	mmio_write_32(sysctrl0_reg, reset_val);//n78 sw reset
	udelay(1);
	//NOTICE("HW reset control(0xF8000164):(%x)\n",mmio_read_32(0xF8000164));

#if 0//enable SW monitor reset
	/* Wait for N78 resetreq(0xF8000164.bit12 to be 1. then it can reset N78 manually with Moon N78 reset reg (0xF800005C.0) */
	for (timeout = 0U; timeout < ETHOSN_RESET_TIMEOUT_US;
			   timeout += ETHOSN_RESET_WAIT_US) {

		if ((mmio_read_32(0xF8000164) & 0x1000) == 0x1000) {
			break;
		}

		udelay(ETHOSN_RESET_WAIT_US);
	}    

	if (timeout >= ETHOSN_RESET_TIMEOUT_US){
        return 0;
	}

	mmio_write_32(0xF800005C, 1|(1<<16));//Enable moon N78 reset
	udelay(1);
	mmio_write_32(0xF800005C, 0|(1<<16));//Reset N78 and disable moon N78 reset
    udelay(1);
	NOTICE("HW reset control(0xF8000164):(%x)\n",mmio_read_32(0xF8000164));
#endif

#if 0//reset work arround
	udelay(1);
    NOTICE("HW reset control(0xF800005C):(%x)\n",mmio_read_32(0xF800005C));
	mmio_write_32(0xF800005C, 1|(1<<16));//moon N78 reset
	udelay(1);
	NOTICE("HW reset control(0xF800005C):(%x)\n",mmio_read_32(0xF800005C));
	mmio_write_32(0xF800005C, 0|(1<<16));
	udelay(1);	
	NOTICE("HW reset control(0xF800005C):(%x)\n",mmio_read_32(0xF800005C));	

	//NOTICE("after swreset\n");
	//NOTICE("0xF821F000-NPUID:(%x)\n",mmio_read_32(0xF821F000));
	//NOTICE("after swreset1\n");
#endif

	/* Wait for reset to complete */
	for (timeout = 0U; timeout < ETHOSN_RESET_TIMEOUT_US;
			   timeout += ETHOSN_RESET_WAIT_US) {

		if ((mmio_read_32(sysctrl0_reg) & reset_val) == 0U) {
			break;
		}

		udelay(ETHOSN_RESET_WAIT_US);
	}

	return timeout < ETHOSN_RESET_TIMEOUT_US;
}

uintptr_t ethosn_smc_handler(uint32_t smc_fid,
                 u_register_t core_addr,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4,
			     void *cookie,
			     void *handle,
			     u_register_t flags)
{
	int hard_reset = 0;
	const uint32_t fid = smc_fid & FUNCID_NUM_MASK;

	/* Only SiP fast calls are expected */
	if ((GET_SMC_TYPE(smc_fid) != SMC_TYPE_FAST) ||
		(GET_SMC_OEN(smc_fid) != OEN_SIP_START)) {
		SMC_RET1(handle, SMC_UNK);
	}

	/* Truncate parameters to 32-bits for SMC32 */
	if (GET_SMC_CC(smc_fid) == SMC_32) {
        core_addr &= 0xFFFFFFFF;
		x2 &= 0xFFFFFFFF;
		x3 &= 0xFFFFFFFF;
		x4 &= 0xFFFFFFFF;
	}

	if (!is_ethosn_fid(smc_fid)) {
		SMC_RET1(handle, SMC_UNK);
	}

	/* Commands that do not require a valid core address */
	switch (fid) {
 	case ETHOSN_FNUM_VERSION:
 		SMC_RET2(handle, ETHOSN_VERSION_MAJOR, ETHOSN_VERSION_MINOR);
	}

	if (!ethosn_is_core_addr_valid(core_addr)) {
		WARN("ETHOSN: Unknown core address given to SMC call.\n");
		SMC_RET1(handle, ETHOSN_UNKNOWN_CORE_ADDRESS);
	}

	/* Commands that require a valid addr */
	switch (fid) {
	case ETHOSN_FNUM_VERSION:
		SMC_RET2(handle, ETHOSN_VERSION_MAJOR, ETHOSN_VERSION_MINOR);
	case ETHOSN_FNUM_IS_SEC:
        SMC_RET1(handle, ethosn_is_sec(core_addr));
	case ETHOSN_FNUM_HARD_RESET:
		hard_reset = 1;
		/* Fallthrough */
	case ETHOSN_FNUM_SOFT_RESET:
		if (!ethosn_reset(core_addr, hard_reset)) {
			SMC_RET1(handle, ETHOSN_FAILURE);
		}
		ethosn_delegate_to_ns(core_addr);
		SMC_RET1(handle, ETHOSN_SUCCESS);
	default:
		SMC_RET1(handle, SMC_UNK);
	}
}