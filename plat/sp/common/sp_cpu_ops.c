/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <platform_def.h>

void sp_cpu_off(u_register_t mpidr)
{

}

void sp_cpu_on(u_register_t mpidr)
{

}

void sp_disable_secondary_cpus(u_register_t primary_mpidr)
{

}

void send_upf_msg_to_cm4(void)
{
	mmio_write_32(SP_MAILBOX_CA55_TO_M4_IRQ0, 0x1234abcd);
}

void send_powerdown_msg_to_cm4(void)
{
	mmio_write_32(SP_MAILBOX_CA55_TO_M4_IRQ0, 0x12348765);
}

