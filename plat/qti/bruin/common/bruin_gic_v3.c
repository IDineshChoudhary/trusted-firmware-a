/*
 * Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/arm_gicv3_common.h>

/*
 * The generic helper assumes every redistributor is quiesced before its
 * per-redistributor save/restore hooks run. Shikra performs local CPU suspend
 * while sibling CPUs remain active and does not use full GICR context save.
 */
void arm_gicv3_distif_pre_save(unsigned int rdist_proc_num)
{
	(void)rdist_proc_num;
}

void arm_gicv3_distif_post_restore(unsigned int rdist_proc_num)
{
	(void)rdist_proc_num;
}
