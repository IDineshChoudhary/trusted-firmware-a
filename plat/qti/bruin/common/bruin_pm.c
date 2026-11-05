/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>

#include <cpucp.h>
#include <platform_def.h>
#include <qti_plat.h>

#define APSS_CPU_IPM_REG_BASE		0x0F800000U
#define APSS_CPU_IPM_REG_OFFSET		0x10000U
#define APSS_CPU_IPM_REG(core)		(APSS_CPU_IPM_REG_BASE + \
					 ((core) * APSS_CPU_IPM_REG_OFFSET))
#define APSS_L3_MEM_SLP_CTL		0x0F88000CU

#define CPU_HEAD_SWITCH_CTL(core)	(APSS_CPU_IPM_REG(core) + 0x08U)
#define CPU_MEM_SLP_CTL(core)		(APSS_CPU_IPM_REG(core) + 0x0cU)
#define CPU_SEQ_FORCE_PWR_CTL_EN(core)	(APSS_CPU_IPM_REG(core) + 0x1cU)
#define CPU_SEQ_FORCE_PWR_CTL_VAL(core)	(APSS_CPU_IPM_REG(core) + 0x20U)
#define CPU_PCHANNEL_FSM_CTL(core)	(APSS_CPU_IPM_REG(core) + 0x44U)
#define CPU_PCHANNEL_MASK_WAKEUP	(1U << 13)
#define QTI_PSCI_WAKE_SGI_ID		8U

static uint32_t saved_gicr_isenabler[PLATFORM_CORE_COUNT];
static bool saved_gicr_isenabler_valid[PLATFORM_CORE_COUNT];

/* Shikra APSS HPG 2.6.4: divider encoding 2 means divide by 3. */
#define MEM_SEQ_CLK_DIV_RATIO_SHIFT	16U
#define MEM_SEQ_CLK_DIV_RATIO_MASK	(0x7U << MEM_SEQ_CLK_DIV_RATIO_SHIFT)
#define MEM_SEQ_CLK_DIV_RATIO_2		(0x2U << MEM_SEQ_CLK_DIV_RATIO_SHIFT)

/* Gold (APSS alias 6) sleep-sequencer termination fields from the HPG. */
#define GOLD_MEM_SEQ_TERMINATION_MASK	0xFFFFU
#define GOLD_MEM_SEQ_TERMINATION_VALUE	0x0F68U

static void shikra_program_mem_slp_ctl(uintptr_t reg, bool is_gold)
{
	uint32_t val = mmio_read_32(reg);

	val &= ~MEM_SEQ_CLK_DIV_RATIO_MASK;
	val |= MEM_SEQ_CLK_DIV_RATIO_2;

	if (is_gold) {
		val &= ~GOLD_MEM_SEQ_TERMINATION_MASK;
		val |= GOLD_MEM_SEQ_TERMINATION_VALUE;
	}

	mmio_write_32(reg, val);
}

static void shikra_program_mem_seq_dividers(void)
{
	unsigned int core_pos;

	/* The boot core programs every CPU alias before any LPM can be entered. */
	for (core_pos = 0U; core_pos < PLATFORM_CORE_COUNT; core_pos++) {
		shikra_program_mem_slp_ctl(CPU_MEM_SLP_CTL(core_pos),
					   core_pos == 3U);
	}

	shikra_program_mem_slp_ctl(APSS_L3_MEM_SLP_CTL, false);
}

/*
 * plat_qti_pwr_psci_init - PSCI backend init hook.
 *
 * The secondary-core reset vector (RVBAR) is programmed by
 * qti_sec_core_remap(), invoked from plat_setup_psci_ops() ahead of this hook,
 * so there is nothing to do here.
 */
int plat_qti_pwr_psci_init(uintptr_t warmboot_entry)
{
	(void)warmboot_entry;
	shikra_program_mem_seq_dividers();

	return PSCI_E_SUCCESS;
}

/*
 * plat_qti_pwr_domain_on - power on a secondary core using the raw APSS IPM
 * reset sequence.
 */
