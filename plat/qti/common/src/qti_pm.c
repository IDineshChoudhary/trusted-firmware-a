/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018, 2020, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>

#include <arch_helpers.h>
#include <bl31/bl31.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/qti/chipinfo/chipinfo.h>
#include <drivers/qti/qtipmlib/qtipmlib.h>
#include <drivers/qti/sec_core/sec_core.h>
#include <drivers/qti/watchdog/watchdog.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <lib/spinlock.h>
#include <platform.h>
#include <platform_def.h>
#include <qti_cpu.h>
#include <qti_plat.h>

#define QTI_LOCAL_PSTATE_MASK		((1 << QTI_LOCAL_PSTATE_WIDTH) - 1)

#if PSCI_OS_INIT_MODE
#define QTI_LAST_AT_PLVL_MASK		(QTI_LOCAL_PSTATE_MASK <<	\
					 (QTI_LOCAL_PSTATE_WIDTH *	\
					  (PLAT_MAX_PWR_LVL + 1)))
#endif

/* QTI_CORE_PWRDN_EN_MASK happens to be same across all CPUs */
#define QTI_CORE_PWRDN_EN_MASK		1

/* Shikra TZ PCU mode 1 is clock-gating; mode 2 is retention. */
#define QTI_PCU_MODE_RET		2
#define QTI_PCU_CPU_MODE_OFF		3
#define QTI_PCU_MODE_DEEPOFF		4

/* Keep C4 bring-up diagnostics useful without flooding the boot log. */
#define QTI_C4_TRACE_LOG_LIMIT		8U

static unsigned int qti_c4_validate_log_count[PLATFORM_CORE_COUNT];
static unsigned int qti_c4_pcu_log_count[PLATFORM_CORE_COUNT];

volatile uint32_t break_before_lpms = 0;

static bool qti_is_c4_candidate(unsigned int power_state)
{
	return (psci_get_pstate_type(power_state) == PSTATE_TYPE_POWERDOWN) &&
		((psci_get_pstate_id(power_state) & QTI_LOCAL_PSTATE_MASK) ==
		 QTI_LOCAL_STATE_DEEPOFF);
}

#ifdef QTI_PMLIB_ENABLED
/* Shikra qtipmlib pcu.c trace symbols and the HWIO addresses it snapshots. */
extern volatile uint32_t qtipmlib_lpm_trace_head;

#define QTI_PCU_SEQ_CTL			0x024U
#define QTI_PCU_SEQ_STS3_DELTA		0x01cU
#define QTI_PCU_CPU_LPM_STATUS1		0x0fd800f0U
#define QTI_PCU_CPU_LPM_STATUS2		0x0fd800f4U
#define QTI_PCU_CLKDOM0_LPM_STATUS	0x0fd90314U
#define QTI_PCU_CLKDOM1_LPM_STATUS	0x0fd91314U
#define QTI_PCU_CLKDOM2_LPM_STATUS	0x0fd92314U
#define QTI_PCU_MUC_HANG_DET_STS	0x0fd98020U

/* Read-only RIMPS state captured at CPU C3/C4 config/resume boundaries. */
#define QTI_RIMPS_TRACE_DEPTH		16U
#define QTI_RIMPS_TRACE_MAGIC_VALUE	0x51524c50U
#define QTI_RIMPSTOP_GLOBAL_ENABLE	0x0fd98004U
#define QTI_RIMPSFAST_MUC_CLK_CTRL	0x0fd80000U
#define QTI_RIMPSFAST_CPU_LPM_ARB_CTL	0x0fd800fcU
#define QTI_RIMPS_CLKDOM0_BASE		0x0fd90000U
#define QTI_RIMPS_CLKDOM1_BASE		0x0fd91000U
#define QTI_RIMPS_CLKDOM2_BASE		0x0fd92000U
#define QTI_RIMPS_CLKDOM_RIMPS_ENABLE	0x000U
#define QTI_RIMPS_CLKDOM_LPM_STATUS	0x314U
#define QTI_RIMPS_CLKDOM_SEQ_CTRL1	0x348U
#define QTI_RIMPS_CLKDOM_SEQ_CTRL	0x34cU

enum qti_rimps_lpm_trace_event {
	QTI_RIMPS_TRACE_CPU_LPM_SUSPEND_ENTRY = 1U,
	QTI_RIMPS_TRACE_CPU_LPM_PCU_CONFIG_DONE,
	QTI_RIMPS_TRACE_CPU_LPM_RESUME_ENTRY,
	QTI_RIMPS_TRACE_CPU_LPM_PCU_UNCONFIG_DONE,
	QTI_RIMPS_TRACE_GOLD_POST_ENABLE,
	QTI_RIMPS_TRACE_CPU_LPM_GIC_GROUPS_DISABLED,
	QTI_RIMPS_TRACE_CPU_LPM_GIC_GROUPS_ENABLED,
	QTI_RIMPS_TRACE_CPU_LPM_FINISH_DONE,
};

