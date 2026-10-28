/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QTI_VPP_H
#define QTI_VPP_H

#include <stdint.h>

#define VPP_MAX_LUT_SIZE	40U

typedef enum {
	VPP_VLVL_LOWSVS		= 64,
	VPP_VLVL_SVS		= 128,
	VPP_VLVL_SVS_L1		= 192,
	VPP_VLVL_NOM		= 256,
	VPP_VLVL_NOM_L1		= 320,
	VPP_VLVL_TUR		= 384,
	VPP_VLVL_BOOST		= 488,
	VPP_VLVL_INVALID	= 0x7fffffff,
} vpp_vlvl_t;

typedef struct {
	uint8_t vc;
	uint8_t band;
	uint16_t ceil_mV;
	uint16_t floor_mV;
	uint16_t target_quot;
	uint8_t ro;
} vpp_cprh_t;

typedef struct {
	uint8_t clk_src;
	uint8_t max_core_count;
	uint8_t lval;
	uint8_t vc;
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
