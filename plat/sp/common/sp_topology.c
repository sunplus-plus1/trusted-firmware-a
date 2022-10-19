#include <platform_def.h>
#include <arch.h>
#include <plat/common/platform.h>

static const unsigned char plat_power_domain_tree_desc[PLAT_MAX_PWR_LVL + 1] = {
	/* One root node for the SoC */
	1,
	/* One node for each cluster */
	PLATFORM_CLUSTER_COUNT,
	/* One set of CPUs per cluster */
	PLATFORM_MAX_CPUS_PER_CLUSTER,
};

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	unsigned int cluster_id, cpu_id;
	mpidr &= MPIDR_AFFINITY_MASK;

	if (mpidr & ~(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK))
		return -1;

	cluster_id = (mpidr >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK;
	cpu_id = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;

	if (cluster_id >= PLATFORM_CLUSTER_COUNT)
		return -1;

	/*
	 * Validate cpu_id by checking whether it represents a CPU in
	 * one of the two clusters present on the platform.
	 */
	if (cpu_id >= PLATFORM_MAX_CPUS_PER_CLUSTER)
		return -1;

	return (cpu_id + (cluster_id * 4));

}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return plat_power_domain_tree_desc;
}
