/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <common/build_message.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/qti/chipinfo/chipinfo.h>
#include <drivers/qti/cprh/cprh.h>
#include <drivers/qti/qtipmlib/qtipmlib.h>
#include <drivers/qti/vpp/vpp.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#define QTI_RIMPSSLOW_CLKDOM0_BASE	0x0fd90000UL
#define QTI_RIMPSSLOW_CLKDOM1_BASE	0x0fd91000UL
#define QTI_RIMPSSLOW_CLKDOM2_BASE	0x0fd92000UL
#define QTI_RIMPSTOP_SEC_ACCESS_OVERRIDE	0x0fd98000UL
#define QTI_RIMPSTOP_GLOBAL_ENABLE	0x0fd98004UL
#define QTI_RIMPSTOP_L3_VOTING_CTRL	0x0fd98024UL
#define QTI_RIMPSFAST_MUC_CLK_CTRL	0x0fd80000UL
#define QTI_RIMPSFAST_SEQ_MEM_0		0x0fd80200UL
#define QTI_RIMPSFAST_CPU_LPM_IRQ_EN	0x0fd800ecUL
#define QTI_RIMPSFAST_CPU_LPM_STATUS1	0x0fd800f0UL
#define QTI_RIMPSFAST_CPU_LPM_STATUS2	0x0fd800f4UL
#define QTI_RIMPSFAST_CLKDOM_LPM_GNT_MASK	0x0fd800f8UL
#define QTI_RIMPSFAST_CPU_LPM_ARB_CTL	0x0fd800fcUL
#define QTI_RIMPSFAST_CL_LPM_IRQ_EN	0x0fd8010cUL
#define QTI_RIMPSFAST_MUC_STATUS	0x0fd8017cUL
#define QTI_RIMPSTOP_MUC_HANG_DET_STS	0x0fd98020UL
#define QTI_EPSS_SHARED_DATA_BASE	0x0fd04000UL
#define QTI_RIMPS_MUC_MEMORY_WORD0	0x0fd00000UL
#define QTI_APSS_SEQ_COMMON_INSTR_0	0x0f8c0000UL
#define QTI_GOLD_PLL_MODE		0x0fa84000UL
#define QTI_EPSS_NR_DOMAINS		3U
#define QTI_EPSS_ACD_LEVELS		7U
#define QTI_EPSS_BCL_LEVELS		4U
#define QTI_EPSS_MAX_VC			0x3fU
#define QTI_EPSS_NO_APM_CROSSOVER	0xffU
#define QTI_EPSS_MAX_VOLTAGE		0xffffU
#define QTI_EPSS_SILVER_AVS_DOWN_DLY	59U

/* Shikra TZ ClockCPU.h: SECURITY_CONTROL_BASE + QFPROM PTE row 1 MSB. */
#define QTI_QFPROM_CORR_PTE_ROW1_MSB	0x01b4418cUL
#define QTI_QFPROM_CORR_PTE_ROW3_LSB	0x01b44198UL
#define QTI_QFPROM_SPEED_BIN_MASK	0x000000fcU
#define QTI_QFPROM_SPEED_BIN_SHIFT	2U
#define QTI_QFPROM_IDDQ_APC_ACTIVE_MASK	0x0007fe00U
#define QTI_QFPROM_IDDQ_APC_ACTIVE_SHIFT	9U
#define QTI_IPEAK_SLOPE			60U
#define QTI_IDDQ_TT_MA			52340U
#define QTI_IPEAK_THRESHOLD_0		1125000U
#define QTI_IPEAK_THRESHOLD_1		1151000U
#define QTI_IPEAK_THRESHOLD_2		1173000U

#define QTI_APSS_SHARED_BASE		0x0f600000UL
#define QTI_APSS_APM_BASE		0x0fb00000UL
#define QTI_APSS_APM_CTL_0		(QTI_APSS_APM_BASE + 0x004U)
#define QTI_APSS_APM_DLY_CNTR_0		(QTI_APSS_APM_BASE + 0x008U)
#define QTI_APSS_APM_CTL_1		(QTI_APSS_APM_BASE + 0x010U)
#define QTI_APSS_APM_DLY_CNTR_1		(QTI_APSS_APM_BASE + 0x014U)
#define QTI_APSS_MAS_DLY0_0		(QTI_APSS_APM_BASE + 0x01cU)
#define QTI_APSS_MAS_DLY1_0		(QTI_APSS_APM_BASE + 0x020U)
#define QTI_APSS_MAS_OVERRIDE(n)	(QTI_APSS_APM_BASE + 0x038U + (0x10U * (n)))
#define QTI_APSS_APM_CTL_OVERRIDE	(QTI_APSS_APM_BASE + 0x0f4U)

#define QTI_APSS_MEM_ACC_GOLD_SVS_0	(QTI_APSS_SHARED_BASE + 0x168U)
#define QTI_APSS_MEM_ACC_GOLD_SVS_1	(QTI_APSS_SHARED_BASE + 0x16cU)
#define QTI_APSS_MEM_ACC_GOLD_NOM_0	(QTI_APSS_SHARED_BASE + 0x250U)
#define QTI_APSS_MEM_ACC_GOLD_NOM_1	(QTI_APSS_SHARED_BASE + 0x254U)
#define QTI_APSS_MEM_ACC_GOLD_TURBO_0	(QTI_APSS_SHARED_BASE + 0x26cU)
#define QTI_APSS_MEM_ACC_GOLD_TURBO_1	(QTI_APSS_SHARED_BASE + 0x270U)
#define QTI_APSS_MEM_ACC_SILVER_SVS_0	(QTI_APSS_SHARED_BASE + 0x160U)
#define QTI_APSS_MEM_ACC_SILVER_SVS_1	(QTI_APSS_SHARED_BASE + 0x164U)
#define QTI_APSS_MEM_ACC_SILVER_NOM_0	(QTI_APSS_SHARED_BASE + 0x248U)
#define QTI_APSS_MEM_ACC_SILVER_NOM_1	(QTI_APSS_SHARED_BASE + 0x24cU)
#define QTI_APSS_MEM_ACC_SILVER_TURBO_0	(QTI_APSS_SHARED_BASE + 0x264U)
#define QTI_APSS_MEM_ACC_SILVER_TURBO_1	(QTI_APSS_SHARED_BASE + 0x268U)
#define QTI_APSS_MEM_ACC_L3_SVS_0	(QTI_APSS_SHARED_BASE + 0x240U)
#define QTI_APSS_MEM_ACC_L3_SVS_1	(QTI_APSS_SHARED_BASE + 0x244U)
#define QTI_APSS_MEM_ACC_L3_NOM_0	(QTI_APSS_SHARED_BASE + 0x25cU)
#define QTI_APSS_MEM_ACC_L3_NOM_1	(QTI_APSS_SHARED_BASE + 0x260U)
#define QTI_APSS_MEM_ACC_L3_TURBO_0	(QTI_APSS_SHARED_BASE + 0x278U)
#define QTI_APSS_MEM_ACC_L3_TURBO_1	(QTI_APSS_SHARED_BASE + 0x27cU)

