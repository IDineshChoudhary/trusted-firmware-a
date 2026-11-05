/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QTI_PMLIB_H
#define QTI_PMLIB_H

#include <stdint.h>

enum qtipmlib_status {
	QTIPMLIB_SUCCESS = 0,
	QTIPMLIB_E_INVAL = -1,
	QTIPMLIB_E_NOMEM = -2,
	QTIPMLIB_E_NOT_FOUND = -3,
	QTIPMLIB_E_TIMEOUT = -4,
};

enum qtipmlib_lpm_node_id {
	QTIPMLIB_LPM_CPU0,
	QTIPMLIB_LPM_CPU1,
	QTIPMLIB_LPM_CPU2,
	QTIPMLIB_LPM_CPU3,
	QTIPMLIB_LPM_L3,
	QTIPMLIB_LPM_NR_NODES,
};

#ifdef QTI_PMLIB_ENABLED
void qti_qtipmlib_init(void);
/* Platform supplies Shikra VPP LUTs and per-domain initial VCs. */
int qtipmlib_rimps_platform_init(uint8_t initial_vc[3],
				 uint16_t vmin0c_violation_mv[3]);
enum qtipmlib_rimps_cprh_stage {
	QTIPMLIB_RIMPS_CPRH_BEFORE_SILVER_OSM,
	QTIPMLIB_RIMPS_CPRH_AFTER_SILVER_OSM,
};
int qtipmlib_rimps_platform_cprh_workaround(
	enum qtipmlib_rimps_cprh_stage stage);
int qtipmlib_init(void);
int qtipmlib_config_lpm(enum qtipmlib_lpm_node_id node_id, int mode_id);
int qtipmlib_unconfig_lpm(enum qtipmlib_lpm_node_id node_id, int mode_id);
int qtipmlib_enable_gold_rimps(void);
#else
static inline void qti_qtipmlib_init(void)
{
}

static inline int qtipmlib_init(void)
{
	return QTIPMLIB_SUCCESS;
}

static inline int qtipmlib_config_lpm(enum qtipmlib_lpm_node_id node_id,
				      int mode_id)
{
	(void)node_id;
	(void)mode_id;

	return QTIPMLIB_SUCCESS;
}

static inline int qtipmlib_unconfig_lpm(enum qtipmlib_lpm_node_id node_id,
					int mode_id)
{
	(void)node_id;
	(void)mode_id;

	return QTIPMLIB_SUCCESS;
}

static inline int qtipmlib_enable_gold_rimps(void)
{
	return QTIPMLIB_SUCCESS;
}
#endif

#endif /* QTI_PMLIB_H */