#define QTI_RIMPS_TRACE_FLAG_REG_SNAPSHOT	(1U << 0)
#define QTI_RIMPS_TRACE_NO_CPU_STATE		0xffffffffU

struct qti_rimps_lpm_trace_record {
	uint64_t counter;
	uint64_t mpidr;
	volatile uint32_t commit;
	uint32_t event;
	uint32_t cpu_state;
	uint32_t flags;
	uint32_t global_enable;
	uint32_t muc_clk_ctrl;
	uint32_t cpu_lpm_arb_ctl;
	struct {
		uint32_t rimps_enable;
		uint32_t lpm_status;
		uint32_t seq_ctrl1;
		uint32_t seq_ctrl;
	} clkdom[3];
};
typedef char qti_rimps_lpm_trace_record_size_check[
	(sizeof(struct qti_rimps_lpm_trace_record) == 96U) ? 1 : -1];
typedef char qti_rimps_lpm_trace_depth_check[
	((QTI_RIMPS_TRACE_DEPTH & (QTI_RIMPS_TRACE_DEPTH - 1U)) == 0U) ?
	1 : -1];

/* Globally visible for post-mortem extraction from the matching BL31 ELF. */
__attribute__((used))
volatile uint32_t qti_rimps_lpm_trace_magic =
	QTI_RIMPS_TRACE_MAGIC_VALUE;
__attribute__((used, aligned(64)))
volatile struct qti_rimps_lpm_trace_record
	qti_rimps_lpm_trace[PLATFORM_CORE_COUNT][QTI_RIMPS_TRACE_DEPTH];
__attribute__((used))
volatile uint32_t qti_rimps_lpm_trace_head[PLATFORM_CORE_COUNT];
/* Preserve the one-time Gold enable baseline independently of the ring. */
__attribute__((used, aligned(8)))
volatile struct qti_rimps_lpm_trace_record qti_rimps_lpm_trace_gold_baseline;

static const uintptr_t qti_rimps_clkdom_base[3] = {
	QTI_RIMPS_CLKDOM0_BASE,
	QTI_RIMPS_CLKDOM1_BASE,
	QTI_RIMPS_CLKDOM2_BASE,
};

/* Gold OSM is enabled on-demand, after the boot-time qtipmlib snapshot. */
static spinlock_t qti_gold_rimps_baseline_lock;
static bool qti_gold_rimps_baseline_captured;

static void qti_rimps_lpm_trace_write_record(
	volatile struct qti_rimps_lpm_trace_record *record,
	uint32_t ticket, uint32_t event, uint32_t cpu_state,
	bool sample_registers)
{
	unsigned int i;
	/* A zero commit marks a slot being rewritten or not yet captured. */
	record->commit = 0U;
	record->event = event;
	record->cpu_state = cpu_state;
	record->flags = 0U;
	record->counter = read_cntpct_el0();
	record->mpidr = read_mpidr_el1();
	if (sample_registers) {
		record->global_enable =
			mmio_read_32(QTI_RIMPSTOP_GLOBAL_ENABLE);
		record->muc_clk_ctrl =
			mmio_read_32(QTI_RIMPSFAST_MUC_CLK_CTRL);
		record->cpu_lpm_arb_ctl =
			mmio_read_32(QTI_RIMPSFAST_CPU_LPM_ARB_CTL);
		for (i = 0U; i < 3U; i++) {
			uintptr_t base = qti_rimps_clkdom_base[i];

			record->clkdom[i].rimps_enable =
				mmio_read_32(base + QTI_RIMPS_CLKDOM_RIMPS_ENABLE);
			record->clkdom[i].lpm_status =
				mmio_read_32(base + QTI_RIMPS_CLKDOM_LPM_STATUS);
			record->clkdom[i].seq_ctrl1 =
				mmio_read_32(base + QTI_RIMPS_CLKDOM_SEQ_CTRL1);
			record->clkdom[i].seq_ctrl =
				mmio_read_32(base + QTI_RIMPS_CLKDOM_SEQ_CTRL);
		}
		record->flags = QTI_RIMPS_TRACE_FLAG_REG_SNAPSHOT;
	} else {
		record->global_enable = 0U;
		record->muc_clk_ctrl = 0U;
		record->cpu_lpm_arb_ctl = 0U;
		for (i = 0U; i < 3U; i++) {
			record->clkdom[i].rimps_enable = 0U;
			record->clkdom[i].lpm_status = 0U;
			record->clkdom[i].seq_ctrl1 = 0U;
			record->clkdom[i].seq_ctrl = 0U;
		}
	}
	/* Make the payload visible to the external debugger before committing it. */
	clean_dcache_range((uintptr_t)record, sizeof(*record));
	record->commit = ticket + 1U;
	clean_dcache_range((uintptr_t)&record->commit, sizeof(record->commit));
}

