/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef QTI_CPRH_H
#define QTI_CPRH_H

#include <stdbool.h>

#ifdef QTI_CPRH_ENABLED
void qti_cprh_init(void);
void qti_cprh_gold_fused_off_workaround(bool after_silver_osm);
#else
static inline void qti_cprh_init(void)
{
}

static inline void qti_cprh_gold_fused_off_workaround(bool after_silver_osm)
{
	(void)after_silver_osm;
}
#endif

#endif /* QTI_CPRH_H */
