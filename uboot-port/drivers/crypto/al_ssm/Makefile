# SPDX-License-Identifier: GPL-2.0-or-later
# Annapurna al_ssm (crypto/RAID) UDMA mem-to-mem test + minimal SSM/RAID HAL.
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>

# Shared plat_api shim + the STAGED Linux HAL (udma/iofic), then this dir for
# the SSM/RAID/m2m headers. al_eth compiles the shared udma/iofic objects and
# this reuses them at link (see Kconfig: depends on AL_ETH).
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim
ccflags-y += -I$(srctree)/drivers/net/al_hal_shared
ccflags-y += -I$(srctree)/drivers/crypto/al_ssm

# al_ssm's vendor sources are a NEWER HAL generation than modules/al_eth. Six
# symbols they need are expressed over the shared HAL in al_ssm_hal_compat.h,
# force-included so the vendor sources stay untouched. Phase 3 of #256.
ccflags-y += -include $(srctree)/drivers/crypto/al_ssm/al_ssm_hal_compat.h

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
