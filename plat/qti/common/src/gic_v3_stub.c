/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <qti_plat.h>

/* Default no-op context hooks; Shikra's retained CPU_SUSPEND path does not use them. */
__attribute__((weak)) void plat_qti_gic_pcpu_save(void)
{
}

__attribute__((weak)) bool plat_qti_gic_pcpu_restore(void)
{
	return false;
}

/* Preserve the historical full CPU-interface handling on other QTI ports. */
__attribute__((weak)) void plat_qti_gic_pcpu_suspend(void)
{
	plat_qti_gic_cpuif_disable();
}

__attribute__((weak)) void plat_qti_gic_pcpu_suspend_finish(void)
{
	plat_qti_gic_cpuif_enable();
}
