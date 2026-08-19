# SPDX-License-Identifier: GPL-2.0-or-later
# Annapurna al_eth 1G RGMII driver + curated HAL subset (MAC + UDMA + MDIO).
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>

# Shared plat_api shim + common HAL headers (al_hal_shim), then the eth-specific
# flattened HAL headers.
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim
ccflags-y += -I$(srctree)/drivers/net/al_eth/hal/include

# HAL is third-party; silence its (non-fatal) style/unused warnings so our
# build stays clean without -Werror games. Applies to this directory only.
ccflags-y += -Wno-unused-variable -Wno-unused-function -Wno-sign-compare
ccflags-y += -Wno-unused-but-set-variable -Wno-missing-prototypes

# VERBOSE TRACE (eth-debug): -DDEBUG makes al_dbg()->debug() and the driver's
# debug()/dev_dbg() print for every eth object (driver + eth/UDMA HAL) - the
# full UDMA descriptor-fetch / adapter bring-up trace. Scoped to this directory
# only (DDR/serdes are separate Makefiles). Remove once the TX path is proven.
ccflags-y += -DDEBUG

obj-$(CONFIG_AL_ETH) += al_eth_dm.o
obj-$(CONFIG_AL_ETH) += al_eth_stubs.o
# 10G SFP+ (eth2, 1c36:0002) - DRAFT front-end; same HAL subset, no new objects.
obj-$(CONFIG_AL_ETH_10G) += al_eth_dm_10g.o

# --- curated HAL subset (delroth-alpine_hal), RGMII 1G path only ---
obj-$(CONFIG_AL_ETH) += hal/iofic/al_hal_iofic.o
obj-$(CONFIG_AL_ETH) += hal/udma/al_hal_udma_main.o
obj-$(CONFIG_AL_ETH) += hal/udma/al_hal_udma_config.o
obj-$(CONFIG_AL_ETH) += hal/udma/al_hal_udma_iofic.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_main.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_mac.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_mac_internal.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_mac_v1_v2.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_common.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_epe.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_field.o
obj-$(CONFIG_AL_ETH) += hal/eth/al_hal_eth_rfw.o
