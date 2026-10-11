/* ---- al_eth_retimer_boost_addr ---- */
	al_eth_retimer_boost_addr[AL_ETH_RETIMER_CHANNEL_MAX][AL_ETH_RETIMER_TYPE_MAX] = {
					/* BR_210  |  BR_410 */
	/* AL_ETH_RETIMER_CHANNEL_A */	{0xf,		0x1a	},
	/* AL_ETH_RETIMER_CHANNEL_B */	{0x16,		0x18	},
	/* AL_ETH_RETIMER_CHANNEL_C */	{0x0,		0x16	},
	/* AL_ETH_RETIMER_CHANNEL_D */	{0x0,		0x14	},
};

/* ---- al_eth_retimer_boost_lens ---- */
static const uint32_t al_eth_retimer_boost_lens[RETIMER_LENS_MAX] = {0, 1, 2, 3, 5};

/* ---- retimer_ds25_25g_mode_tx_ch ---- */
static struct retimer_config_reg retimer_ds25_25g_mode_tx_ch[] = {
	{.addr = 0x0A, .value = 0x0C, .mask = 0xff },
	{.addr = 0x2F, .value = 0x54, .mask = 0xff },
	{.addr = 0x31, .value = 0x20, .mask = 0xff },
	{.addr = 0x1E, .value = 0xE9, .mask = 0xff },
	{.addr = 0x1F, .value = 0x0B, .mask = 0xff },
	{.addr = 0xA6, .value = 0x43, .mask = 0xff },
	{.addr = 0x2A, .value = 0x5A, .mask = 0xff },
	{.addr = 0x2B, .value = 0x0A, .mask = 0xff },
	{.addr = 0x2C, .value = 0xF6, .mask = 0xff },
	{.addr = 0x70, .value = 0x05, .mask = 0xff },
	{.addr = 0x6A, .value = 0x21, .mask = 0xff },
	{.addr = 0x35, .value = 0x0F, .mask = 0xff },
	{.addr = 0x12, .value = 0x83, .mask = 0xff },
	{.addr = 0x9C, .value = 0x24, .mask = 0xff },
	{.addr = 0x98, .value = 0x00, .mask = 0xff },
	{.addr = 0x42, .value = 0x50, .mask = 0xff },
	{.addr = 0x44, .value = 0x90, .mask = 0xff },
	{.addr = 0x45, .value = 0xC0, .mask = 0xff },
	{.addr = 0x46, .value = 0xD0, .mask = 0xff },
	{.addr = 0x47, .value = 0xD1, .mask = 0xff },
	{.addr = 0x48, .value = 0xD5, .mask = 0xff },
	{.addr = 0x49, .value = 0xD8, .mask = 0xff },
	{.addr = 0x4A, .value = 0xEA, .mask = 0xff },
	{.addr = 0x4B, .value = 0xF7, .mask = 0xff },
	{.addr = 0x4C, .value = 0xFD, .mask = 0xff },
	{.addr = 0x8E, .value = 0x00, .mask = 0xff },
	{.addr = 0x3D, .value = 0x94, .mask = 0xff },
	{.addr = 0x3F, .value = 0x40, .mask = 0xff },
	{.addr = 0x3E, .value = 0x43, .mask = 0xff },
	{.addr = 0x0A, .value = 0x00, .mask = 0xff },
};

/* ---- retimer_ds25_25g_mode_rx_ch ---- */
static struct retimer_config_reg retimer_ds25_25g_mode_rx_ch[] = {
	{.addr = 0x0A, .value = 0x0C, .mask = 0xff},
	{.addr = 0x2F, .value = 0x54, .mask = 0xff},
	{.addr = 0x31, .value = 0x40, .mask = 0xff},
	{.addr = 0x1E, .value = 0xE3, .mask = 0xff},
	{.addr = 0x1F, .value = 0x0B, .mask = 0xff},
	{.addr = 0xA6, .value = 0x43, .mask = 0xff},
	{.addr = 0x2A, .value = 0x5A, .mask = 0xff},
	{.addr = 0x2B, .value = 0x0A, .mask = 0xff},
	{.addr = 0x2C, .value = 0xF6, .mask = 0xff},
	{.addr = 0x70, .value = 0x05, .mask = 0xff},
	{.addr = 0x6A, .value = 0x21, .mask = 0xff},
	{.addr = 0x35, .value = 0x0F, .mask = 0xff},
	{.addr = 0x12, .value = 0x83, .mask = 0xff},
	{.addr = 0x9C, .value = 0x24, .mask = 0xff},
	{.addr = 0x98, .value = 0x00, .mask = 0xff},
	{.addr = 0x42, .value = 0x50, .mask = 0xff},
	{.addr = 0x44, .value = 0x90, .mask = 0xff},
	{.addr = 0x45, .value = 0xC0, .mask = 0xff},
	{.addr = 0x46, .value = 0xD0, .mask = 0xff},
	{.addr = 0x47, .value = 0xD1, .mask = 0xff},
	{.addr = 0x48, .value = 0xD5, .mask = 0xff},
	{.addr = 0x49, .value = 0xD8, .mask = 0xff},
	{.addr = 0x4A, .value = 0xEA, .mask = 0xff},
	{.addr = 0x4B, .value = 0xF7, .mask = 0xff},
	{.addr = 0x4C, .value = 0xFD, .mask = 0xff},
	{.addr = 0x8E, .value = 0x00, .mask = 0xff},
	{.addr = 0x3D, .value = 0x94, .mask = 0xff},
	{.addr = 0x3F, .value = 0x40, .mask = 0xff},
	{.addr = 0x3E, .value = 0x43, .mask = 0xff},
	{.addr = 0x0A, .value = 0x00, .mask = 0xff},
};

/* ---- retimer_ds25_auto_mode_tx_ch ---- */
static struct retimer_config_reg retimer_ds25_auto_mode_tx_ch[] = {
	{.addr = 0x0A, .value = 0x0C, .mask = 0xff },
	{.addr = 0x2F, .value = 0x64, .mask = 0xff },
	{.addr = 0x31, .value = 0x20, .mask = 0xff },
	{.addr = 0x1E, .value = 0xE9, .mask = 0xff },
	{.addr = 0x1F, .value = 0x0B, .mask = 0xff },
	{.addr = 0xA6, .value = 0x43, .mask = 0xff },
	{.addr = 0x2A, .value = 0x5A, .mask = 0xff },
	{.addr = 0x2B, .value = 0x0A, .mask = 0xff },
	{.addr = 0x2C, .value = 0xF6, .mask = 0xff },
	{.addr = 0x70, .value = 0x05, .mask = 0xff },
	{.addr = 0x6A, .value = 0x23, .mask = 0xff },
	{.addr = 0x35, .value = 0x0F, .mask = 0xff },
	{.addr = 0x12, .value = 0x83, .mask = 0xff },
	{.addr = 0x9C, .value = 0x24, .mask = 0xff },
	{.addr = 0x98, .value = 0x00, .mask = 0xff },
	{.addr = 0x42, .value = 0x50, .mask = 0xff },
	{.addr = 0x44, .value = 0x90, .mask = 0xff },
	{.addr = 0x45, .value = 0xC0, .mask = 0xff },
	{.addr = 0x46, .value = 0xD0, .mask = 0xff },
	{.addr = 0x47, .value = 0xD1, .mask = 0xff },
	{.addr = 0x48, .value = 0xD5, .mask = 0xff },
	{.addr = 0x49, .value = 0xD8, .mask = 0xff },
	{.addr = 0x4A, .value = 0xEA, .mask = 0xff },
	{.addr = 0x4B, .value = 0xF7, .mask = 0xff },
	{.addr = 0x4C, .value = 0xFD, .mask = 0xff },
	{.addr = 0x8E, .value = 0x00, .mask = 0xff },
	{.addr = 0x3D, .value = 0x94, .mask = 0xff },
	{.addr = 0x3F, .value = 0x40, .mask = 0xff },
	{.addr = 0x3E, .value = 0x43, .mask = 0xff },
	{.addr = 0x0A, .value = 0x00, .mask = 0xff },
};

/* ---- retimer_ds25_auto_mode_rx_ch ---- */
static struct retimer_config_reg retimer_ds25_auto_mode_rx_ch[] = {
	{.addr = 0x0A, .value = 0x0C, .mask = 0xff},
	{.addr = 0x2F, .value = 0x64, .mask = 0xff},
	{.addr = 0x31, .value = 0x40, .mask = 0xff},
	{.addr = 0x1E, .value = 0xE3, .mask = 0xff},
	{.addr = 0x1F, .value = 0x0B, .mask = 0xff},
	{.addr = 0xA6, .value = 0x43, .mask = 0xff},
	{.addr = 0x2A, .value = 0x5A, .mask = 0xff},
	{.addr = 0x2B, .value = 0x0A, .mask = 0xff},
	{.addr = 0x2C, .value = 0xF6, .mask = 0xff},
	{.addr = 0x70, .value = 0x05, .mask = 0xff},
	{.addr = 0x6A, .value = 0x23, .mask = 0xff},
	{.addr = 0x35, .value = 0x0F, .mask = 0xff},
	{.addr = 0x12, .value = 0x83, .mask = 0xff},
	{.addr = 0x9C, .value = 0x24, .mask = 0xff},
	{.addr = 0x98, .value = 0x00, .mask = 0xff},
	{.addr = 0x42, .value = 0x50, .mask = 0xff},
	{.addr = 0x44, .value = 0x90, .mask = 0xff},
	{.addr = 0x45, .value = 0xC0, .mask = 0xff},
	{.addr = 0x46, .value = 0xD0, .mask = 0xff},
	{.addr = 0x47, .value = 0xD1, .mask = 0xff},
	{.addr = 0x48, .value = 0xD5, .mask = 0xff},
	{.addr = 0x49, .value = 0xD8, .mask = 0xff},
	{.addr = 0x4A, .value = 0xEA, .mask = 0xff},
	{.addr = 0x4B, .value = 0xF7, .mask = 0xff},
	{.addr = 0x4C, .value = 0xFD, .mask = 0xff},
	{.addr = 0x8E, .value = 0x00, .mask = 0xff},
	{.addr = 0x3D, .value = 0x94, .mask = 0xff},
	{.addr = 0x3F, .value = 0x40, .mask = 0xff},
	{.addr = 0x3E, .value = 0x43, .mask = 0xff},
	{.addr = 0x0A, .value = 0x00, .mask = 0xff},
};