static void qti_rimps_lpm_trace_capture(uint32_t event, uint32_t cpu_state,
					bool sample_registers)
{
	unsigned int core_pos = plat_my_core_pos();
	uint32_t ticket;
	uint32_t slot;
	volatile struct qti_rimps_lpm_trace_record *record;

	if ((core_pos >= PLATFORM_CORE_COUNT) ||
	    (qti_rimps_lpm_trace_magic != QTI_RIMPS_TRACE_MAGIC_VALUE)) {
		return;
	}

	ticket = qti_rimps_lpm_trace_head[core_pos];
	slot = ticket & (QTI_RIMPS_TRACE_DEPTH - 1U);
	record = &qti_rimps_lpm_trace[core_pos][slot];
	qti_rimps_lpm_trace_write_record(record, ticket, event, cpu_state,
					 sample_registers);
	qti_rimps_lpm_trace_head[core_pos] = ticket + 1U;
	clean_dcache_range((uintptr_t)&qti_rimps_lpm_trace_head[core_pos],
			   sizeof(qti_rimps_lpm_trace_head[core_pos]));
}

static void qti_gold_rimps_baseline_capture_once(void)
{
	spin_lock(&qti_gold_rimps_baseline_lock);
	if ((!qti_gold_rimps_baseline_captured) &&
	    (qti_rimps_lpm_trace_magic == QTI_RIMPS_TRACE_MAGIC_VALUE)) {
		qti_rimps_lpm_trace_write_record(
			&qti_rimps_lpm_trace_gold_baseline, 0U,
			QTI_RIMPS_TRACE_GOLD_POST_ENABLE,
			QTI_RIMPS_TRACE_NO_CPU_STATE, true);
		qti_gold_rimps_baseline_captured = true;
	}
	spin_unlock(&qti_gold_rimps_baseline_lock);
}

struct qti_pcu_lpm_snapshot {
	uint32_t seq_ctl;
	uint32_t seq_sts3;
	uint32_t cpu_lpm_status1;
	uint32_t cpu_lpm_status2;
	uint32_t clkdom_lpm_status;
	uint32_t hang_det;
};

static uintptr_t qti_pcu_lpm_seq_ctl_address(
	enum qtipmlib_lpm_node_id node_id)
{
	static const uintptr_t ipm_base[QTIPMLIB_LPM_NR_NODES] = {
		0x0f800000U, /* APSS alias 0 / CPU0 */
		0x0f810000U, /* APSS alias 1 / CPU1 */
		0x0f820000U, /* APSS alias 2 / CPU2 */
		0x0f830000U, /* APSS alias 6 / CPU3 (Gold) */
		0x0f880000U, /* APSS cluster IPM / L3 */
	};

	assert(node_id < QTIPMLIB_LPM_NR_NODES);
	return ipm_base[node_id] + QTI_PCU_SEQ_CTL;
}

static void qti_pcu_lpm_snapshot(enum qtipmlib_lpm_node_id node_id,
				 struct qti_pcu_lpm_snapshot *snapshot)
{
	uintptr_t clkdom_lpm_status;

	if (node_id <= QTIPMLIB_LPM_CPU2) {
		clkdom_lpm_status = QTI_PCU_CLKDOM1_LPM_STATUS;
	} else if (node_id == QTIPMLIB_LPM_CPU3) {
		clkdom_lpm_status = QTI_PCU_CLKDOM2_LPM_STATUS;
	} else {
		clkdom_lpm_status = QTI_PCU_CLKDOM0_LPM_STATUS;
	}

	snapshot->seq_ctl = mmio_read_32(
		qti_pcu_lpm_seq_ctl_address(node_id));
	/* SEQ_STS3 is at IPM +0x40; SEQ_CTL is at IPM +0x24. */
	snapshot->seq_sts3 = mmio_read_32(
		qti_pcu_lpm_seq_ctl_address(node_id) +
		QTI_PCU_SEQ_STS3_DELTA);
	snapshot->cpu_lpm_status1 = mmio_read_32(QTI_PCU_CPU_LPM_STATUS1);
	snapshot->cpu_lpm_status2 = mmio_read_32(QTI_PCU_CPU_LPM_STATUS2);
	snapshot->clkdom_lpm_status = mmio_read_32(clkdom_lpm_status);
	snapshot->hang_det = mmio_read_32(QTI_PCU_MUC_HANG_DET_STS);
}

