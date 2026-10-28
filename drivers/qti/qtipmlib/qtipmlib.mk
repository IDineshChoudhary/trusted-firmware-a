#
# Copyright (c) 2026, Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES	+=	-Iinclude/drivers/qti/qtipmlib

ifeq ($(QTIPMLIB_PATH),)
$(warning QTIPMLIB_PATH is not provided, using qtipmlib stubs)
else
$(eval $(call add_define,QTI_PMLIB_ENABLED))
LDLIBS		+=	$(QTIPMLIB_PATH)
endif
