# SPDX-License-Identifier: GPL-2.0-or-later
# Annapurna Labs DDR HAL (alpine_v2 subset) + the U-Boot `ddr` command.
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
# Co-authored with Claude (Anthropic).

obj-y += al_hal_ddr.o
obj-y += al_hal_ddr_alpine_v2.o
obj-y += al_hal_sys_fabric_utils.o
obj-y += ddr_cmd.o

# The HAL is multi-SoC via AL_DEV_ID; pin the whole subset to Alpine V2 (AL-324)
# so the header aliases al_ddr_* -> _al_ddr_* and only the V2 struct layout /
# ZQ-segment counts / MR set compile. Numeric values per the Ubiquiti GPL
# kernel's drivers/soc/alpine/alpine_hal.mk (V1=0 V2=1 V3=2 V4=3).
ccflags-y += -DAL_DEV_ID_ALPINE_V1=0
ccflags-y += -DAL_DEV_ID_ALPINE_V2=1
ccflags-y += -DAL_DEV_ID_ALPINE_V3=2
ccflags-y += -DAL_DEV_ID_ALPINE_V4=3
ccflags-y += -DAL_DEV_REV_ID_V2=0
ccflags-y += -DAL_DEV_REV_ID_V3=1
ccflags-y += -DAL_DEV_REV_ID=AL_DEV_REV_ID_V2
ccflags-y += -DAL_DEV_ID=AL_DEV_ID_ALPINE_V2

# Resolve the ddr-specific HAL headers (this dir) + the shared plat_api shim and
# common HAL headers (al_hal_shim).
ccflags-y += -I$(srctree)/board/annapurna/alpine/al_ddr
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim

# Third-party HAL: silence its style warnings so it builds under -Werror.
ccflags-y += -Wno-unused-variable -Wno-unused-but-set-variable
ccflags-y += -Wno-unused-function
