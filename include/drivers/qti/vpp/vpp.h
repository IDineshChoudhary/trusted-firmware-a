/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QTI_VPP_H
#define QTI_VPP_H

#include <stdint.h>

#define VPP_MAX_LUT_SIZE	40U

/* Keep the Shikra voltage-level ordering used by TZ vpp.h. */
enum vpp_vlvl {
	VPP_VLVL_OFF = 0,
	VPP_VLVL_RET = 1,
	VPP_VLVL_MINSVS = 2,
	VPP_VLVL_LOWSVS = 3,
	VPP_VLVL_SVS = 4,
	VPP_VLVL_SVS_L1 = 5,
	VPP_VLVL_NOM = 6,
	VPP_VLVL_NOM_L1 = 7,
	VPP_VLVL_NOM_L2 = 8,
	VPP_VLVL_TUR = 9,
	VPP_VLVL_TUR_L1 = 10,
	VPP_VLVL_TUR_L2 = 11,
	VPP_VLVL_TUR_L3 = 12,
	VPP_VLVL_TUR_BOOST = 13,
	VPP_VLVL_BOOST = 14,
	VPP_VLVL_BOOSTP = 15,
	VPP_VLVL_BIN = 16,
	VPP_VLVL_TOTAL = 17,
	VPP_VLVL_INVALID = 18,
};

typedef struct {
	uint8_t vc;
	uint8_t band;
	uint16_t ceil_mV;
	uint16_t floor_mV;
	uint16_t target_quot;
	uint8_t ro;
} vpp_cprh_t;

typedef struct {
	uint32_t freq_khz;
	uint8_t clk_src;
	uint8_t max_core_count;
	uint8_t lval;
	uint8_t vc;
	uint8_t vlvl;
	uint32_t acd_acdcr;
	uint32_t acd_acdsscr;
	uint32_t acd_avg_cfg_2;
} vpp_osm_t;

typedef struct {
	const char *rail_name;
	const char *clk_name;
	uint16_t cnt;
	uint16_t osm_cnt;
	vpp_osm_t osm[VPP_MAX_LUT_SIZE];
	vpp_cprh_t cprh[VPP_MAX_LUT_SIZE];
} vpp_t;

#ifdef QTI_VPP_ENABLED
void qti_vpp_init(void);
const vpp_t *qti_vpp_get_plan(const char *rail, const char *clk);

#else
static inline void qti_vpp_init(void)
{
}

static inline const vpp_t *qti_vpp_get_plan(const char *rail, const char *clk)
{
	(void)rail;
	(void)clk;

	return NULL;
}
#endif

#endif /* QTI_VPP_H */