int plat_qti_pwr_domain_on(u_register_t mpidr, int core_pos)
{
	(void)mpidr;

	/* Reapply the field-preserving settings before this core's power-up. */
	shikra_program_mem_slp_ctl(CPU_MEM_SLP_CTL(core_pos), core_pos == 3);

	/* Program skew between en_few and en_rest. */
	mmio_write_32(CPU_HEAD_SWITCH_CTL(core_pos), 0x28U);

	/* Clear power-control enables. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_EN(core_pos), 0x0U);

	/* Close the core logic head switch. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x642U);
	udelay(2);

	/* Deassert core memory and logic clamp. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x402U);

	/* Deassert core memory slp_nret_n and slp_ret_n. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x40aU);
	udelay(4);
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x40eU);
	udelay(4);

	/* Assert and deassert wl_en_clk. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x50eU);
	udelay(2);
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x40eU);

	/* Deassert clock off. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x40cU);
	udelay(2);

	/* Assert the core P-channel power-up request. */
	mmio_write_32(CPU_PCHANNEL_FSM_CTL(core_pos), 0x1U);

	/* Deassert core reset. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x43cU);

	/* Deassert the core P-channel power-up request. */
	mmio_write_32(CPU_PCHANNEL_FSM_CTL(core_pos), 0x0U);

	/* Assert OSM core active. */
	mmio_write_32(CPU_SEQ_FORCE_PWR_CTL_VAL(core_pos), 0x443cU);

	/* Assert CPU_PWRDUP. */
	mmio_write_32(CPU_HEAD_SWITCH_CTL(core_pos), 0x428U);

	return PSCI_E_SUCCESS;
}

/*
 * plat_qti_pwr_domain_on_finish - per-core setup once it has come online.
 */
void plat_qti_pwr_domain_on_finish(int core_pos, const uint8_t *states)
{
	(void)core_pos;
	(void)states;

	/* USE_GIC_DRIVER=0: initialize this core's redistributor here. */
	plat_qti_gic_pcpu_init();
	/* 
	 * TODO:
	 * Stubbed function cpucp_clkdom_init 
	 * as part of initial enablement.
	 * cpucp_clkdom_init(); 
	 */
}

/*
 * Match Shikra TZ's PSCI C3/C4 handling: disable the CPU-interface Group 0,
 * Group 1 Secure, and Group 1 Non-secure enables. Do not put the redistributor
 * to sleep here; TZ leaves its lifecycle alone for CPU_SUSPEND.
 */
void plat_qti_gic_pcpu_suspend(void)
{
	write_icc_igrpen0_el1(read_icc_igrpen0_el1() &
			      ~IGRPEN1_EL1_ENABLE_G0_BIT);
	write_icc_igrpen1_el3(read_icc_igrpen1_el3() &
			      ~(IGRPEN1_EL3_ENABLE_G1S_BIT |
				IGRPEN1_EL3_ENABLE_G1NS_BIT));
	isb();
	dsbsy();
}

void plat_qti_gic_pcpu_suspend_finish(void)
{
	/* Match TZ: restore the NS priority threshold before reopening G1. */
	write_icc_pmr_el1(QTI_GIC_PMR_LOWEST);
	dsbsy();

	write_icc_igrpen0_el1(read_icc_igrpen0_el1() |
			      IGRPEN1_EL1_ENABLE_G0_BIT);
	write_icc_igrpen1_el3(read_icc_igrpen1_el3() |
			      (IGRPEN1_EL3_ENABLE_G1S_BIT |
			       IGRPEN1_EL3_ENABLE_G1NS_BIT));
	isb();
	dsbsy();
}

/*
 * TZ preserves the per-CPU GICR enable bitmap across CPU_OFF/CPU_ON, while
 * leaving SGI 8 enabled as its CPU-on wake interrupt. Do not use these hooks
 * for CPU_SUSPEND: C3/C4 retain the GICR enable state and Linux owns its idle
 * interrupt lifecycle.
 */
