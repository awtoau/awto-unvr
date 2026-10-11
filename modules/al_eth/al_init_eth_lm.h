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
 *  Ethernet
 *  @{
 * @file   al_init_eth_lm.h
 *
 * @brief ethernet link management common utilities
 *
 * Common operation example:
 * @code
 *      int main()
 *      {
 *		struct al_eth_lm_context lm_context;
 *		struct al_eth_lm_init_params lm_params;
 *		enum al_eth_lm_link_mode old_mode;
 *		enum al_eth_lm_link_mode new_mode;
 *		al_bool	fault;
 *		al_bool				link_up;
 *		int rc = 0;
 *
 *		lm_params.adapter = hal_adapter;
 *		lm_params.serdes_obj = serdes;
 *		lm_params.grp = grp;
 *		lm_params.lane = lane;
 *		lm_params.sfp_detection = true;
 *		lm_params.link_training = true;
 *		lm_params.rx_equal = true
 *		lm_params.static_values = true;
 *		lm_params.kr_fec_enable = false;
 *		lm_params.eeprom_read = &my_eeprom_read;
 *		lm_params.eeprom_context = context;
 *		lm_params.get_random_byte = &my_rand_byte;
 *		lm_params.default_mode = AL_ETH_LM_MODE_10G_DA;
 *
 *		al_eth_lm_init(&lm_context, &lm_params);
 *
 *		rc = al_eth_lm_link_detection(&lm_context, &fault, &old_mode, &new_mode);
 *		if (fault == false)
 *			return; // in this case the link is still up
 *
 *		if (rc) {
 *			printf("link detection failed on error\n");
 *			return;
 *		}
 *
 *		if (old_mode != new_mode) {
 *			 // perform serdes configuration if needed
 *
 *			 // mac stop / start / config if needed
 *		}
 *
 *		spin_lock(lock);
 *		rc = al_eth_lm_link_establish($lm_context, &link_up);
 *		spin_unlock(lock);
 *		if (rc) {
 *			printf("establish link failed\n");
 *			return;
 *		}
 *
 *		if (link_up)
 *			printf("Link established successfully\n");
 *		else
 *			printf("No signal found. probably the link partner is disconnected\n");
 *      }
 * @endcode
 *
 */

#ifndef __AL_INIT_ETH_LM_H__
#define __AL_INIT_ETH_LM_H__

#include "al_serdes.h"
#include "al_hal_eth.h"

enum al_eth_lm_link_mode {
	AL_ETH_LM_MODE_DISCONNECTED,
	AL_ETH_LM_MODE_10G_OPTIC,
	AL_ETH_LM_MODE_10G_DA,
	AL_ETH_LM_MODE_1G,
	AL_ETH_LM_MODE_25G,
};

enum al_eth_lm_max_speed {
	AL_ETH_LM_MAX_SPEED_MAX,
	AL_ETH_LM_MAX_SPEED_25G,
	AL_ETH_LM_MAX_SPEED_10G,
	AL_ETH_LM_MAX_SPEED_1G,
};

struct al_eth_lm_context {
	struct al_hal_eth_adapter	*adapter;
	struct al_serdes_grp_obj	*serdes_obj;
	enum al_serdes_lane		lane;

	al_bool				tx_param_dirty;
	al_bool				serdes_tx_params_valid;
	struct al_serdes_adv_tx_params	tx_params_override;
	al_bool				rx_param_dirty;
	al_bool				serdes_rx_params_valid;
	struct al_serdes_adv_rx_params	rx_params_override;

	enum al_eth_lm_link_mode	mode;
	al_bool				debug;
	al_bool				static_values;
};

struct al_eth_lm_init_params {
	/* pointer to HAL context */
	struct al_hal_eth_adapter	*adapter;
	/* pointer to serdes object */
	struct al_serdes_grp_obj	*serdes_obj;
	/* serdes lane for this port */
	enum al_serdes_lane		lane;
	/* apply the static TX/RX tables when no override is set */
	al_bool				static_values;
};

/**
 * initialize the context (SerDes object, lane, static-table switch)
 *
 * @param  lm_context pointer to link management context
 * @param  params  parameters passed from upper layer
 *
 * @return 0
 */
int al_eth_lm_init(struct al_eth_lm_context	*lm_context,
		   struct al_eth_lm_init_params	*params);

/**
 * override the default static parameters
 *
 * @param lm_context pointer to link management context
 * @param tx_params pointer to new tx params
 * @param rx_params pointer to new rx params
 *
 * @return  0 in case of success. otherwise on failure.
 **/
int al_eth_lm_static_parameters_override(struct al_eth_lm_context	*lm_context,
					 struct al_serdes_adv_tx_params *tx_params,
					 struct al_serdes_adv_rx_params *rx_params);

/**
 * get the static parameters that are being used
 * if the parameters was override - return the override values
 * else return the current values of the parameters
 *
 * @param  lm_context pointer to link management context
 * @param  tx_params  pointer to new tx params
 * @param  rx_params  pointer to new rx params
 *
 * @return  0 in case of success. otherwise on failure.
 */
int al_eth_lm_static_parameters_get(struct al_eth_lm_context	*lm_context,
				    struct al_serdes_adv_tx_params *tx_params,
				    struct al_serdes_adv_rx_params *rx_params);

/**
 * set the debug flag (read back by nothing since the detect/establish
 * state machine was retired; kept for the lm_debug sysfs attribute)
 *
 * @param lm_context pointer to link management context
 * @param enable     set to true to enable debug mode
 */
void al_eth_lm_debug_mode_set(struct al_eth_lm_context	*lm_context,
			      al_bool			enable);

/**
 * apply the lane's static/override TX and RX SerDes tuning tables.
 * Called by al_eth_phylink.c's pcs_config (10G SFP+ port).
 * No-ops unless *_param_dirty is set.
 *
 * @param lm_context pointer to link management context
 */
void al_eth_serdes_static_tx_params_set(struct al_eth_lm_context *lm_context);
void al_eth_serdes_static_rx_params_set(struct al_eth_lm_context *lm_context);

#endif