/* ---- retimer_ds25_10g_mode ---- */
static struct retimer_config_reg retimer_ds25_10g_mode[] = {
	/* Assert CDR reset (6.3) */
	{.addr = 0x0A, .value = 0x0C, .mask = 0x0C},
	/* Select 10.3125Gbps standard rate mode (6.6) */
	{.addr = 0x2F, .value = 0x00, .mask = 0xF0},
	/* Enable loop filter auto-adjust */
	{.addr = 0x1F, .value = 0x08, .mask = 0x08},
	/* Set Adapt Mode 1 (6.13) */
	{.addr = 0x31, .value = 0x20, .mask = 0x60},
	/* Disable the DFE since most applications do not need it (6.18) */
	{.addr = 0x1E, .value = 0x08, .mask = 0x08},
	/* Release CDR reset (6.4) */
	{.addr = 0x0A, .value = 0x00, .mask = 0x0C},
	/* Enable FIR (6.12) */
	{.addr = 0x3D, .value = 0x80, .mask = 0x80},
	/* Set Main-cursor tap sign to positive (6.12) */
	{.addr = 0x3D, .value = 0x00, .mask = 0x40},
	/* Set Post-cursor tap sign to negative (6.12) */
	{.addr = 0x3F, .value = 0x40, .mask = 0x40},
	/* Set Pre-cursor tap sign to negative (6.12) */
	{.addr = 0x3E, .value = 0x40, .mask = 0x40},
	/* Set Main-cursor tap magnitude to 13 (6.12) */
	{.addr = 0x3D, .value = 0x0D, .mask = 0x1F},
};

/* ---- retimer ---- */
static struct al_eth_lm_retimer retimer[] = {
	{.config = al_eth_lm_retimer_br210_config, .signal_detect = NULL,
		.reset = NULL, .cdr_lock = NULL, .rx_adaptation = NULL, .speed_get = NULL},
	{.config = al_eth_lm_retimer_br410_config, .signal_detect = NULL,
		.reset = NULL, .cdr_lock = NULL, .rx_adaptation = NULL, .speed_get = NULL},
	{.config = al_eth_lm_retimer_ds25_full_config,
		.signal_detect = al_eth_lm_retimer_ds25_signal_detect,
		.reset = al_eth_lm_retimer_ds25_cdr_reset,
		.cdr_lock = al_eth_lm_retimer_ds25_cdr_lock,
		.rx_adaptation = al_eth_lm_retimer_25g_rx_adaptation_step,
		.speed_get = al_eth_lm_retimer_ds25_speed_get},
};

/* ---- elapsed_time_msec ---- */
static inline al_bool elapsed_time_msec(unsigned int current_time,
					unsigned int *start_time,
					unsigned int time_interval)
{
	if ((current_time - *start_time) >= time_interval) {
		*start_time = current_time;
		return AL_TRUE;
	}

	return AL_FALSE;
}

/* ---- al_eth_sfp_detect ---- */
static int al_eth_sfp_detect(struct al_eth_lm_context	*lm_context,
			     enum al_eth_lm_link_mode	*new_mode)
{
	int rc = 0;
	uint8_t sfp_10g;
	uint8_t sfp_1g;
	uint8_t sfp_cable_tech;
	uint8_t sfp_da_len;
	uint8_t signal_rate;

	do {
		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  SFP_I2C_HEADER_10G_IDX, &sfp_10g);
		if (rc)
			break;

		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  SFP_I2C_HEADER_1G_IDX, &sfp_1g);
		if (rc)
			break;

		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  SFP_I2C_HEADER_10G_DA_IDX,
					  &sfp_cable_tech);
		if (rc)
			break;

		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  SFP_I2C_HEADER_10G_DA_LEN_IDX,
					  &sfp_da_len);

		if (rc)
			break;

		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  SFP_I2C_HEADER_SIGNAL_RATE,
					  &signal_rate);
	} while (0);

	if (rc) {
		if (rc == -ETIMEDOUT) {
			/* ETIMEDOUT is returned when no SFP is connected */
			if (lm_context->mode != AL_ETH_LM_MODE_DISCONNECTED)
				lm_debug("%s: SFP Disconnected\n", __func__);
			*new_mode = AL_ETH_LM_MODE_DISCONNECTED;
		} else {
			return rc;
		}
	} else if (sfp_cable_tech & (SFP_10G_DA_PASSIVE | SFP_10G_DA_ACTIVE)) {
		if ((signal_rate >= SFP_MIN_SIGNAL_RATE_25G) &&
			((lm_context->max_speed == AL_ETH_LM_MAX_SPEED_25G) ||
			(lm_context->max_speed == AL_ETH_LM_MAX_SPEED_MAX)))
			*new_mode = AL_ETH_LM_MODE_25G;
		else if ((signal_rate >= SFP_MIN_SIGNAL_RATE_10G) &&
			((lm_context->max_speed == AL_ETH_LM_MAX_SPEED_10G) ||
			(lm_context->max_speed == AL_ETH_LM_MAX_SPEED_MAX)))
			*new_mode = AL_ETH_LM_MODE_10G_DA;
		else
			*new_mode = AL_ETH_LM_MODE_1G;

		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED)) {
			lm_debug("%s: %s DAC (%d M) detected (max signal rate %d)\n",
				__func__,
				(sfp_cable_tech & SFP_10G_DA_PASSIVE) ? "Passive" : "Active",
				sfp_da_len,
				signal_rate);
		}

		/* for active direct attached need to use len 0 in the retimer configuration */
		lm_context->da_len = (sfp_cable_tech & SFP_10G_DA_PASSIVE) ? sfp_da_len : 0;
	} else if (sfp_10g != 0) {
		*new_mode = AL_ETH_LM_MODE_10G_OPTIC;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			lm_debug("%s: 10 SFP detected\n", __func__);
	} else if (sfp_1g != 0) {
		*new_mode = AL_ETH_LM_MODE_1G;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			lm_debug("%s: 1G SFP detected\n", __func__);
	} else {
		*new_mode = lm_context->default_mode;
		lm_context->da_len = lm_context->default_dac_len;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			pr_warn("%s: unknown SFP inserted. eeprom content: 10G compliance 0x%x, 1G compliance 0x%x, sfp+cable 0x%x. default to %s\n",
				__func__, sfp_10g, sfp_1g, sfp_cable_tech,
				al_eth_lm_mode_convert_to_str(lm_context->default_mode));
	}

	if ((lm_context->sfp_detect_force_mode) && (*new_mode != AL_ETH_LM_MODE_DISCONNECTED) &&
	    (*new_mode != lm_context->default_mode)) {
			if (lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED)
				pr_warn("%s: Force mode to default (%s). mode based of the SFP EEPROM %s\n",
				__func__, al_eth_lm_mode_convert_to_str(lm_context->default_mode),
				al_eth_lm_mode_convert_to_str(*new_mode));

		*new_mode = lm_context->default_mode;
	}

	lm_context->mode = *new_mode;

	return 0;
}

/* ---- al_eth_qsfp_detect ---- */
static int al_eth_qsfp_detect(struct al_eth_lm_context	*lm_context,
			      enum al_eth_lm_link_mode	*new_mode)
{
	int rc = 0;
	uint8_t qsfp_comp_code;
	uint8_t qsfp_da_len;

	do {
		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  QSFP_COMPLIANCE_CODE_IDX, &qsfp_comp_code);
		if (rc)
			break;

		rc = lm_context->i2c_read(lm_context->i2c_context,
					  lm_context->sfp_bus_id,
					  lm_context->sfp_i2c_addr,
					  QSFP_CABLE_LEN_IDX, &qsfp_da_len);
		if (rc)
			break;
	} while (0);

	if (rc) {
		if (rc == -ETIMEDOUT) {
			/* ETIMEDOUT is returned when no SFP is connected */
			lm_debug("%s: SFP Disconnected\n", __func__);
			*new_mode = AL_ETH_LM_MODE_DISCONNECTED;
		} else {
			return rc;
		}
	} else if (qsfp_comp_code & QSFP_COMPLIANCE_CODE_DAC) {
		*new_mode = AL_ETH_LM_MODE_10G_DA;
		lm_context->da_len = qsfp_da_len;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			lm_debug("%s: 10G passive DAC (%d M) detected\n", __func__, qsfp_da_len);
	} else if (qsfp_comp_code & QSFP_COMPLIANCE_CODE_OPTIC) {
		*new_mode = AL_ETH_LM_MODE_10G_OPTIC;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			lm_debug("%s: 10G optic module detected\n", __func__);
	} else {
		*new_mode = lm_context->default_mode;
		lm_context->da_len = lm_context->default_dac_len;
		if ((lm_context->mode != *new_mode) &&
			(lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED))
			pr_warn("%s: unknown QSFP inserted. eeprom content: 10G compliance 0x%x."
			" default to %s\n",
			__func__, qsfp_comp_code,
			al_eth_lm_mode_convert_to_str(lm_context->default_mode));
	}

	lm_context->mode = *new_mode;

	return 0;
}

