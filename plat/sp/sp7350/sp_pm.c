/*
 * Copyright (c) 2017-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/gicv2.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>

#include <sp_private.h>
#include <lib/libc/errno.h>
#include <lib/el3_runtime/context_mgmt.h>

#include <sp_pm.h>

extern uint64_t sp_sec_entry_point;

#define SP_CORE_PWR_STATE(state) 	((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define SP_CLUSTER_PWR_STATE(state)	((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define SP_SYSTEM_PWR_STATE(state)	((state)->pwr_domain_state[PLAT_MAX_PWR_LVL])

#define PM_DATA_SAVE_ADDRESS   0xFA29E000   /* Save the maindomain register and User data */

#define RGST_SECURE_REG       RF_GRP(502,0)
#define SECGRP1_MAIN_REG      RF_GRP(113,0)

static void sp_platform_save_context(void)
{
	uint8_t *save_data = (uint8_t *)PM_DATA_SAVE_ADDRESS;

	/* save secure register,restore in warmboot xboot !! */
	memcpy((void *)save_data,(void *)RGST_SECURE_REG, sizeof(uint32_t) * 32);
	save_data += sizeof(uint32_t) * 32;

	memcpy((void *)save_data, (void *)SECGRP1_MAIN_REG,sizeof(uint32_t) * 3 * 32); //G113.G114.G115
	save_data += sizeof(uint32_t) * 3 * 32;

	memcpy((void *)save_data,(void *)CORE_CPU_START_POS(3), (CORE_CPU_START_POS(0)-CORE_CPU_START_POS(3)));
	save_data += (CORE_CPU_START_POS(0)-CORE_CPU_START_POS(3));

}

static int sp_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned int pos = plat_core_pos_by_mpidr(mpidr);
	uintptr_t hold_base = PLAT_SP_HOLD_BASE;
	assert(pos < PLATFORM_CORE_COUNT);

	hold_base -= pos * 8;
	mmio_write_64(hold_base, PLAT_SP_HOLD_STATE_GO);
	/* No cache maintenance here, hold_base is mapped as device memory. */
	flush_dcache_range((uintptr_t)PLAT_SP_HOLD_BASE,PLAT_SP_HOLD_SIZE);

	dsb();
	isb();
	sev();

	return rc;
}


static void sp_pwr_domain_off(const psci_power_state_t *target_state)
{
	/* set core/cluster power down in CM4*/
	assert(SP_CORE_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE);

	gicv2_cpuif_disable();
}

static void sp_pwr_domain_on_finish(const psci_power_state_t *target_state)
{

	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] == PLAT_LOCAL_STATE_OFF);
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();

}
void __dead2 plat_secondary_cold_boot_setup(void);

static void __dead2 sp_pwr_down_wfi(const psci_power_state_t *target_state)
{
	dcsw_op_all(DCCISW); //flush cache

	/* cpu off flow, detemine by system power state */
	if (SP_SYSTEM_PWR_STATE(target_state) != PLAT_MAX_OFF_STATE)
	{
		plat_secondary_cold_boot_setup();
	}
	else
	{
		unsigned int pos = plat_my_core_pos();
		sp_cpu_off(read_mpidr());

		if (pos == 0)
		{
			/* for suspend to ram: all of core wfi here, then send msg to CM4 to power down main domain */
			/* for others: core0 should off/reset other core */
			send_upf_msg_to_cm4();// CM4 will determine whether to turn off the power
		}

		dsb();
		asm volatile ("msr S3_0_C15_C2_7 ,%0" :: "r" (0x1));
		isb();

		asm volatile ("wfi");

		// for deep sleep warmboot, not power down maindomain
		write_scr_el3(read_scr_el3() & ~SCR_IRQ_BIT);
		write_rmr_el3(RMR_EL3_RR_BIT | RMR_EL3_AA64_BIT); //for warm reset test

		panic();
	}
}

static void __dead2 sp_system_off(void)
{
	gicv2_cpuif_disable();

	NOTICE("%s: L#%d\n", __func__, __LINE__);

	/* Turn off all secondary CPUs */
	sp_disable_secondary_cpus(read_mpidr());
	send_powerdown_msg_to_cm4();

	NOTICE("%s: halt\n", __func__);

	/* coverity[no_escape] */
	while (1) {
		wfi();
	}
}