#define QTI_APM_CTL_USE_CLK_GATE_ACK	(1U << 16U)
#define QTI_APM_CTL_CONT_LOAD_SHIFTING_EN	(1U << 7U)
#define QTI_APM_CTL_APC0_AUTO_MODE_EN	(1U << 5U)
#define QTI_APM_CTL_APC1_AUTO_MODE_EN	(1U << 6U)
#define QTI_APM_CTL_EN_CSR_MODE		(1U << 3U)
#define QTI_APM_CTL_FORCE_CLK_DIS		(1U << 2U)
#define QTI_APM_CTL_MODE_SW_HW_SEL	(1U << 0U)
#define QTI_APM_CTL0_AUTO_MASK		(QTI_APM_CTL_USE_CLK_GATE_ACK | \
					 QTI_APM_CTL_CONT_LOAD_SHIFTING_EN | \
					 QTI_APM_CTL_APC0_AUTO_MODE_EN | \
					 QTI_APM_CTL_EN_CSR_MODE | \
					 QTI_APM_CTL_FORCE_CLK_DIS | \
					 QTI_APM_CTL_MODE_SW_HW_SEL)
#define QTI_APM_CTL1_AUTO_MASK		(QTI_APM_CTL_USE_CLK_GATE_ACK | \
					 QTI_APM_CTL_CONT_LOAD_SHIFTING_EN | \
					 QTI_APM_CTL_APC1_AUTO_MODE_EN | \
					 QTI_APM_CTL_EN_CSR_MODE | \
					 QTI_APM_CTL_FORCE_CLK_DIS | \
					 QTI_APM_CTL_MODE_SW_HW_SEL)
#define QTI_APM_SER_FREEZE_DIS_MASK	0x00000006U
#define QTI_APM_SER_FREEZE_DIS_VALUE	0x00000006U
#define QTI_APM_DELAY_VALUE		0x0b0c0d0eU
#define QTI_APM_MAS_DELAY_MASK		0x003f3f3fU
#define QTI_APM_MAS_DELAY_VALUE		0x000f0f0fU
#define QTI_APM_MAS_OVERRIDE_VALUE	0x01000000U
#define QTI_APM_READBACK_TIMEOUT_US	100U

#define QTI_SILVER_PLL_BASE		0x0fa80000UL
#define QTI_GOLD_PLL_BASE		0x0fa84000UL
#define QTI_L3_PLL_BASE			0x0fa88000UL
#define QTI_PLL_L_VAL			0x004U
#define QTI_PLL_USER_CTL		0x00cU
#define QTI_PLL_CFG_GFMUX		0x084U
static int qti_rimps_program_apm(void);
static void qti_rimps_program_memacc(void);
static void qti_rimps_apm_update_bits(uintptr_t address, uint32_t mask,
				      uint32_t value);
static int qti_rimps_check_apm_reg(const char *name, uintptr_t address,
				   uint32_t mask, uint32_t expected);
void qtipmlib_report_readback(const char *name, uint32_t actual,
			      uint32_t expected);
#define QTI_PLL_POSTDIV_MASK		0x00000300U
#define QTI_PLL_POSTDIV_SHIFT		8U
#define QTI_PLL_GFMUX_SRC_MASK		0x0000000fU
#define QTI_CLOCK_SRC_XO		0U
#define QTI_CLOCK_SRC_GPLL0		1U
#define QTI_CLOCK_SRC_CPUPLL_EARLY	2U
#define QTI_CLOCK_XO_HZ			19200000ULL
#define QTI_CLOCK_GPLL0_HZ		300000000ULL

#define QTI_CLKDOM_LUT_FREQ(n)		(0x100U + (4U * (n)))
#define QTI_CLKDOM_LUT_VOLT(n)		(0x200U + (4U * (n)))
#define QTI_CLKDOM_RIMPS_ENABLE		0x000U
#define QTI_CLKDOM_MAX_CORE_CNT		0x014U
#define QTI_CLKDOM_NON_BOOST_VC		0x018U
#define QTI_CLKDOM_FVA_STATUS		0x01cU
#define QTI_CLKDOM_DOMAIN_STATE		0x020U
#define QTI_CLKDOM_LLM_DCVS_VC_VOTE	0x024U
#define QTI_CLKDOM_LLM_DCVS_VOL_VOTE	0x028U
#define QTI_CLKDOM_FVA_INT_EN		0x058U
#define QTI_CLKDOM_CORE_DCVS_CTRL	0x0b0U
#define QTI_CLKDOM_PLL_FSM_CTRL		0x0d8U
#define QTI_CLKDOM_RIMPS_INTR_EN		0x304U
#define QTI_CLKDOM_LPM_STATUS		0x314U
#define QTI_CLKDOM_CORE_PERF_STATE_DESIRED_0	0x320U
#define QTI_CLKDOM_SEQ_CTRL1		0x348U
#define QTI_CLKDOM_SEQ_CTRL		0x34cU
#define QTI_CLKDOM_SEQ_BR_CTRL(n)		(0x350U + (4U * (n)))
#define QTI_OSM_LUT_ROWS		12U

#define QTI_OSM_LUT_FREQ_CLKSRC_SHIFT	30U
#define QTI_OSM_LUT_FREQ_CC_SHIFT	16U
#define QTI_OSM_LUT_FREQ_LVAL_SHIFT	0U
#define QTI_OSM_LUT_FREQ_CLKSRC_MASK	0x40000000U
#define QTI_OSM_LUT_FREQ_CC_MASK	0x000f0000U
#define QTI_OSM_LUT_FREQ_LVAL_MASK	0x000000ffU
#define QTI_OSM_LUT_VOLT_VC_SHIFT	16U
#define QTI_OSM_LUT_VOLT_OLV_SHIFT	0U
#define QTI_OSM_LUT_VOLT_VC_MASK	0x003f0000U
#define QTI_OSM_LUT_VOLT_OLV_MASK	0x00000fffU

typedef struct {
	uintptr_t base;
	uintptr_t pll_base;
	const char *rail_name;
	const char *clk_name;
} qti_rimps_clkdom_t;

static const qti_rimps_clkdom_t qti_rimps_clkdoms[] = {
	{ QTI_RIMPSSLOW_CLKDOM0_BASE, QTI_L3_PLL_BASE, "APC", "L3" },
	{ QTI_RIMPSSLOW_CLKDOM1_BASE, QTI_SILVER_PLL_BASE, "APC", "Silver" },
	{ QTI_RIMPSSLOW_CLKDOM2_BASE, QTI_GOLD_PLL_BASE, "APC", "Gold" },
};