/* ---- al_eth_module_detect ---- */
static int al_eth_module_detect(struct al_eth_lm_context	*lm_context,
				enum al_eth_lm_link_mode	*new_mode)
{
	int i;
	int rc = 0;
	uint8_t module_idx;
	int sfp_present = SFP_PRESENT;
	al_bool use_gpio_present = AL_FALSE;
	al_bool bounce_detected = AL_FALSE;

	if ((lm_context->gpio_get) && (lm_context->gpio_present != 0)) {
		int sfp_present_debounce;

		use_gpio_present = AL_TRUE;
		sfp_present = lm_context->gpio_get(lm_context->gpio_present);
		for (i = 0; i < SFP_PRESENT_GPIO_DEBOUNCE_ITERS; i++) {
			al_udelay(1);
			sfp_present_debounce = lm_context->gpio_get(lm_context->gpio_present);
			if (sfp_present_debounce != sfp_present) {
				bounce_detected = AL_TRUE;
				break;
			}
		}
	}

	if (bounce_detected) {
		*new_mode = lm_context->mode;

		return 0;
	}

	if (sfp_present == SFP_NOT_PRESENT) {
		if (lm_context->mode != AL_ETH_LM_MODE_DISCONNECTED)
			lm_debug("%s: SFP not present\n", __func__);
		*new_mode = AL_ETH_LM_MODE_DISCONNECTED;

		return 0;
	} else if (use_gpio_present && lm_context->sfp_detect_force_mode) {
		if (lm_context->mode == AL_ETH_LM_MODE_DISCONNECTED)
			lm_debug("%s: Force mode to default (%s)\n",
			__func__, al_eth_lm_mode_convert_to_str(lm_context->default_mode));
		*new_mode = lm_context->default_mode;

		return 0;
	}

	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->sfp_bus_id,
				  lm_context->sfp_i2c_addr,
				  MODULE_IDENTIFIER_IDX, &module_idx);
	if (rc) {
		if (rc == -ETIMEDOUT) {
			if (use_gpio_present) {
				sfp_present = lm_context->gpio_get(lm_context->gpio_present);
				if (sfp_present == SFP_PRESENT) {
					pr_warn("%s: SFP present, but i2c read from SFP failed. Force mode to default (%s).\n",
						__func__, al_eth_lm_mode_convert_to_str(
						lm_context->default_mode));
					*new_mode = lm_context->default_mode;
				} else {
					lm_debug("%s: SFP disconnected during i2c read\n",
						__func__);
					*new_mode = AL_ETH_LM_MODE_DISCONNECTED;

					return 0;
				}
			} else {
				/* ETIMEDOUT is returned when no SFP is connected */
				if (lm_context->mode != AL_ETH_LM_MODE_DISCONNECTED)
					lm_debug("%s: SFP Disconnected\n", __func__);
				*new_mode = AL_ETH_LM_MODE_DISCONNECTED;
			}
			return 0;
		} else {
			return rc;
		}
	}

	if (module_idx == MODULE_IDENTIFIER_QSFP)
		return al_eth_qsfp_detect(lm_context, new_mode);
	else
		return al_eth_sfp_detect(lm_context, new_mode);

	return 0;
}

/* ---- al_eth_rx_equal_run ---- */
static int al_eth_rx_equal_run(struct al_eth_lm_context	*lm_context)
{
	struct al_serdes_adv_rx_params rx_params;
	int dcgain;
	int best_dcgain = -1;
	int i;
	int best_score  = -1;
	int test_score = -1;

	rx_params.override = AL_FALSE;
	lm_context->serdes_obj->rx_advanced_params_set(
					lm_context->serdes_obj,
					lm_context->lane,
					&rx_params);

	lm_debug("score | dcgain | dfe3db | dfegain | tap1 | tap2 | tap3 | tap4 | low freq | high freq\n");

	for (dcgain = 0; dcgain < AL_ETH_LM_MAX_DCGAIN; dcgain++) {
		lm_context->serdes_obj->dcgain_set(
					lm_context->serdes_obj,
					dcgain);

		test_score = lm_context->serdes_obj->rx_equalization(
					lm_context->serdes_obj,
					lm_context->lane);

		if (test_score < 0) {
			pr_warn("serdes rx equalization failed on error\n");
			return test_score;
		}

		if (test_score > best_score) {
			best_score = test_score;
			best_dcgain = dcgain;
		}

		lm_context->serdes_obj->rx_advanced_params_get(
					lm_context->serdes_obj,
					lm_context->lane,
					&rx_params);

		lm_debug("%6d|%8x|%8x|%9x|%6x|%6x|%6x|%6x|%10x|%10x|\n",
			test_score, rx_params.dcgain, rx_params.dfe_3db_freq,
			rx_params.dfe_gain, rx_params.dfe_first_tap_ctrl,
			rx_params.dfe_secound_tap_ctrl, rx_params.dfe_third_tap_ctrl,
			rx_params.dfe_fourth_tap_ctrl, rx_params.low_freq_agc_gain,
			rx_params.high_freq_agc_boost);

	}

	lm_context->serdes_obj->dcgain_set(
					lm_context->serdes_obj,
					best_dcgain);

	best_score = -1;
	for(i = 0; i < AL_ETH_LM_EQ_ITERATIONS; i++) {
		test_score = lm_context->serdes_obj->rx_equalization(
						lm_context->serdes_obj,
						lm_context->lane);

		if (test_score < 0) {
			pr_warn("serdes rx equalization failed on error\n");
			return test_score;
		}

		if (test_score > best_score) {
			best_score = test_score;
			lm_context->serdes_obj->rx_advanced_params_get(
						lm_context->serdes_obj,
						lm_context->lane,
						&rx_params);
		}
	}

	rx_params.precal_code_sel = 0;
	rx_params.override = AL_TRUE;
	lm_context->serdes_obj->rx_advanced_params_set(
					lm_context->serdes_obj,
					lm_context->lane,
					&rx_params);

	lm_debug("-------------------- best dcgain %d ------------------------------------\n", best_dcgain);
	lm_debug("%6d|%8x|%8x|%9x|%6x|%6x|%6x|%6x|%10x|%10x|\n",
		best_score, rx_params.dcgain, rx_params.dfe_3db_freq,
		rx_params.dfe_gain, rx_params.dfe_first_tap_ctrl,
		rx_params.dfe_secound_tap_ctrl, rx_params.dfe_third_tap_ctrl,
		rx_params.dfe_fourth_tap_ctrl, rx_params.low_freq_agc_gain,
		rx_params.high_freq_agc_boost);

	return 0;

}

/* ---- al_eth_lm_retimer_br210_config ---- */
static int al_eth_lm_retimer_br210_config(struct al_eth_lm_context	*lm_context)
{
	int i;
	int rc = 0;
	uint8_t boost = 0;
	uint32_t boost_addr =
		al_eth_retimer_boost_addr[lm_context->retimer_channel][lm_context->retimer_type];

	if (lm_context->mode != AL_ETH_LM_MODE_10G_DA) {
		boost = al_eth_retimer_params[0][lm_context->retimer_type].eq_boost;
	} else {
		for (i = 0; i < RETIMER_LENS_MAX; i++) {
			if (lm_context->da_len <= al_eth_retimer_boost_lens[i]) {
				boost = al_eth_retimer_params[i][lm_context->retimer_type].eq_boost;
				break;
			}

		}

		if (i == RETIMER_LENS_MAX)
			boost = al_eth_retimer_params[RETIMER_LENS_MAX][lm_context->retimer_type].
				eq_boost;
	}

	lm_debug("config retimer boost in channel %d (addr %x) to 0x%x\n",
		 lm_context->retimer_channel, boost_addr, boost);

	rc = lm_context->i2c_write(lm_context->i2c_context,
				   lm_context->retimer_bus_id,
				   lm_context->retimer_i2c_addr,
				   boost_addr,
				   boost);

	if (rc) {
		pr_err("%s: Error occurred (%d) while writing retimer configuration (bus-id %x i2c-addr %x)\n",
		       __func__, rc, lm_context->retimer_bus_id, lm_context->retimer_i2c_addr);
		return rc;
	}


	return 0;
}

