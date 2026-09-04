/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Annapurna Labs al_eth - EC RX-forwarding config shared by eth1 and eth2.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 */

#ifndef _AL_ETH_RXFWD_H
#define _AL_ETH_RXFWD_H

struct al_hal_eth_adapter;

/* Steer every received frame to UDMA 0 / queue 0. Call after
 * al_eth_queue_enable() and before al_eth_mac_start(); without it the EC drops
 * all RX before the S2M ring (#234). */
void al_eth_rxfwd_config(struct al_hal_eth_adapter *adapter);

#endif /* _AL_ETH_RXFWD_H */