static void qti_pcu_lpm_log_event(const char *event,
				  enum qtipmlib_lpm_node_id node_id,
				  int mode_id, int rc, uint32_t head_before,
				  const struct qti_pcu_lpm_snapshot *before,
				  const struct qti_pcu_lpm_snapshot *after)
{
	VERBOSE("LPMTRACE %s core=%u node=%u mode=%d rc=%d head=%08x->%08x "
	     "SEQ_CTL=%08x->%08x SEQ_STS3=%08x->%08x CPU_LPM_STATUS1=%08x->%08x "
	     "CPU_LPM_STATUS2=%08x->%08x CLKDOM_LPM_STATUS=%08x->%08x "
	     "MUC_HANG_DET_STS=%08x->%08x\n",
	     event, plat_my_core_pos(), (unsigned int)node_id, mode_id, rc,
	     head_before, qtipmlib_lpm_trace_head,
	     before->seq_ctl, after->seq_ctl,
	     before->seq_sts3, after->seq_sts3,
	     before->cpu_lpm_status1, after->cpu_lpm_status1,
	     before->cpu_lpm_status2, after->cpu_lpm_status2,
	     before->clkdom_lpm_status, after->clkdom_lpm_status,
	     before->hang_det, after->hang_det);
}
#endif /* QTI_PMLIB_ENABLED */

/* cpu power control happens to be same across all CPUs */
DEFINE_RENAME_SYSREG_RW_FUNCS(cpu_pwrctrl_val, S3_0_C15_C2_7)

static int qti_pcu_cpu_mode(plat_local_state_t state)
{
	if (state == QTI_LOCAL_STATE_RET) {
		return QTI_PCU_MODE_RET;
	}
	if (state == QTI_LOCAL_STATE_OFF) {
		return QTI_PCU_CPU_MODE_OFF;
	}
	if (state == QTI_LOCAL_STATE_DEEPOFF) {
		return QTI_PCU_MODE_DEEPOFF;
	}

	return -1;
}

static int qti_pcu_l3_mode(plat_local_state_t state)
{
	if (state == QTI_LOCAL_STATE_RET) {
		return QTI_PCU_MODE_RET;
	}
	if ((state == QTI_LOCAL_STATE_OFF) ||
	    (state == QTI_LOCAL_STATE_DEEPOFF)) {
		return QTI_PCU_MODE_DEEPOFF;
	}

	return -1;
}

static void qti_pcu_config_lpm_node(enum qtipmlib_lpm_node_id node_id,
				    int mode_id)
{
	int rc;

#ifdef QTI_PMLIB_ENABLED
	struct qti_pcu_lpm_snapshot before;
	struct qti_pcu_lpm_snapshot after;
	uint32_t head_before;

	qti_pcu_lpm_snapshot(node_id, &before);
	head_before = qtipmlib_lpm_trace_head;
#endif
	rc = qtipmlib_config_lpm(node_id, mode_id);
#ifdef QTI_PMLIB_ENABLED
	qti_pcu_lpm_snapshot(node_id, &after);
	qti_pcu_lpm_log_event("config", node_id, mode_id, rc, head_before,
			      &before, &after);
#endif

	if (rc != QTIPMLIB_SUCCESS) {
		ERROR("qtipmlib_config_lpm(node=%d, mode=%d) failed: %d\n",
		      node_id, mode_id, rc);
		panic();
	}
}

static void qti_pcu_unconfig_lpm_node(enum qtipmlib_lpm_node_id node_id,
				      int mode_id)
{
	int rc;

#ifdef QTI_PMLIB_ENABLED
	struct qti_pcu_lpm_snapshot before;
	struct qti_pcu_lpm_snapshot after;
	uint32_t head_before;

	qti_pcu_lpm_snapshot(node_id, &before);
	head_before = qtipmlib_lpm_trace_head;
#endif
	rc = qtipmlib_unconfig_lpm(node_id, mode_id);
#ifdef QTI_PMLIB_ENABLED
	qti_pcu_lpm_snapshot(node_id, &after);
	qti_pcu_lpm_log_event("unconfig", node_id, mode_id, rc, head_before,
			      &before, &after);
#endif

	if (rc != QTIPMLIB_SUCCESS) {
		ERROR("qtipmlib_unconfig_lpm(node=%d, mode=%d) failed: %d\n",
		      node_id, mode_id, rc);
		panic();
	}
}