/* ---- al_eth_lm_retimer_br410_config ---- */
static int al_eth_lm_retimer_br410_config(struct al_eth_lm_context	*lm_context)
{
	int rc = 0;
	struct al_eth_retimer_params retimer_params;
	uint32_t boost_addr;
	uint8_t reg;
	uint8_t vod_val;

	al_assert((((int)lm_context->retimer_channel) >= 0) && (lm_context->retimer_channel < 4));

	al_memset(&retimer_params, 0, sizeof(retimer_params));

	boost_addr =
		al_eth_retimer_boost_addr[lm_context->retimer_channel][lm_context->retimer_type];

	/* config eq_boost value */
	if ((lm_context->da_len == 1) || (lm_context->da_len == 3))
		retimer_params =
			al_eth_retimer_params[lm_context->da_len][lm_context->retimer_type];
	else {
		pr_err("%s: Unsupported cable length (%dm) for retimer_type %d on this board!\n",
			__func__, lm_context->da_len, lm_context->retimer_type);
		pr_err("Currently only 1m and 3m cables are supported\n");
		return -EIO;
	}

	if (lm_context->da_len <= 1)
		retimer_params =
			al_eth_retimer_params[1][lm_context->retimer_type];
	else
		retimer_params =
			al_eth_retimer_params[3][lm_context->retimer_type];

	if ((lm_context->da_len != 1) || (lm_context->da_len != 3)) {
		pr_warn("%s: Uncalibrated cable length (%dm) for retimer_type %d on this board is being used! Link might not be optimal.\n",
			__func__, lm_context->da_len, lm_context->retimer_type);
		pr_warn("Retimer params have been calibrated for 1m and 3m cables only\n");
	}

	lm_debug("%s: Configuring eq_boost for retimer_type %d in channel %d (addr 0x%x) to 0x%x\n",
		__func__, lm_context->retimer_type, lm_context->retimer_channel, boost_addr,
		retimer_params.eq_boost);

	rc = lm_context->i2c_write(lm_context->i2c_context,
				   lm_context->retimer_bus_id,
				   lm_context->retimer_i2c_addr,
				   boost_addr,
				   retimer_params.eq_boost);

	if (rc) {
		pr_err("%s: Error occurred (%d) while writing to eq_boost to retimer (bus-id %x i2c-addr %x)\n",
		       __func__, rc, lm_context->retimer_bus_id, lm_context->retimer_i2c_addr);
		return rc;
	}

	/* config de_emphasis value */
	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  RETIMER_BR410_DE_EMPHASIS_CTRL_REG,
				  &reg);

	if (rc) {
		pr_err("%s: Error occurred (%d) while reading de_emphasis control reg from retimer (bus-id 0x%x i2c-addr 0x%x internal_reg 0x%x)\n",
			__func__, rc, lm_context->retimer_bus_id, lm_context->retimer_i2c_addr,
			RETIMER_BR410_DE_EMPHASIS_CTRL_REG);
		return rc;
	}

	AL_REG_FIELD_SET(reg,
			 RETIMER_BR410_DE_EMPHASIS_CTRL_MASK(lm_context->retimer_channel),
			 RETIMER_BR410_DE_EMPHASIS_CTRL_SHIFT(lm_context->retimer_channel),
			 retimer_params.de_emphasis);

	lm_debug("%s: Configuring de_emphasis for retimer_type %d in channel %d to 0x%x\n",
		__func__, lm_context->retimer_type, lm_context->retimer_channel,
		retimer_params.de_emphasis);

	rc = lm_context->i2c_write(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  RETIMER_BR410_DE_EMPHASIS_CTRL_REG,
				  reg);

	if (rc) {
		pr_err("%s: Error occurred (%d) while writing to de_emphasis control reg of retimer (bus-id 0x%x i2c-addr 0x%x internal_reg 0x%x)\n",
			__func__, rc, lm_context->retimer_bus_id, lm_context->retimer_i2c_addr,
			RETIMER_BR410_DE_EMPHASIS_CTRL_REG);
		return rc;
	}

	/* config Vod value */
	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  RETIMER_BR410_DRIVER_VOD_CTRL_REG,
				  &reg);

	if (rc) {
		pr_err("%s: Error occurred (%d) while reading Vod control reg from retimer (bus-id 0x%x i2c-addr 0x%x internal_reg 0x%x)\n",
			__func__, rc, lm_context->retimer_bus_id, lm_context->retimer_i2c_addr,
			RETIMER_BR410_DRIVER_VOD_CTRL_REG);
		return rc;
	}

	vod_val = AL_REG_FIELD_GET(reg, RETIMER_BR410_DRIVER_VOD_CTRL_MASK,
				RETIMER_BR410_DRIVER_VOD_CTRL_SHIFT);

	if (vod_val != retimer_params.vod) {
		AL_REG_FIELD_SET(reg,
				RETIMER_BR410_DRIVER_VOD_CTRL_MASK,
				RETIMER_BR410_DRIVER_VOD_CTRL_SHIFT,
				retimer_params.vod);

		lm_debug("%s: Configuring Vod for retimer_type %d in channel %d to 0x%x\n",
		__func__, lm_context->retimer_type, lm_context->retimer_channel,
		retimer_params.vod);

		rc = lm_context->i2c_write(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  RETIMER_BR410_DRIVER_VOD_CTRL_REG,
				  reg);

		if (rc) {
			pr_err("%s: Error occurred (%d) while writing to Vod control reg in retimer (bus-id 0x%x i2c-addr 0x%x internal_reg 0x%x)\n",
				__func__, rc, lm_context->retimer_bus_id,
				lm_context->retimer_i2c_addr,
				RETIMER_BR410_DRIVER_VOD_CTRL_REG);
		return rc;
		}
	}

	return 0;
}

/* ---- al_eth_lm_retimer_ds25_write_reg ---- */
static int al_eth_lm_retimer_ds25_write_reg(struct al_eth_lm_context	*lm_context,
					    uint8_t			reg_addr,
					    uint8_t			reg_mask,
					    uint8_t			reg_value)
{
	uint8_t reg = 0;
	int rc;

	al_assert((reg_mask & reg_value) == reg_value);

	if (reg_mask != 0xff) {
		rc = lm_context->i2c_read(lm_context->i2c_context,
					lm_context->retimer_bus_id,
					lm_context->retimer_i2c_addr,
					reg_addr,
					&reg);

		if (rc)
			return -EIO;

		reg &= ~(reg_mask);
	}

	reg |= reg_value;

	rc = lm_context->i2c_write(lm_context->i2c_context,
				   lm_context->retimer_bus_id,
				   lm_context->retimer_i2c_addr,
				   reg_addr,
				   reg);

	if (rc)
		return -EIO;

	return 0;
}

/* ---- al_eth_lm_retimer_ds25_channel_select ---- */
static int al_eth_lm_retimer_ds25_channel_select(struct al_eth_lm_context	*lm_context,
						 uint8_t			channel)
{
	int rc = 0;

	/* Write to specific channel */
	rc = al_eth_lm_retimer_ds25_write_reg(lm_context,
					      LM_DS25_CHANNEL_EN_REG,
					      LM_DS25_CHANNEL_EN_MASK,
					      LM_DS25_CHANNEL_EN_VAL);

	if (rc)
		return rc;

	rc = al_eth_lm_retimer_ds25_write_reg(lm_context,
					      LM_DS25_CHANNEL_SEL_REG,
					      LM_DS25_CHANNEL_SEL_MASK,
					      (1 << channel));

	return rc;
}

/* ---- al_eth_lm_retimer_ds25_channel_config ---- */
static int al_eth_lm_retimer_ds25_channel_config(struct al_eth_lm_context	*lm_context,
						 uint8_t			channel,
						 struct retimer_config_reg	*config,
						 uint8_t			config_size)
{
	uint8_t i;
	int rc;

	rc = al_eth_lm_retimer_ds25_channel_select(lm_context, channel);
	if (rc)
		goto config_error;

	for (i = 0; i < config_size; i++) {
		rc = al_eth_lm_retimer_ds25_write_reg(lm_context,
						      config[i].addr,
						      config[i].mask,
						      config[i].value);

		if (rc)
			goto config_error;
	}

	lm_debug("%s: retimer channel config done for channel %d\n", __func__, channel);

	return 0;

config_error:
	pr_err("%s: failed to access to the retimer\n", __func__);

	return rc;
}

/* ---- al_eth_lm_retimer_ds25_cdr_reset ---- */
static int al_eth_lm_retimer_ds25_cdr_reset(struct al_eth_lm_context *lm_context, uint32_t channel)
{
	int rc;

	lm_debug("Perform CDR reset to channel %d\n", channel);

	rc = al_eth_lm_retimer_ds25_channel_select(lm_context, channel);
	if (rc)
		goto config_error;

	rc = al_eth_lm_retimer_ds25_write_reg(lm_context,
					      LM_DS25_CDR_RESET_REG,
					      LM_DS25_CDR_RESET_MASK,
					      LM_DS25_CDR_RESET_ASSERT);

	if (rc)
		goto config_error;

	rc = al_eth_lm_retimer_ds25_write_reg(lm_context,
					      LM_DS25_CDR_RESET_REG,
					      LM_DS25_CDR_RESET_MASK,
					      LM_DS25_CDR_RESET_RELEASE);

	if (rc)
		goto config_error;

	return 0;

config_error:
	pr_err("%s: failed to access to the retimer\n", __func__);

	return rc;
}

/* ---- al_eth_lm_retimer_ds25_signal_detect ---- */
static al_bool al_eth_lm_retimer_ds25_signal_detect(struct al_eth_lm_context *lm_context,
						    uint32_t channel)
{
	int rc = 0;
	uint8_t reg;

	rc = al_eth_lm_retimer_ds25_channel_select(lm_context, channel);
	if (rc)
		goto config_error;

	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  LM_DS25_SIGNAL_DETECT_REG,
				  &reg);

	if (rc)
		goto config_error;

	if (reg & LM_DS25_SIGNAL_DETECT_MASK)
		return AL_TRUE;

	return AL_FALSE;

config_error:
	pr_err("%s: failed to access to the retimer\n", __func__);

	return AL_FALSE;
}

/* ---- al_eth_lm_retimer_ds25_cdr_lock ---- */
static al_bool al_eth_lm_retimer_ds25_cdr_lock(struct al_eth_lm_context *lm_context,
					       uint32_t channel)
{
	int rc = 0;
	uint8_t reg;

	rc = al_eth_lm_retimer_ds25_channel_select(lm_context, channel);
	if (rc)
		goto config_error;

	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  LM_DS25_CDR_LOCK_REG,
				  &reg);

	if (rc)
		goto config_error;

	if (reg & LM_DS25_CDR_LOCK_MASK)
		return AL_TRUE;

	return AL_FALSE;

config_error:
	pr_err("%s: failed to access to the retimer\n", __func__);

	return AL_FALSE;
}

/* ---- al_eth_lm_retimer_ds25_speed_get ---- */
static enum speed_detection al_eth_lm_retimer_ds25_speed_get(
					struct al_eth_lm_context *lm_context,
					uint32_t channel)
{
	int rc = 0;
	uint8_t reg;

	rc = al_eth_lm_retimer_ds25_channel_select(lm_context, channel);
	if (rc)
		goto i2c_error;

	/* the following write will set the div value register LM_DS25_DIV_REG */
	rc = al_eth_lm_retimer_ds25_write_reg(
					lm_context,
					LM_DS25_CDR_STATUS_CTRL_REG,
					LM_DS25_CDR_STATUS_CTRL_DIV_MASK,
					LM_DS25_CDR_STATUS_CTRL_DIV_VAL);

	if (rc)
		goto i2c_error;

	rc = lm_context->i2c_read(lm_context->i2c_context,
				  lm_context->retimer_bus_id,
				  lm_context->retimer_i2c_addr,
				  LM_DS25_CDR_STATUS_REG,
				  &reg);

	if (rc)
		goto i2c_error;

	if (reg & LM_DS25_CDR_STATUS_DIV_10G)
		return SPEED_DETECTION_10G;

	return SPEED_DETECTION_25G;

i2c_error:
	pr_err("%s: failed to access to the retimer\n", __func__);

	return SPEED_DETECTION_25G;
}