/* Exact Shikra ClockCPU.h epssSharedDataType layout (3 domains, 7 ACD rows,
 * 4 BCL levels, one APC rail). Preserve the fields TZ does not update here.
 */
typedef struct {
	uint32_t version_id;
	uint8_t apm_crossover_corner[QTI_EPSS_NR_DOMAINS];
	uint8_t reserved1;
	uint8_t acd_dvm_vc[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint32_t acd_dvm_val[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint8_t acd_avg_vc[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint32_t acd_avg_cfg[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint8_t acd_acdsscr_vc[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint32_t acd_acdsscr_val[QTI_EPSS_NR_DOMAINS][QTI_EPSS_ACD_LEVELS];
	uint8_t bcl_lval[QTI_EPSS_NR_DOMAINS][QTI_EPSS_BCL_LEVELS];
	uint8_t park_pll_lval[QTI_EPSS_NR_DOMAINS];
	uint8_t park_vc[QTI_EPSS_NR_DOMAINS];
	uint16_t park_olv[QTI_EPSS_NR_DOMAINS];
	uint8_t vc_880mv[QTI_EPSS_NR_DOMAINS];
	uint32_t reserved2;
	uint8_t mem_acc_nom_vc[QTI_EPSS_NR_DOMAINS];
	uint8_t mem_acc_tur_vc[QTI_EPSS_NR_DOMAINS];
	uint32_t reserved3;
	uint8_t mx_nom_data;
	uint8_t mx_tur_data;
	uint32_t dcvs_down_saw_avs_dly[1];
	uint16_t gplus_vmax_75c;
	uint16_t gplus_vmax_80c;
	uint16_t gplus_vmax_85c;
	uint16_t gplus_vmax_90c;
	uint16_t gplus_vmax_95c;
	uint16_t gplus_vmax_100c;
	uint32_t ns_logbuf_access_en;
	uint32_t speed_bin;
	uint32_t sku_id;
	uint32_t iddq;
	uint32_t qultivate_fuse;
	uint32_t reserved4;
	uint32_t mpmm_gear;
} qti_epss_shared_data_t;

typedef char qti_epss_abi_bcl_offset_check[
	(offsetof(qti_epss_shared_data_t, bcl_lval) == 332U) ? 1 : -1];
typedef char qti_epss_abi_memacc_offset_check[
	(offsetof(qti_epss_shared_data_t, mem_acc_nom_vc) == 364U) ? 1 : -1];
typedef char qti_epss_abi_saw_delay_offset_check[
	(offsetof(qti_epss_shared_data_t, dcvs_down_saw_avs_dly) == 380U) ? 1 : -1];
typedef char qti_epss_abi_mpmm_gear_offset_check[
	(offsetof(qti_epss_shared_data_t, mpmm_gear) == 420U) ? 1 : -1];

static uint32_t qti_rimps_calc_mpmm_gear(const vpp_t *plan,
					 uint32_t *iddq_ma)
{
	uint32_t iddq_field =
		(mmio_read_32(QTI_QFPROM_CORR_PTE_ROW1_MSB) &
		 QTI_QFPROM_IDDQ_APC_ACTIVE_MASK) >>
		QTI_QFPROM_IDDQ_APC_ACTIVE_SHIFT;
	uint32_t iddq_fraction = (iddq_field & 0x3U) * 25U;
	uint32_t iddq = ((iddq_field >> 2U) * 1000U) + iddq_fraction;
	uint32_t vturbo_mv = plan->cprh[plan->cnt - 1U].ceil_mV;
	uint32_t ipeak_cal = vturbo_mv * 1000U +
		QTI_IPEAK_SLOPE * (iddq * 1000U / QTI_IDDQ_TT_MA);

	*iddq_ma = iddq;
	if (iddq == 0U)
		return 0x2U;
	if (ipeak_cal < QTI_IPEAK_THRESHOLD_0)
		return 0x3U;
	if (ipeak_cal < QTI_IPEAK_THRESHOLD_1)
		return 0x0U;
	if (ipeak_cal < QTI_IPEAK_THRESHOLD_2)
		return 0x1U;

	return 0x2U;
}

static int qti_rimps_update_epss_shared_data(uint32_t domain,
						     const vpp_t *plan)
{
	volatile qti_epss_shared_data_t *shared =
		(volatile qti_epss_shared_data_t *)QTI_EPSS_SHARED_DATA_BASE;
	uint8_t dvm_last_vc[QTI_EPSS_ACD_LEVELS];
	uint8_t avg_last_vc[QTI_EPSS_ACD_LEVELS];
	uint8_t acdsscr_last_vc[QTI_EPSS_ACD_LEVELS];
	uint32_t dvm_idx = 0U;
	uint32_t avg_idx = 0U;
	uint32_t acdsscr_idx = 0U;
	uint32_t svs_l1_row = UINT32_MAX;
	uint32_t turbo_row = UINT32_MAX;
	uint32_t iddq_ma;
	uint32_t mpmm_gear;
	uint32_t speed_bin;
	uint32_t qultivate_fuse;
	uint32_t row;
	const vpp_osm_t *previous = NULL;
	const vpp_osm_t *last;

	if ((domain >= QTI_EPSS_NR_DOMAINS) || (plan == NULL) ||
	    (plan->cnt == 0U) || (plan->cnt > VPP_MAX_LUT_SIZE) ||
	    (plan->cnt != plan->osm_cnt)) {
		return QTIPMLIB_E_INVAL;
	}

	/* Mirror TZ Clock_ParseVPP() and Clock_UpdateEPSSSharedData(). */
	for (row = 0U; row < plan->cnt; row++) {
		const vpp_osm_t *osm = &plan->osm[row];

		if ((svs_l1_row == UINT32_MAX) &&
		    (osm->vlvl >= VPP_VLVL_SVS_L1))
			svs_l1_row = row;
		if ((turbo_row == UINT32_MAX) &&
		    (osm->vlvl >= VPP_VLVL_TUR))
			turbo_row = row;

		if ((previous != NULL) &&
		    (osm->acd_acdcr != previous->acd_acdcr)) {
			if (dvm_idx >= QTI_EPSS_ACD_LEVELS)
				return QTIPMLIB_E_INVAL;
			dvm_last_vc[dvm_idx++] = previous->vc;
		}
		if (dvm_idx >= QTI_EPSS_ACD_LEVELS)
			return QTIPMLIB_E_INVAL;
		shared->acd_dvm_val[domain][dvm_idx] = osm->acd_acdcr;

		if ((previous != NULL) &&
		    (osm->acd_acdsscr != previous->acd_acdsscr)) {
			if (acdsscr_idx >= QTI_EPSS_ACD_LEVELS)
				return QTIPMLIB_E_INVAL;
			acdsscr_last_vc[acdsscr_idx++] = previous->vc;
		}
		if (acdsscr_idx >= QTI_EPSS_ACD_LEVELS)
			return QTIPMLIB_E_INVAL;
		shared->acd_acdsscr_val[domain][acdsscr_idx] = osm->acd_acdsscr;

		if ((previous != NULL) &&
		    (osm->acd_avg_cfg_2 != previous->acd_avg_cfg_2)) {
			if (avg_idx >= QTI_EPSS_ACD_LEVELS)
				return QTIPMLIB_E_INVAL;
			avg_last_vc[avg_idx++] = previous->vc;
		}
		if (avg_idx >= QTI_EPSS_ACD_LEVELS)
			return QTIPMLIB_E_INVAL;
		shared->acd_avg_cfg[domain][avg_idx] = osm->acd_avg_cfg_2;

		previous = osm;
	}

	last = &plan->osm[plan->cnt - 1U];
	for (; dvm_idx < QTI_EPSS_ACD_LEVELS; dvm_idx++) {
		shared->acd_dvm_val[domain][dvm_idx] = last->acd_acdcr;
		dvm_last_vc[dvm_idx] = last->vc;
	}
	for (; acdsscr_idx < QTI_EPSS_ACD_LEVELS; acdsscr_idx++) {
		shared->acd_acdsscr_val[domain][acdsscr_idx] =
			last->acd_acdsscr;
		acdsscr_last_vc[acdsscr_idx] = last->vc;
	}
	for (; avg_idx < QTI_EPSS_ACD_LEVELS; avg_idx++) {
		shared->acd_avg_cfg[domain][avg_idx] = last->acd_avg_cfg_2;
		avg_last_vc[avg_idx] = last->vc;
	}

	for (row = 0U; row < QTI_EPSS_ACD_LEVELS; row++) {
		shared->acd_dvm_vc[domain][row] = dvm_last_vc[row];
		shared->acd_acdsscr_vc[domain][row] = acdsscr_last_vc[row];
		shared->acd_avg_vc[domain][row] = avg_last_vc[row];
	}

	/*
	 * TZ writes 0xff when APM is in auto mode, and also for L3 when L3
	 * MX voting is disabled. This is a distinct sentinel from MAX_VC (0x3f)
	 * used for absent MEMACC corners.
	 */
	shared->apm_crossover_corner[domain] = QTI_EPSS_NO_APM_CROSSOVER;
	if (domain == 0U) {
		shared->mx_nom_data = 0U;
		shared->mx_tur_data = 0U;
	}

	/* TZ preserves CLOCK_OSM_MAX_VC if the VPP lacks either corner. */
	shared->mem_acc_nom_vc[domain] =
		(svs_l1_row == UINT32_MAX) ? QTI_EPSS_MAX_VC :
		plan->osm[svs_l1_row].vc;
	shared->mem_acc_tur_vc[domain] =
		(turbo_row == UINT32_MAX) ? QTI_EPSS_MAX_VC :
		plan->osm[turbo_row].vc;
	shared->park_pll_lval[domain] = plan->osm[0].lval;
	shared->park_vc[domain] = plan->osm[0].vc;

	for (row = 0U; row < QTI_EPSS_BCL_LEVELS; row++) {
		uint32_t lval = (uint32_t)last->lval -
			((uint32_t)last->lval * 20U * row) / 100U;
		uint32_t svs_l1_lval = (svs_l1_row == UINT32_MAX) ? 0U :
			plan->osm[svs_l1_row].lval;

		if (lval < svs_l1_lval)
			lval = svs_l1_lval;
		shared->bcl_lval[domain][row] = (uint8_t)lval;
	}

	/* TZ writes the APC0 (only Shikra APC rail) down-delay on every domain. */
	shared->dcvs_down_saw_avs_dly[0] = QTI_EPSS_SILVER_AVS_DOWN_DLY;
	shared->iddq = 0U;
	/* No TF-A secure-debug policy provider exists; deny NS log access. */
	shared->ns_logbuf_access_en = 0U;
	/* Match Shikra TZ VPP_SPEED_BIN from QFPROM PTE row 3, bits 7:2. */
	speed_bin = (mmio_read_32(QTI_QFPROM_CORR_PTE_ROW3_LSB) &
		     QTI_QFPROM_SPEED_BIN_MASK) >>
		    QTI_QFPROM_SPEED_BIN_SHIFT;
	shared->speed_bin = speed_bin;
	/* Shikra TZ leaves sku_identifier unset; the zero-initialized device
	 * context therefore publishes SKU_ID=0.
	 */
	shared->sku_id = 0U;
	/* Match TZ's success-gated QULTIVATE_FUSE shared-data write. */
	if (chipinfo_get_disabled_cpus(0U, &qultivate_fuse) ==
	    CHIPINFO_SUCCESS) {
		shared->qultivate_fuse = qultivate_fuse;
	}

	/* Match TZ ipeak_measurments() for this domain's final VPP corner. */
	mpmm_gear = qti_rimps_calc_mpmm_gear(plan, &iddq_ma);
	shared->mpmm_gear = mpmm_gear;

	__asm__ volatile("dsb sy" ::: "memory");
	if (shared->mpmm_gear != mpmm_gear) {
		ERROR("qtipmlib: MPMM_GEAR readback mismatch domain=%u got=0x%x expected=0x%x\n",
		      domain, shared->mpmm_gear, mpmm_gear);
		return QTIPMLIB_E_TIMEOUT;
	}
	if (domain == (QTI_EPSS_NR_DOMAINS - 1U)) {
		INFO("qtipmlib: EPSS speed_bin=%u sku_id=0\n", speed_bin);
		INFO("qtipmlib: MPMM_GEAR fuse=0x%x IDDQ=%u mA Vturbo=%u mV gear=%u\n",
		     (mmio_read_32(QTI_QFPROM_CORR_PTE_ROW1_MSB) &
		      QTI_QFPROM_IDDQ_APC_ACTIVE_MASK) >>
			     QTI_QFPROM_IDDQ_APC_ACTIVE_SHIFT,
		     iddq_ma, plan->cprh[plan->cnt - 1U].ceil_mV, mpmm_gear);
	}
	return QTIPMLIB_SUCCESS;
}

static uint32_t qti_rimps_pack_lut_freq(const vpp_osm_t *osm)
{
	return (((uint32_t)osm->clk_src << QTI_OSM_LUT_FREQ_CLKSRC_SHIFT) &
		QTI_OSM_LUT_FREQ_CLKSRC_MASK) |
	       (((uint32_t)osm->max_core_count << QTI_OSM_LUT_FREQ_CC_SHIFT) &
		QTI_OSM_LUT_FREQ_CC_MASK) |
	       (((uint32_t)osm->lval << QTI_OSM_LUT_FREQ_LVAL_SHIFT) &
		QTI_OSM_LUT_FREQ_LVAL_MASK);
}

static uint32_t qti_rimps_pack_lut_volt(const vpp_osm_t *osm,
					const vpp_cprh_t *cprh)
{
	return (((uint32_t)osm->vc << QTI_OSM_LUT_VOLT_VC_SHIFT) &
		QTI_OSM_LUT_VOLT_VC_MASK) |
	       (((uint32_t)cprh->ceil_mV << QTI_OSM_LUT_VOLT_OLV_SHIFT) &
		QTI_OSM_LUT_VOLT_OLV_MASK);
}

static void qti_rimps_program_lut_row(uintptr_t base, uint32_t row,
				      const vpp_osm_t *osm,
				      const vpp_cprh_t *cprh)
{
	uintptr_t freq_addr = base + QTI_CLKDOM_LUT_FREQ(row);
	uintptr_t volt_addr = base + QTI_CLKDOM_LUT_VOLT(row);
	uint32_t freq = qti_rimps_pack_lut_freq(osm);
	uint32_t volt = qti_rimps_pack_lut_volt(osm, cprh);
	uint32_t value;

	/* Match TZ HWIO_OUTXFI field writes and preserve reserved register bits. */
	value = mmio_read_32(freq_addr);
	value = (value & ~(QTI_OSM_LUT_FREQ_CLKSRC_MASK |
			   QTI_OSM_LUT_FREQ_CC_MASK |
			   QTI_OSM_LUT_FREQ_LVAL_MASK)) | freq;
	mmio_write_32(freq_addr, value);
	value = mmio_read_32(volt_addr);
	value = (value & ~(QTI_OSM_LUT_VOLT_VC_MASK |
			   QTI_OSM_LUT_VOLT_OLV_MASK)) | volt;
	mmio_write_32(volt_addr, value);
}

static uint64_t qti_cpu_freq_hz(uint32_t src, uint32_t lval,
				uint32_t postdiv)
{
	if (src == QTI_CLOCK_SRC_XO)
		return QTI_CLOCK_XO_HZ;
	if (src == QTI_CLOCK_SRC_GPLL0)
		return QTI_CLOCK_GPLL0_HZ;
	if (src == QTI_CLOCK_SRC_CPUPLL_EARLY)
		return (uint64_t)lval * QTI_CLOCK_XO_HZ;

	return ((uint64_t)lval * QTI_CLOCK_XO_HZ) / (postdiv + 1U);
}

static int qti_rimps_program_lut(const qti_rimps_clkdom_t *clkdom,
				 uint8_t *initial_vc,
				 uint16_t *vmin0c_violation_mv)
{
	const vpp_t *plan = qti_vpp_get_plan(clkdom->rail_name,
					     clkdom->clk_name);
	uint32_t row;
	uint32_t svs_l1_row = UINT32_MAX;
	uint32_t current_lval;
	uint32_t current_src;
	uint32_t postdiv;
	uint64_t current_freq;

	if (plan == NULL) {
		ERROR("qtipmlib: missing VPP plan for %s/%s\n",
		      clkdom->rail_name, clkdom->clk_name);
		return QTIPMLIB_E_NOT_FOUND;
	}
	if (plan->cnt == 0U) {
		ERROR("qtipmlib: empty VPP plan for %s/%s\n",
		      clkdom->rail_name, clkdom->clk_name);
		return QTIPMLIB_E_NOT_FOUND;
	}
	if ((initial_vc == NULL) || (vmin0c_violation_mv == NULL))
		return QTIPMLIB_E_INVAL;
	if ((plan->cnt != plan->osm_cnt) ||
	    (plan->cnt > VPP_MAX_LUT_SIZE) ||
	    (plan->cnt > QTI_OSM_LUT_ROWS))
		return QTIPMLIB_E_INVAL;

	/* Preserve the raw TZ/VPP inputs used below to choose the initial VC,
	 * VMIN0C threshold, and every OSM LUT row. This is one boot-time dump.
	 */
	for (row = 0U; row < plan->cnt; row++) {
		const vpp_osm_t *osm = &plan->osm[row];
		const vpp_cprh_t *cprh = &plan->cprh[row];

		INFO("qtipmlib: VPP raw domain=%u row=%u freq_khz=%u src=%u max_core=%u lval=%u osm_vc=%u vlvl=%u cprh_vc=%u floor_mV=%u ceil_mV=%u target_quot=%u ro=%u acdcr=0x%x acdsscr=0x%x avg_cfg2=0x%x\n",
		     (unsigned int)(clkdom - qti_rimps_clkdoms), row,
		     (unsigned int)osm->freq_khz, (unsigned int)osm->clk_src,
		     (unsigned int)osm->max_core_count, (unsigned int)osm->lval,
		     (unsigned int)osm->vc, (unsigned int)osm->vlvl,
		     (unsigned int)cprh->vc, (unsigned int)cprh->floor_mV,
		     (unsigned int)cprh->ceil_mV,
		     (unsigned int)cprh->target_quot, (unsigned int)cprh->ro,
		     (unsigned int)osm->acd_acdcr,
		     (unsigned int)osm->acd_acdsscr,
		     (unsigned int)osm->acd_avg_cfg_2);
	}

	for (row = 0U; row < plan->cnt; row++) {
		if ((svs_l1_row == UINT32_MAX) &&
		    (plan->osm[row].vlvl >= VPP_VLVL_SVS_L1))
			svs_l1_row = row;
	}
	/*
	 * TZ Clock_ParseVPP() leaves Vmin0cViolation at MAX_VOLTAGE when no
	 * SVS_L1-or-higher row exists; absence of a corner is not an init error.
	 */
	*vmin0c_violation_mv = (svs_l1_row == UINT32_MAX) ?
		QTI_EPSS_MAX_VOLTAGE : plan->cprh[svs_l1_row].ceil_mV;

	for (row = 0U; row < QTI_OSM_LUT_ROWS; row++) {
		uint32_t idx = MIN(row, (uint32_t)plan->cnt - 1U);
		const vpp_osm_t *osm = &plan->osm[idx];
		const vpp_cprh_t *cprh = &plan->cprh[idx];

		qti_rimps_program_lut_row(clkdom->base, row, osm, cprh);
	}

	/* Publish the complete LUT before checking it or enabling any OSM. */
	__asm__ volatile("dsb sy" ::: "memory");
	for (row = 0U; row < QTI_OSM_LUT_ROWS; row++) {
		uint32_t idx = MIN(row, (uint32_t)plan->cnt - 1U);
		const vpp_osm_t *osm = &plan->osm[idx];
		const vpp_cprh_t *cprh = &plan->cprh[idx];
		uint32_t expected_freq = qti_rimps_pack_lut_freq(osm);
		uint32_t expected_volt = qti_rimps_pack_lut_volt(osm, cprh);
		uint32_t actual_freq = mmio_read_32(
			clkdom->base + QTI_CLKDOM_LUT_FREQ(row));
		uint32_t actual_volt = mmio_read_32(
			clkdom->base + QTI_CLKDOM_LUT_VOLT(row));

		if (((actual_freq & (QTI_OSM_LUT_FREQ_CLKSRC_MASK |
				     QTI_OSM_LUT_FREQ_CC_MASK |
				     QTI_OSM_LUT_FREQ_LVAL_MASK)) != expected_freq) ||
		    ((actual_volt & (QTI_OSM_LUT_VOLT_VC_MASK |
				     QTI_OSM_LUT_VOLT_OLV_MASK)) != expected_volt)) {
			ERROR("qtipmlib: OSM LUT readback failed domain=%u row=%u freq=0x%x/0x%x volt=0x%x/0x%x\n",
			      (unsigned int)(clkdom - qti_rimps_clkdoms), row,
			      actual_freq, expected_freq, actual_volt, expected_volt);
			return QTIPMLIB_E_TIMEOUT;
		}
	}

	/* TZ calls Clock_GetInitialVC() after programming the domain LUT. */
	current_lval = mmio_read_32(clkdom->pll_base + QTI_PLL_L_VAL) & 0xffU;
	postdiv = (mmio_read_32(clkdom->pll_base + QTI_PLL_USER_CTL) &
		   QTI_PLL_POSTDIV_MASK) >> QTI_PLL_POSTDIV_SHIFT;
	current_src = mmio_read_32(clkdom->pll_base + QTI_PLL_CFG_GFMUX) &
		      QTI_PLL_GFMUX_SRC_MASK;
	current_freq = qti_cpu_freq_hz(current_src, current_lval, postdiv);

	/* Select the first VPP row at or above the live PLL frequency, as TZ does. */
	*initial_vc = 0U;
	for (row = 0U; row < plan->cnt; row++) {
		uint64_t row_freq = (uint64_t)plan->osm[row].freq_khz * 1000U;

		if (row_freq >= current_freq) {
			*initial_vc = (uint8_t)row;
			break;
		}
	}

	return QTIPMLIB_SUCCESS;
}

int qtipmlib_rimps_platform_init(uint8_t initial_vc[3],
				 uint16_t vmin0c_violation_mv[3])
{
	uint32_t i;
	int ret;

	if ((initial_vc == NULL) || (vmin0c_violation_mv == NULL))
		return QTIPMLIB_E_INVAL;

	NOTICE("qtipmlib: Shikra RIMPS platform-init hook entered\n");
	for (i = 0U; i < ARRAY_SIZE(qti_rimps_clkdoms); i++) {
		const vpp_t *plan = qti_vpp_get_plan(
			qti_rimps_clkdoms[i].rail_name,
			qti_rimps_clkdoms[i].clk_name);
		ret = qti_rimps_program_lut(&qti_rimps_clkdoms[i],
					     &initial_vc[i],
					     &vmin0c_violation_mv[i]);

		if (ret != QTIPMLIB_SUCCESS) {
			ERROR("qtipmlib: RIMPS LUT setup failed domain=%u rail=%s clock=%s rc=%d\n",
			      i, qti_rimps_clkdoms[i].rail_name,
			      qti_rimps_clkdoms[i].clk_name, ret);
			return ret;
		}

		/* TZ updates this domain's EPSS ABI immediately after its LUT/VC. */
		ret = qti_rimps_update_epss_shared_data(i, plan);
		if (ret != QTIPMLIB_SUCCESS) {
			ERROR("qtipmlib: EPSS shared-data update failed domain=%u rail=%s clock=%s rc=%d\n",
			      i, qti_rimps_clkdoms[i].rail_name,
			      qti_rimps_clkdoms[i].clk_name, ret);
			return ret;
		}
		INFO("qtipmlib: VPP domain=%u rail=%s clock=%s rows=%u initial_vc=%u vmin0c_mv=%u\n",
		     i, qti_rimps_clkdoms[i].rail_name,
		     qti_rimps_clkdoms[i].clk_name, plan->cnt, initial_vc[i],
		     vmin0c_violation_mv[i]);
	}

	/* TZ order: program domain LUTs, configure APM auto mode, then MEMACC. */
	ret = qti_rimps_program_apm();
	if (ret != QTIPMLIB_SUCCESS)
		return ret;
	qti_rimps_program_memacc();
	/* Complete EPSS/APM/MEMACC writes before qtipmlib enables OSM domains. */
	__asm__ volatile("dsb sy" ::: "memory");

	return QTIPMLIB_SUCCESS;
}

int qtipmlib_rimps_platform_cprh_workaround(
	enum qtipmlib_rimps_cprh_stage stage)
{
	if (stage == QTIPMLIB_RIMPS_CPRH_BEFORE_SILVER_OSM) {
		qti_cprh_gold_fused_off_workaround(false);
	} else if (stage == QTIPMLIB_RIMPS_CPRH_AFTER_SILVER_OSM) {
		qti_cprh_gold_fused_off_workaround(true);
	} else {
		return QTIPMLIB_E_INVAL;
	}

	return QTIPMLIB_SUCCESS;
}

static int qti_rimps_program_apm(void)
{
	uint32_t value;
	uint32_t i;
	int ret;
	bool readback_failed = false;

	/* Shikra TZ VPP policy is VPP_AUTO_APM=1 (hardware auto mode). */
	value = mmio_read_32(QTI_APSS_APM_CTL_OVERRIDE);
	value = (value & ~QTI_APM_SER_FREEZE_DIS_MASK) |
		QTI_APM_SER_FREEZE_DIS_VALUE;
	mmio_write_32(QTI_APSS_APM_CTL_OVERRIDE, value);

	mmio_write_32(QTI_APSS_APM_DLY_CNTR_0, QTI_APM_DELAY_VALUE);
	mmio_write_32(QTI_APSS_APM_DLY_CNTR_1, QTI_APM_DELAY_VALUE);

	value = mmio_read_32(QTI_APSS_MAS_DLY0_0);
	value = (value & ~QTI_APM_MAS_DELAY_MASK) | QTI_APM_MAS_DELAY_VALUE;
	mmio_write_32(QTI_APSS_MAS_DLY0_0, value);
	value = mmio_read_32(QTI_APSS_MAS_DLY1_0);
	value = (value & ~QTI_APM_MAS_DELAY_MASK) | QTI_APM_MAS_DELAY_VALUE;
	mmio_write_32(QTI_APSS_MAS_DLY1_0, value);

	/* Match TZ Clock_InitAPM(): select auto mode before MAS overrides. */
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_EN_CSR_MODE, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_APC0_AUTO_MODE_EN,
				  QTI_APM_CTL_APC0_AUTO_MODE_EN);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_MODE_SW_HW_SEL,
				  QTI_APM_CTL_MODE_SW_HW_SEL);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_EN_CSR_MODE, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_APC1_AUTO_MODE_EN,
				  QTI_APM_CTL_APC1_AUTO_MODE_EN);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_MODE_SW_HW_SEL,
				  QTI_APM_CTL_MODE_SW_HW_SEL);

	for (i = 0U; i < 10U; i++)
		mmio_write_32(QTI_APSS_MAS_OVERRIDE(i),
			       QTI_APM_MAS_OVERRIDE_VALUE);

	/* TZ clears these APM mode controls after programming the MAS overrides. */
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_FORCE_CLK_DIS, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_FORCE_CLK_DIS, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_CONT_LOAD_SHIFTING_EN, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_CONT_LOAD_SHIFTING_EN, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_0,
				  QTI_APM_CTL_USE_CLK_GATE_ACK, 0U);
	qti_rimps_apm_update_bits(QTI_APSS_APM_CTL_1,
				  QTI_APM_CTL_USE_CLK_GATE_ACK, 0U);

	/* Drain the ordered Device writes before the existing bounded readback. */
	__asm__ volatile("dsb sy" ::: "memory");
	INFO("qtipmlib: Shikra APM delay readback DLY0=0x%x DLY1=0x%x\n",
	     mmio_read_32(QTI_APSS_APM_DLY_CNTR_0),
	     mmio_read_32(QTI_APSS_APM_DLY_CNTR_1));

	ret = qti_rimps_check_apm_reg("APM_CTL_OVERRIDE",
				      QTI_APSS_APM_CTL_OVERRIDE,
				      QTI_APM_SER_FREEZE_DIS_MASK,
				      QTI_APM_SER_FREEZE_DIS_VALUE);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("APM_DLY_CNTR_0",
				      QTI_APSS_APM_DLY_CNTR_0,
				      UINT32_MAX, QTI_APM_DELAY_VALUE);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("APM_DLY_CNTR_1",
				      QTI_APSS_APM_DLY_CNTR_1,
				      UINT32_MAX, QTI_APM_DELAY_VALUE);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("MAS_DLY0_0", QTI_APSS_MAS_DLY0_0,
				      QTI_APM_MAS_DELAY_MASK,
				      QTI_APM_MAS_DELAY_VALUE);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("MAS_DLY1_0", QTI_APSS_MAS_DLY1_0,
				      QTI_APM_MAS_DELAY_MASK,
				      QTI_APM_MAS_DELAY_VALUE);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("APM_CTL_0", QTI_APSS_APM_CTL_0,
				      QTI_APM_CTL0_AUTO_MASK,
				      QTI_APM_CTL_MODE_SW_HW_SEL |
				      QTI_APM_CTL_APC0_AUTO_MODE_EN);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	ret = qti_rimps_check_apm_reg("APM_CTL_1", QTI_APSS_APM_CTL_1,
				      QTI_APM_CTL1_AUTO_MASK,
				      QTI_APM_CTL_MODE_SW_HW_SEL |
				      QTI_APM_CTL_APC1_AUTO_MODE_EN);
	if (ret != QTIPMLIB_SUCCESS)
		readback_failed = true;
	for (i = 0U; i < 10U; i++) {
		ret = qti_rimps_check_apm_reg("APM_MAS_OVERRIDE",
					      QTI_APSS_MAS_OVERRIDE(i),
					      QTI_APM_MAS_OVERRIDE_VALUE,
					      QTI_APM_MAS_OVERRIDE_VALUE);
		if (ret != QTIPMLIB_SUCCESS)
			readback_failed = true;
	}

	if (readback_failed) {
		/*
		 * Keep bring-up bootable while capturing all failed reads. This is
		 * diagnostic-only: APM auto mode is NOT considered validated, and
		 * this must not be treated as a production-safe LPM configuration.
		 */
		WARN("qtipmlib: APM readback failed after bounded waits; continuing for diagnostics only (base=0x%lx)\n",
		     (unsigned long)QTI_APSS_APM_BASE);
	} else {
		INFO("qtipmlib: Shikra APM config readback ok (base=0x%lx)\n",
		     (unsigned long)QTI_APSS_APM_BASE);
	}
	return QTIPMLIB_SUCCESS;
}

