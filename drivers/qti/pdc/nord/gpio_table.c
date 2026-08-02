/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>

#include <drivers/qti/pdc/pdc_internal.h>

/*============================================================================
 *                              GLOBAL VARIABLES
 *===========================================================================*/
/* Valid GPIO mux input numbers for this target */
struct pdc_gpio_inputs g_pdc_gpio_inputs[] =
{
	/* GPIO         Configured Mux */
	/* Input 0 */
	{0,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_0 */
	{1,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_1 */
	{3,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_3 */
	{4,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_4 */

	/* Input 4 */
	{6,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_6 */
	{7,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_7 */
	{9,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_9 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 8 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{102,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_102 */
	{108,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_108 */

	/* Input 12 */
	{161,			PDC_GPIO_INVALID}, /* aoss_wakeup_gpio_161 */
	{114,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_114 */
	{116,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_116 */
	{2,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_2 */

	/* Input 16 */
	{5,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_5 */
	{8,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_8 */
	{11,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_11 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 20 */
	{138,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_138 */
	{142,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_142 */
	{144,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_144 */
	{153,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_153 */

	/* Input 24 */
	{157,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_157 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 28 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{46,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_46 */

	/* Input 32 */
	{126,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_126 */
	{128,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_128 */
	{132,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_132 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 36 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 40 */
	{45,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_45 */
	{124,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_124 */
	{166,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_166 */
	{160,			PDC_GPIO_INVALID}, /* aoss_wakeup_gpio_160 */

	/* Input 44 */
	{168,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_168 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 48 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{120,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_120 */
	{159,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_159 */

	/* Input 52 */
	{10,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_10 */
	{110,			PDC_GPIO_INVALID}, /* core_bi_px_core_in_mx_gpio_110 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 56 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 60 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 64 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 68 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 72 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 76 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 80 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 84 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 88 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */

	/* Input 92 */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
	{PDC_MUX_OPEN,	PDC_GPIO_INVALID}, /* gp_irq_hvm[46] */
};

/* Size of above table */
const uint32_t g_pdc_gpio_input_size = sizeof(g_pdc_gpio_inputs) / sizeof(g_pdc_gpio_inputs[0]);