static void qti_pcu_config_lpm(const psci_power_state_t *target_state)
{
	unsigned int core_pos = plat_my_core_pos();
	plat_local_state_t cpu_state =
		target_state->pwr_domain_state[QTI_PWR_LVL0];
	bool c4_log = false;
	int mode_id;

	if (core_pos < QTIPMLIB_LPM_L3) {
		mode_id = qti_pcu_cpu_mode(cpu_state);
		if ((cpu_state == QTI_LOCAL_STATE_DEEPOFF) &&
		    (core_pos < PLATFORM_CORE_COUNT) &&
		    (qti_c4_pcu_log_count[core_pos] < QTI_C4_TRACE_LOG_LIMIT)) {
			qti_c4_pcu_log_count[core_pos]++;
			c4_log = true;
			VERBOSE("QTI: C4 PCU path core=%u state0=%u state1=%u "
			     "selected_mode=%d cpu_node=%u will_configure=%u\n",
			     core_pos, (unsigned int)cpu_state,
			     (unsigned int)target_state->pwr_domain_state[QTI_PWR_LVL1],
			     mode_id, core_pos, mode_id >= 0 ? 1U : 0U);
		}
		if (mode_id >= 0) {
			qti_pcu_config_lpm_node(
				(enum qtipmlib_lpm_node_id)core_pos, mode_id);
			if (c4_log) {
				VERBOSE("QTI: C4 PCU config returned core=%u node=%u "
				     "mode=%d\n", core_pos, core_pos, mode_id);
			}
		}
	}

	mode_id = qti_pcu_l3_mode(
		target_state->pwr_domain_state[QTI_PWR_LVL1]);
	if (mode_id >= 0) {
		qti_pcu_config_lpm_node(QTIPMLIB_LPM_L3, mode_id);
	}
}

/*******************************************************************************
 * QTI standard platform handler called to check the validity of the power
 * state parameter. The power state parameter has to be a composite power
 * state.
 ******************************************************************************/