/* ---- al_eth_lm_wait_for_lock_step ---- */
/* return value is 0 if done, or -EINPROGRESS if still waiting
 * lock parameter is true if lock succeded (or exited on error), and false if not
 */
static int al_eth_lm_wait_for_lock_step(struct al_eth_lm_context	*lm_context,
					uint32_t			channel,
					al_bool				*lock)
{
	unsigned int current_time;
	struct al_eth_lm_step_retimer_data *retimer_data = &lm_context->step_data.retimer_data;

	current_time = lm_context->get_msec();
	*lock = AL_FALSE;

	switch (retimer_data->wfl_state) {
	case LM_STEP_RETIMER_WAIT_FOR_LOCK_INIT:
		lm_context->step_data.start_time = current_time;
		retimer_data->wfl_last_time_checked =
			lm_context->step_data.start_time;
		retimer_data->wfl_state =
			LM_STEP_RETIMER_WAIT_FOR_LOCK_IN_PROGRESS;
		fallthrough;
	case LM_STEP_RETIMER_WAIT_FOR_LOCK_IN_PROGRESS:

		/* check lock in 10ms intervals */
		if (elapsed_time_msec(current_time, &retimer_data->wfl_last_time_checked,
				      LM_STEP_RETIMER_WAIT_FOR_LOCK_CHECK_INTERVAL))
			*lock = retimer[lm_context->retimer_type].cdr_lock(lm_context, channel);

		if (*lock == AL_TRUE ||
		    TIMED_OUT(current_time, lm_context->step_data.start_time,
			      AL_ETH_LM_RETIMER_WAIT_FOR_LOCK)) {
			retimer_data->wfl_state =
				LM_STEP_RETIMER_WAIT_FOR_LOCK_INIT;
			return 0;
		}

		break;
	default:
		pr_err("%s: Reached undefined state: %d\n", __func__,
		       retimer_data->wfl_state);
		retimer_data->wfl_state =
			LM_STEP_RETIMER_WAIT_FOR_LOCK_INIT;
		return 0;
	}

	pr_debug("%s: wfl_state:%d:, lock:%d, current_time:%d, start_time:%d\n",
		 __func__, retimer_data->wfl_state, *lock,
		 current_time, lm_context->step_data.start_time);

	return -EINPROGRESS;
}

/* ---- al_eth_lm_retimer_signal_lock_check_step ---- */
/* return value is 0 if done, or -EINPROGRESS if still waiting
 * ready parameter is true if lock succeded (or exited on error), and false if not
 */
static al_bool al_eth_lm_retimer_signal_lock_check_step(struct al_eth_lm_context	*lm_context,
						uint32_t			channel,
						struct al_eth_lm_retimer_channel_status
							*last_channel_status,
						al_bool				force_reset,
						al_bool				*lock)
{
	al_bool signal_detect = AL_TRUE;
	al_bool cdr_lock = AL_TRUE;
	int rc = 0;

	switch (lm_context->step_data.retimer_data.slc_state) {
	case LM_STEP_RETIMER_SIGNAL_LOCK_INIT:
		if (!retimer[lm_context->retimer_type].signal_detect)
			break;

		signal_detect = retimer[lm_context->retimer_type].signal_detect(
			lm_context, channel);

		if (!signal_detect) {
			if (signal_detect != last_channel_status->signal_detect)
				lm_debug("%s: no signal detected on retimer channel %d\n", __func__,
					channel);
			break;
		}

		if (!retimer[lm_context->retimer_type].cdr_lock)
			break;
		cdr_lock = retimer[lm_context->retimer_type].cdr_lock(lm_context, channel);

		if (!force_reset && cdr_lock)
			break;

		if (!retimer[lm_context->retimer_type].reset)
			break;
		retimer[lm_context->retimer_type].reset(lm_context, channel);

		lm_context->step_data.retimer_data.slc_state =
			LM_STEP_RETIMER_SIGNAL_LOCK_IN_PROGRESS;
		fallthrough;
	case LM_STEP_RETIMER_SIGNAL_LOCK_IN_PROGRESS:
		rc = al_eth_lm_wait_for_lock_step(lm_context, channel, &cdr_lock);
		if (rc == -EINPROGRESS)
			goto exit_in_progress;

		break;
	default:
		pr_err("%s: Reached undefined state: %d\n", __func__,
		       lm_context->step_data.retimer_data.slc_state);
	}

	*lock = (cdr_lock == AL_TRUE) && (signal_detect == AL_TRUE);

	lm_context->step_data.retimer_data.slc_state =
		LM_STEP_RETIMER_SIGNAL_LOCK_INIT;

	if (((signal_detect != last_channel_status->signal_detect) ||
			(cdr_lock != last_channel_status->cdr_lock)) ||
		last_channel_status->first_check)
		lm_debug("%s: (channel %d) signal %d cdr lock %d rc:%d\n",
			__func__, channel, signal_detect, (signal_detect) ? cdr_lock : 0, rc);


exit_in_progress:

	last_channel_status->signal_detect = signal_detect;
	last_channel_status->cdr_lock = cdr_lock;
	last_channel_status->first_check = AL_FALSE;

	return rc;
}

/* ---- al_eth_lm_retimer_ds25_full_config ---- */
static int al_eth_lm_retimer_ds25_full_config(struct al_eth_lm_context *lm_context)
{
	int rc = 0;
	struct retimer_config_reg *config_tx;
	uint32_t config_tx_size;
	struct retimer_config_reg *config_rx;
	uint32_t config_rx_size;

	if (lm_context->speed_detection == AL_TRUE) {
		config_tx = retimer_ds25_auto_mode_tx_ch;
		config_tx_size = AL_ARR_SIZE(retimer_ds25_auto_mode_tx_ch);

		config_rx = retimer_ds25_auto_mode_rx_ch;
		config_rx_size = AL_ARR_SIZE(retimer_ds25_auto_mode_rx_ch);
	} else if (lm_context->mode == AL_ETH_LM_MODE_25G) {
		config_tx = retimer_ds25_25g_mode_tx_ch;
		config_tx_size = AL_ARR_SIZE(retimer_ds25_25g_mode_tx_ch);

		config_rx = retimer_ds25_25g_mode_rx_ch;
		config_rx_size = AL_ARR_SIZE(retimer_ds25_25g_mode_rx_ch);
	} else {
		config_tx = retimer_ds25_10g_mode;
		config_tx_size = AL_ARR_SIZE(retimer_ds25_10g_mode);

		config_rx = retimer_ds25_10g_mode;
		config_rx_size = AL_ARR_SIZE(retimer_ds25_10g_mode);
	}

	rc = al_eth_lm_retimer_ds25_channel_config(lm_context,
					lm_context->retimer_channel,
					config_rx,
					config_rx_size);

	if (rc)
		return rc;

	rc = al_eth_lm_retimer_ds25_channel_config(lm_context,
					lm_context->retimer_tx_channel,
					config_tx,
					config_tx_size);

	if (rc)
		return rc;

	if (lm_context->serdes_obj->type_get() == AL_SRDS_TYPE_25G) {
		lm_debug("%s: serdes 25G - perform tx and rx gearbox reset\n", __func__);
		al_eth_gearbox_reset(lm_context->adapter, AL_TRUE, AL_TRUE);
		al_udelay(AL_ETH_LM_GEARBOX_RESET_DELAY);
	}

	lm_debug("%s: retimer full configuration done\n", __func__);

	return rc;
}

/* ---- al_eth_lm_retimer_25g_rx_adaptation_step ---- */
static int al_eth_lm_retimer_25g_rx_adaptation_step(struct al_eth_lm_context *lm_context)
{
	al_bool lock;
	unsigned int current_time;
	int rc;

	switch (lm_context->step_data.retimer_data.rx_adap_state) {
	case LM_STEP_RETIMER_RX_ADAPTATION_RETIMER_LOCK_TO:
		rc = al_eth_lm_retimer_signal_lock_check_step(lm_context,
							lm_context->retimer_channel,
							&lm_context->retimer_rx_channel_last_status,
							AL_FALSE,
							&lock);

		if (rc == -EINPROGRESS)
			break;

		if (!lock) {
			pr_debug("%s: no signal detected on retimer Rx channel (%d)\n",
				 __func__,  lm_context->retimer_channel);

			return -EIO;
		}

		lm_context->step_data.start_time =
			lm_context->get_msec();

		lm_context->step_data.retimer_data.rx_adap_state =
				LM_STEP_RETIMER_RX_ADAPTATION_SERDES_LOCK_DELAY;
		fallthrough;
	case LM_STEP_RETIMER_RX_ADAPTATION_SERDES_LOCK_DELAY:
		current_time = lm_context->get_msec();

		pr_debug("%s: In SERDES_LOCK_DELAY. current_time:%d, start_time:%d, waiting delay:%d\n",
			 __func__, current_time, lm_context->step_data.start_time,
			 AL_ETH_LM_SERDES_WAIT_FOR_LOCK);

		if (TIMED_OUT(current_time, lm_context->step_data.start_time,
			      AL_ETH_LM_SERDES_WAIT_FOR_LOCK)) {
			lm_context->step_data.retimer_data.rx_adap_state =
				LM_STEP_RETIMER_RX_ADAPTATION_RETIMER_LOCK_TO;

			return 0;
		}
		break;
	default:
		pr_err("%s: Reached undefined state: %d\n", __func__,
		       lm_context->step_data.retimer_data.rx_adap_state);
		return -EINVAL;
	}

	return -EINPROGRESS;
}

