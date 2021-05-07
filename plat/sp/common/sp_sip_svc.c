/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/pmf/pmf.h>
#include <lib/mmio.h>
#include <tools_share/uuid.h>

#include "sp_sip_svc.h"

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
	uint32_t ns;
	int call_count = 0;

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

#if ARM_ETHOSN_NPU_DRIVER
	if (is_ethosn_fid(smc_fid)) {
		return ethosn_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
					  handle, flags);
	}

#endif /* ARM_ETHOSN_NPU_DRIVER */

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);
	if (!ns)
		SMC_RET1(handle, SMC_UNK);

	switch (smc_fid) {
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
