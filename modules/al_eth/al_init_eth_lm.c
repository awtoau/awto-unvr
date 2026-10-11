/*******************************************************************************
Copyright (C) 2015 Annapurna Labs Ltd.

This file may be licensed under the terms of the Annapurna Labs Commercial
License Agreement.

Alternatively, this file can be distributed under the terms of the GNU General
Public License V2 as published by the Free Software Foundation and can be
found at http://www.gnu.org/licenses/gpl-2.0.html

Alternatively, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions are
met:

*     Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

*     Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/**
 *  @{
 * @file   al_init_eth_lm.c
 *
 * @brief ethernet link management common utilities
 *
 */

#include "al_init_eth_lm.h"

static struct al_serdes_adv_tx_params da_tx_params = {
	.override		= AL_TRUE,
	.amp			= 0x1,
	.total_driver_units	= 0x13,
	.c_plus_1		= 0x2,
	.c_plus_2		= 0,
	.c_minus_1		= 0x2,
	.slew_rate		= 0,
};


static struct al_serdes_adv_rx_params da_rx_params = {
	.override		= AL_TRUE,
	.dcgain			= 0x4,
	.dfe_3db_freq		= 0x4,
	.dfe_gain		= 0x3,
	.dfe_first_tap_ctrl	= 0x5,
	.dfe_secound_tap_ctrl	= 0x1,
	.dfe_third_tap_ctrl	= 0x8,
	.dfe_fourth_tap_ctrl	= 0x1,
	.low_freq_agc_gain	= 0x7,
	.precal_code_sel	= 0,
	.high_freq_agc_boost	= 0x1d,
};

/* c_plus_1 (first post-cursor tap) was 0x2: under-equalised TX, root cause of
 * #121. Clean window is 4-7; 0x5 centres it rather than sitting on the cliff.
 * Sweep + measurements: issue #121, scripts/serdes-tx-sweep.py. */
static struct al_serdes_adv_tx_params optic_tx_params = {
	.override		= AL_TRUE,
	.amp			= 0x1,
	.total_driver_units	= 0x13,
	.c_plus_1		= 0x5,
	.c_plus_2		= 0,
	.c_minus_1		= 0,
	.slew_rate		= 0,
};

static struct al_serdes_adv_rx_params optic_rx_params = {
	.override		= AL_TRUE,
	.dcgain			= 0x0,
	.dfe_3db_freq		= 0x7,
	.dfe_gain		= 0x0,
	.dfe_first_tap_ctrl	= 0x0,
	.dfe_secound_tap_ctrl	= 0x8,
	.dfe_third_tap_ctrl	= 0x0,
	.dfe_fourth_tap_ctrl	= 0x8,
	.low_freq_agc_gain	= 0x7,
	.precal_code_sel	= 0,
	.high_freq_agc_boost	= 0x4,
};



void al_eth_serdes_static_tx_params_set(struct al_eth_lm_context *lm_context)
{
	if (lm_context->tx_param_dirty == 0)
		return;

	if (lm_context->serdes_tx_params_valid) {
		lm_context->tx_param_dirty = 0;

		lm_context->tx_params_override.override = AL_TRUE;

		if (!(lm_context->serdes_obj->tx_advanced_params_set)) {
			pr_err("tx_advanced_params_set is not supported for this serdes group\n");
			return;
		}

		lm_context->serdes_obj->tx_advanced_params_set(
					lm_context->serdes_obj,
					lm_context->lane,
					&lm_context->tx_params_override);

	} else if (lm_context->static_values) {
		lm_context->tx_param_dirty = 0;

		if (!(lm_context->serdes_obj->tx_advanced_params_set)) {
			pr_err("tx_advanced_params_set is not supported for this serdes group\n");
			return;
		}

		if (lm_context->mode == AL_ETH_LM_MODE_10G_DA)
			lm_context->serdes_obj->tx_advanced_params_set(
						lm_context->serdes_obj,
						lm_context->lane,
						&da_tx_params);
		else
			lm_context->serdes_obj->tx_advanced_params_set(
						lm_context->serdes_obj,
						lm_context->lane,
						&optic_tx_params);
	}
}