/* ---- al_eth_lm_check_for_link ---- */
static int al_eth_lm_check_for_link(struct al_eth_lm_context *lm_context, al_bool *link_up)
{
	struct al_eth_link_status status;
	int ret = 0;

	if (al_eth_link_status_clear(lm_context->adapter))
		lm_debug("%s: failed to clear link status latch\n", __func__);
	if (al_eth_link_status_get(lm_context->adapter, &status)) {
		pr_err("%s: failed to read link status\n", __func__);
		*link_up = AL_FALSE;
		return -EIO;
	}

	if (status.link_up == AL_TRUE) {
		lm_debug("%s: >>>> Link state DOWN ==> UP\n", __func__);
		if (al_eth_led_set(lm_context->adapter, AL_TRUE))
			lm_debug("%s: failed to set link LED\n", __func__);
		lm_context->link_state = AL_ETH_LM_LINK_UP;
		*link_up = AL_TRUE;

		return 0;
	} else if (status.local_fault) {
		lm_context->link_state = AL_ETH_LM_LINK_DOWN;
		if (al_eth_led_set(lm_context->adapter, AL_FALSE))
			lm_debug("%s: failed to set link LED\n", __func__);

		if ((lm_context->mode == AL_ETH_LM_MODE_25G) && lm_context->auto_fec_enable)
			lm_debug("%s: Failed to establish link\n", __func__);
		else
        {
			pr_err("%s: Failed to establish link\n", __func__);
        }
		ret = -1;
	} else {
		lm_debug("%s: >>>> Link state DOWN ==> DOWN_RF\n", __func__);
		lm_context->link_state = AL_ETH_LM_LINK_DOWN_RF;
		if (al_eth_led_set(lm_context->adapter, AL_FALSE))
			lm_debug("%s: failed to set link LED\n", __func__);

		ret = 0;
	}

	*link_up = AL_FALSE;
	return ret;
}

/* ---- al_eth_lm_link_state_check_for_detection ---- */
static int al_eth_lm_link_state_check_for_detection(struct al_eth_lm_context *lm_context,
						al_bool *link_fault,
						struct al_eth_link_status *status)
{
	switch (lm_context->link_state) {
	case AL_ETH_LM_LINK_UP:
		if (al_eth_link_status_get(lm_context->adapter, status)) {
			pr_err("%s: failed to read link status\n", __func__);
			return -EIO;
		}

		if (status->link_up) {
			if (link_fault)
				*link_fault = AL_FALSE;

			if (al_eth_led_set(lm_context->adapter, AL_TRUE))
				lm_debug("%s: failed to set link LED\n", __func__);

			return 0;
		} else if (status->local_fault) {
			lm_debug("%s: >>>> Link state UP ==> DOWN\n", __func__);
			lm_context->link_state = AL_ETH_LM_LINK_DOWN;
		} else {
			lm_debug("%s: >>>> Link state UP ==> DOWN_RF\n", __func__);
			lm_context->link_state = AL_ETH_LM_LINK_DOWN_RF;
		}

		break;
	case AL_ETH_LM_LINK_DOWN_RF:
		if (al_eth_link_status_get(lm_context->adapter, status)) {
			pr_err("%s: failed to read link status\n", __func__);
			return -EIO;
		}

		if (status->local_fault) {
			lm_debug("%s: >>>> Link state DOWN_RF ==> DOWN\n", __func__);
			lm_context->link_state = AL_ETH_LM_LINK_DOWN;

			break;
		} else if (status->remote_fault == AL_FALSE) {
			lm_debug("%s: >>>> Link state DOWN_RF ==> UP\n", __func__);
			lm_context->link_state = AL_ETH_LM_LINK_UP;
		}
		/* in case of remote fault only no need to check SFP again */
		return 0;
	case AL_ETH_LM_LINK_DOWN:
		break;
	};

	return -ENETDOWN;
}

/* ---- al_eth_lm_auto_fec_init ---- */
static void al_eth_lm_auto_fec_init(struct al_eth_lm_context	*lm_context)
{
	lm_context->auto_fec_state = AL_ETH_LM_AUTO_FEC_INIT_ENABLED;
	lm_context->auto_fec_wait_to_toggle = lm_context->auto_fec_initial_timeout;
	if (al_eth_fec_enable(lm_context->adapter, AL_TRUE))
		pr_err("%s: failed to enable FEC\n", __func__);

	pr_info("%s: Auto-FEC mode enabled. FEC state initialized to be Enabled\n",
		__func__);
	lm_context->fec_state_last_time_toggled = lm_context->get_msec();
	lm_debug("Initial sys_time (msec): %u, Initial timeout: %u, Toggle timeout: %u\n",
		 lm_context->fec_state_last_time_toggled,
		 lm_context->auto_fec_initial_timeout,
		 lm_context->auto_fec_toggle_timeout);
}

/* ---- al_eth_lm_fec_config ---- */
static void al_eth_lm_fec_config(struct al_eth_lm_context	*lm_context)
{
	if (lm_context->mode == AL_ETH_LM_MODE_25G) {
		if (lm_context->auto_fec_enable) {
			/* Auto-FEC mode for 25G */
			if (lm_context->auto_fec_state == AL_ETH_LM_AUTO_FEC_INIT)
				al_eth_lm_auto_fec_init(lm_context);
		} else {
			/* manual control of fec for 25G */
			lm_debug("%s: manual fec enabled %d\n", __func__,
				 lm_context->local_adv.fec_capability);
			if (al_eth_fec_enable(lm_context->adapter,
					  lm_context->local_adv.fec_capability))
				pr_err("%s: failed to configure FEC\n", __func__);
		}
	} else {
		if (al_eth_fec_enable(lm_context->adapter, AL_FALSE))
			pr_err("%s: failed to disable FEC\n", __func__);
	}
}

/* ---- al_eth_lm_link_detection_step ---- */
/*NOTICE: need to preserve old_mode and new_mode until return value is no longer -EINPROGRESS*/
int al_eth_lm_link_detection_step(struct al_eth_lm_context	*lm_context,
			     al_bool			*link_fault,
			     enum al_eth_lm_link_mode	*old_mode,
			     enum al_eth_lm_link_mode	*new_mode)
{
	int rc;
	int ret = -EIO;
	struct al_eth_link_status status;

	al_assert(lm_context != NULL);
	al_assert(old_mode != NULL);
	al_assert(new_mode != NULL);

	if (link_fault)
		*link_fault = AL_TRUE;

