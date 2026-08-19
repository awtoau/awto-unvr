# S2 stage2_loader — function-name ledger

Machine form: `preboot-s2-names.sym` (applied via ApplyAlRegs / annotate-preboot.py).
Auto-built by `scripts/build-s2-name-ledger.py`. 68 names (43 confirmed, 25 speculative).

DDR-init subgraph (from al_ddr_init + orchestrator): 56 functions, 56 named (31 confirmed, 25 speculative), 0 bare.

| addr | name | confidence | rationale |
|---|---|---|---|
| 0xf2200020 | `s2_entry_a32_stub` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf220005c | `cpu_affinity_id` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200098 | `memcpy32_unrolled` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22000b4 | `memset_unrolled` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200100 | `s_freq_helper_a` | speculative | under ddr_freq_change_according_to_spd |
| 0xf2200114 | `s_freq_helper_b` | speculative | under s_freq_helper chain |
| 0xf2200124 | `s2_main` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22002c8 | `bootstrap_parse_nb_pll_init` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200340 | `s_freq_helper_c` | speculative | under ddr_freq_change_according_to_spd |
| 0xf22003b0 | `rec_read_aa` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22003b8 | `rec_read_bb` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22003c0 | `rec_read_cc` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22003c8 | `rec_read_dd` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22003d0 | `s_rec_present_check` | speculative | shared by rec readers/impedance/voltage |
| 0xf22003d8 | `ddr_bringup_orchestrator` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf220061c | `s_orch_helper` | speculative | orchestrator helper |
| 0xf2200628 | `ddr_freq_change_according_to_spd` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200814 | `s_cfg_get_helper` | speculative | orchestrator helper |
| 0xf2200816 | `ddr_build_addrmap` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf220093c | `ddr_init_spd_get` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200a58 | `set_dram_impedance_ctrl_from_eeprom` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200d10 | `dram_voltage_gpio` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200dcc | `jedec_crc16` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200df8 | `ddr3_cwl_from_tck` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200e74 | `ddr4_cwl_from_tck` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200ed8 | `ddr_cl_solve` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2200f30 | `al_ddr3_spd_parse` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2201140 | `al_ddr4_spd_parse` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22013e8 | `al_ddr_spd_parse` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22014dc | `s_printf_emit` | speculative | under formatter chain |
| 0xf22014f8 | `s_printf_putc` | speculative | under formatter chain |
| 0xf220150a | `s_printf_fmt_num` | speculative | under s_vprintf_core |
| 0xf220159c | `s_printf_fmt_str` | speculative | under s_vprintf_core |
| 0xf22015e4 | `s_printf_pad` | speculative | under s_vprintf_core |
| 0xf2201618 | `s_vprintf_core` | speculative | 112-ins formatter under s_al_err_printf |
| 0xf2201716 | `s_al_err_printf` | speculative | error/log printer, called by al_ddr_*/spd_parse |
| 0xf220178c | `s_printf_helper` | speculative | under s_printf_emit |
| 0xf22017a4 | `s_spd4_timing_get` | speculative | helper of al_ddr4_spd_parse |
| 0xf220180e | `s_ddr_train_step` | speculative | callee of al_ddr_init |
| 0xf220186a | `s_ddr_delay_step` | speculative | callee of al_ddr_init (calls s_udelay) |
| 0xf2201894 | `al_ddr_mode_register_set` | confirmed | __func__ string "al_ddr_mode_register_set" |
| 0xf22018f4 | `al_ddr_cfg_init` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2201948 | `s_ddr_train_step2` | speculative | callee of al_ddr_init |
| 0xf2201968 | `al_ddr_ctrl_wait_for_normal_operating_mode` | confirmed | __func__ string "al_ddr_ctrl_wait_for_normal_operating_mode" |
| 0xf22019a0 | `al_ddr_phy_wait_for_init_done` | confirmed | __func__ string "al_ddr_phy_wait_for_init_done" |
| 0xf22019f4 | `s_ddr_train_step3` | speculative | callee of al_ddr_init |
| 0xf2201a10 | `al_ddr_phy_vt_calc_disable` | confirmed | __func__ string "al_ddr_phy_vt_calc_disable" |
| 0xf2201a90 | `al_ddr_init` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2203d7e | `s_pll_reg_a` | speculative | under al_pll_channel_div_set |
| 0xf2203d90 | `s_pll_reg_b` | speculative | under al_pll_channel_div_set |
| 0xf2203db0 | `al_pll_init` | confirmed | __func__ string "al_pll_init" |
| 0xf2203e50 | `al_pll_freq_set` | confirmed | __func__ string "al_pll_freq_set" |
| 0xf2203ee8 | `al_pll_channel_freq_get` | confirmed | __func__ string "al_pll_channel_freq_get" |
| 0xf2203f50 | `al_pll_channel_div_set` | confirmed | __func__ string "al_pll_channel_div_set" |
| 0xf2203ff8 | `_pre_boot_validate` | confirmed | __func__ string "_pre_boot_validate" |
| 0xf220410c | `al_flash_toc_validate` | confirmed | __func__ string "al_flash_toc_validate" |
| 0xf22041ec | `al_flash_toc_find_id_with_fallback` | confirmed | __func__ string "al_flash_toc_find_id_with_fallback" |
| 0xf22042c4 | `al_flash_toc_stage2_active_instance_get_with_fallback` | confirmed | __func__ string "al_flash_toc_stage2_active_instance_get_with_fallback" |
| 0xf2204394 | `al_flash_obj_header_read_and_validate` | confirmed | __func__ string "al_flash_obj_header_read_and_validate" |
| 0xf2204430 | `al_flash_obj_data_load` | confirmed | __func__ string "al_flash_obj_data_load" |
| 0xf22044b8 | `shared_params_write_magic` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf22044c8 | `s_shared_params_write_size` | speculative | orchestrator, after magic write |
| 0xf22044d0 | `i2c_eeprom_read` | confirmed | curated (ddr call graph / hand-RE) |
| 0xf2204518 | `s_udelay` | speculative | busy-wait, called by al_ddr_init/pll |
| 0xf2204574 | `al_i2c_xfer_finish` | confirmed | __func__ string "al_i2c_xfer_finish" |
| 0xf2204660 | `al_i2c_perform_write` | confirmed | __func__ string "al_i2c_perform_write" |
| 0xf2204694 | `s_i2c_xfer_a` | speculative | callee of i2c_eeprom_read |
| 0xf22046fc | `s_i2c_xfer_b` | speculative | callee of i2c_eeprom_read |