int qti_validate_power_state(unsigned int power_state,
			     psci_power_state_t *req_state)
{
	unsigned int state_id;
	int i;
	bool c4_candidate = qti_is_c4_candidate(power_state);
	const unsigned int *idle_states = plat_qti_pm_idle_states();

	assert(req_state);

	//unsigned int core_pos1 = plat_my_core_pos();
	//if(!break_before_lpms)NOTICE("[Linux][%u] Inside qti_validate_power_state, power_state: %u", core_pos1, power_state);
	/*
	 *  Currently we are using a linear search for finding the matching
	 *  entry in the idle power state array. This can be made a binary
	 *  search if the number of entries justify the additional complexity.
	 */
	for (i = 0; !!idle_states[i]; i++) {
#if PSCI_OS_INIT_MODE
		if ((power_state & ~QTI_LAST_AT_PLVL_MASK) ==
		    idle_states[i])
#else
		if (power_state == idle_states[i])
#endif
			break;
	}

	/* Return error if entry not found in the idle state array */
	if (!idle_states[i]) {
		if (c4_candidate &&
		    (plat_my_core_pos() < PLATFORM_CORE_COUNT) &&
		    (qti_c4_validate_log_count[plat_my_core_pos()] <
		     QTI_C4_TRACE_LOG_LIMIT)) {
			unsigned int core_pos = plat_my_core_pos();

			qti_c4_validate_log_count[core_pos]++;
			VERBOSE("QTI: C4 power-state validate core=%u pstate=%08x "
			     "id=%07x matched=0 rc=%d\n", core_pos, power_state,
			     psci_get_pstate_id(power_state),
			     PSCI_E_INVALID_PARAMS);
		}
		return PSCI_E_INVALID_PARAMS;
	}

	if (c4_candidate &&
	    (plat_my_core_pos() < PLATFORM_CORE_COUNT) &&
	    (qti_c4_validate_log_count[plat_my_core_pos()] <
	     QTI_C4_TRACE_LOG_LIMIT)) {
		unsigned int core_pos = plat_my_core_pos();

		qti_c4_validate_log_count[core_pos]++;
		VERBOSE("QTI: C4 power-state validate core=%u pstate=%08x "
		     "id=%07x matched=1 index=%d rc=%d\n", core_pos,
		     power_state, psci_get_pstate_id(power_state), i,
		     PSCI_E_SUCCESS);
	}

	i = 0;
	state_id = psci_get_pstate_id(power_state);

	/* Parse the State ID and populate the state info parameter */
	for (i = QTI_PWR_LVL0; i <= PLAT_MAX_PWR_LVL; i++) {
		req_state->pwr_domain_state[i] = state_id &
		    QTI_LOCAL_PSTATE_MASK;
		state_id >>= QTI_LOCAL_PSTATE_WIDTH;
	}
#if PSCI_OS_INIT_MODE
	req_state->last_at_pwrlvl = state_id & QTI_LOCAL_PSTATE_MASK;
#endif

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * PLATFORM FUNCTIONS
 ******************************************************************************/

static void qti_set_cpupwrctlr_val(void)
{
	unsigned long val;

	val = read_cpu_pwrctrl_val();
	val |= QTI_CORE_PWRDN_EN_MASK;
	write_cpu_pwrctrl_val(val);

	isb();
}

static void qti_clear_cpupwrctlr_val(void)
{
	unsigned long val;

	val = read_cpu_pwrctrl_val();
	val &= ~QTI_CORE_PWRDN_EN_MASK;
	write_cpu_pwrctrl_val(val);

	isb();
}

/**
 * CPU power on function - ideally we want a wrapper since this function is
 * target specific. But to unblock teams.
 */
static int qti_cpu_power_on(u_register_t mpidr)
{
	int core_pos = plat_core_pos_by_mpidr(mpidr);

	/* If not valid mpidr, return error */
	if (core_pos < 0 || core_pos >= PLATFORM_CORE_COUNT) {
		return PSCI_E_INVALID_PARAMS;
	}

	/*
	 * TZ's Clock_CPUColdBoot() establishes Gold's safe 652.8 MHz PLL
	 * frequency and enables its OSM before the Gold CPU proceeds with its
	 * cold boot. Do the equivalent while handling CPU_ON, before releasing
	 * the target core from reset. In particular, do not retune Gold's PLL
	 * from qti_cpu_power_on_finish(): that callback is already executing on
	 * the newly powered CPU, so a Gold-only CPU_ON would change the clock of
	 * the code performing the transition itself.
	 */
	if ((core_pos == 3) &&
	    !chipinfo_is_part_disabled(CHIPINFO_PART_CPU, 6U)) {
		int rc;

		INFO("Shikra: prepare Gold RIMPS before CPU_ON (caller core=%u)\n",
		     plat_my_core_pos());
		rc = qtipmlib_enable_gold_rimps();
		if (rc != QTIPMLIB_SUCCESS) {
			ERROR("Gold RIMPS/PLL preparation failed before CPU_ON: %d\n",
			      rc);
			return PSCI_E_INTERN_FAIL;
		}
#ifdef QTI_PMLIB_ENABLED
		/*
		 * The boot-time snapshot sees Gold disabled. Record its first
		 * post-enable state in SRAM, before releasing the Gold CPU, without
		 * adding another shared-UART print to the CPU_ON path.
		 */
		qti_gold_rimps_baseline_capture_once();
#endif
	}

	return plat_qti_pwr_domain_on(mpidr, core_pos);
}

static bool is_cpu_off(const psci_power_state_t *target_state)
{
	if ((target_state->pwr_domain_state[QTI_PWR_LVL0] ==
	     QTI_LOCAL_STATE_OFF) ||
	    (target_state->pwr_domain_state[QTI_PWR_LVL0] ==
	     QTI_LOCAL_STATE_DEEPOFF)) {
		return true;
	} else {
		return false;
	}
}

/*
 * Undo the per-node PCU programming after a CPU_SUSPEND resume or a CPU_ON
 * following CPU_OFF. The downstream TZ CPU-on notifier performs this same
 * unconfig operation for the state from which the core is being brought up.
 */
static void qti_pcu_unconfig_lpm(const psci_power_state_t *target_state)
{
	unsigned int core_pos = plat_my_core_pos();
	int mode_id;

	if (core_pos < QTIPMLIB_LPM_L3) {
		mode_id = qti_pcu_cpu_mode(
			target_state->pwr_domain_state[QTI_PWR_LVL0]);
		if (mode_id >= 0) {
			qti_pcu_unconfig_lpm_node(
				(enum qtipmlib_lpm_node_id)core_pos, mode_id);
		}
	}

	mode_id = qti_pcu_l3_mode(
		target_state->pwr_domain_state[QTI_PWR_LVL1]);
	if (mode_id >= 0) {
		qti_pcu_unconfig_lpm_node(QTIPMLIB_LPM_L3, mode_id);
	}
}

static void qti_cpu_power_on_finish(const psci_power_state_t *target_state)
{
	const uint8_t *pwr_states =
	    (const uint8_t *)target_state->pwr_domain_state;
	unsigned int core_pos = plat_my_core_pos();
	bool warm_on = false;

	plat_qti_pwr_domain_on_finish(core_pos, pwr_states);
	if (is_cpu_off(target_state)) {
		/* CPU_OFF may have removed local GICR state; restore TZ's saved mask. */
		warm_on = plat_qti_gic_pcpu_restore();
		qti_watchdog_set_target(QTI_WATCHDOG_CPU_WAKEUP);
		if (warm_on) {
			qti_clear_cpupwrctlr_val();
			qti_pcu_unconfig_lpm(target_state);
		}
		plat_qti_gic_cpuif_enable();
	}
}

static void qti_cpu_standby(plat_local_state_t cpu_state)
{
	u_register_t scr;

	assert(cpu_state == QTI_LOCAL_STATE_STB);

	/*
	 * Enter standby retention with a WFI, routing non-secure interrupts to
	 * EL3 to wake the CPU, then restore SCR_EL3 so they are handled in the
	 * non-secure world. Route both IRQ and FIQ: GICv3 delivers NS Group 1
	 * interrupts as FIQ, GICv2 as IRQ.
	 */
	scr = read_scr_el3();
	write_scr_el3(scr | SCR_IRQ_BIT | SCR_FIQ_BIT);
	isb();

	dsb();
	wfi();

	write_scr_el3(scr);
	isb();
}

static void qti_node_power_off(const psci_power_state_t *target_state)
{
	/* TZ arms the P-channel wake mask before preparing a hot-unplug. */
	plat_qti_pwr_domain_off((const uint8_t *)target_state->pwr_domain_state);

	if (is_cpu_off(target_state)) {
		qti_watchdog_set_target(QTI_WATCHDOG_CPU_HOTPLUG);
		/* Preserve the OS's local interrupt enables across CPU_OFF. */
		plat_qti_gic_pcpu_save();
		plat_qti_gic_cpuif_disable();
	}

	qti_pcu_config_lpm(target_state);
	if (is_cpu_off(target_state)) {
		qti_set_cpupwrctlr_val();
	}
}

static void qti_node_suspend(const psci_power_state_t *target_state)
{
	while(break_before_lpms);
	bool cpu_off = is_cpu_off(target_state);
	uint32_t cpu_state = target_state->pwr_domain_state[QTI_PWR_LVL0];

	if (cpu_off) {
		/* Do not route the secure watchdog bark to a collapsing CPU. */
		qti_watchdog_set_target(QTI_WATCHDOG_CPU_SUSPEND);
	}

#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_SUSPEND_ENTRY, cpu_state, false);
	}
#endif
	qti_pcu_config_lpm(target_state);
#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_PCU_CONFIG_DONE, cpu_state, true);
	}
