/* ---- al_crypto_error_ints_mask_get ---- */
/******************************************************************************
 *****************************************************************************/
void al_crypto_error_ints_mask_get(unsigned int rev_id,
				   uint32_t *crypto_a_mask,
				   uint32_t *crypto_b_mask,
				   uint32_t *crypto_c_mask,
				   uint32_t *crypto_d_mask)
{
	al_assert(crypto_a_mask);
	al_assert(crypto_b_mask);
	al_assert(crypto_c_mask);
	al_assert(crypto_d_mask);

	if (rev_id <= AL_CRYPTO_REV_ID_2)
		*crypto_a_mask = AL_CRYPTO_IOFIC_GROUP_A_ERROR_INTS;
	else
		*crypto_a_mask = AL_CRYPTO_IOFIC_GROUP_A_ERROR_INTS_V3;

	/**
	 * Addressing RMN: 9991
	 *
	 * RMN description:
	 * registers of group B in the intc are not accessible.
	 * only 0x40 bytes of memory space were allocated for the crypto intc,
	 * which is equivalent for a single interrupt group.
	 * The following 0x40 bytes of memory space , which should have been
	 * allocated to group B of the interrupt controller,
	 * are allocated to other registers of the regfile
	 *
	 * Software flow:
	 * On V1 do not use intc group B
	 */
	if (rev_id <= AL_CRYPTO_REV_ID_1)
		*crypto_b_mask = 0;
	else if (rev_id == AL_CRYPTO_REV_ID_2)
		*crypto_b_mask = AL_CRYPTO_IOFIC_GROUP_B_ERROR_INTS_V2;
	else
		*crypto_b_mask = AL_CRYPTO_IOFIC_GROUP_B_ERROR_INTS_V3;

	if (rev_id <= AL_CRYPTO_REV_ID_2)
		*crypto_c_mask = 0;
	else
		*crypto_c_mask = AL_CRYPTO_IOFIC_GROUP_C_ERROR_INTS;

	*crypto_d_mask = 0;
}

/* ---- al_crypto_error_ints_unmask ---- */
/******************************************************************************
 *****************************************************************************/
void al_crypto_error_ints_unmask(uint8_t rev_id,
				 void __iomem *crypto_regs_base,
				 void __iomem *udma_regs_base)
{
	/* static: al_udma.udma_q[DMA_MAX_Q_MAX] is too large for the stack (#101);
	 * not reentrant/concurrent - init-time only, one call in flight at a time. */
	static struct al_udma udma;
	struct al_udma_params udma_params = {
		.udma_regs_base = udma_regs_base
	};
	uint32_t a_mask;
	uint32_t b_mask;
	uint32_t c_mask;
	uint32_t d_mask;
	void *iofic_regs_base;
	uint32_t ext_app_bit;


	if (al_udma_handle_init(&udma, &udma_params))
		pr_err("%s: al_udma_handle_init failed\n", __func__);

	al_crypto_error_ints_mask_get(rev_id,
				      &a_mask,
				      &b_mask,
				      &c_mask,
				      &d_mask);

	iofic_regs_base = (uint8_t *)crypto_regs_base + AL_CRYPTO_APP_IOFIC_OFFSET;

	if (al_iofic_config(iofic_regs_base,
			AL_INT_GROUP_A,
			INT_CONTROL_GRP_MASK_MSI_X))
		pr_err("%s: al_iofic_config failed (group A)\n", __func__);

	al_iofic_unmask(iofic_regs_base,
			AL_INT_GROUP_A,
			a_mask);

	if (b_mask) {
		if (al_iofic_config(iofic_regs_base,
			AL_INT_GROUP_B,
			INT_CONTROL_GRP_MASK_MSI_X))
			pr_err("%s: al_iofic_config failed (group B)\n", __func__);

		al_iofic_unmask(iofic_regs_base,
			AL_INT_GROUP_B,
			b_mask);
	}

	if (c_mask) {
		if (al_iofic_config(iofic_regs_base,
			AL_INT_GROUP_C,
			INT_CONTROL_GRP_MASK_MSI_X))
			pr_err("%s: al_iofic_config failed (group C)\n", __func__);

		al_iofic_unmask(iofic_regs_base,
			AL_INT_GROUP_C,
			c_mask);
	}

	if (al_iofic_config(
			al_udma_iofic_reg_base_get_adv(&udma,
				AL_UDMA_IOFIC_LEVEL_PRIMARY),
			AL_INT_GROUP_D,
			INT_CONTROL_GRP_SET_ON_POSEDGE |
			INT_CONTROL_GRP_MASK_MSI_X))
		pr_err("%s: al_iofic_config failed (primary, group D)\n", __func__);

	ext_app_bit = al_udma_iofic_get_ext_app_bit(&udma);

	al_udma_iofic_unmask_adv(&udma,
			     AL_UDMA_IOFIC_LEVEL_PRIMARY,
			     AL_INT_GROUP_D,
			     ext_app_bit);
}

