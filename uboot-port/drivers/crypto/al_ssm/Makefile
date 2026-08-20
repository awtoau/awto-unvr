# SPDX-License-Identifier: GPL-2.0-or-later
# Annapurna al_ssm (crypto/RAID) UDMA mem-to-mem test + minimal SSM/RAID HAL.
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>

# Shared plat_api shim + common HAL headers, then the al_eth-flattened HAL
# headers (udma/iofic/m2m). REUSE al_eth's udma/iofic HAL objects at link -
# only the SSM/RAID/m2m glue is compiled here (see Kconfig: depends on AL_ETH).
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim
ccflags-y += -I$(srctree)/drivers/net/al_eth/hal/include
ccflags-y += -I$(srctree)/drivers/crypto/al_ssm

# HAL is third-party; silence its (non-fatal) style/unused warnings.
ccflags-y += -Wno-unused-variable -Wno-unused-function -Wno-sign-compare
ccflags-y += -Wno-unused-but-set-variable -Wno-missing-prototypes

# VERBOSE TRACE: -DDEBUG makes al_dbg()->debug() print the UDMA descriptor path.
# Scoped to this directory only. Remove once the SSM UDMA test is proven on-box.
ccflags-y += -DDEBUG

obj-$(CONFIG_AL_SSM) += al_ssm_dma.o
# curated SSM/RAID HAL + the M2M-UDMA compound (not built by the al_eth subset)
obj-$(CONFIG_AL_SSM) += al_hal_ssm.o
obj-$(CONFIG_AL_SSM) += al_hal_ssm_raid.o
obj-$(CONFIG_AL_SSM) += al_hal_m2m_udma.o
