/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/pmf/pmf.h>
#include <lib/mmio.h>
#include <tools_share/uuid.h>
#include "ethosn.h"
#include <lib/debugfs.h>
#include "sp_sip_svc.h"
#include <plat/arm/common/plat_arm.h>

/* SP SiP Service UUID */
DEFINE_SVC_UUID2(sp_sip_svc_uid,
	0x01c8882a, 0xd4b4, 0x4a19, 0xbb, 0x47,
	0x90, 0x83, 0x79, 0x72, 0xd2, 0xc2);

static int32_t sp_svc_setup(void)
{
#if ENABLE_PMF
	if (pmf_setup() != 0) {
		return 1;
	}
#endif	

#if USE_DEBUGFS

	if (debugfs_smc_setup() != 0) {
		return 1;
	}

#endif /* USE_DEBUGFS */

	return 0;
}

/*
 * This function is responsible for handling all SiP calls from the NS world
 */
uintptr_t sip_smc_handler(uint32_t smc_fid,
			  u_register_t x1,
			  u_register_t x2,
			  u_register_t x3,
			  u_register_t x4,
			  void *cookie,
			  void *handle,
			  u_register_t flags)
{
//	uint32_t ns;
	int call_count = 0;

	NOTICE("sip_smc_handler:(%x), (%lx), (%lx), (%lx), (%lx), (%x), (%x), (%lx)\n",smc_fid,x1,x2,x3,x4,*(volatile uint32_t*)(cookie),*(volatile uint32_t*)(handle),flags);

#if ENABLE_PMF
	/*
	 * Dispatch PMF calls to PMF SMC handler and return its return
	 * value
	 */
	if (is_pmf_fid(smc_fid)) {
		return pmf_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
				handle, flags);
	}
#endif /* ENABLE_PMF */

#if USE_DEBUGFS

	if (is_debugfs_fid(smc_fid)) {
		return debugfs_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
					   handle, flags);
	}

#endif /* USE_DEBUGFS */

    NOTICE("sip_smc_handler:(%x)\n",smc_fid);
#if ARM_ETHOSN_NPU_DRIVER
	if (is_ethosn_fid(smc_fid)) {
		NOTICE("ethosn_smc_handler\n");
		return ethosn_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
					  handle, flags);
	}
	else {NOTICE("is_ethosn_fid:(%x)\n",(((smc_fid) & ETHOSN_FID_MASK) == ETHOSN_FID_VALUE));}

#endif /* ARM_ETHOSN_NPU_DRIVER */

	/* Determine which security state this SMC originated from */
//	ns = is_caller_non_secure(flags);
//	if (!ns)
//		SMC_RET1(handle, SMC_UNK);

	switch (smc_fid) {
	case SP_SIP_SVC_EXE_STATE_SWITCH: {
		/* Execution state can be switched only if EL3 is AArch64 */
#ifdef __aarch64__
		/* Allow calls from non-secure only */
		if (!is_caller_non_secure(flags))
			SMC_RET1(handle, STATE_SW_E_DENIED);

		/*
		 * Pointers used in execution state switch are all 32 bits wide
		 */
//		return (uintptr_t) arm_execution_state_switch(smc_fid,
//				(uint32_t) x1, (uint32_t) x2, (uint32_t) x3,
//				(uint32_t) x4, handle);
#else
		/* State switch denied */
		SMC_RET1(handle, STATE_SW_E_DENIED);
#endif /* __aarch64__ */
        WARN("SP_SIP_SVC_EXE_STATE_SWITCH: \n");
		}

	case SP_SIP_SVC_CALL_COUNT:
	/* PMF calls */
#if ENABLE_PMF	
		call_count += PMF_NUM_SMC_CALLS;
#endif /* ENABLE_PMF */

#if ARM_ETHOSN_NPU_DRIVER
		/* ETHOSN calls */
		call_count += ETHOSN_NUM_SMC_CALLS;
#endif /* ARM_ETHOSN_NPU_DRIVER */

		/* State switch call */
		call_count += 1;

		SMC_RET1(handle, call_count);

	case SP_SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, sp_sip_svc_uid);

	case SP_SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, SP_SIP_SVC_VERSION_MAJOR,
			SP_SIP_SVC_VERSION_MINOR);

	default:
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	sp_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	sp_svc_setup,
	sip_smc_handler
);
