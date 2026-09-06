# SPDX-License-Identifier: GPL-2.0-or-later
# alu_eth - U-Boot glue for the Annapurna al_eth MAC, on the SHARED Linux HAL.
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>

# Shared plat_api shim + common HAL headers, then the STAGED Linux HAL
# (scripts/stage_hal.py writes it from modules/al_eth/ every build). There is
# no second checked-in copy of the HAL - that is the point of #256.
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim
ccflags-y += -I$(srctree)/drivers/net/al_hal_shared

# HAL is third-party; silence its (non-fatal) style/unused warnings so our
# build stays clean without -Werror games. Applies to this directory only.
ccflags-y += -Wno-unused-variable -Wno-unused-function -Wno-sign-compare
ccflags-y += -Wno-unused-but-set-variable -Wno-missing-prototypes

# VERBOSE TRACE, off by default: -DDEBUG makes al_dbg()->debug() and the
# driver's debug()/dev_dbg() print for every eth object - the full UDMA
# descriptor-fetch and adapter bring-up trace, thousands of lines per boot.
ccflags-$(CONFIG_AL_ETH_DEBUG) += -DDEBUG

# --- U-Boot glue (alu_*) ---
obj-$(CONFIG_AL_ETH) += alu_eth_core.o		# rings, adapter, send/recv
obj-$(CONFIG_AL_ETH) += alu_eth_1g.o		# RGMII front end (1c36:0001)
obj-$(CONFIG_AL_ETH) += alu_eth_port.o		# port idx -> PCI fn -> BARs
obj-$(CONFIG_AL_ETH) += alu_eth_boardparams.o	# MAC scratchpad seed + edit
obj-$(CONFIG_AL_ETH) += alu_eth_hwaddr.o	# NOR MAC -> per-port EC filter
obj-$(CONFIG_AL_ETH) += alu_eth_rxfwd.o		# EC RX forwarding -> UDMA0/Q0
obj-$(CONFIG_AL_ETH) += alu_eth_stats.o		# `eth stats` + the `eth` command
obj-$(CONFIG_AL_ETH) += alu_eth_diag.o		# `eth diag`
obj-$(CONFIG_AL_ETH_10G) += alu_eth_10g.o	# SFP+ front end (1c36:0002)

# --- shared Linux HAL, compiled from the staged tree ---
# Built here rather than in the staged directory: the staged tree is a build
# artifact with no Makefile of its own, so kbuild never descends into it.
AL_HAL_SHARED := ../al_hal_shared
obj-$(CONFIG_AL_ETH) += $(AL_HAL_SHARED)/al_hal_iofic.o
obj-$(CONFIG_AL_ETH) += $(AL_HAL_SHARED)/al_hal_udma_main.o
obj-$(CONFIG_AL_ETH) += $(AL_HAL_SHARED)/al_hal_udma_config.o
obj-$(CONFIG_AL_ETH) += $(AL_HAL_SHARED)/al_hal_udma_iofic.o
obj-$(CONFIG_AL_ETH) += $(AL_HAL_SHARED)/al_hal_eth_main.o
