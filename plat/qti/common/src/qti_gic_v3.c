/*
 * Copyright (c) 2015-2024, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2024, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>

#include <platform.h>
#include <platform_def.h>
#include <qti_plat.h>

/* The GICv3 driver only needs to be initialized in EL3 */
static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static gicv3_driver_data_t qti_gic_data = {
	.gicd_base = QTI_GICD_BASE,
	.gicr_base = QTI_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.mpidr_to_core_pos = plat_qti_core_pos_by_mpidr
};

void plat_qti_gic_driver_init(void)
{
	qti_gic_data.interrupt_props =
		plat_qti_get_interrupt_props(&qti_gic_data.interrupt_props_num);

	/*
	 * The GICv3 driver is initialized in EL3 and does not need
	 * to be initialized again in SEL1. This is because the S-EL1
	 * can use GIC system registers to manage interrupts and does
	 * not need GIC interface base addresses to be configured.
	 */
	gicv3_driver_init(&qti_gic_data);
}

/******************************************************************************
 * ARM common helper to initialize the GIC. Only invoked by BL31
 *****************************************************************************/
void plat_qti_gic_init(void)
{
	const interrupt_prop_t *interrupt_props;
	unsigned int interrupt_props_num;
	unsigned int i;

	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());

	interrupt_props = plat_qti_get_interrupt_props(&interrupt_props_num);

	/* Route secure spi interrupt to ANY. */
	for (i = 0; i < interrupt_props_num; i++) {
		unsigned int int_id = interrupt_props[i].intr_num;

		if (plat_ic_is_spi(int_id)) {
			gicv3_set_spi_routing(int_id, GICV3_IRM_ANY, 0x0);
		}
	}
}

void gic_set_spi_routing(unsigned int id, unsigned int irm, u_register_t target)
{
	gicv3_set_spi_routing(id, irm, target);
}

/******************************************************************************
 * USE_GIC_DRIVER=0 leaves per-CPU GIC lifecycle management to the platform.
 *****************************************************************************/
void plat_qti_gic_cpuif_enable(void)
{
	gicv3_cpuif_enable(plat_my_core_pos());

	/*
	 * Match Shikra TZ's warm-boot setup: HAL_QGIC_PRIORITY_LOWEST is 254
	 * (0xFE). The generic TF-A helper initializes ICC_PMR_EL1 to 0xFF,
	 * so restore the platform threshold before enabling Group 1 Non-secure.
	 */
	write_icc_pmr_el1(QTI_GIC_PMR_LOWEST);
	dsbsy();

	/*
	 * gicv3_cpuif_enable() only restores EnableGrp1S; EnableGrp1NS is
	 * cleared by gicv3_cpuif_disable() and nothing else sets it again.
	 * Without this, EL3 keeps Non-secure Group 1 interrupts masked after
	 * the first CPU_SUSPEND and Linux never takes another IRQ.
	 */
	write_icc_igrpen1_el3(read_icc_igrpen1_el3() |
			       IGRPEN1_EL3_ENABLE_G1NS_BIT);
	isb();
}

/******************************************************************************
 * ARM common helper to disable the GIC CPU interface
 *****************************************************************************/
void plat_qti_gic_cpuif_disable(void)
{
	gicv3_cpuif_disable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to initialize the per-CPU redistributor interface in GICv3
 *****************************************************************************/
void plat_qti_gic_pcpu_init(void)
{
	gicv3_rdistif_init(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helpers to power GIC redistributor interface
 *****************************************************************************/
void plat_qti_gic_redistif_on(void)
{
	gicv3_rdistif_on(plat_my_core_pos());
}

void plat_qti_gic_redistif_off(void)
{
	gicv3_rdistif_off(plat_my_core_pos());
}

/* Expose a core's redistributor base for suspend-time pending-state diagnostics. */
uintptr_t plat_qti_gic_rdistif_base_addr(unsigned int core_pos)
{
	return rdistif_base_addrs[core_pos];
}
