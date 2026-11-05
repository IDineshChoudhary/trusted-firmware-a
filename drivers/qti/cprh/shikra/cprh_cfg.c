/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../cprh_internal.h"

#include <arch_helpers.h>
#include <drivers/qti/chipinfo/chipinfo.h>
#include <lib/mmio.h>

#define SHIKRA_GOLD_CORES_DISABLED_MASK	0xc0U
#define SHIKRA_CPRH_MASK_THREAD(thread)	(0x080cU + (0x440U * (thread)))
#define SHIKRA_CPRH_THREAD_OVERRIDE_CTL(thread) \
	(0x3c70U + (0x4U * (thread)))
#define SHIKRA_CPRH_DISABLE_THREAD		(1U << 31U)
#define SHIKRA_CPRH_GOLD_OVERRIDE_MASK		0x3U

static const cprh_thread_cfg_t shikra_cprh_threads[] = {
	{
		.clk_name = "Silver",
		.thread_num = 0,
		.dn_threshold = 2,
		.up_threshold = 2,
	},
	{
		.clk_name = "L3",
		.thread_num = 1,
		.dn_threshold = 2,
		.up_threshold = 2,
	},
	{
		.clk_name = "Gold",
		.thread_num = 2,
		.dn_threshold = 2,
		.up_threshold = 2,
	},
};

const cprh_controller_cfg_t qti_cprh_controller = {
	.rail_name = "APC",
	.base_addr = 0x0FBA0000UL,
	.fsm_ctl = 0x00002814,
	.cprh_ctl = 0x080000C8,
	.timer_auto_cont = 0x17700,
	.step_quot_init = 0x1C7,
	.timer_clamp = 0x08000000,
	.saw_error_step_limit = 0x21,
	.misc_register = 0x61000004,
	.margin_adj_ctl = 0x01024000,
	.margin_temp_core_timers = 0x005CCB00,
	.sensor_count = 10,
	.sensor_thread_mask = 0x0F,
	.sensor_thread_mask_thread2 = 0x380,
	.sensor_mask = 0x380,
	.sensor_bypass_mask = 0,
	.thread_count = 3,
	.threads = shikra_cprh_threads,
};

/* Mirror TZ's fuse-gated CPRH workaround around Silver's first OSM request. */
void qti_cprh_gold_fused_off_workaround(bool after_silver_osm)
{
	uint32_t disabled_cpus;
	uintptr_t reg;
	uint32_t value;

	if (chipinfo_get_disabled_cpus(0U, &disabled_cpus) != CHIPINFO_SUCCESS ||
	    (disabled_cpus & SHIKRA_GOLD_CORES_DISABLED_MASK) !=
	    SHIKRA_GOLD_CORES_DISABLED_MASK) {
		return;
	}

	if (after_silver_osm) {
		/* TZ sets IGNORE_OOL_FLR_AGGREGATION and OVERRIDE_OSM_FSM_OUTPUTS. */
		reg = qti_cprh_controller.base_addr +
		      SHIKRA_CPRH_THREAD_OVERRIDE_CTL(2U);
		value = mmio_read_32(reg) | SHIKRA_CPRH_GOLD_OVERRIDE_MASK;
	} else {
		/* TZ disables CPRH thread 2 before Silver's initial OSM request. */
		reg = qti_cprh_controller.base_addr + SHIKRA_CPRH_MASK_THREAD(2U);
		value = mmio_read_32(reg) | SHIKRA_CPRH_DISABLE_THREAD;
	}

	mmio_write_32(reg, value);
	dsbsy();
}