#endif
	/* Match TZ: arm the PCU first, then disable all CPU-interface groups. */
	if (cpu_off) {
		plat_qti_gic_pcpu_suspend();
	}
#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_GIC_GROUPS_DISABLED,
			cpu_state, false);
	}
#endif
	plat_qti_pwr_domain_suspend((const uint8_t *)target_state->pwr_domain_state);
	if (cpu_off) {
		qti_set_cpupwrctlr_val();
		/* PSCI executes WFI after this callback; that is the PCU trigger. */
		NOTICE("QTI LPM: PCU armed; returning to PSCI/WFI core=%u "
		       "cpu_state=%u cluster_state=%u\n",
		       plat_my_core_pos(), cpu_state,
		       (unsigned int)target_state->pwr_domain_state[QTI_PWR_LVL1]);
	}
}

static void qti_node_suspend_finish(const psci_power_state_t *target_state)
{
	const uint8_t *pwr_states =
	    (const uint8_t *)target_state->pwr_domain_state;
	bool cpu_off = is_cpu_off(target_state);
	uint32_t cpu_state = target_state->pwr_domain_state[QTI_PWR_LVL0];

	unsigned int core_pos = plat_my_core_pos();

	if (cpu_off) {
		/* Reached only after the CPU has returned from the suspend WFI. */
		NOTICE("QTI LPM: returned from PSCI/WFI core=%u cpu_state=%u "
		       "cluster_state=%u\n",
		       core_pos, cpu_state,
		       (unsigned int)target_state->pwr_domain_state[QTI_PWR_LVL1]);
	}

#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_RESUME_ENTRY, cpu_state, false);
	}
