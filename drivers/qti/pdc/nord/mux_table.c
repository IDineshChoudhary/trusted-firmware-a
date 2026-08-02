/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stddef.h>
#include <stdint.h>

#include <drivers/qti/pdc/pdc_internal.h>

/*============================================================================
 *                              GLOBAL VARIABLES
 *===========================================================================*/
/* Local interrupt vector mapping for the PDC MUX outputs */
struct pdc_gpio_mapping g_pdc_gpio_mapping[] = 
{
	/* Default trigger type   Owner DRV    GPIO ptr  Subsystem Interrupt#  */
	/* Mux 0 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	575 }, /* core_bi_px_core_in_mx_gpio_0 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	576 }, /* core_bi_px_core_in_mx_gpio_1 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	577 }, /* core_bi_px_core_in_mx_gpio_3 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	578 }, /* core_bi_px_core_in_mx_gpio_4 */

	/* Mux 4 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	579 }, /* core_bi_px_core_in_mx_gpio_6 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	580 }, /* core_bi_px_core_in_mx_gpio_7 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	581 }, /* core_bi_px_core_in_mx_gpio_9 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	585 }, /* core_bi_px_core_in_mx_gpio_102 */

	/* Mux 8 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	586 }, /* core_bi_px_core_in_mx_gpio_108 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	587 }, /* aoss_wakeup_gpio_161 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	588 }, /* core_bi_px_core_in_mx_gpio_114 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	589 }, /* core_bi_px_core_in_mx_gpio_116 */

	/* Mux 12 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	590 }, /* core_bi_px_core_in_mx_gpio_2 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	591 }, /* core_bi_px_core_in_mx_gpio_5 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	592 }, /* core_bi_px_core_in_mx_gpio_8 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	593 }, /* core_bi_px_core_in_mx_gpio_11 */

	/* Mux 16 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	595 }, /* core_bi_px_core_in_mx_gpio_138 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	596 }, /* core_bi_px_core_in_mx_gpio_142 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	597 }, /* core_bi_px_core_in_mx_gpio_144 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	598 }, /* core_bi_px_core_in_mx_gpio_153 */

	/* Mux 20 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	599 }, /* core_bi_px_core_in_mx_gpio_157 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	641 }, /* core_bi_px_core_in_mx_gpio_46 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	642 }, /* core_bi_px_core_in_mx_gpio_126 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	643 }, /* core_bi_px_core_in_mx_gpio_128 */

	/* Mux 24 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	644 }, /* core_bi_px_core_in_mx_gpio_132 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	650 }, /* core_bi_px_core_in_mx_gpio_45 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	651 }, /* core_bi_px_core_in_mx_gpio_124 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	652 }, /* core_bi_px_core_in_mx_gpio_166 */

	/* Mux 28 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	653 }, /* aoss_wakeup_gpio_160 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	654 }, /* core_bi_px_core_in_mx_gpio_168 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	660 }, /* core_bi_px_core_in_mx_gpio_120 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	661 }, /* core_bi_px_core_in_mx_gpio_159 */

	/* Mux 32 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	662 }, /* core_bi_px_core_in_mx_gpio_10 */
	{{TRIGGER_RISING_EDGE,	 PDC_DRV2},	NULL,	663 }, /* core_bi_px_core_in_mx_gpio_110 */
};

/* Size of above table */
const uint32_t g_pdc_gpio_mapping_size = sizeof(g_pdc_gpio_mapping) / sizeof(g_pdc_gpio_mapping[0]);
