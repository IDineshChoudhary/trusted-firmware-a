#
# Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Selects optional platform-specific per-core GIC context hooks.
#
# GICV3_OVERRIDE_DISTIF_PWR_OPS=1 means arm_gicv3_common.c's generic
# distif_pre_save()/post_restore() hooks are excluded (drivers/arm/gic/v3/
# gicv3.mk); every other platform leaves it at its default (0) and gets
# gic_v3_stub.c's empty hooks instead.
#
# This file is shared by every qti platform, so it must not hardcode any
# single target's path: a platform.mk that sets GICV3_OVERRIDE_DISTIF_PWR_OPS
# := 1 must also set QTI_GIC_V3_OVERRIDE_SRC to its own override source file
# before including this file.
#
ifeq (${GICV3_OVERRIDE_DISTIF_PWR_OPS},1)
QTI_GIC_V3_PLAT_SOURCES	:=	${QTI_GIC_V3_OVERRIDE_SRC}
else
QTI_GIC_V3_PLAT_SOURCES	:=	plat/qti/common/src/gic_v3_stub.c
endif