	switch (lm_context->step_data.detection_state) {
	case LM_STEP_DETECTION_INIT:

		/**
		 * if Link management is disabled, report no link fault in case the link was up
		 * before and set new mode to disconnected to avoid calling to link establish
		 * if the link wasn't up.
		 */
		if (lm_context->lm_pause) {
			al_bool lm_pause = lm_context->lm_pause(lm_context->i2c_context);
			if (lm_pause == AL_TRUE) {
				*new_mode = AL_ETH_LM_MODE_DISCONNECTED;
				if (link_fault) {
					if (lm_context->link_state == AL_ETH_LM_LINK_UP)
						*link_fault = AL_FALSE;
					else
						*link_fault = AL_TRUE;
				}
				return 0;
			}
		}

		*old_mode = lm_context->mode;
		*new_mode = lm_context->mode;

		rc = al_eth_lm_link_state_check_for_detection(lm_context, link_fault, &status);
		if (!rc)
			return 0;


		if (al_eth_led_set(lm_context->adapter, AL_FALSE))
			lm_debug("%s: failed to set link LED\n", __func__);

		if (lm_context->link_state == AL_ETH_LM_LINK_DOWN) {
			if (lm_context->sfp_detection) {
				rc = al_eth_module_detect(lm_context, new_mode);
				if (rc) {
					pr_err("module_detection failed!\n");
					goto exit_error;
				}

				lm_context->mode = *new_mode;

				if (*new_mode == AL_ETH_LM_MODE_DISCONNECTED &&
				    lm_context->auto_fec_enable) {
					lm_context->auto_fec_state = AL_ETH_LM_AUTO_FEC_INIT;
					lm_context->link_prev_up = AL_FALSE;
				}
			} else {
				lm_context->mode = lm_context->default_mode;
				*new_mode = lm_context->mode;
			}
		}

		if ((lm_context->link_state == AL_ETH_LM_LINK_DOWN) &&
		    (lm_context->retimer_exist) &&
		    (*new_mode != AL_ETH_LM_MODE_DISCONNECTED)) {
			if (*old_mode != *new_mode) {
				lm_context->rx_param_dirty = 1;
				lm_context->tx_param_dirty = 1;

				al_eth_serdes_static_rx_params_set(lm_context);
				al_eth_serdes_static_tx_params_set(lm_context);

				if (retimer[lm_context->retimer_type].config(lm_context)) {
					pr_info("%s: failed to configure the retimer\n", __func__);
					rc = -EIO;
					goto exit_error;
				}

				al_udelay(AL_ETH_LM_RETIMER_CONFIG_DELAY);
			}

			if (lm_context->speed_detection) {
				if (retimer[lm_context->retimer_type].speed_get == NULL) {
					pr_info("Speed detection not supported in the retimer\n");
					rc = -EOPNOTSUPP;
					goto exit_error;
				}

				if (retimer[lm_context->retimer_type].rx_adaptation)
					LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state,
							     LM_STEP_DETECTION_RETIMER_RX_ADAPT_WAIT
						);
				else
					LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state,
							     LM_STEP_DETECTION_RETIMER_RX_ADAPT_POST
						);
				break;
			}
		}

		LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state, LM_STEP_DETECTION_FINAL)
			;
		break;
	case LM_STEP_DETECTION_RETIMER_RX_ADAPT_WAIT:
		ret = retimer[lm_context->retimer_type].rx_adaptation(lm_context);

		pr_debug("%s: post rx_adaptation. ret:%d\n", __func__, ret);

		if (ret == -EINPROGRESS)
			break;

		LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state,
				     LM_STEP_DETECTION_RETIMER_RX_ADAPT_POST);
		fallthrough;
	case LM_STEP_DETECTION_RETIMER_RX_ADAPT_POST:
		if (ret) {
			pr_debug("%s: Rx channel is not locked\n", __func__);
		} else {
			int speed = 0;

			speed = retimer[lm_context->retimer_type].speed_get(
				lm_context,
				lm_context->retimer_channel);

			if (speed == SPEED_DETECTION_10G) {
				if (lm_context->last_detected_mode ==
					AL_ETH_LM_MODE_25G)
					lm_context->speed_change = AL_TRUE;

				*new_mode = AL_ETH_LM_MODE_10G_DA;
				lm_context->mode = AL_ETH_LM_MODE_10G_DA;
			} else {
				if (lm_context->last_detected_mode !=
					AL_ETH_LM_MODE_25G)
					lm_context->speed_change = AL_TRUE;

				*new_mode = AL_ETH_LM_MODE_25G;
				lm_context->mode = AL_ETH_LM_MODE_25G;
			}
		}

		if (lm_context->mode != AL_ETH_LM_MODE_DISCONNECTED)
			lm_context->last_detected_mode = lm_context->mode;

		LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state,
				     LM_STEP_DETECTION_FINAL);
		fallthrough;
	case LM_STEP_DETECTION_FINAL:
		if (*old_mode != *new_mode) {
			pr_info("%s: New SFP mode detected %s -> %s\n",
				__func__, al_eth_lm_mode_convert_to_str(*old_mode),
				al_eth_lm_mode_convert_to_str(*new_mode));

			if ((*new_mode != AL_ETH_LM_MODE_DISCONNECTED) &&
			    (lm_context->led_config)) {
				struct al_eth_lm_led_config_data data = {0};

				switch (*new_mode) {
				case AL_ETH_LM_MODE_10G_OPTIC:
				case AL_ETH_LM_MODE_10G_DA:
					data.speed = AL_ETH_LM_LED_CONFIG_10G;
					break;
				case AL_ETH_LM_MODE_1G:
					data.speed = AL_ETH_LM_LED_CONFIG_1G;
					break;
				case AL_ETH_LM_MODE_25G:
					data.speed = AL_ETH_LM_LED_CONFIG_25G;
					break;
				default:
					pr_err("%s: unknown LM mode!\n", __func__);
				};

				lm_context->led_config(lm_context->i2c_context, &data);
			}
		}

		LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state, LM_STEP_DETECTION_INIT);
		return 0;
	default:
		pr_err("%s: Undefined lm_step_destection_state: %d\n", __func__,
		       lm_context->step_data.detection_state);
		rc = -EINVAL;
		goto exit_error;
	}

	return -EINPROGRESS;

exit_error:
	LM_STEP_CHANGE_STATE(lm_context->step_data.detection_state, LM_STEP_DETECTION_INIT);
	lm_debug("%s: Exiting with error %d\n", __func__, rc);
	return rc;
}

/* ---- al_eth_lm_link_detection ---- */
int al_eth_lm_link_detection(struct al_eth_lm_context	*lm_context,
			     al_bool			*link_fault,
			     enum al_eth_lm_link_mode	*old_mode,
			     enum al_eth_lm_link_mode	*new_mode)
{
	int rc;

	while (1) {
		rc = al_eth_lm_link_detection_step(lm_context, link_fault, old_mode, new_mode);
		if (rc != -EINPROGRESS)
			break;
		al_udelay(LM_STEP_LOOP_INTERVAL);
	}

	return rc;
}

/* ---- al_eth_lm_link_establish_step ---- */
int al_eth_lm_link_establish_step(struct al_eth_lm_context	*lm_context,
				  al_bool			*link_up)
{
	al_bool signal_detected;
	int ret = 0;
	int rc;
	al_bool lock;
	unsigned int current_time = 0;

	signal_detected = lm_context->serdes_obj->signal_is_detected(
			lm_context->serdes_obj,
			lm_context->lane);

	if (signal_detected == AL_FALSE) {
		/* if no signal detected there is nothing to do */
		lm_debug("serdes signal is down\n");
		*link_up = AL_FALSE;
		LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
			LM_STEP_ESTABLISH_INIT);

		return -EIO;
	}

	switch (lm_context->step_data.establish_state) {
	case LM_STEP_ESTABLISH_INIT:
		switch (lm_context->link_state) {
		case AL_ETH_LM_LINK_UP:
			*link_up = AL_TRUE;
			lm_debug("%s: return link up\n", __func__);

			return 0;
		case AL_ETH_LM_LINK_DOWN_RF:
			*link_up = AL_FALSE;
			lm_debug("%s: return link down (DOWN_RF)\n", __func__);

			return 0;
		case AL_ETH_LM_LINK_DOWN:
			break;
		};

		/**
		 * At this point we will get LM disable only if changed to disable after link
		 * detection finished. in this case link will not be established until LM will
		 * be enable again.
		 */
		if (lm_context->lm_pause) {
			al_bool lm_pause = lm_context->lm_pause(lm_context->i2c_context);
			if (lm_pause == AL_TRUE) {
				*link_up = AL_FALSE;

				return 0;
			}
		}

		if (lm_context->retimer_exist)
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_RETIMER_SIGNAL_LOCK_CHECK);
		else
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_FINAL);

		break;
	case LM_STEP_ESTABLISH_RETIMER_SIGNAL_LOCK_CHECK:
		rc = al_eth_lm_retimer_signal_lock_check_step(lm_context,
							lm_context->retimer_tx_channel,
							&lm_context->retimer_tx_channel_last_status,
							AL_FALSE,
							&lock);

		pr_debug("%s: post signal_lock_check_step. rc:%d, lock:%d\n",
		       __func__, rc, lock);

		if (rc == -EINPROGRESS)
			break;

		if (!lock) {
			pr_info("%s: Failed to lock tx channel\n", __func__);
			rc = -EIO;
			goto exit_error;
		}

		if (lm_context->speed_change) {
			lm_debug("%s: serdes 25G - perform tx gearbox reset\n", __func__);
			al_eth_gearbox_reset(lm_context->adapter, AL_TRUE, AL_FALSE);
			lm_context->speed_change = AL_FALSE;
		}

		if (retimer[lm_context->retimer_type].rx_adaptation)
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_RETIMER_RX_ADAPT_WAIT);
		else
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_MIDDLE);

		break;
	case LM_STEP_ESTABLISH_RETIMER_RX_ADAPT_WAIT:
		ret = retimer[lm_context->retimer_type].rx_adaptation(lm_context);

		pr_debug("%s: post rx_adaptation. ret:%d\n", __func__, ret);

		if (ret == -EINPROGRESS) {
			*link_up = AL_FALSE;
			break;
		}

		if (ret) {
			pr_debug("%s: retimer rx is not ready\n", __func__);
			*link_up = AL_FALSE;

			rc = ret;
			goto exit_error;
		}

		LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
				     LM_STEP_ESTABLISH_MIDDLE);
		fallthrough;
	case LM_STEP_ESTABLISH_MIDDLE:
		signal_detected = lm_context->serdes_obj->signal_is_detected(
			lm_context->serdes_obj,
			lm_context->lane);

		if (signal_detected == AL_FALSE) {
			/* if no signal detected there is nothing to do */
			lm_debug("serdes signal is down\n");
			*link_up = AL_FALSE;

			rc = -EIO;
			goto exit_error;
		}

		if (lm_context->serdes_obj->type_get() == AL_SRDS_TYPE_25G) {
			if (lm_context->link_training == AL_FALSE)
				al_eth_lm_fec_config(lm_context);

			lm_debug("%s: serdes 25G - perform rx gearbox reset\n", __func__);
			al_eth_gearbox_reset(lm_context->adapter, AL_FALSE, AL_TRUE);
			al_udelay(AL_ETH_LM_GEARBOX_RESET_DELAY);
		}

		if (lm_context->retimer_exist) {
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_RETIMER_EXISTS_LINK_STATUS_DELAY);
			lm_context->step_data.start_time = lm_context->get_msec();
		} else {
			LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state,
					     LM_STEP_ESTABLISH_FINAL);
		}
		break;
	case LM_STEP_ESTABLISH_RETIMER_EXISTS_LINK_STATUS_DELAY:
		current_time = lm_context->get_msec();

		pr_debug("%s: step_state:%d, current_time:%d, start_time:%d, waiting delay: %d\n",
		       __func__, lm_context->step_data.establish_state, current_time,
		       lm_context->step_data.start_time,
		       AL_ETH_LM_RETIMER_LINK_STATUS_DELAY);

		if (!TIMED_OUT(current_time, lm_context->step_data.start_time,
			       AL_ETH_LM_RETIMER_LINK_STATUS_DELAY))
			break;

		ret = al_eth_lm_check_for_link(lm_context, link_up);

		if (ret == 0) {
			lm_debug("%s: link is %s with retimer\n", __func__,
				(link_up ? "UP" : "DOWN_RF"));
			if ((lm_context->mode == AL_ETH_LM_MODE_25G) &&
				lm_context->auto_fec_enable) {
				if (lm_context->auto_fec_state == AL_ETH_LM_AUTO_FEC_INIT_ENABLED) {
					/* Case when link went up during initial toggling cycle */
					lm_context->auto_fec_state = AL_ETH_LM_AUTO_FEC_ENABLED;
					lm_context->auto_fec_wait_to_toggle =
						lm_context->auto_fec_toggle_timeout;
				}
				lm_context->link_prev_up = AL_TRUE;
				lm_debug("%s: Auto FEC state is %s\n",
					__func__, lm_context->auto_fec_state ?
					"Enabled" : "Disabled");
			}

			goto exit_done;
		}

		if ((lm_context->mode == AL_ETH_LM_MODE_25G) && lm_context->auto_fec_enable) {
			unsigned int new_time = lm_context->get_msec();
			if (lm_context->link_prev_up == AL_TRUE) {
				/* Case when link just went down - want to preserve the FEC
				 * state */
				lm_context->link_prev_up = AL_FALSE;
				lm_context->fec_state_last_time_toggled = new_time;
				lm_debug("%s: Link went DOWN. Starting Auto FEC toggling in %s state\n",
					__func__, (lm_context->auto_fec_state ==
						AL_ETH_LM_AUTO_FEC_ENABLED) ?
					"Enabled" : "Disabled");
			} else {
				if ((new_time - lm_context->fec_state_last_time_toggled) >=
					lm_context->auto_fec_wait_to_toggle) {
					/* If new_time wraps around,the calculation will
					 * still be correct, since LHS will be modulo
					 * reduced and get_msec() returns time which wraps
					 * around full range of uint
					 */
					al_bool fec_enabled_bool;
					lm_context->fec_state_last_time_toggled = new_time;
					switch (lm_context->auto_fec_state) {
					case AL_ETH_LM_AUTO_FEC_INIT_ENABLED:
						lm_context->auto_fec_wait_to_toggle =
							lm_context->auto_fec_toggle_timeout;
						fallthrough;
					case AL_ETH_LM_AUTO_FEC_ENABLED:
						lm_context->auto_fec_state =
							AL_ETH_LM_AUTO_FEC_DISABLED;
						fec_enabled_bool = AL_FALSE;
						break;
					case AL_ETH_LM_AUTO_FEC_DISABLED:
						lm_context->auto_fec_state =
							AL_ETH_LM_AUTO_FEC_ENABLED;
						fec_enabled_bool = AL_TRUE;
						break;
					default:
						pr_err("%s: Invalid Auto FEC state value: %d\n",
						       __func__, lm_context->auto_fec_state);
						rc = -EINVAL;
						goto exit_error;
					}
					if (al_eth_fec_enable(lm_context->adapter,
							  fec_enabled_bool))
						pr_err("%s: failed to configure FEC\n", __func__);
					pr_info("%s: Auto FEC state is %s\n", __func__,
						fec_enabled_bool ? "Enabled" : "Disabled");
				}
			}
		}
		rc = ret;
		goto exit_error;

	case LM_STEP_ESTABLISH_FINAL:

		if ((lm_context->mode == AL_ETH_LM_MODE_10G_DA) && (lm_context->link_training)) {
			lm_context->local_adv.transmitted_nonce = lm_context->get_random_byte();
			lm_context->local_adv.transmitted_nonce &= 0x1f;

			ret = al_eth_an_lt_execute(lm_context->adapter,
						lm_context->serdes_obj,
						lm_context->lane,
						&lm_context->local_adv,
						&lm_context->partner_adv);

			lm_context->rx_param_dirty = 1;
			lm_context->tx_param_dirty = 1;

			if (ret == 0) {
				pr_info("%s: link training finished successfully\n", __func__);
				lm_context->link_training_failures = 0;
				ret = al_eth_lm_check_for_link(lm_context, link_up);

				if (ret == 0) {
					lm_debug("%s: link is up with LT\n", __func__);

					goto exit_done;
				}
			}

			lm_context->link_training_failures++;
			if (lm_context->link_training_failures > AL_ETH_LT_FAILURES_TO_RESET) {
				lm_debug("%s: failed to establish LT %d times. reset serdes\n",
					 __func__, AL_ETH_LT_FAILURES_TO_RESET);

				lm_context->serdes_obj->pma_hard_reset_lane(
					lm_context->serdes_obj,
					lm_context->lane,
					AL_TRUE);
				lm_context->serdes_obj->pma_hard_reset_lane(
					lm_context->serdes_obj,
					lm_context->lane,
					AL_FALSE);
				lm_context->link_training_failures = 0;
			}
		}

		al_eth_serdes_static_tx_params_set(lm_context);

		if ((lm_context->mode == AL_ETH_LM_MODE_10G_DA) && (lm_context->rx_equal)) {
			ret = al_eth_rx_equal_run(lm_context);

			if (ret == 0) {
				al_udelay(AL_ETH_LM_LINK_STATUS_DELAY);
				ret = al_eth_lm_check_for_link(lm_context, link_up);

				if (ret == 0) {
					lm_debug("%s: link is up with Rx Equalization\n", __func__);
					goto exit_done;
				}
			}
		}

		al_eth_serdes_static_rx_params_set(lm_context);

		al_udelay(AL_ETH_LM_LINK_STATUS_DELAY);

		ret = al_eth_lm_check_for_link(lm_context, link_up);

		if (ret == 0) {
			lm_debug("%s: link is up with static parameters\n", __func__);

			goto exit_done;
		}

		*link_up = AL_FALSE;
		rc = -1;
		goto exit_error;
	default:
		pr_err("%s: Undefined lm_step_establish_state: %d\n", __func__,
		       lm_context->step_data.establish_state);
		*link_up = AL_FALSE;
		rc = -EINVAL;
		goto exit_error;
	};

	return -EINPROGRESS;

