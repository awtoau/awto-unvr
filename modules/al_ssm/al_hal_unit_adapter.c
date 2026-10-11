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
 * @defgroup group_unit_adapter Unit Adapter
 *  @{
 * @file   al_hal_unit_adapter.c
 *
 * @brief  Unit Adapter HAL
 *
 */

#include "al_hal_unit_adapter.h"
#include "al_hal_unit_adapter_regs.h"

#define ADAPTER_READ32(where, val)	\
	do {				\
		unit_adapter->pcie_read_config_u32(unit_adapter->handle, (where), (val));	\
	} while (0)

#define ADAPTER_WRITE32(where, val)	\
	do {				\
		unit_adapter->pcie_write_config_u32(unit_adapter->handle, (where), (val));	\
	} while (0)

#define ADAPTER_WRITE32_MASKED(where, mask, val)			\
	do {								\
		uint32_t temp;						\
		ADAPTER_READ32(where, &temp);				\
		ADAPTER_WRITE32(where, AL_MASK_VAL(mask, val, temp));	\
	} while (0)

#define FLR_MDELAY		3

/******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************/
void al_unit_adapter_snoop_enable(
	struct al_unit_adapter	*unit_adapter,
	unsigned int	func_num,
	al_bool	snoop_en)
{
	unsigned int axcache_mask = 0x3ff;
	unsigned int axcache_val = snoop_en ? axcache_mask : 0;

	ADAPTER_WRITE32_MASKED(
		AL_ADAPTER_SMCC + (AL_ADAPTER_SMCC_BUNDLE_SIZE * func_num),
		AL_ADAPTER_SMCC_CONF_SNOOP_OVR |
		AL_ADAPTER_SMCC_CONF_SNOOP_ENABLE,
		(snoop_en ?
			(AL_ADAPTER_SMCC_CONF_SNOOP_OVR |
			AL_ADAPTER_SMCC_CONF_SNOOP_ENABLE) : 0));

	/* configure awcache, arcache (PF only) */
	if (func_num) {
		uint32_t reg_val;

		ADAPTER_READ32(AL_ADAPTER_GENERIC_CONTROL_11, &reg_val);
		al_assert((reg_val & axcache_mask) == (axcache_val));
	} else {
		ADAPTER_WRITE32_MASKED(AL_ADAPTER_GENERIC_CONTROL_11, axcache_mask, axcache_val);
	}
}

/******************************************************************************
 ******************************************************************************/
void al_unit_adapter_error_track_enable(
	struct al_unit_adapter	*unit_adapter,
	unsigned int	func_num,
	al_bool	error_track_en)
{
	ADAPTER_WRITE32_MASKED(
		AL_ADAPTER_SMCC_CONF_2 +
			(AL_ADAPTER_SMCC_BUNDLE_SIZE * func_num),
		AL_ADAPTER_SMCC_CONF_2_DIS_ERROR_TRACK,
		(error_track_en ? 0 :
			AL_ADAPTER_SMCC_CONF_2_DIS_ERROR_TRACK));

}

/******************************************************************************
 ******************************************************************************/
void al_pcie_perform_flr(
	struct al_unit_adapter	*unit_adapter)
{
	int i;
	uint32_t cfg_reg_store[15];

	i = 0;
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_COMMAND, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_DEVICE_ID, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_0, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_0_HI, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_2, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_2_HI, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_4, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_4_HI, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_0, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_1, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_2, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_3, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_APP_CONTROL, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_PCI_VF_BASE_ADDRESS_0, &cfg_reg_store[i++]);
	unit_adapter->pcie_read_config_u32(
		unit_adapter->handle, AL_ADAPTER_SMCC_CONF_2, &cfg_reg_store[i++]);


	unit_adapter->pcie_flr(unit_adapter->handle);

	i = 0;
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_COMMAND, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_DEVICE_ID, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_0, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_0_HI, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_2, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_2_HI, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_4, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_BASE_ADDRESS_4_HI, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_0, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_1, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_2, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_AXI_CFG_AND_CTR_3, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_APP_CONTROL, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_PCI_VF_BASE_ADDRESS_0, cfg_reg_store[i++]);
	unit_adapter->pcie_write_config_u32(
		unit_adapter->handle, AL_ADAPTER_SMCC_CONF_2, cfg_reg_store[i++]);
}

/******************************************************************************
 ******************************************************************************/
void al_unit_adapter_rob_cfg(
	struct al_unit_adapter	*unit_adapter,
	al_bool			rd_rob_en,
	al_bool			rd_rob_force_in_order,
	al_bool			wr_rob_en,
	al_bool			wr_rob_force_in_order)
{
	/**
	 * Addressing RMN: TBD
	 *
	 * RMN description:
	 * TBD
	 * Software flow:
	 * Reset ROB prior to it's configuration
	 */
	ADAPTER_WRITE32_MASKED(
		AL_ADAPTER_GENERIC_CONTROL_19,
		AL_ADPTR_GEN_CTL_19_READ_ROB_SW_RESET | AL_ADPTR_GEN_CTL_19_WRITE_ROB_SW_RESET,
		AL_ADPTR_GEN_CTL_19_READ_ROB_SW_RESET | AL_ADPTR_GEN_CTL_19_WRITE_ROB_SW_RESET);
	ADAPTER_WRITE32_MASKED(
		AL_ADAPTER_GENERIC_CONTROL_19,
		AL_ADPTR_GEN_CTL_19_READ_ROB_SW_RESET | AL_ADPTR_GEN_CTL_19_WRITE_ROB_SW_RESET, 0);

	ADAPTER_WRITE32_MASKED(
		AL_ADAPTER_GENERIC_CONTROL_19,
		AL_ADPTR_GEN_CTL_19_READ_ROB_EN |
		AL_ADPTR_GEN_CTL_19_READ_ROB_FORCE_INORDER |
		AL_ADPTR_GEN_CTL_19_WRITE_ROB_EN |
		AL_ADPTR_GEN_CTL_19_WRITE_ROB_FORCE_INORDER,
		(rd_rob_en ? AL_ADPTR_GEN_CTL_19_READ_ROB_EN : 0) |
		(rd_rob_force_in_order ? AL_ADPTR_GEN_CTL_19_READ_ROB_FORCE_INORDER : 0) |
		(wr_rob_en ? AL_ADPTR_GEN_CTL_19_WRITE_ROB_EN : 0) |
		(wr_rob_force_in_order ? AL_ADPTR_GEN_CTL_19_WRITE_ROB_FORCE_INORDER : 0));
}

/******************************************************************************
 ******************************************************************************/
void al_unit_adapter_init(
		struct al_unit_adapter	*unit_adapter)
{
	int func_num;

	al_assert(unit_adapter);

	for (func_num = 0; func_num < AL_ADAPTER_SMCC_BUNDLE_NUM; func_num++) {
		/* i/o cache coherency enablement */
		al_unit_adapter_snoop_enable(unit_adapter,
				func_num, AL_TRUE);
	}
	/* Error tracking disable */
	al_unit_adapter_error_track_enable(unit_adapter,
			0, AL_FALSE);
	/* ROB/WOB enable */
	al_unit_adapter_rob_cfg(unit_adapter,
		AL_TRUE, AL_FALSE, AL_TRUE, AL_FALSE);
}

/** @} */