static void qti_rimps_apm_update_bits(uintptr_t address, uint32_t mask,
				      uint32_t value)
{
	uint32_t current = mmio_read_32(address);

	mmio_write_32(address, (current & ~mask) | (value & mask));
}

static int qti_rimps_check_apm_reg(const char *name, uintptr_t address,
				   uint32_t mask, uint32_t expected)
{
	uint32_t actual = 0U;
	uint32_t elapsed;

	for (elapsed = 0U; elapsed <= QTI_APM_READBACK_TIMEOUT_US; elapsed++) {
		actual = mmio_read_32(address) & mask;
		if (actual == expected) {
			qtipmlib_report_readback(name, actual, expected);
			return QTIPMLIB_SUCCESS;
		}
		if (elapsed != QTI_APM_READBACK_TIMEOUT_US)
			udelay(1U);
	}

	ERROR("qtipmlib: %s failed to settle at 0x%lx after %u us: got 0x%x expected 0x%x (mask 0x%x)\n",
	      name, (unsigned long)address, QTI_APM_READBACK_TIMEOUT_US,
	      actual, expected, mask);
	return QTIPMLIB_E_TIMEOUT;
}

static void qti_rimps_program_memacc(void)
{
	static const uintptr_t memacc_registers[] = {
		QTI_APSS_MEM_ACC_GOLD_SVS_0,
		QTI_APSS_MEM_ACC_GOLD_SVS_1,
		QTI_APSS_MEM_ACC_GOLD_NOM_0,
		QTI_APSS_MEM_ACC_GOLD_NOM_1,
		QTI_APSS_MEM_ACC_GOLD_TURBO_0,
		QTI_APSS_MEM_ACC_GOLD_TURBO_1,
		QTI_APSS_MEM_ACC_SILVER_SVS_0,
		QTI_APSS_MEM_ACC_SILVER_SVS_1,
		QTI_APSS_MEM_ACC_SILVER_NOM_0,
		QTI_APSS_MEM_ACC_SILVER_NOM_1,
		QTI_APSS_MEM_ACC_SILVER_TURBO_0,
		QTI_APSS_MEM_ACC_SILVER_TURBO_1,
		QTI_APSS_MEM_ACC_L3_SVS_1,
		QTI_APSS_MEM_ACC_L3_SVS_0,
		QTI_APSS_MEM_ACC_L3_NOM_1,
		QTI_APSS_MEM_ACC_L3_NOM_0,
		QTI_APSS_MEM_ACC_L3_TURBO_0,
		QTI_APSS_MEM_ACC_L3_TURBO_1,
	};
	uint32_t i;

	/* Mirror TZ Clock_InitMEMACC(): initialize every corner table to zero. */
	for (i = 0U; i < ARRAY_SIZE(memacc_registers); i++)
		mmio_write_32(memacc_registers[i], 0U);
}