void al_eth_serdes_static_rx_params_set(struct al_eth_lm_context *lm_context)
{
	if (lm_context->rx_param_dirty == 0)
		return;

	if (lm_context->serdes_rx_params_valid) {
		lm_context->rx_param_dirty = 0;

		lm_context->rx_params_override.override = AL_TRUE;

		if (!(lm_context->serdes_obj->rx_advanced_params_set)) {
			pr_err("rx_advanced_params_set is not supported for this serdes group\n");
			return;
		}

		lm_context->serdes_obj->rx_advanced_params_set(
					lm_context->serdes_obj,
					lm_context->lane,
					&lm_context->rx_params_override);

	} else if (lm_context->static_values) {
		lm_context->rx_param_dirty = 0;

		if (!(lm_context->serdes_obj->rx_advanced_params_set)) {
			pr_err("rx_advanced_params_set is not supported for this serdes group\n");
			return;
		}

		if (lm_context->mode == AL_ETH_LM_MODE_10G_DA)
			lm_context->serdes_obj->rx_advanced_params_set(
						lm_context->serdes_obj,
						lm_context->lane,
						&da_rx_params);
		else
			lm_context->serdes_obj->rx_advanced_params_set(
						lm_context->serdes_obj,
						lm_context->lane,
						&optic_rx_params);
	}
}

int al_eth_lm_init(struct al_eth_lm_context	*lm_context,
		   struct al_eth_lm_init_params	*params)
{
	lm_context->adapter = params->adapter;
	lm_context->serdes_obj = params->serdes_obj;
	lm_context->lane = params->lane;
	lm_context->static_values = params->static_values;

	lm_context->mode = AL_ETH_LM_MODE_DISCONNECTED;
	lm_context->serdes_tx_params_valid = AL_FALSE;
	lm_context->serdes_rx_params_valid = AL_FALSE;

	lm_context->rx_param_dirty = 1;
	lm_context->tx_param_dirty = 1;

	return 0;
}

int al_eth_lm_static_parameters_override(struct al_eth_lm_context	*lm_context,
					 struct al_serdes_adv_tx_params *tx_params,
					 struct al_serdes_adv_rx_params *rx_params)
{
	if (tx_params) {
		lm_context->tx_params_override = *tx_params;
		lm_context->tx_param_dirty = 1;
		lm_context->serdes_tx_params_valid = AL_TRUE;
	}

	if (rx_params) {
		lm_context->rx_params_override = *rx_params;
		lm_context->rx_param_dirty = 1;
		lm_context->serdes_rx_params_valid = AL_TRUE;
	}

	return 0;
}

int al_eth_lm_static_parameters_get(struct al_eth_lm_context	*lm_context,
				    struct al_serdes_adv_tx_params *tx_params,
				    struct al_serdes_adv_rx_params *rx_params)
{
	if (tx_params) {
		if (lm_context->serdes_tx_params_valid) {
			*tx_params = lm_context->tx_params_override;
		} else {
			/* NULL on a port with no SerDes (1G is RGMII+external
			 * PHY). Dereferencing it oopsed a plain sysfs read
			 * (#277). The _set path already guards this way. */
			if (!lm_context->serdes_obj ||
			    !lm_context->serdes_obj->tx_advanced_params_get)
				return -ENODEV;
			lm_context->serdes_obj->tx_advanced_params_get(
							lm_context->serdes_obj,
							lm_context->lane,
							tx_params);
		}
	}

	if (rx_params) {
		if (lm_context->serdes_rx_params_valid) {
			*rx_params = lm_context->rx_params_override;
		} else {
			if (!lm_context->serdes_obj ||
			    !lm_context->serdes_obj->rx_advanced_params_get)
				return -ENODEV;
			lm_context->serdes_obj->rx_advanced_params_get(
							lm_context->serdes_obj,
							lm_context->lane,
							rx_params);
		}
	}

	return 0;
}

void al_eth_lm_debug_mode_set(struct al_eth_lm_context	*lm_context,
			      al_bool			enable)
{
	lm_context->debug = enable;
}