void plat_qti_gic_pcpu_save(void)
{
	unsigned int core_pos = plat_my_core_pos();
	uintptr_t gicr_base;

	if (core_pos >= PLATFORM_CORE_COUNT) {
		return;
	}

	gicr_base = plat_qti_gic_rdistif_base_addr(core_pos);
	if (gicr_base == 0U) {
		return;
	}

	saved_gicr_isenabler[core_pos] =
		mmio_read_32(gicr_base + GICR_ISENABLER0);
	saved_gicr_isenabler_valid[core_pos] = true;
	mmio_write_32(gicr_base + GICR_ICENABLER0,
		      ~(1U << QTI_PSCI_WAKE_SGI_ID));
	while ((mmio_read_32(gicr_base + GICR_CTLR) &
		GICR_CTLR_RWP_BIT) != 0U) {
		;
	}
	dsbsy();
}

bool plat_qti_gic_pcpu_restore(void)
{
	unsigned int core_pos = plat_my_core_pos();
	uintptr_t gicr_base;

	if ((core_pos >= PLATFORM_CORE_COUNT) ||
	    !saved_gicr_isenabler_valid[core_pos]) {
		return false;
	}

	gicr_base = plat_qti_gic_rdistif_base_addr(core_pos);
	if (gicr_base == 0U) {
		return false;
	}

	mmio_write_32(gicr_base + GICR_ISENABLER0,
		      saved_gicr_isenabler[core_pos]);
	while ((mmio_read_32(gicr_base + GICR_CTLR) &
		GICR_CTLR_RWP_BIT) != 0U) {
		;
	}
	dsbsy();
	saved_gicr_isenabler_valid[core_pos] = false;
	return true;
}

/*
 * PSCI CPU_OFF is distinct from Linux CPU_SUSPEND: Shikra TZ masks P-channel
 * wake for OFF/FREEZE, but leaves it unmasked for the SUSPEND context used by
 * CPU idle C3/C4.
 */
void plat_qti_pwr_domain_off(const uint8_t *states)
{
	unsigned int core_pos = plat_my_core_pos();
	uint32_t val;

	if ((states[QTI_PWR_LVL0] != QTI_LOCAL_STATE_OFF) &&
	    (states[QTI_PWR_LVL0] != QTI_LOCAL_STATE_DEEPOFF)) {
		return;
	}

	/*
	 * TZ masks P-channel wake only for PSCI OFF/FREEZE contexts. Keep the
	 * wake path armed for CPU_SUSPEND, including the Shikra C4 idle state.
	 */
	val = mmio_read_32(CPU_PCHANNEL_FSM_CTL(core_pos));
	mmio_write_32(CPU_PCHANNEL_FSM_CTL(core_pos),
		      val | CPU_PCHANNEL_MASK_WAKEUP);
	dsbsy();
}

void plat_qti_pwr_domain_suspend(const uint8_t *states)
{
	/* Shikra TZ does not mask P-channel wake in PSCI_CONTEXT_SUSPEND. */
	(void)states;
}

void plat_qti_pwr_domain_suspend_finish(const uint8_t *states)
{
	(void)states;
	/* qti_node_suspend_finish() restores the GIC groups before this hook. */
}

void plat_qti_invoke_unhandled_isr(uint32_t id, void *handle)
{
	(void)id;
	(void)handle;
}

const unsigned int *plat_qti_pm_idle_states(void)
{
	static const unsigned int idle_states[] = {
		qti_make_pwrstate_lvl0(QTI_LOCAL_STATE_STB,
				       PSTATE_TYPE_STANDBY),
		/* Shikra CPU PCU mode 3 is C3. */
		qti_make_pwrstate_lvl0(QTI_LOCAL_STATE_OFF,
				       PSTATE_TYPE_POWERDOWN),
		/* CPU-level DEEPOFF/C4 is intentionally enabled for LPM support.
		 * The coupled level-1/L3 DEEPOFF state remains unavailable until its
		 * separate cluster sequencing and ownership path is implemented.
		 */
		qti_make_pwrstate_lvl0(QTI_LOCAL_STATE_DEEPOFF,
				       PSTATE_TYPE_POWERDOWN),
		0,
	};

	return idle_states;
}