exit_done:
	LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state, LM_STEP_ESTABLISH_INIT);
	return 0;

exit_error:
	LM_STEP_CHANGE_STATE(lm_context->step_data.establish_state, LM_STEP_ESTABLISH_INIT);
	pr_debug("%s: Exiting with error %d\n", __func__, rc);
	return rc;
}

/* ---- al_eth_lm_link_establish ---- */
int al_eth_lm_link_establish(struct al_eth_lm_context	*lm_context,
			     al_bool			*link_up)
{
	int rc;

	while (1) {
		rc = al_eth_lm_link_establish_step(lm_context, link_up);
		if (rc != -EINPROGRESS)
			break;
		al_udelay(LM_STEP_LOOP_INTERVAL);
	}

	return rc;
}

/* ---- al_eth_lm_static_parameters_override_disable ---- */
int al_eth_lm_static_parameters_override_disable(
					struct al_eth_lm_context *lm_context,
					al_bool			 tx_params,
					al_bool			 rx_params)
{
	if (tx_params)
		lm_context->serdes_tx_params_valid = AL_FALSE;
	if (rx_params)
		/* was serdes_TX_params_valid - copy-paste, so disabling an RX
		 * override dropped the TX one and left RX active. */
		lm_context->serdes_rx_params_valid = AL_FALSE;

	return 0;
}

/* ---- al_eth_lm_mode_convert_to_str ---- */
const char *al_eth_lm_mode_convert_to_str(enum al_eth_lm_link_mode val)
{
	switch (val) {
	case AL_ETH_LM_MODE_DISCONNECTED:
		return "AL_ETH_LM_MODE_DISCONNECTED";
	case AL_ETH_LM_MODE_10G_OPTIC:
		return "AL_ETH_LM_MODE_10G_OPTIC";
	case AL_ETH_LM_MODE_10G_DA:
		return "AL_ETH_LM_MODE_10G_DA";
	case AL_ETH_LM_MODE_1G:
		return "AL_ETH_LM_MODE_1G";
	case AL_ETH_LM_MODE_25G:
		return "AL_ETH_LM_MODE_25G";
	}

	return "N/A";
}

/* ---- al_eth_lm_retimer_tx_cdr_lock_get ---- */
al_bool al_eth_lm_retimer_tx_cdr_lock_get(struct al_eth_lm_context *lm_context)
{
	al_bool lock = AL_FALSE;

	if (lm_context->retimer_exist && retimer[lm_context->retimer_type].cdr_lock) {
		lock = retimer[lm_context->retimer_type].cdr_lock(lm_context,
				lm_context->retimer_tx_channel);
	}

	return lock;
}

/* ---- al_eth_lm_link_state_get ---- */
enum al_eth_lm_link_state al_eth_lm_link_state_get(struct al_eth_lm_context *lm_context)
{
	return lm_context->link_state;
}

/* ---- al_eth_lm_link_check ---- */
int al_eth_lm_link_check(struct al_eth_lm_context *lm_context,
			enum al_eth_lm_link_state *link_state)
{
	struct al_eth_link_status status;
	int rc;

	rc = al_eth_link_status_get(lm_context->adapter, &status);
	if (rc) {
		pr_err("%s: Error getting link status from MAC\n", __func__);
		return rc;
	}

	if (status.local_fault)
		*link_state = AL_ETH_LM_LINK_DOWN;
	else if (status.remote_fault)
		*link_state = AL_ETH_LM_LINK_DOWN_RF;
	else if (status.link_up)
		*link_state = AL_ETH_LM_LINK_UP;
	else {
		pr_err("%s: Invalid link state!\n", __func__);
		return -EIO;
	}

	pr_debug("%s: link_state is %s\n", __func__,
		((*link_state == AL_ETH_LM_LINK_DOWN) ? "DOWN" :
		((*link_state == AL_ETH_LM_LINK_DOWN_RF) ? "DOWN_RF" :
		((*link_state == AL_ETH_LM_LINK_UP) ? "UP" : "UNKNOWN"))));

	return 0;
}

/* ---- al_eth_lm_pause_check ---- */
al_bool al_eth_lm_pause_check(struct al_eth_lm_context *lm_context)
{
	if (lm_context->lm_pause)
		return lm_context->lm_pause(lm_context->i2c_context);

	return AL_FALSE;
}