/*
 * Overrides qtipmlib's weak no-op default so the single-shot readback checks
 * that replaced its register-polling loops (MUC clock enable, Gold PLL lock
 * detect) are actually visible on the BL31 console.
 */
void qtipmlib_report_readback(const char *name, uint32_t actual, uint32_t expected)
{
	if (actual != expected) {
		ERROR("qtipmlib: %s readback mismatch: got 0x%x expected 0x%x\n",
		      name, actual, expected);
	} else {
		INFO("qtipmlib: %s readback ok: 0x%x\n", name, actual);
	}
}

void qti_qtipmlib_init(void)
{
	int rc;
	uint32_t i;

	NOTICE("qtipmlib: BL31 image %s\n", build_version_string);
	rc = qtipmlib_init();
	if (rc != QTIPMLIB_SUCCESS) {
		ERROR("qtipmlib_init() failed: %d\n", rc);
		panic();
	}

	/* Read-only RIMPS snapshot for comparison with the no-T32 SRAM trace. */
	INFO("qtipmlib: RIMPSFAST MUC_CLK_CTRL=0x%x SEQ_MEM_0=0x%x CPU_LPM_ARB_CTL=0x%x CPU_LPM_IRQ_EN=0x%x CL_LPM_IRQ_EN=0x%x\n",
	     mmio_read_32(QTI_RIMPSFAST_MUC_CLK_CTRL),
	     mmio_read_32(QTI_RIMPSFAST_SEQ_MEM_0),
	     mmio_read_32(QTI_RIMPSFAST_CPU_LPM_ARB_CTL),
	     mmio_read_32(QTI_RIMPSFAST_CPU_LPM_IRQ_EN),
	     mmio_read_32(QTI_RIMPSFAST_CL_LPM_IRQ_EN));
	INFO("qtipmlib: RIMPSTOP SECURE_ACCESS=0x%x GLOBAL_ENABLE=0x%x L3_VOTING_CTRL=0x%x MUC_HANG_DET_STS=0x%x\n",
	     mmio_read_32(QTI_RIMPSTOP_SEC_ACCESS_OVERRIDE),
	     mmio_read_32(QTI_RIMPSTOP_GLOBAL_ENABLE),
	     mmio_read_32(QTI_RIMPSTOP_L3_VOTING_CTRL),
	     mmio_read_32(QTI_RIMPSTOP_MUC_HANG_DET_STS));
	INFO("qtipmlib: RIMPSFAST CPU_LPM_STATUS1=0x%x CPU_LPM_STATUS2=0x%x CLKDOM_LPM_GNT_MASK=0x%x MUC_STATUS=0x%x\n",
	     mmio_read_32(QTI_RIMPSFAST_CPU_LPM_STATUS1),
	     mmio_read_32(QTI_RIMPSFAST_CPU_LPM_STATUS2),
	     mmio_read_32(QTI_RIMPSFAST_CLKDOM_LPM_GNT_MASK),
	     mmio_read_32(QTI_RIMPSFAST_MUC_STATUS));
	for (i = 0U; i < ARRAY_SIZE(qti_rimps_clkdoms); i++) {
		uintptr_t base = qti_rimps_clkdoms[i].base;

		INFO("qtipmlib: CLKDOM%u RIMPS_ENABLE=0x%x PLL_FSM_CTRL=0x%x CORE_DCVS_CTRL=0x%x LLM_DCVS_VC_VOTE=0x%x LLM_DCVS_VOL_VOTE=0x%x RIMPS_INTR_EN=0x%x FVA_INT_EN=0x%x DESIRED_VC=0x%x LPM_STATUS=0x%x SEQ_CTRL1=0x%x SEQ_CTRL=0x%x BR_CTRL1=0x%x\n",
		     i, mmio_read_32(base + QTI_CLKDOM_RIMPS_ENABLE),
		     mmio_read_32(base + QTI_CLKDOM_PLL_FSM_CTRL),
		     mmio_read_32(base + QTI_CLKDOM_CORE_DCVS_CTRL),
		     mmio_read_32(base + QTI_CLKDOM_LLM_DCVS_VC_VOTE),
		     mmio_read_32(base + QTI_CLKDOM_LLM_DCVS_VOL_VOTE),
		     mmio_read_32(base + QTI_CLKDOM_RIMPS_INTR_EN),
		     mmio_read_32(base + QTI_CLKDOM_FVA_INT_EN),
		     mmio_read_32(base + QTI_CLKDOM_CORE_PERF_STATE_DESIRED_0),
		     mmio_read_32(base + QTI_CLKDOM_LPM_STATUS),
		     mmio_read_32(base + QTI_CLKDOM_SEQ_CTRL1),
		     mmio_read_32(base + QTI_CLKDOM_SEQ_CTRL),
		     mmio_read_32(base + QTI_CLKDOM_SEQ_BR_CTRL(1)));
	}
	/* Capture the additional lab-dump addresses before debugger attachment. */
	INFO("qtipmlib: APSS_SEQ_COMMON_INSTR_0=0x%x GOLD_PLL_MODE=0x%x\n",
	     mmio_read_32(QTI_APSS_SEQ_COMMON_INSTR_0),
	     mmio_read_32(QTI_GOLD_PLL_MODE));
	INFO("qtipmlib: CLKDOM2 MAX_CORE_CNT=0x%x NON_BOOST_VC=0x%x FVA_STATUS=0x%x DOMAIN_STATE=0x%x SEQ_BR_CTRL_0=0x%x\n",
	     mmio_read_32(QTI_RIMPSSLOW_CLKDOM2_BASE +
                 QTI_CLKDOM_MAX_CORE_CNT),
	     mmio_read_32(QTI_RIMPSSLOW_CLKDOM2_BASE +
                 QTI_CLKDOM_NON_BOOST_VC),
	     mmio_read_32(QTI_RIMPSSLOW_CLKDOM2_BASE +
                 QTI_CLKDOM_FVA_STATUS),
	     mmio_read_32(QTI_RIMPSSLOW_CLKDOM2_BASE +
                 QTI_CLKDOM_DOMAIN_STATE),
	     mmio_read_32(QTI_RIMPSSLOW_CLKDOM2_BASE +
                 QTI_CLKDOM_SEQ_BR_CTRL(0)));
	INFO("qtipmlib: RIMPS_MUC_MEMORY_WORD0=0x%x EPSS_SHARED_DATA_WORD0=0x%x\n",
	     mmio_read_32(QTI_RIMPS_MUC_MEMORY_WORD0),
	     mmio_read_32(QTI_EPSS_SHARED_DATA_BASE));
}