#endif

	if (cpu_off) {
		/* Make this core eligible for watchdog routing again after wake. */
		qti_watchdog_set_target(QTI_WATCHDOG_CPU_WAKEUP);
		NOTICE("[%u] Before qti_clear_cpupwrctlr_val\n", core_pos);
		/*
		 * CPU_PWRDN_EN is armed before WFI on entry. Clear it immediately
		 * after resume so later WFI calls cannot collapse the CPU unless
		 * PSCI explicitly requests a power-down state again.
		 */
		qti_clear_cpupwrctlr_val();
		NOTICE("[%u] After qti_clear_cpupwrctlr_val\n", core_pos);
	}

	/* Disarm the PCU nodes configured on entry, in CPU-then-cluster order. */
	if (cpu_off) {
		NOTICE("[%u] Before qti_pcu_unconfig_lpm\n", core_pos);
		qti_pcu_unconfig_lpm(target_state);
		NOTICE("[%u] After qti_pcu_unconfig_lpm\n", core_pos);
	}
#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_PCU_UNCONFIG_DONE,
			cpu_state, true);
	}
#endif
	NOTICE("[%u] Before plat_qti_pwr_domain_suspend_finish\n", core_pos);

	plat_qti_pwr_domain_suspend_finish(pwr_states);
	NOTICE("[%u] After plat_qti_pwr_domain_suspend_finish\n", core_pos);
	if (cpu_off) {
		NOTICE("[%u] Before plat_qti_gic_pcpu_suspend_finish\n", core_pos);
		plat_qti_gic_pcpu_suspend_finish();
		NOTICE("[%u] After plat_qti_gic_pcpu_suspend_finish\n", core_pos);
	}
#ifdef QTI_PMLIB_ENABLED
	if (cpu_off) {
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_GIC_GROUPS_ENABLED,
			cpu_state, false);
		/* One full, read-only RIMPS sample per completed C3/C4 resume. */
		qti_rimps_lpm_trace_capture(
			QTI_RIMPS_TRACE_CPU_LPM_FINISH_DONE,
			cpu_state, true);
	}
#endif
}

static __dead2 void assert_ps_hold(void)
{
#ifdef QTI_PS_HOLD_REG
	mmio_write_32(QTI_PS_HOLD_REG, 0);
	mdelay(1000);
#endif

	/* Should be dead before reaching this. */
	panic();
}

__dead2 void qti_system_off(void)
{
	qti_pmic_prepare_shutdown();
	assert_ps_hold();
}

__dead2 void qti_system_reset(void)
{
	qti_pmic_prepare_reset();
	assert_ps_hold();
}

void qti_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;
	unsigned int state_id, power_state;
	const unsigned int *idle_states = plat_qti_pm_idle_states();

	for (i = 0; i <= PLAT_MAX_PWR_LVL; i++) {
		req_state->pwr_domain_state[i] = PSCI_LOCAL_STATE_RUN;
	}

	/*
	 * Find deepest state.
	 * The idle-states array is 0-terminated, so the deepest state is the
	 * last non-zero element, i.e. the one immediately before the terminator.
	 */
	i = 0;
	while (idle_states[i] != 0U) {
		i++;
	}

	/* Need at least one valid state before the terminator. */
	assert(i > 0);
	power_state = idle_states[i - 1];
	state_id = psci_get_pstate_id(power_state);

	i = 0;

	/* Parse the State ID and populate the state info parameter */
	while (state_id) {
		req_state->pwr_domain_state[i++] =
		    state_id & QTI_LOCAL_PSTATE_MASK;
		state_id >>= QTI_LOCAL_PSTATE_WIDTH;
	}

#if PSCI_OS_INIT_MODE
	req_state->last_at_pwrlvl = PLAT_MAX_PWR_LVL;
#endif
}

/*
 * Structure containing platform specific PSCI operations. Common
 * PSCI layer will use this.
 */
const plat_psci_ops_t plat_qti_psci_pm_ops = {
	.pwr_domain_on = qti_cpu_power_on,
	.pwr_domain_on_finish = qti_cpu_power_on_finish,
	.cpu_standby = qti_cpu_standby,
	.pwr_domain_off = qti_node_power_off,
	.pwr_domain_suspend = qti_node_suspend,
	.pwr_domain_suspend_finish = qti_node_suspend_finish,
	.system_off = qti_system_off,
	.system_reset = qti_system_reset,
	.get_node_hw_state = NULL,
	.translate_power_state_by_mpidr = NULL,
	.get_sys_suspend_power_state = qti_get_sys_suspend_power_state,
	.validate_power_state = qti_validate_power_state,
};

/**
 * The QTI Standard platform definition of platform porting API
 * `plat_setup_psci_ops`.
 */
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	int err;

	qti_sec_core_remap((uintptr_t)bl31_warm_entrypoint);

	err = plat_qti_pwr_psci_init((uintptr_t)bl31_warm_entrypoint);
	if (err == PSCI_E_SUCCESS) {
		*psci_ops = &plat_qti_psci_pm_ops;
	}

	return err;
}
