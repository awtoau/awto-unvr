# SPDX-License-Identifier: GPL-2.0-or-later
# Annapurna Labs 25G SerDes HAL (alpine_v2 subset) + fixed-10G SFP+ bring-up.
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
# Co-authored with Claude (Anthropic).

obj-y += al_hal_serdes_25g.o	# vendor PMA HAL (25G/10G group config)
obj-y += al_hal_serdes_hssp.o	# vendor PMA HAL (HSSP group, real 10G-optic EQ)
obj-y += al_serdes_10g.o	# fixed-10G init + PCS + `serdes` command

# Resolve the serdes-specific HAL headers (this dir) + the shared plat_api shim
# and common HAL headers (al_hal_shim).
ccflags-y += -I$(srctree)/drivers/phy/al_serdes
ccflags-y += -I$(srctree)/drivers/net/al_hal_shim

# Third-party HAL: silence its style warnings so it builds under -Werror.
ccflags-y += -Wno-unused-variable -Wno-unused-but-set-variable

# VERBOSE TRACE, off by default: -DDEBUG turns on al_dbg()->debug() across the
# SerDes HALs + 10G bring-up, so every PMA register access is logged - the last
# line before an SError names the faulting register. Shares the eth switch.
ccflags-$(CONFIG_AL_ETH_DEBUG) += -DDEBUG
ccflags-y += -Wno-unused-function