static void __dead2 sp_system_reset(void)
{
	gicv2_cpuif_disable();

	console_flush();

	mmio_write_32(SP_AO_RGST_BASE + 0x4, RF_MASK_V_SET(1 << 0)); /* G0.1[0] */

	mmio_write_32(SP_WDG_ENABLE,0x0600); /* enable watedog reset */

	/* STC: watchdog control */
	mmio_write_32(SP_WDG_CTRL,0x3877);
	mmio_write_32(SP_WDG_CTRL,0xAB00);
	mmio_write_32(SP_WDG_CNT,0x0001);
	mmio_write_32(SP_WDG_CTRL,0x4A4B);

	dsbsy();
	isb();
	/* Wait before panicking */
	mdelay(1000);

	wfi();
	ERROR("System Reset: operation not handled.\n");
	panic();

}

static void sp_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	if (is_local_state_off(SP_SYSTEM_PWR_STATE(target_state)))
	{
		sp_platform_save_context();

		dsb();
		write_scr_el3(read_scr_el3() | SCR_IRQ_BIT);
		isb();
	}
}

static void sp_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{

	if (is_local_state_off(SP_SYSTEM_PWR_STATE(target_state)))
		gicv2_distif_init();
	if (is_local_state_off(SP_CORE_PWR_STATE(target_state))) {
		gicv2_pcpu_distif_init();
		gicv2_cpuif_enable();
	}

}

/*******************************************************************************
 * This handler is called by the PSCI implementation during the `SYSTEM_SUSPEND`
 * call to get the `power_state` parameter. This allows the platform to encode
 * the appropriate State-ID field within the `power_state` parameter which can
 * be utilized in `pwr_domain_suspend()` to suspend to system affinity level.
******************************************************************************/
static void sp_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	/* all affinities use system suspend state id */
	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

static void sp_cpu_standby(plat_local_state_t cpu_state)
{
	u_register_t scr;

	scr = read_scr_el3();
	/* Enable PhysicalIRQ bit for NS world to wake the CPU */
	write_scr_el3(scr | SCR_IRQ_BIT);
	isb();
	dsb();
	wfi();
	write_scr_el3(scr);
}


void sp_pwr_domain_suspend_pwrdown_early(const psci_power_state_t *target_state)
{

}

 int32_t sp_validate_power_state(uint32_t power_state,
				   psci_power_state_t *req_state)
{
	/* only used in cpu suspend mode  !!!
	   "echo mem > /sys/power/state" will not in here
	   pstate: cpu in standby/powerdown mode.
	   standby set retation, powerdown set off mode.
	*/
	int pstate = psci_get_pstate_type(power_state);
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);
	int i;

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	if (pstate == PSTATE_TYPE_STANDBY) {
		/*
		 * It's possible to enter standby only on power level 0
		 * Ignore any other power level.
		 */
		if (pwr_lvl != MPIDR_AFFLVL0)
			return PSCI_E_INVALID_PARAMS;

		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_LOCAL_STATE_RET;
	} else {
		for (i = MPIDR_AFFLVL0; i <= pwr_lvl; i++)
			req_state->pwr_domain_state[i] = PLAT_LOCAL_STATE_OFF;
	}

	/*
	 * We expect the 'state id' to be zero.
	 */
	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}

static int32_t sp_validate_ns_entrypoint(uintptr_t entrypoint)
{

	if ((entrypoint > SP_DRAM_BASE) && (entrypoint < (SP_DRAM_BASE + SP_DRAM_SIZE)))
	   return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

static plat_psci_ops_t sp_psci_ops = {
	.cpu_standby			= sp_cpu_standby,
	.pwr_domain_on			= sp_pwr_domain_on,
	.pwr_domain_off			= sp_pwr_domain_off,
	.pwr_domain_suspend		= sp_pwr_domain_suspend,
	.pwr_domain_suspend_finish	= sp_pwr_domain_suspend_finish,
	.validate_power_state		= sp_validate_power_state,
	.validate_ns_entrypoint		= sp_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= sp_get_sys_suspend_power_state,

	.system_off			= sp_system_off,
	.system_reset			= sp_system_reset,
	.pwr_domain_on_finish		= sp_pwr_domain_on_finish,
	.pwr_domain_pwr_down_wfi	= sp_pwr_down_wfi,
};


int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	NOTICE("PSCI: %s\n", __func__);

	assert(psci_ops);

	sp_sec_entry_point = sec_entrypoint;

	*psci_ops = &sp_psci_ops;

	return 0;
}
