/* @ 0x1000000  FUN_01000000 */

void FUN_01000000(undefined4 param_1)

{
  int iVar1;
  undefined4 extraout_r3;
  int iVar2;
  
  *DAT_0100007c = param_1;
  FUN_01000038();
  iVar1 = FUN_01000084();
  iVar2 = *(int *)(DAT_01000080 + iVar1 * 4);
  *(undefined4 *)(iVar2 + -4) = extraout_r3;
  *(BADSPACEBASE **)(iVar2 + -8) = register0x00000054;
  FUN_0100cbc8();
                    /* WARNING: Could not recover jumptable at 0x01000034. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(iVar2 + -4))();
  return;
}



/* @ 0x1000038  FUN_01000038 */

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Removing unreachable block (ram,0x0100004c) */

void FUN_01000038(void)

{
  return;
}



/* @ 0x1000084  FUN_01000084 */

uint FUN_01000084(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return uVar1 & 0xf;
}



/* @ 0x1000090  FUN_01000090 */

undefined4 FUN_01000090(void)

{
  return 0;
}



/* @ 0x10000a0  FUN_010000a0 */

int FUN_010000a0(void)

{
  uint uVar1;
  undefined4 in_cr15;
  
  coprocessor_movefromRt(0xf,0,in_cr15);
  uVar1 = coprocessor_movefromRt2(0xf,0,in_cr15);
  return (uVar1 & 0xf) * 2;
}



/* @ 0x10000b8  FUN_010000b8 */

void FUN_010000b8(uint param_1,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  undefined4 in_cr0;
  undefined4 in_cr7;
  undefined4 in_cr14;
  
  if (param_3 != 0) {
    uVar3 = coproc_movefrom_Control();
    coproc_moveto_Control(uVar3 & 0xfffffffb);
    InstructionSynchronizationBarrier(0xf);
    DataSynchronizationBarrier(0xf);
  }
  uVar3 = 0;
  do {
    if (uVar3 == 0) {
      iVar6 = 2;
    }
    else {
      iVar6 = 0x10;
    }
    coprocessor_moveto(0xf,2,0,uVar3,in_cr0,in_cr0);
    uVar1 = coprocessor_movefromRt(0xf,1,0,in_cr0,in_cr0);
    iVar2 = 0;
    do {
      iVar5 = 0;
      do {
        if (iVar6 == 2) {
          uVar4 = iVar2 << 0x1f;
        }
        else {
          uVar4 = iVar2 << 0x1c;
        }
        uVar4 = uVar4 | iVar5 << 6 | uVar3;
        if (param_2 == 0) {
          coproc_moveto_Invalidate_Entire_Data_by_Index(uVar4);
        }
        else {
          coprocessor_moveto(0xf,0,2,uVar4,in_cr7,in_cr14);
        }
        iVar5 = iVar5 + 1;
      } while (((DAT_01000220 & uVar1) >> 0xd) + 1 != iVar5);
      iVar2 = iVar2 + 1;
    } while (iVar2 != iVar6);
    InstructionSynchronizationBarrier(0xf);
    DataSynchronizationBarrier(0xf);
    bVar7 = uVar3 != (param_1 & 0xfffffffe);
    uVar3 = param_1 & 0xfffffffe;
  } while (bVar7);
  return;
}



/* @ 0x1000160  FUN_01000160 */

void FUN_01000160(void)

{
  FUN_010000b8();
  return;
}



/* @ 0x100016c  FUN_0100016c */

void FUN_0100016c(void)

{
  coproc_moveto_Invalidate_Entire_Instruction(0);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return;
}



/* @ 0x1000180  FUN_01000180 */

void FUN_01000180(undefined4 param_1,undefined4 param_2)

{
  coproc_moveto_Invalidate_unified_TLB_unlocked(param_2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return;
}



/* @ 0x10001e0  FUN_010001e0 */

ulonglong FUN_010001e0(undefined4 param_1)

{
  uint uVar1;
  undefined4 in_cr2;
  undefined4 in_cr10;
  
  coprocessor_moveto2(0xf,0,param_1,0,in_cr2);
  coprocessor_moveto2(0xf,1,param_1,0,in_cr2);
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x40);
  coproc_moveto_Translation_table_control(DAT_01000224);
  coprocessor_moveto(0xf,0,0,DAT_01000228,in_cr10,in_cr2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return CONCAT44(uVar1,DAT_01000228) | 0x4000000000;
}



/* @ 0x100022c  FUN_0100022c */

void FUN_0100022c(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  do {
    uVar1 = *param_2;
    uVar2 = param_2[1];
    param_2 = param_2 + 2;
    *param_1 = uVar1;
    param_1[1] = uVar2;
    param_1 = param_1 + 2;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  return;
}



/* @ 0x1000248  FUN_01000248 */

void FUN_01000248(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  
  do {
    uVar1 = *param_2;
    uVar2 = param_2[1];
    uVar3 = param_2[2];
    uVar4 = param_2[3];
    param_2 = param_2 + 4;
    *param_1 = uVar1;
    param_1[1] = uVar2;
    param_1[2] = uVar3;
    param_1[3] = uVar4;
    param_1 = param_1 + 4;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  return;
}



/* @ 0x1000264  FUN_01000264 */

void FUN_01000264(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  
  do {
    uVar1 = *param_2;
    uVar2 = param_2[1];
    uVar3 = param_2[2];
    uVar4 = param_2[3];
    uVar5 = param_2[4];
    uVar6 = param_2[5];
    uVar7 = param_2[6];
    uVar8 = param_2[7];
    param_2 = param_2 + 8;
    *param_1 = uVar1;
    param_1[1] = uVar2;
    param_1[2] = uVar3;
    param_1[3] = uVar4;
    param_1[4] = uVar5;
    param_1[5] = uVar6;
    param_1[6] = uVar7;
    param_1[7] = uVar8;
    param_1 = param_1 + 8;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  return;
}



/* @ 0x1000280  FUN_01000280 */

undefined8 FUN_01000280(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 in_cr14;
  
  uVar1 = coprocessor_movefromRt(0xf,0,in_cr14);
  uVar2 = coprocessor_movefromRt2(0xf,0,in_cr14);
  return CONCAT44(uVar2,uVar1);
}



/* @ 0x100036c  FUN_0100036c */

undefined4 FUN_0100036c(int param_1,int param_2,int param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  if (param_3 != 0 && param_1 == 0) {
    return 0xffffffea;
  }
  uVar3 = (uint)(param_1 == 0);
  uVar1 = (uint)(param_1 != 0);
  if (param_2 == 0) {
    uVar3 = uVar3 | 0x1000;
  }
  else {
    uVar1 = uVar1 | 0x1000;
  }
  if (param_3 == 0) {
    uVar3 = uVar3 | 4;
  }
  else {
    uVar1 = uVar1 | 4;
  }
  if (param_4 == 0) {
    uVar3 = uVar3 | 0x800;
  }
  else {
    uVar1 = uVar1 | 0x800;
  }
  uVar2 = coproc_movefrom_Control();
  coproc_moveto_Control((uVar1 | uVar2) & ~uVar3);
  DataSynchronizationBarrier(0xf);
  return 0;
}



/* @ 0x10003e0  thunk_FUN_010001e0 */

ulonglong thunk_FUN_010001e0(undefined4 param_1)

{
  uint uVar1;
  undefined4 in_cr2;
  undefined4 in_cr10;
  
  coprocessor_moveto2(0xf,0,param_1,0,in_cr2);
  coprocessor_moveto2(0xf,1,param_1,0,in_cr2);
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x40);
  coproc_moveto_Translation_table_control(DAT_01000224);
  coprocessor_moveto(0xf,0,0,DAT_01000228,in_cr10,in_cr2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return CONCAT44(uVar1,DAT_01000228) | 0x4000000000;
}



/* @ 0x10003e4  FUN_010003e4 */

void FUN_010003e4(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_01010bfc(&DAT_01028ee4,s_>>TC_>>_01028ed8);
  FUN_01010be0(&LAB_01010af4,0,param_1,&uStack_c);
  FUN_01010bfc(&DAT_01028ee4,s_al_flash_toc_find_id_failed__boo_01030940 + 0x28);
  return;
}



/* @ 0x1000454  FUN_01000454 */

void FUN_01000454(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_01000084();
  FUN_010003e4(s_CPU__d_failed_miserably_01028ee8,uVar1);
  if (param_1 != 0) {
    FUN_0100ceac(s_The_test_s_final_words_are___s_01028f04,param_1);
    FUN_01010bfc(s_The_test_s_final_words_are___s_01028f04,param_1);
  }
  FUN_010077fc(0x80ff);
  if (DAT_01049300 == 0) {
    FUN_01007270(0,1,0x100);
    DAT_01049300 = 1;
  }
  FUN_0100cef4(10000);
  FUN_0100ceac(s_Press__r__to_reset_01028f24);
  FUN_01010bfc(s_Press__r__to_reset_01028f24);
  FUN_0100ceac(s_Press_CTRL_R_to_print_trace_agai_01028f38);
  FUN_01010bfc(s_Press_CTRL_R_to_print_trace_agai_01028f38);
  FUN_0100ceac(s_Press_CTRL_F_to_print_full_trace_01028f5c);
  FUN_01010bfc(s_Press_CTRL_F_to_print_full_trace_01028f5c);
  do {
    while( true ) {
      do {
        iVar2 = uart_is_input_available(uVar1);
      } while (iVar2 == 0);
      iVar2 = uart_read_byte(uVar1);
      if (iVar2 == 0x72) break;
      if (iVar2 == 0x12) {
        FUN_0100cef4(10000);
      }
      else if (iVar2 == 6) {
        FUN_0100cef4(0);
      }
    }
    __wd0_board_reset();
  } while( true );
}



/* @ 0x1000578  FUN_01000578 */

void set_vectors(undefined4 param_1,int param_2)

{
  uint uVar1;
  undefined4 unaff_r4;
  
  DAT_01049304 = param_1;
  thunk_FUN_010001e0();
  if (param_2 == -0x10000) {
    uVar1 = coproc_movefrom_Control();
    coproc_moveto_Control(uVar1 | 0x2000);
    DataSynchronizationBarrier(0xf);
    return;
  }
  if (param_2 != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s_vectors_01028ea0,param_2,
                 s_arch_arm_src_mmu_c_01028e8c,s_set_vectors_01028e80,0x46,unaff_r4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s_vectors_01028ea0,param_2,
                 s_arch_arm_src_mmu_c_01028e8c,s_set_vectors_01028e80,0x46);
    FUN_01000454(0);
    uVar1 = coproc_movefrom_Control();
    coproc_moveto_Control(uVar1 & 0xffffdfff);
    DataSynchronizationBarrier(0xf);
    return;
  }
  uVar1 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar1 & 0xffffdfff);
  DataSynchronizationBarrier(0xf);
  return;
}



/* @ 0x10005a8  FUN_010005a8 */

/* WARNING: Control flow encountered bad instruction data */

void FUN_010005a8(void)

{
  FUN_01002e8c();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



/* @ 0x1000614  FUN_01000614 */

void FUN_01000614(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x100063c  FUN_0100063c */

void bac1d52(void)

{
  FUN_0100ceac(s__________________________________01028f80);
  FUN_01010bfc(s__________________________________01028f80);
  FUN_0100ceac(s_Stage_3_version___s_01028fc0,s_2_22_0_01028fb8);
  FUN_01010bfc(s_Stage_3_version___s_01028fc0,s_2_22_0_01028fb8);
  FUN_0100ceac(s_Commit_ID___s_01028fe0,s_6088bc3_01028fd8);
  FUN_01010bfc(s_Commit_ID___s_01028fe0,s_6088bc3_01028fd8);
  FUN_0100ceac(s_CVOS_commit_ID___s_01028ff8,s_bac1d52_01028ff0);
  FUN_01010bfc(s_CVOS_commit_ID___s_01028ff8,s_bac1d52_01028ff0);
  FUN_0100ceac(s_HAL_commit_ID___s_01029014,s_61afa9c_0102900c);
  FUN_01010bfc(s_HAL_commit_ID___s_01029014,s_61afa9c_0102900c);
  FUN_0100ceac(s_Build_date___s__s_01029040,s_Jan_15_2021_01029034,s_15_08_28_01029028);
  FUN_01010bfc(s_Build_date___s__s_01029040,s_Jan_15_2021_01029034,s_15_08_28_01029028);
  FUN_0100ceac(s__________________________________01029054);
  FUN_01010bfc(s__________________________________01029054);
  return;
}



/* @ 0x1000778  FUN_01000778 */

undefined4 FUN_01000778(int param_1,undefined1 *param_2,uint param_3)

{
  undefined1 *puVar1;
  uint uVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  uint uVar5;
  
  uVar5 = param_1 + 0xf8000000;
  if ((((uint)param_2 | uVar5) & 3) == 0 && 0x1f < param_3) {
    FUN_01000264(param_2,uVar5,param_3 >> 5);
    uVar2 = param_3 & 0xffffffe0;
    param_3 = param_3 & 0x1f;
    param_2 = param_2 + uVar2;
    uVar5 = uVar5 + uVar2;
  }
  if (param_3 != 0) {
    puVar1 = (undefined1 *)(uVar5 - 1);
    puVar3 = param_2;
    do {
      puVar1 = puVar1 + 1;
      puVar4 = puVar3 + 1;
      *puVar3 = *puVar1;
      puVar3 = puVar4;
    } while (puVar4 != param_2 + param_3);
  }
  return 0;
}



/* @ 0x1000834  FUN_01000834 */

undefined4 enabled(int param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  undefined1 auStack_6c [4];
  undefined1 auStack_68 [80];
  
  FUN_01010b84(auStack_68,s__soc_board_cfg_ethernet_port_d_01030608,param_1);
  iVar1 = FUN_01007f84(DAT_01049d88,auStack_68);
  if (iVar1 < 0) {
    uVar4 = 0xffffffea;
  }
  else {
    iVar2 = FUN_01007eb8(DAT_01049d88,iVar1,s_status_01030628,0);
    if ((iVar2 == 0) || (iVar2 = FUN_01012954(iVar2,s_enabled_01030630), iVar2 != 0)) {
      uVar4 = 0;
    }
    else {
      iVar2 = FUN_01007eb8(DAT_01049d88,iVar1,&DAT_01030638,0);
      if (iVar2 != 0) {
        iVar2 = FUN_01012954(iVar2,s_nbase_t_01030680);
        uVar4 = DAT_01049d88;
        (&DAT_01049324)[param_1] = (uint)(iVar2 == 0);
        iVar2 = FUN_01007e64(uVar4,iVar1,s_serdes_grp_01030688,auStack_6c);
        if (iVar2 == 0) {
          (&DAT_0104a57c)[param_1] = 3;
        }
        else {
          uVar3 = *(uint *)(iVar2 + 0xc);
          (&DAT_0104a57c)[param_1] =
               uVar3 << 0x18 | (uVar3 >> 8 & 0xff) << 0x10 | (uVar3 >> 0x10 & 0xff) << 8 |
               uVar3 >> 0x18;
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar1,s_serdes_lane_01030694,auStack_6c);
        if (iVar1 == 0) {
          *(int *)(&DAT_0104a58c + param_1 * 4) = 3 - param_1;
        }
        else {
          uVar3 = *(uint *)(iVar1 + 0xc);
          *(uint *)(&DAT_0104a58c + param_1 * 4) =
               uVar3 << 0x18 | (uVar3 >> 8 & 0xff) << 0x10 | (uVar3 >> 0x10 & 0xff) << 8 |
               uVar3 >> 0x18;
        }
        FUN_01010b84(auStack_68,s__soc_board_cfg_ethernet_port_d_e_010306a0,param_1);
        iVar1 = FUN_01007f84(DAT_01049d88,auStack_68);
        *(uint *)(&DAT_01049314 + param_1 * 4) = (uint)(0 < iVar1);
        (&DAT_0104a56c)[param_1] = 1;
        return 0;
      }
      uVar4 = 0xffffffea;
      FUN_0100ceac(s_ethernet_port___d__mode_not_spec_01030640,param_1);
      FUN_01010bfc(s_ethernet_port___d__mode_not_spec_01030640,param_1);
    }
  }
  if (((param_1 == 2) || (param_1 == 3)) || (param_1 == 1)) {
    FUN_01003f30(param_1,0);
  }
  else {
    FUN_01003f30(0);
  }
  return uVar4;
}



/* @ 0x1000a4c  FUN_01000a4c */

void FUN_01000a4c(int *param_1)

{
  undefined1 uStack_2d;
  undefined1 auStack_2c [4];
  int local_28;
  undefined1 auStack_24 [20];
  
  al_bootstrap_parse(0xfd8a8000,param_1);
  al_pll_init(0xfd860c00,s_NB_PLL_0103056c,param_1[5] != 25000000,auStack_24);
  al_pll_freq_get(auStack_24,&uStack_2d,auStack_2c);
  al_pll_channel_freq_get(auStack_24,0,&local_28);
  param_1[1] = local_28 * 1000;
  al_pll_init(0xfd860d00,s_CPU_PLL_01030574,param_1[5] != 25000000,auStack_24);
  al_pll_freq_get(auStack_24,&uStack_2d,auStack_2c);
  al_pll_channel_freq_get(auStack_24,0,&local_28);
  *param_1 = local_28 * 1000;
  return;
}



/* @ 0x1000b30  FUN_01000b30 */


undefined4 thermal_sensor_trim_init(void)

{
  int iVar1;
  uint uVar2;
  undefined1 auStack_50 [8];
  undefined4 local_48;
  undefined4 uStack_44;
  undefined4 local_40;
  undefined4 uStack_3c;
  undefined4 local_38;
  undefined1 auStack_34 [24];
  
  local_48 = DAT_0102908c;
  uStack_44 = DAT_01029090;
  local_40 = DAT_01029094;
  uStack_3c = DAT_01029098;
  local_38 = DAT_0102909c;
  FUN_0100533c();
  DAT_01049310 = &DAT_fbff4120;
  if (((_DAT_fbff4120 & 0xffffff00) == 0xf0e1d200) && (0xc2 < (_DAT_fbff4120 & 0xff))) {
    DAT_0104a50c = 1;
  }
  else {
    DAT_01049310 = (undefined1 *)0x0;
    DAT_0104a50c = 0;
    FUN_01000614();
  }
  FUN_01000a4c(DAT_01001104);
  uart_init_soft(0,DAT_01049da4,DAT_01049db8);
  FUN_0100ccdc(0);
  FUN_0100d2c0();
  time_init(1,DAT_01049d98,DAT_01049d9c,DAT_01049da4);
  bac1d52();
  iVar1 = FUN_01002f04();
  if ((iVar1 == 0) ||
     (iVar1 = FUN_01002f28(),
     *(uint *)(iVar1 + 0xc) < 2 || *(uint *)(iVar1 + 0xc) == 2 && *(int *)(iVar1 + 8) == 0)) {
    al_addr_map_dram_remap_set(0xfd8a8000,3,0,2,0x1e);
    al_addr_map_dram_remap_set(0xfd8a8000,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0,2,0x1e);
    FUN_01023a0c(0xfd8a8000,2,0,2,0xc0000000,0,2);
  }
  else {
    al_addr_map_dram_remap_set(0xfd8a8000,0,0,0,0x1f);
    al_addr_map_dram_remap_set(0xfd8a8000,5,0,0,0x1f);
    al_addr_map_dram_remap_set(0xfd8a8000,1,0,4,0x1f);
    al_addr_map_dram_remap_set(0xfd8a8000,6,0,4,0x1f);
    al_addr_map_dram_remap_set(0xfd8a8000,2,0,1,0x20);
    al_addr_map_dram_remap_set(0xfd8a8000,7,0,1,0x20);
    al_addr_map_dram_remap_set(0xfd8a8000,3,0,2,0x21);
    al_addr_map_dram_remap_set(0xfd8a8000,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0,2,0x21);
    FUN_01023a0c(0xfd8a8000,3,0,4,0x80000000,0,3);
  }
  al_pbs_axi_timeout_set(0xfd8a8000,0x2000,0x1002000);
  al_pbs_apb_mem_config_set(0xfd8a8000,&local_48);
  al_addr_map_dram_remap_set(0xfd8a8000,0x14,0,0,0xf);
  al_addr_map_dram_remap_set(0xfd8a8000,AL_I2C_INTR_MASK_START_DET_SHIFT,0xc0000000,0,0x1b);
  al_addr_map_dram_remap_set(0xfd8a8000,AL_I2C_INTR_MASK_GEN_CALL_SHIFT,0xc8000000,0,0x1b);
  al_addr_map_dram_remap_set(0xfd8a8000,AL_I2C_TAR_10BIT_ADDR_SHIFT,0xd0000000,0,0x1b);
  al_addr_map_dram_remap_set(0xfd8a8000,0xd,0xd8000000,0,0x1b);
  DAT_01049d90 = FUN_01021fd4(0xfd8a8000);
  DAT_01049d94 = FUN_01021fe0(0xfd8a8000);
  iVar1 = al_otp_handle_init(auStack_50,0xfd896000,0xfd8a8000);
  if (iVar1 == 0) {
    uVar2 = al_otp_read_word(auStack_50,0x1d);
    uVar2 = (uVar2 & 0x3ff) >> 6;
    if (uVar2 != 0) {
      iVar1 = FUN_01024b8c(auStack_34,0xfd860a00,0xfd8a8000);
      if (iVar1 == 0) {
        al_thermal_sensor_trim_set(auStack_34,uVar2);
      }
      else {
        FUN_0100ceac(s__s__al_thermal_sensor_handle_ini_01030df0,s_thermal_sensor_trim_init_01029110
                    );
        FUN_01010bfc(s__s__al_thermal_sensor_handle_ini_01030df0,s_thermal_sensor_trim_init_01029110
                    );
      }
    }
  }
  else {
    FUN_0100ceac(s__s__al_otp_handle_init_failed__01030dd0,s_thermal_sensor_trim_init_01029110);
    FUN_01010bfc(s__s__al_otp_handle_init_failed__01030dd0,s_thermal_sensor_trim_init_01029110);
  }
  _DAT_fd8a81dc = _DAT_fd8a81dc | 0x10000;
  al_sys_fabric_cluster_pd_pu_timer_set(PTR_DAT_010492cc,400);
  uRamf00700d0 = uRamf00700d0 & 0xffffcfff;
  _DAT_fc005098 = _DAT_fc005098 & 0xfffffff0;
  _DAT_fc105098 = _DAT_fc105098 & 0xfffffff0;
  _DAT_fc205098 = _DAT_fc205098 & 0xfffffff0;
  _DAT_fc305098 = _DAT_fc305098 & 0xfffffff0;
  _DAT_fc40930c = _DAT_fc40930c & 0xfffffff0;
  _DAT_fc40a004 = _DAT_fc40a004 & 0xfffffff0;
  _DAT_fc50930c = _DAT_fc50930c & 0xfffffff0;
  _DAT_fc50a004 = _DAT_fc50a004 & 0xfffffff0;
  if (DAT_0104a50c == 0) {
    FUN_010129d8(&SUB_fbff4200,&LAB_0102ef70,0x15fc);
    FUN_0100016c();
    (*(code *)&SUB_fbff4200)();
    FUN_01027558(DAT_01049d90 != 0 || DAT_01049d94 != 0);
    DAT_01049310 = &DAT_fbff4120;
    if (((_DAT_fbff4120 & 0xffffff00) != 0xf0e1d200) || ((_DAT_fbff4120 & 0xff) < 0xc3)) {
      DAT_01049310 = (undefined1 *)0x0;
      FUN_0100ceac(s__s__cpu_resume_regs_invalid__01030e1c,s_stg3_early_init_0102912c);
      FUN_01010bfc(s__s__cpu_resume_regs_invalid__01030e1c,s_stg3_early_init_0102912c);
    }
  }
  return 0;
}



/* @ 0x1001108  FUN_01001108 */


undefined4 stg3_board_init(void)

{
  ushort uVar1;
  undefined1 *puVar2;
  ushort uVar3;
  bool bVar4;
  int iVar5;
  undefined4 uVar6;
  byte bVar7;
  uint uVar8;
  undefined1 *puVar9;
  uint uVar10;
  undefined1 *puVar11;
  uint *puVar12;
  undefined4 local_1258;
  int local_1254;
  uint local_1250;
  int local_124c;
  uint local_1248 [4];
  undefined4 local_1238;
  undefined1 auStack_1228 [256];
  int local_1128 [64];
  undefined1 auStack_1028 [4100];
  
  local_1258 = 0;
  iVar5 = al_pll_init(0xfd860c00,s_NB_PLL_0103056c,DAT_01049dac != 25000000,auStack_1028);
  if (((iVar5 == 0) && (iVar5 = al_pll_freq_get(auStack_1028,local_1248), iVar5 == 0)) &&
     (iVar5 = al_pll_channel_freq_get(auStack_1028,0,local_1128), iVar5 == 0)) {
    DAT_01049d9c = local_1128[0] * 1000;
    pll_fixup_crypto_boost();
    if (DAT_01049db1 == '\0') goto LAB_0100119c;
LAB_010012ec:
    FUN_0100ceac(s_I2C_Preload_Disabled__010306c8);
    FUN_01010bfc(s_I2C_Preload_Disabled__010306c8);
    DAT_0104a3a8 = &DAT_01049024;
    DAT_0104a568 = 1;
LAB_01001318:
    bVar7 = DAT_fbff410c;
    FUN_01012ae0(&DAT_fbff4100,0,0x20);
    _DAT_fbff410a = 0x400;
    DAT_fbff410c = bVar7 & 3 | DAT_fbff410c;
  }
  else {
    FUN_0100ceac(s__s__PLL_init_failed_01030cc8,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__PLL_init_failed_01030cc8,s_stg3_board_init_0102913c);
    if (DAT_01049db1 != '\0') goto LAB_010012ec;
LAB_0100119c:
    DAT_0104a3a8 = &DAT_01049024;
    if (DAT_0104a568 != 0) goto LAB_01001318;
    if (DAT_fbff4100 == '\0') {
      FUN_0100ceac(s_Violation__zero_device_ID__010306e0);
      FUN_01010bfc(s_Violation__zero_device_ID__010306e0);
      DAT_0104a568 = 1;
      goto LAB_01001318;
    }
  }
  FUN_0100ceac(s_EEPROM_Revision_ID____02x_010306fc,DAT_fbff4102);
  FUN_01010bfc(s_EEPROM_Revision_ID____02x_010306fc,DAT_fbff4102);
  DAT_0104a59c = (uint)_DAT_fbff4100;
  FUN_0100ceac(s_Device_ID____04x_01030718,DAT_0104a59c);
  FUN_01010bfc(s_Device_ID____04x_01030718,DAT_0104a59c);
  bVar7 = DAT_fbff4103 & 3;
  if (bVar7 == 2) {
    DAT_0104a5a0 = 0x6f4312aa;
  }
  else if (bVar7 == 3) {
    DAT_0104a5a0 = 4000000000;
  }
  else if (bVar7 == 1) {
    DAT_0104a5a0 = 1600000000;
  }
  else {
    DAT_0104a5a0 = 0x4f790d55;
  }
  if ((DAT_fbff4103 & 4) == 0) {
    DAT_0104a5a4 = 0x20;
  }
  else {
    DAT_0104a5a4 = 0x40;
  }
  DAT_0104a5a8 = (DAT_fbff4103 & 0xf) >> 3;
  switch(DAT_fbff4108 >> 5) {
  case 0:
    DAT_0104a5ac = 1000000000;
    break;
  case 1:
    DAT_0104a5ac = 1400000000;
    break;
  case 2:
    DAT_0104a5ac = 1700000000;
    break;
  case 3:
    DAT_0104a5ac = 2000000000;
    break;
  case 4:
    DAT_0104a5ac = 2200000000;
    break;
  default:
    DAT_0104a5ac = 4000000000;
  }
  uVar8 = (DAT_fbff4103 & 0x3f) >> 4;
  if (uVar8 == 1) {
    DAT_0104a5b0 = 2;
  }
  else if (uVar8 == 3) {
    DAT_0104a5b0 = 4;
  }
  else {
    if (uVar8 != 0) {
      FUN_0100ceac(&DAT_0103072c);
      FUN_01010bfc(&DAT_0103072c);
    }
    DAT_0104a5b0 = 1;
  }
  puVar9 = DAT_010023b8;
  puVar11 = DAT_010023b8 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
  DAT_0104a5b4 = (DAT_fbff4104 & 0x7f) >> 6;
  DAT_0104a5b8 = (uint)(DAT_fbff4104 >> 7);
  DAT_0104a5bc = (DAT_fbff4109 & 0x3f) >> 4;
  DAT_0104a5c0 = (uint)(DAT_fbff4105 >> 6);
  DAT_0104a5c4 = (DAT_fbff4108 & 0x1f) >> 4;
  DAT_0104a5c8 = DAT_fbff4104 & 3;
  DAT_0104a5cc = (DAT_fbff4104 & 0x3f) >> 2;
  DAT_0104a5d0 = DAT_fbff4105 & 3;
  DAT_0104a5d4 = (DAT_fbff4105 & 0x3f) >> 2;
  DAT_0104a5d8 = DAT_fbff4106 & 3;
  DAT_0104a5dc = (DAT_fbff4106 & 0xf) >> 2;
  DAT_0104a5e0 = (DAT_fbff4106 & 0x3f) >> 4;
  DAT_0104a5e4 = (uint)(DAT_fbff4106 >> 6);
  DAT_0104a5e8 = DAT_fbff4107 & 3;
  DAT_0104a5ec = (DAT_fbff4107 & 0xf) >> 2;
  DAT_0104a5f0 = (DAT_fbff4107 & 0x3f) >> 4;
  DAT_0104a5f4 = (uint)(DAT_fbff4107 >> 6);
  DAT_0104a5f8 = DAT_fbff4108 & 3;
  DAT_0104a600 = 3;
  DAT_0104a604 = 2;
  DAT_0104a5fc = (DAT_fbff4108 & 0xf) >> 2;
  DAT_0104a608 = DAT_fbff4109 & 1;
  DAT_0104a60c = (DAT_fbff4109 & 3) >> 1;
  DAT_0104a610 = (DAT_fbff4109 & 7) >> 2;
  DAT_0104a618 = 1;
  DAT_0104a614 = (DAT_fbff4109 & 0xf) >> 3;
  DAT_0104a61c = (uint)_DAT_fbff410a;
  FUN_0100ceac(s_Device_Info__01030734);
  FUN_01010bfc(s_Device_Info__01030734);
  do {
    puVar2 = puVar9 + -0x505650b;
    puVar9 = puVar9 + 1;
    *puVar9 = *puVar2;
    FUN_0100ceac(&DAT_01030744,*puVar2);
    FUN_01010bfc(&DAT_01030744,*puVar9);
  } while (puVar9 != puVar11);
  FUN_0100ceac(s_al_flash_toc_find_id_failed__boo_01030940 + 0x28);
  FUN_01010bfc(s_al_flash_toc_find_id_failed__boo_01030940 + 0x28);
  iVar5 = FUN_01003444();
  if (iVar5 != 0) {
    FUN_0100ceac(s__s__I2C_initialization_failed__01030748,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__I2C_initialization_failed__01030748,s_stg3_board_init_0102913c);
  }
  if (DAT_0104a568 == 0) {
    iVar5 = eeprom_preload_parser(auStack_1028,&local_1258,auStack_1228,local_1128);
    if (iVar5 == 0) {
      if ((DAT_0104a568 == 0) &&
         (iVar5 = FUN_01002f38(auStack_1028,local_1258,auStack_1228,local_1128), iVar5 != 0)) {
        FUN_0100ceac(s__s__failed_to_authenticate_EEPRO_0103078c,s_stg3_board_init_0102913c);
        FUN_01010bfc(s__s__failed_to_authenticate_EEPRO_0103078c,s_stg3_board_init_0102913c);
        DAT_0104a568 = 1;
      }
    }
    else {
      FUN_0100ceac(s__s__failed_to_read_EEPROM_conten_01030768,s_stg3_board_init_0102913c);
      FUN_01010bfc(s__s__failed_to_read_EEPROM_conten_01030768,s_stg3_board_init_0102913c);
      DAT_0104a568 = 1;
    }
  }
  if (DAT_0104a5a0 < (uint)(DAT_01049d9c << 1)) {
    FUN_0100ceac(s_DRAM_frequency_violation__010307bc);
    FUN_01010bfc(s_DRAM_frequency_violation__010307bc);
    DAT_0104a568 = 1;
  }
  if (DAT_0104a5ac < DAT_01049d98) {
    FUN_0100ceac(s_CPU_frequency_violation__010307d8);
    FUN_01010bfc(s_CPU_frequency_violation__010307d8);
    DAT_0104a568 = 1;
  }
  switch(DAT_01049db4) {
  case 0:
    DAT_01049d8c = (code *)&LAB_01000768;
    break;
  case 1:
    DAT_01049d8c = (code *)&LAB_0100080c;
    DAT_0104a510 = 1;
    break;
  case 2:
  case 3:
    DAT_01049d8c = FUN_01000778;
    break;
  default:
    FUN_0100ceac(s__s__bootstrap_s_boot_device_isnt_010307f4,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__bootstrap_s_boot_device_isnt_010307f4,s_stg3_board_init_0102913c);
    DAT_01049d8c = FUN_01000778;
    DAT_0104a568 = 1;
  }
  if ((DAT_fbff410c & 2) == 0) {
    if ((DAT_fbff410c & 1) == 0) {
      if (DAT_0104a510 == 0) goto LAB_01001628;
    }
    else {
      DAT_0104a510 = 1;
      DAT_01049d8c = (code *)&LAB_0100080c;
    }
    FUN_0100ceac(s_UART_recovery__01030848);
    FUN_01010bfc(s_UART_recovery__01030848);
  }
  else {
    DAT_0104a510 = 0;
    DAT_01049d8c = FUN_01000778;
  }
LAB_01001628:
  iVar5 = FUN_01027bc8(DAT_01049d8c,0,0x20000,0x10,DAT_010023bc);
  if (iVar5 == 0) {
    iVar5 = al_flash_toc_stage2_active_instance_get_with_fallback(0xfbff4000,0,DAT_01049d8c,DAT_0104a500,&local_1250,&local_124c);
    if (iVar5 != 0) {
      FUN_0100ceac(s_al_flash_toc_stage2_active_insta_01030878);
      FUN_01010bfc(s_al_flash_toc_stage2_active_insta_01030878);
      local_1250 = 0;
      local_124c = 0;
    }
    uVar8 = (*DAT_01049d8c)(0x1f000c,&DAT_0104930a,2);
    if (uVar8 == 0) {
      uVar3 = DAT_0104930a >> 8;
      uVar1 = DAT_0104930a << 8;
      DAT_0104930a = uVar3 | uVar1;
      FUN_01010bfc(s_subsystem_id__0x_04x_010308cc,uVar3 | uVar1);
      uVar8 = (*DAT_01049d8c)(0x1f0010,&DAT_0104930c);
      if (uVar8 == 0) {
        uVar8 = DAT_0104930c << 0x18 | (DAT_0104930c >> 8 & 0xff) << 0x10 |
                (DAT_0104930c >> 0x10 & 0xff) << 8 | DAT_0104930c >> 0x18;
        DAT_01049309 = (undefined1)(DAT_0104930c >> 0x18);
        DAT_0104930c = uVar8;
        FUN_01010bfc(s_hardware_revision_id__0x_08x_0103090c,uVar8);
        switch(DAT_0104930a) {
        case 0xea16:
          uVar10 = 0;
          DAT_01049308 = 0;
          break;
        default:
          uVar10 = 0xff;
          DAT_01049308 = 0xff;
          break;
        case 0xea1a:
          uVar10 = 3;
          DAT_01049308 = 3;
          break;
        case 0xea20:
          uVar10 = 1;
          DAT_01049308 = 1;
          break;
        case 0xea21:
          uVar10 = 2;
          DAT_01049308 = 2;
          break;
        case 0xea30:
          uVar10 = 4;
          DAT_01049308 = 4;
        }
      }
      else {
        uVar10 = uVar8 & 0xff;
        FUN_01010bfc(s_fetch_hardware_revision_ID_faile_010308e4,uVar8);
      }
    }
    else {
      uVar10 = uVar8 & 0xff;
      FUN_01010bfc(s_fetch_subsystem_ID_failed___d_010308ac,uVar8);
    }
    DAT_0104a504 = uVar10;
    local_1250 = uVar10;
    FUN_01010bfc(s_instance_num____d_0103092c,uVar10);
    DAT_0104a508 = local_124c;
    iVar5 = FUN_01027de8(DAT_01049d8c,DAT_0104a500,0,0,&local_1254,local_1248);
    if (iVar5 == 0) {
      if ((-1 < local_1254) &&
         (iVar5 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_1238,&DAT_010493dc), iVar5 == 0)) {
        if (DAT_01049404 < 0x41) {
          iVar5 = al_flash_obj_data_load(DAT_01049d8c,local_1238,DAT_010023c0);
          if (iVar5 != 0) {
            FUN_0100ceac(s_al_flash_obj_data_load_failed__b_01030980);
            FUN_01010bfc(s_al_flash_obj_data_load_failed__b_01030980);
            goto LAB_01001878;
          }
          goto LAB_01001ee0;
        }
        FUN_0100ceac(s_Boot_mode_too_big__0103096c);
        FUN_01010bfc(s_Boot_mode_too_big__0103096c);
      }
      DAT_0104a518 = 5;
    }
    else {
      FUN_0100ceac(s_al_flash_toc_find_id_failed__boo_01030940);
      FUN_01010bfc(s_al_flash_toc_find_id_failed__boo_01030940);
      DAT_0104a518 = 5;
    }
LAB_01001ee0:
    iVar5 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,local_1250 << 0x1c | 2,local_124c << 0x1c | 2,0,
                         &local_1254,local_1248);
    if (iVar5 == 0) {
      if (local_1254 < 0) {
        FUN_0100ceac(s_DT_not_found_in_TOC__010309dc);
        FUN_01010bfc(s_DT_not_found_in_TOC__010309dc);
      }
      else {
        iVar5 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_1238,&DAT_01049424);
        if (iVar5 == 0) {
          DAT_0104946c = local_1238;
          DAT_01049470 = 1;
        }
        else {
          FUN_0100ceac(s_al_flash_obj_header_read_and_val_010309f4,local_1250);
          FUN_01010bfc(s_al_flash_obj_header_read_and_val_010309f4,local_1250);
        }
      }
    }
    else {
      FUN_0100ceac(s_al_flash_toc_find_id_failed__DT__010309ac,local_1250);
      FUN_01010bfc(s_al_flash_toc_find_id_failed__DT__010309ac,local_1250);
    }
    iVar5 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,local_1250 << 0x1c | 0x100,
                         local_124c << 0x1c | 0x100,0,&local_1254,local_1248);
    if (((iVar5 == 0) && (-1 < local_1254)) &&
       (iVar5 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_1238,&DAT_0104933c), iVar5 == 0)) {
      DAT_01049384 = local_1238;
      DAT_01049388 = 1;
    }
    FUN_01005860();
LAB_01001fec:
    iVar5 = uart_is_input_available(0);
    if (iVar5 != 0) goto LAB_010020a0;
    while( true ) {
      iVar5 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,DAT_0104a518 | local_1250 << 0x1c,
                           DAT_0104a518 | local_124c << 0x1c,0,&local_1254,local_1248);
      uVar8 = DAT_0104a518;
      if (iVar5 != 0) {
        FUN_0100ceac(s_al_flash_toc_find_id_failed__app_01030aa8,local_1250);
        FUN_01010bfc(s_al_flash_toc_find_id_failed__app_01030aa8,local_1250);
        goto LAB_010018b0;
      }
      if (-1 < local_1254) break;
      uVar6 = al_flash_obj_id_to_str(DAT_0104a518);
      FUN_0100ceac(s_App_not_found_in_TOC___x__s__u___01030adc,uVar8,uVar6,local_1250);
      uVar8 = DAT_0104a518;
      uVar6 = al_flash_obj_id_to_str(DAT_0104a518);
      FUN_01010bfc(s_App_not_found_in_TOC___x__s__u___01030adc,uVar8,uVar6,local_1250);
      if (DAT_0104a518 == 5) goto LAB_010018b0;
      while( true ) {
        DAT_0104a518 = 5;
        iVar5 = uart_is_input_available(0);
        if (iVar5 == 0) break;
LAB_010020a0:
        uVar8 = uart_read_byte(0);
        if (uVar8 == 0x70) {
          if (DAT_01049474 == 0) {
            FUN_0100ceac(s_User_request_for_skipping_APCEA__01030a38);
            FUN_01010bfc(s_User_request_for_skipping_APCEA__01030a38);
            DAT_01049474 = 1;
          }
        }
        else {
          if ((uVar8 == 0x65) && (DAT_01049338 == 0)) {
            FUN_0100ceac(s_User_request_for_keeping_early_A_01030a5c);
            FUN_01010bfc(s_User_request_for_keeping_early_A_01030a5c);
            DAT_01049338 = 1;
          }
          if ((uVar8 & 0xef) != 0x65) goto LAB_01001fec;
        }
        if (DAT_0104a518 == 5) goto LAB_01001fec;
        FUN_0100ceac(s_User_request_for_loading_U_Boot__01030a84);
        FUN_01010bfc(s_User_request_for_loading_U_Boot__01030a84);
      }
    }
    iVar5 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_1238,&DAT_0104938c);
    if (iVar5 == 0) {
      DAT_010493d4 = local_1238;
      DAT_010493d8 = 1;
    }
    else {
      FUN_0100ceac(s_al_flash_obj_header_read_and_val_01030b00,local_1250);
      FUN_01010bfc(s_al_flash_obj_header_read_and_val_01030b00,local_1250);
    }
  }
  else {
    FUN_0100ceac(s_al_flash_toc_search_failed__01030858);
    FUN_01010bfc(s_al_flash_toc_search_failed__01030858);
LAB_01001878:
    FUN_0100ceac(s__s__TOC_parsing_failed__01030b44,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__TOC_parsing_failed__01030b44,s_stg3_board_init_0102913c);
  }
LAB_010018b0:
  pci_devices_init();
  if (DAT_01049470 == 0) {
    FUN_0100ceac(s_Loading_DT_from__08x_to__08X___u_01030bbc,0xc0000,0x1100000,0x10000);
    FUN_01010bfc(s_Loading_DT_from__08x_to__08X___u_01030bbc,0xc0000,0x1100000,0x10000);
    (*DAT_01049d8c)(0xc0000,0x1100000,0x10000);
LAB_01001908:
    DAT_01049d88 = 0x1100000;
    FUN_01010bfc(s_obj_hdr_dt_offset__0x_04x_01030be8,DAT_0104946c);
  }
  else {
    FUN_0100ceac(s_Loading_DT_to__08X___u_bytes_____01030b60,0x1100000,DAT_0104944c);
    FUN_01010bfc(s_Loading_DT_to__08X___u_bytes_____01030b60,0x1100000,DAT_0104944c);
    iVar5 = al_flash_obj_data_load(DAT_01049d8c,DAT_0104946c,0x1100000);
    if (iVar5 == 0) goto LAB_01001908;
    FUN_0100ceac(s_al_flash_obj_data_load_failed__01030b84);
    FUN_01010bfc(s_al_flash_obj_data_load_failed__01030b84);
    FUN_0100ceac(s__s__DT_reading_failed__01030ba4,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__DT_reading_failed__01030ba4,s_stg3_board_init_0102913c);
  }
  iVar5 = FUN_01007f84(DAT_01049d88,s__soc_board_cfg_01030c04);
  if ((iVar5 < 0) || (iVar5 = FUN_01007eb8(DAT_01049d88,iVar5,&DAT_01030c14,0), iVar5 == 0)) {
    FUN_0100ceac(s__s__DT_based_initialization_fail_01030ca0,s_stg3_board_init_0102913c);
    FUN_01010bfc(s__s__DT_based_initialization_fail_01030ca0,s_stg3_board_init_0102913c);
  }
  else {
    FUN_0100ceac(s_Board_config_ID___s_01030c18,iVar5);
    FUN_01010bfc(s_Board_config_ID___s_01030c18,iVar5);
    iVar5 = FUN_01007f84(DAT_01049d88,s__soc_board_cfg_pcie_01030c30);
    if (((-1 < iVar5) &&
        (iVar5 = FUN_01007e64(DAT_01049d88,iVar5,s_ep_ports_01030c44,local_1248), iVar5 != 0)) &&
       (local_1248[0] = local_1248[0] >> 2, local_1248[0] != 0)) {
      puVar12 = (uint *)(iVar5 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
      iVar5 = 0;
      do {
        while( true ) {
          puVar12 = puVar12 + 1;
          uVar8 = *puVar12;
          uVar8 = uVar8 << 0x18 | (uVar8 >> 8 & 0xff) << 0x10 | (uVar8 >> 0x10 & 0xff) << 8 |
                  uVar8 >> 0x18;
          if (uVar8 < 4) break;
          iVar5 = iVar5 + 1;
          FUN_0100ceac(s__s__invalid_port_num___d___01030c50,s_dt_based_init_pcie_010290ec,uVar8);
          FUN_01010bfc(s__s__invalid_port_num___d___01030c50,s_dt_based_init_pcie_010290ec,uVar8);
          if ((int)local_1248[0] <= iVar5) goto LAB_01001a44;
        }
        iVar5 = iVar5 + 1;
        *(undefined4 *)(DAT_0104a3a8 + uVar8 * 0x14 + 0xc) = 1;
      } while (iVar5 < (int)local_1248[0]);
    }
LAB_01001a44:
    enabled(0);
    enabled(1);
    enabled(2);
    enabled(3);
    iVar5 = dt_based_init_serdes_group();
    if (iVar5 != 0) {
      FUN_0100ceac(s__s__serdes_initialization_failed_01030c6c,s_dt_based_init_01029100);
      FUN_01010bfc(s__s__serdes_initialization_failed_01030c6c,s_dt_based_init_01029100);
    }
    iVar5 = FUN_01007f84(DAT_01049d88,s__hypervisor__01030c90);
    if (-1 < iVar5) {
      DAT_01049334 = 1;
    }
  }
  if (DAT_0104a5b4 == 0) {
LAB_01001c04:
    if (DAT_0104a5c0 != 0) {
      if (DAT_0104a568 != 0) goto LAB_01001aec;
      FUN_01003f30(4,1);
    }
  }
  else {
    if (DAT_0104a568 == 0) {
      FUN_01003f60(0,1);
      FUN_01003f60(1);
      FUN_01003f60(2,1);
      FUN_01003f60(3,1);
      goto LAB_01001c04;
    }
    if (DAT_0104a5c0 != 0) goto LAB_01001aec;
  }
  if ((DAT_0104a5b8 != 0) && (DAT_0104a568 == 0)) {
    FUN_01003f60(5,1);
    FUN_01003f60(4,1);
  }
LAB_01001aec:
  if (DAT_01049310 == 0) {
    FUN_0100ceac(s__s__cpu_resume_regs_invalid__01030e1c,s_power_down_secondary_cpus_010290c0);
    FUN_01010bfc(s__s__cpu_resume_regs_invalid__01030e1c,s_power_down_secondary_cpus_010290c0);
    DAT_0104a568 = 1;
  }
  else {
    if (DAT_01049334 == 0) {
      uVar6 = 0;
    }
    else {
      uVar6 = 2;
    }
    *(undefined4 *)(DAT_01049310 + 4) = uVar6;
    uVar8 = 1;
    do {
      bVar4 = true;
      if (DAT_0104a568 == 0) {
        if (uVar8 < DAT_0104a5b0) {
          bVar4 = false;
        }
        else {
          bVar4 = true;
        }
      }
      al_sys_fabric_core_aarch64_setup(PTR_DAT_010492cc,uVar8,0,0x1000);
      InstructionSynchronizationBarrier(0xf);
      DataSynchronizationBarrier(0xf);
      if (bVar4) {
        iVar5 = DAT_01049310 + uVar8 * 8;
        uVar10 = *(uint *)(iVar5 + 8);
        *(uint *)(iVar5 + 8) = ~uVar10 & 4 ^ uVar10;
      }
      uVar10 = uVar8 + 1;
      al_sys_fabric_core_reset_deassert(PTR_DAT_010492cc,uVar8);
      uVar8 = uVar10;
    } while (uVar10 != 4);
  }
  FUN_0100ca0c(0,1);
  enabled(DAT_01049d88);
  return 0;
}



/* @ 0x100245c  FUN_0100245c */

undefined4 dram_clear(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  uint local_50;
  uint uStack_4c;
  undefined1 auStack_38 [20];
  
  iVar1 = iodma_pci_init();
  if (iVar1 != 0) {
    FUN_0100ceac(s__s__iodma_init_failed__0103057c,s_dram_clear_0102914c);
    FUN_01010bfc(s__s__iodma_init_failed__0103057c,s_dram_clear_0102914c);
    return 0xffffffff;
  }
  iVar1 = FUN_01002f04();
  if (iVar1 == 0) {
    local_50 = 0x20000000;
    uStack_4c = 0;
  }
  else {
    iVar1 = FUN_01002f28();
    local_50 = *(uint *)(iVar1 + 8);
    uStack_4c = *(uint *)(iVar1 + 0xc);
    if (uStack_4c == 0 && local_50 < 0x2000001) {
LAB_0100274c:
      FUN_01004430();
      al_ddr_rev_get(0xf0070000,0xf0080000,0xf0088000,auStack_38);
      FUN_01021e48(auStack_38);
      FUN_01021e5c(auStack_38);
      FUN_01021eac(auStack_38);
      FUN_01021ec0(auStack_38);
      return 0;
    }
  }
  uVar2 = 0x2000000;
  uVar3 = 0;
  do {
    while (uVar4 = uVar2, uVar5 = uVar3, uStack_4c < 2 || uStack_4c == 2 && local_50 == 0) {
      if (uVar3 == 0 && uVar2 < 0xc0000000) {
        uVar7 = 0xc0000000;
        uVar8 = 0;
      }
      else if (uVar3 == 0) {
        uVar7 = 0;
        uVar8 = 1;
        uVar4 = uVar2 + 0x40000000;
        uVar5 = (0xbfffffff < uVar2) + 1;
      }
      else {
        bVar9 = uVar3 != 0;
        if (uVar3 == 1) {
          bVar9 = 0xfffffffe < uVar2;
        }
        if (bVar9 && (uVar3 != 1 || uVar2 != 0xffffffff)) {
LAB_01002724:
          FUN_0100ceac(s__s__DDR_size_not_supported__01030594,s_dram_clear_0102914c);
          FUN_01010bfc(s__s__DDR_size_not_supported__01030594,s_dram_clear_0102914c);
          goto LAB_0100274c;
        }
        uVar7 = 0;
        uVar8 = 2;
      }
LAB_01002500:
      bVar9 = uStack_4c <= uVar8;
      if (uVar8 == uStack_4c) {
        bVar9 = local_50 <= uVar7;
      }
      if (bVar9 && (uVar8 != uStack_4c || uVar7 != local_50)) {
        uVar8 = uStack_4c;
        uVar7 = local_50;
      }
      iVar1 = (uVar7 - uVar2) + uVar4;
      iVar6 = (uVar8 - (uVar3 + (uVar7 < uVar2))) + uVar5 + (uint)CARRY4(uVar7 - uVar2,uVar4);
      FUN_0100ceac(s__s__clearing_physical_memory__08_010305b4,s_dram_clear_0102914c,uVar5,uVar4,
                   iVar6,iVar1);
      FUN_01010bfc(s__s__clearing_physical_memory__08_010305b4,s_dram_clear_0102914c,uVar5,uVar4,
                   iVar6,iVar1);
      iVar1 = al_udma_desc_action_add(uVar4,uVar5,0);
      if (iVar1 != 0) goto LAB_01002654;
      bVar9 = uStack_4c <= uVar8;
      if (uVar8 == uStack_4c) {
        bVar9 = local_50 <= uVar7;
      }
      uVar2 = uVar7;
      uVar3 = uVar8;
      if (bVar9) goto LAB_0100274c;
    }
    if (uVar3 == 0 && uVar2 < 0x80000000) {
      uVar7 = 0x80000000;
      uVar8 = 0;
    }
    else if (uVar3 == 0) {
      uVar7 = 0;
      uVar8 = 1;
      uVar4 = uVar2 + 0x80000000;
      uVar5 = (0x7fffffff < uVar2) + 3;
    }
    else {
      bVar9 = uVar3 != 0;
      if (uVar3 == 1) {
        bVar9 = 0xfffffffe < uVar2;
      }
      if (bVar9 && (uVar3 != 1 || uVar2 != 0xffffffff)) {
        uVar8 = 4;
        uVar7 = 0;
        if (uVar3 == 4 && uVar2 == 0) goto LAB_01002724;
        goto LAB_01002500;
      }
      uVar7 = 0;
      uVar8 = 2;
    }
    iVar1 = uVar4 + (uVar7 - uVar2);
    iVar6 = uVar5 + (uVar8 - (uVar3 + (uVar7 < uVar2))) + (uint)CARRY4(uVar4,uVar7 - uVar2);
    FUN_0100ceac(s__s__clearing_physical_memory__08_010305b4,s_dram_clear_0102914c,uVar5,uVar4,iVar6
                 ,iVar1);
    FUN_01010bfc(s__s__clearing_physical_memory__08_010305b4,s_dram_clear_0102914c,uVar5,uVar4,iVar6
                 ,iVar1);
    iVar1 = al_udma_desc_action_add(uVar4,uVar5,0);
    uVar2 = uVar7;
    uVar3 = uVar8;
  } while (iVar1 == 0);
LAB_01002654:
  FUN_0100ceac(s__s__iodma_memset_failed__010305ec,s_dram_clear_0102914c);
  FUN_01010bfc(s__s__iodma_memset_failed__010305ec,s_dram_clear_0102914c);
  return 0xffffffff;
}



/* @ 0x1002800  FUN_01002800 */

undefined4 FUN_01002800(void)

{
  undefined4 *puVar1;
  int *piVar2;
  undefined8 uVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  undefined4 *puVar10;
  int *piVar11;
  uint local_6c;
  int local_68 [2];
  undefined1 auStack_60 [16];
  undefined8 local_50;
  undefined4 local_48;
  undefined4 uStack_44;
  int local_40;
  int local_3c [3];
  undefined8 local_30;
  undefined4 local_28;
  undefined4 uStack_24;
  
  iVar7 = 0;
  piVar11 = local_3c;
  local_50 = DAT_010290a0;
  local_48 = DAT_010290a8;
  uStack_44 = DAT_010290ac;
  local_40 = DAT_0104a5d8;
  local_3c[0] = DAT_0104a5dc;
  local_3c[1] = DAT_0104a5e0;
  local_3c[2] = DAT_0104a5e4;
  local_30 = DAT_010290b0;
  uVar3 = local_30;
  local_28 = DAT_010290b8;
  uStack_24 = DAT_010290bc;
  puVar10 = (undefined4 *)((int)&local_30 + 4);
  local_30._0_4_ = (undefined4)DAT_010290b0;
  uVar5 = (undefined4)local_30;
  iVar6 = DAT_0104a5d8;
  local_30 = uVar3;
  if (DAT_0104a5d8 == 2) goto LAB_010028e4;
LAB_01002890:
  iVar4 = FUN_01003f48(uVar5);
  puVar1 = puVar10;
  piVar2 = piVar11;
  if (iVar4 == 0) goto joined_r0x010028d0;
  iVar4 = DAT_0104a568;
  if (iVar6 != 3) goto LAB_010028b0;
  do {
    puVar1 = puVar10;
    piVar2 = piVar11;
    if (iVar4 != 0) {
LAB_010028b0:
      iVar6 = *(int *)((int)&local_50 + iVar7 * 4);
      *(uint *)(iVar6 + 0x964) = *(uint *)(iVar6 + 0x964) & 0xffff3fff | 0x4000;
      puVar1 = puVar10;
      piVar2 = piVar11;
    }
joined_r0x010028d0:
    do {
      iVar7 = iVar7 + 1;
      if (iVar7 == 4) {
        if (DAT_0104a568 != 0) {
          DAT_fbff4100 = 0;
        }
        al_ddr_rev_get(0xf0070000,0xf0080000,0xf0088000,auStack_60);
        FUN_01021e18(auStack_60,local_68);
        if (local_68[0] == 1) {
          dram_clear();
        }
        iVar7 = FUN_01007f84(DAT_01049d88,s__soc_trace_01030e3c);
        if (-1 < iVar7) {
          iVar7 = FUN_01007e64(DAT_01049d88,iVar7,&DAT_01030e48,&local_6c);
          if (iVar7 == 0) {
            FUN_0100ceac(s_Trace__reg__property_not_found__01030e88);
            FUN_01010bfc(s_Trace__reg__property_not_found__01030e88);
          }
          else {
            local_6c = local_6c >> 2;
            if (local_6c == 4) {
              uVar8 = *(uint *)(iVar7 + 0x10);
              uVar9 = *(uint *)(iVar7 + 0x18);
              uVar8 = uVar8 << 0x18 | (uVar8 >> 8 & 0xff) << 0x10 | (uVar8 >> 0x10 & 0xff) << 8 |
                      uVar8 >> 0x18;
              iVar7 = uVar8 + (uVar9 << 0x18 | (uVar9 >> 8 & 0xff) << 0x10 |
                               (uVar9 >> 0x10 & 0xff) << 8 | uVar9 >> 0x18);
              FUN_0100ceac(s_Trace_moved_to__08x____08x_01030e4c,uVar8,iVar7);
              FUN_01010bfc(s_Trace_moved_to__08x____08x_01030e4c,uVar8,iVar7);
              if (uVar8 != 0) {
                FUN_0100cd38(0,uVar8,iVar7);
                return 0;
              }
            }
            else {
              FUN_0100ceac(s_Trace__reg__property_invalid__01030e68);
              FUN_01010bfc(s_Trace__reg__property_invalid__01030e68);
            }
          }
        }
        FUN_0100cdf4(0);
        return 0;
      }
      piVar11 = piVar2 + 1;
      iVar6 = *piVar2;
      puVar10 = puVar1 + 1;
      uVar5 = *puVar1;
      if (iVar6 != 2) goto LAB_01002890;
LAB_010028e4:
      if (((&DAT_01049324)[iVar7] != 1) || (*(int *)(&DAT_01049314 + iVar7 * 4) != 1)) {
        iVar4 = FUN_01003f48(uVar5);
        break;
      }
      iVar6 = FUN_01003f48(uVar5);
      puVar1 = puVar10;
      piVar2 = piVar11;
      iVar4 = DAT_0104a568;
    } while (iVar6 == 0);
  } while( true );
}



/* @ 0x1002a88  FUN_01002a88 */

/* WARNING: Restarted to delay deadcode elimination for space: ram */

void FUN_01002a88(void)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  uint local_24;
  undefined1 auStack_20 [8];
  
  if ((DAT_01049474 != 0 || DAT_01049338 != 0) ||
     ((DAT_01049388 != 0 &&
      (iVar1 = al_flash_obj_data_load(DAT_01049d8c,DAT_01049384,DAT_0104936c), iVar1 != 0)))) {
    DAT_01049388 = 0;
  }
  if (DAT_010493d8 == 0) {
    (*DAT_01049d8c)(0x40000,&local_24);
    FUN_0100ceac(s_Loading_application_from__08x_to_01030d0c,0x40000,0x1100000,local_24);
    FUN_01010bfc(s_Loading_application_from__08x_to_01030d0c,0x40000,0x1100000,local_24);
    if (0x100000 < local_24) {
      FUN_0100ceac(s_Invalid_application_size__01030d44);
      FUN_01010bfc(s_Invalid_application_size__01030d44);
      goto LAB_01002c8c;
    }
    (*DAT_01049d8c)(0x40004);
    uVar3 = 0x1100000;
  }
  else {
    FUN_0100ceac(s_Loading_application_to__08X___u_b_01030ce0,DAT_010493bc,DAT_010493b4);
    FUN_01010bfc(s_Loading_application_to__08X___u_b_01030ce0,DAT_010493bc,DAT_010493b4);
    iVar1 = al_flash_obj_data_load(DAT_01049d8c,DAT_010493d4,DAT_010493bc);
    uVar3 = DAT_010493c4;
    if (iVar1 != 0) {
      FUN_0100ceac(s_al_flash_obj_data_load_failed__01030b84);
      FUN_01010bfc(s_al_flash_obj_data_load_failed__01030b84);
      goto LAB_01002c8c;
    }
  }
  FUN_0102753c(0xf0090000);
  FUN_0102752c(0xf0070000);
  FUN_01000160(2,1);
  FUN_0100036c(0,1,0);
  FUN_01000180();
  FUN_01000160(2,0);
  FUN_0100016c();
  if (DAT_01049388 == 0) {
    if (((DAT_01049338 == 0) && (DAT_01049310 != 0)) && (*(int *)(DAT_01049310 + 0x14) != 0)) {
      FUN_0100ceac(s_Stopping_early_APCEA_01030d60);
      FUN_01010bfc(s_Stopping_early_APCEA_01030d60);
      iVar1 = DAT_01049310;
      *(uint *)(DAT_01049310 + 0x10) =
           ~*(uint *)(DAT_01049310 + 0x10) & 0x80000000 ^ *(uint *)(DAT_01049310 + 0x10);
      do {
      } while ((*(uint *)(iVar1 + 0x10) & 0x40000000) == 0);
      FUN_0100ceac(s_Early_APCEA_stopped_01030d78);
      FUN_01010bfc(s_Early_APCEA_stopped_01030d78);
      DataMemoryBarrier(0xf);
      FUN_01022318(0xfd882000);
    }
    else {
LAB_01002c14:
      DataMemoryBarrier(0xf);
      FUN_01022318(0xfd882000);
    }
    iVar1 = 0;
    FUN_0100ceac(s_Executing_application____01030d90);
    FUN_01010bfc(s_Executing_application____01030d90);
  }
  else {
    al_uart_handle_init(auStack_20,0xfd883000);
    do {
      uVar2 = al_uart_tx_fifo_level_get(auStack_20);
      iVar1 = DAT_01049374;
    } while (uVar2 < 0x40);
    if (DAT_01049374 == 0) goto LAB_01002c14;
    FUN_0100ceac(s_Executing_APCEA___application____01030dac);
    FUN_01010bfc(s_Executing_APCEA___application____01030dac);
  }
  al_sys_fabric_core_aarch64_setup(PTR_DAT_010492cc,0,0,0x1000);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  _DAT_fbff40f8 = uVar3;
  _DAT_fbff40fc = iVar1;
  al_sys_fabric_core_power_on_reset(PTR_DAT_010492cc,0);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  WaitForInterrupt();
LAB_01002c8c:
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x1002dec  FUN_01002dec */

void exec_via_agent(void)

{
  undefined *puVar1;
  int iVar2;
  undefined1 auStack_10 [4];
  code *local_c [2];
  
  thermal_sensor_trim_init();
  iVar2 = DAT_01049310;
  puVar1 = PTR_DAT_010492cc;
  if (DAT_0104a50c == 0) {
    if (DAT_01049310 == 0) {
      FUN_0100ceac(s__s__cpu_resume_regs_invalid_for_e_01030eac,s_exec_via_agent_010290dc);
      FUN_01010bfc(s__s__cpu_resume_regs_invalid_for_e_01030eac,s_exec_via_agent_010290dc);
    }
    else {
      *(undefined4 *)(DAT_01049310 + 8) = 0;
      *(code **)(iVar2 + 0xc) = FUN_010005a8;
      al_sys_fabric_core_aarch32_setup_get(puVar1,0,auStack_10,local_c);
      (*local_c[0])();
    }
  }
  else {
    FUN_010005a8();
  }
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x1002e8c  FUN_01002e8c */


bool FUN_01002e8c(void)

{
  FUN_010129d8(0x1000,&DAT_01029158,0x5e18);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  set_vectors(&DAT_01049000,0xffff0000);
  FUN_0100036c(1,1,1);
  FUN_010274e4(0xf0070000,0);
  FUN_01027504(0xf0090000,1);
  stg3_board_init();
  FUN_01002800();
  FUN_01002a88();
  return _DAT_fbff4150 == 0x31415926;
}



/* @ 0x1002f04  FUN_01002f04 */


bool FUN_01002f04(void)

{
  return _DAT_fbff4150 == 0x31415926;
}



/* @ 0x1002f28  FUN_01002f28 */

undefined1 * FUN_01002f28(void)

{
  return &DAT_fbff4150;
}



/* @ 0x1002f38  FUN_01002f38 */

undefined4 FUN_01002f38(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  bool bVar7;
  undefined4 local_27c;
  undefined1 auStack_278 [28];
  int iStack_25c;
  int local_258 [8];
  undefined8 local_238;
  undefined4 local_230;
  undefined4 uStack_22c;
  undefined4 local_228;
  undefined4 uStack_224;
  undefined1 auStack_158 [32];
  undefined8 local_138;
  undefined8 local_130;
  undefined8 local_128;
  undefined8 local_120;
  
  local_27c = 0x100;
  FUN_01009a54(&local_138);
  FUN_01009c40(&local_138,param_3,0x100);
  FUN_01009ae0(&local_138,local_258);
  piVar5 = (int *)&DAT_fd89608c;
  piVar3 = &iStack_25c;
  while( true ) {
    iVar4 = *piVar5;
    piVar5 = piVar5 + 1;
    piVar3 = piVar3 + 1;
    if (*piVar3 != iVar4) break;
    if (piVar5 == (int *)0xfd8960ac) goto LAB_01002fbc;
  }
  piVar5 = (int *)&DAT_fd89608c;
  iVar4 = 0;
  local_138 = 0;
  local_130 = 0;
  local_128 = 0;
  local_120 = 0;
  piVar3 = (int *)((int)&local_138 + 4);
  while( true ) {
    iVar2 = *piVar5;
    piVar5 = piVar5 + 1;
    if (iVar2 != iVar4) {
      FUN_0100ceac(s_eFuse_hashed_modulues_does_not_m_01030f7c);
      FUN_01010bfc(s_eFuse_hashed_modulues_does_not_m_01030f7c);
      return 0xfffffffb;
    }
    if (piVar5 == (int *)0xfd8960ac) break;
    iVar4 = *piVar3;
    piVar3 = piVar3 + 1;
  }
LAB_01002fbc:
  local_238 = DAT_01003218;
  local_230 = DAT_01003228;
  uStack_22c = DAT_0100322c;
  local_228 = DAT_01003220;
  uStack_224 = DAT_01003224;
  FUN_01009a54(&local_138);
  iVar4 = param_2;
  iVar2 = param_1;
  do {
    if (iVar4 == 0) {
LAB_01003070:
      iVar4 = 2;
      FUN_01009ae0(&local_138,auStack_278);
      while( true ) {
        iVar2 = FUN_01008098(param_4,0x100,&local_238,&local_27c,&DAT_01030ee0,3,param_3,0x100);
        if (iVar2 != 0) {
          FUN_0100ceac(s_RSA_decryption_failed__01030f24,&local_238,0xe0);
          FUN_01010bfc(s_RSA_decryption_failed__01030f24);
          return 0xfffffffb;
        }
        iVar2 = FUN_01012b04(&DAT_01031074);
        if ((iVar2 == 0) && (iVar2 = FUN_01012b04(auStack_278,auStack_158), iVar2 == 0)) break;
        FUN_0100ceac(s_signature_decryption_does_not_ma_01030f3c);
        FUN_01010bfc(s_signature_decryption_does_not_ma_01030f3c);
        bVar7 = iVar4 == 1;
        iVar4 = 1;
        if (bVar7) {
          return 0xfffffffb;
        }
        FUN_01009a54(&local_138);
        FUN_01009c40(&local_138,param_1,param_2);
        FUN_01009ae0(&local_138,auStack_278);
      }
      return 0;
    }
    if (iVar4 < AL_I2C_INTR_MASK_GEN_CALL_SHIFT + 1) {
      FUN_0100ceac(s_eeprom_data_is_not_aligned_to__d_01030efc,AL_I2C_TAR_10BIT_ADDR_SHIFT);
      FUN_01010bfc(s_eeprom_data_is_not_aligned_to__d_01030efc,AL_I2C_TAR_10BIT_ADDR_SHIFT);
      FUN_01009c40(&local_138,iVar2,iVar4);
      goto LAB_01003070;
    }
    iVar6 = iVar2 + 4;
    iVar1 = FUN_01012b04(iVar6,&local_238,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
    if (iVar1 != 0) {
      iVar1 = FUN_01012b04(iVar6,&local_230,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
      if (iVar1 != 0) {
        iVar1 = FUN_01012b04(iVar6,&local_228,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
        if (iVar1 != 0) {
          FUN_01009c40(&local_138,iVar2,AL_I2C_TAR_10BIT_ADDR_SHIFT);
        }
      }
    }
    iVar4 = iVar4 + -AL_I2C_TAR_10BIT_ADDR_SHIFT;
    iVar2 = iVar2 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
  } while( true );
}



/* @ 0x1003230  FUN_01003230 */

undefined4 eeprom_preload_parser(int param_1,int *param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  uVar3 = 0;
  do {
    iVar1 = FUN_01003460(DAT_01049dbf,uVar3,2,param_1 + uVar3,AL_I2C_TAR_10BIT_ADDR_SHIFT);
    if (iVar1 != 0) {
      FUN_0100ceac(s__s__i2c_read___d_failed__01030fbc,s_eeprom_preload_parser_01030ee4,uVar3 / 0xc,
                   uVar3 * -0x55555555);
      FUN_01010bfc(s__s__i2c_read___d_failed__01030fbc,s_eeprom_preload_parser_01030ee4,uVar3 / 0xc)
      ;
      return 0xfffffffb;
    }
    iVar1 = FUN_01012b04(param_1 + uVar3,&DAT_01030ed4,AL_I2C_TAR_10BIT_ADDR_SHIFT);
    iVar2 = uVar3 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
    if (iVar1 == 0) {
      if (uVar3 != 0) {
        if (iVar2 < 0x1000) {
          *param_2 = iVar2;
          iVar1 = FUN_01003460(DAT_01049dbf,iVar2,2,param_3,0x100);
          if (iVar1 != 0) {
            FUN_0100ceac(s__s__i2c_modulues_read_failed__01031034,s_eeprom_preload_parser_01030ee4);
            FUN_01010bfc(s__s__i2c_modulues_read_failed__01031034,s_eeprom_preload_parser_01030ee4);
            return 0xfffffffb;
          }
          iVar1 = FUN_01003460(DAT_01049dbf,*param_2 + 0x100,2,param_4,0x100);
          if (iVar1 == 0) {
            return 0;
          }
          FUN_0100ceac(s__s__i2c_signature_read_failed__01031054,s_eeprom_preload_parser_01030ee4);
          FUN_01010bfc(s__s__i2c_signature_read_failed__01031054,s_eeprom_preload_parser_01030ee4);
          return 0xfffffffb;
        }
LAB_010033e4:
        FUN_0100ceac(s__s__failed_to_find_closing_EEPRO_01031000,s_eeprom_preload_parser_01030ee4);
        FUN_01010bfc(s__s__failed_to_find_closing_EEPRO_01031000,s_eeprom_preload_parser_01030ee4);
        return 0xffffffea;
      }
    }
    else {
      if (uVar3 == 0) {
        FUN_0100ceac(s__s__no_opening_EEPROM_encapsulat_01030fd8,s_eeprom_preload_parser_01030ee4);
        FUN_01010bfc(s__s__no_opening_EEPROM_encapsulat_01030fd8,s_eeprom_preload_parser_01030ee4);
        return 0xffffffea;
      }
      if (0xfff < uVar3 + AL_I2C_TAR_10BIT_ADDR_SHIFT) goto LAB_010033e4;
    }
    uVar3 = uVar3 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
  } while( true );
}



/* @ 0x1003444  FUN_01003444 */

/* WARNING: Removing unreachable block (ram,0x01022c84) */
/* WARNING: Removing unreachable block (ram,0x01022c20) */
/* WARNING: Removing unreachable block (ram,0x01022bc0) */

undefined4 FUN_01003444(void)

{
  DAT_01049478 = 0xfd880000;
  uRamfd88006c = uRamfd88006c & 0xfffe;
  if (DAT_01031160 == '\x01') {
    if (DAT_01031178 == '\x01') {
      uRamfd880004 = 0x1000;
      uRamfd880000 = DAT_01031179 | 0x71;
    }
    else {
      uRamfd880004 = 0;
      uRamfd880000 = DAT_01031179 | 0x61;
    }
  }
  else {
    if (DAT_01031178 == '\x01') {
      uRamfd880000 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
    else {
      uRamfd880000 = 0;
    }
    uRamfd880008 = DAT_01031170;
  }
  uRamfd880038 = DAT_01031168;
  uRamfd88003c = DAT_0103116c;
  if (DAT_01031164 == 0) {
    uRamfd880038 = 0;
    uRamfd88003c = 0;
  }
  DAT_0104947c = DAT_01031174;
  DAT_01049480 = &LAB_01022998;
  DAT_01049484 = 0;
  return 0;
}



/* @ 0x1003460  FUN_01003460 */

int FUN_01003460(undefined4 param_1,uint param_2,uint param_3,int param_4,int param_5)

{
  int iVar1;
  undefined2 local_2c;
  
  if (param_4 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer______void___0___010311d4,1,
                 s__home_winder_projects_data_custo_0103117c,s_i2c_read_01031153 + 1,0x59);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer______void___0___010311d4,1,
                 s__home_winder_projects_data_custo_0103117c,s_i2c_read_01031153 + 1,0x59);
    FUN_01000454(0);
  }
  if (2 < param_3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____unsigned_int_i2c_addr_len_<__2_010311f0,1,
                 s__home_winder_projects_data_custo_0103117c,s_i2c_read_01031153 + 1,0x5a);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____unsigned_int_i2c_addr_len_<__2_010311f0,1,
                 s__home_winder_projects_data_custo_0103117c,s_i2c_read_01031153 + 1,0x5a);
    FUN_01000454(0);
  }
  if (param_5 == 0) {
    iVar1 = -5;
  }
  else {
    param_4 = param_4 - param_2;
    do {
      local_2c = (ushort)((param_2 & 0xff) << 8) | (ushort)(param_2 >> 8) & 0xff;
      if ((int)param_3 < 2) {
        local_2c = 0;
      }
      iVar1 = param_4 + param_2;
      if ((int)param_3 < 2) {
        local_2c = CONCAT11((char)local_2c,(char)param_2);
      }
      param_2 = param_2 + 0x80;
      if (param_5 < 0x81) {
        iVar1 = al_i2c_read(&DAT_01049478,param_1,param_3 & 0xff,&local_2c,param_5,iVar1);
        return iVar1;
      }
      param_5 = param_5 + -0x80;
      iVar1 = al_i2c_read(&DAT_01049478,param_1,param_3 & 0xff,&local_2c,0x80,iVar1);
    } while (iVar1 == 0);
  }
  return iVar1;
}



/* @ 0x10036a4  FUN_010036a4 */

undefined4 FUN_010036a4(int param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint local_14 [2];
  
  uVar1 = *(undefined4 *)(param_1 + 0x1c);
  uVar2 = *(undefined4 *)(param_1 + 0x28);
  al_pcie_write_config(uVar1,uVar2,0x1e8,local_14);
  FUN_0100c42c(uVar1,uVar2,0x1e8,local_14[0] | 0x80);
  return 0;
}



/* @ 0x1003814  FUN_01003814 */

undefined4 al_pcie_int_adapter_pd(int param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined2 local_16;
  uint local_14;
  
  uVar1 = *(undefined4 *)(param_1 + 0x1c);
  uVar2 = *(undefined4 *)(param_1 + 0x28);
  FUN_0100c2b4(uVar1,uVar2,2,&local_16);
  switch(local_16) {
  case 1:
  case 2:
  case 3:
  case 0x11:
  case 0x21:
  case 0x22:
    uVar3 = 0x8f000000;
    break;
  default:
    FUN_0100ceac(s__s__unexpected_dev_id____04x_0103126c,s_al_pcie_int_adapter_pd_01031230);
    FUN_01010bfc(s__s__unexpected_dev_id____04x_0103126c,s_al_pcie_int_adapter_pd_01031230,local_16)
    ;
    return 0xffffffea;
  case 0x31:
    uVar3 = 0x10000000;
    break;
  case 0x41:
    uVar3 = 0x80000000;
  }
  al_pcie_write_config(uVar1,uVar2,0x1ec,&local_14);
  FUN_0100c42c(uVar1,uVar2,0x1ec,uVar3 | local_14);
  return 0;
}



/* @ 0x1003acc  FUN_01003acc */

undefined4 FUN_01003acc(undefined4 param_1)

{
  FUN_01025fd4(param_1,0);
  FUN_01026044(param_1,1,0,1,0);
  return 0;
}



/* @ 0x1003b20  FUN_01003b20 */


undefined4 pci_skip_dev(undefined4 param_1,uint param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  code *pcVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  undefined *puVar8;
  short local_26;
  uint local_24;
  
  if ((param_2 & 0xffffff) >> 0x10 == 0) {
    FUN_0100c2b4(param_1,param_2,0,&local_26);
    if ((ushort)(local_26 - 1U) < 0xfffe) {
      puVar8 = &DAT_01049074;
      if (param_2 == DAT_0104909c) {
        iVar6 = 0;
        iVar2 = 0;
      }
      else {
        iVar6 = 1;
        while (*(uint *)(puVar8 + 100) != param_2) {
          iVar6 = iVar6 + 1;
          puVar8 = puVar8 + 0x3c;
          if (iVar6 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
            uVar7 = (param_2 & 0xffff) >> 0xb;
            uVar5 = (param_2 & 0x7ff) >> 8;
            FUN_0100ceac(s__d__d__d_implicitly_disabled_01031394,0,uVar7,uVar5);
            FUN_01010bfc(s__d__d__d_implicitly_disabled_01031394,0,uVar7,uVar5);
            _FIQ = 0;
                    /* WARNING: Does not return */
            pcVar4 = (code *)software_udf(0,0x1003e6c);
            (*pcVar4)();
          }
        }
        iVar2 = iVar6 * 0x10;
        puVar8 = &DAT_01049074 + iVar6 * 0x3c;
      }
      iVar3 = iVar2 - iVar6;
      (&DAT_01049090)[iVar3] = param_1;
      if ((&DAT_01049094)[iVar3] == 0) {
        pcVar4 = *(code **)(iVar3 * 4 + 0x10490a0);
        if ((pcVar4 != (code *)0x0) && (iVar2 = (*pcVar4)(puVar8), iVar2 != 0)) {
          FUN_0100ceac(s__s__cfg_entry_>pd_failed__01031378,s_pci_skip_dev_01031248);
          FUN_01010bfc(s__s__cfg_entry_>pd_failed__01031378,s_pci_skip_dev_01031248);
        }
        al_pcie_write_config(param_1,param_2,0x1e0,&local_24);
        FUN_0100c42c(param_1,param_2,0x1e0,local_24 | 1);
        al_pcie_write_config(param_1,param_2,0x84,&local_24);
        FUN_0100c42c(param_1,param_2,0x84,local_24 | 0x103);
        al_pcie_write_config(param_1,param_2,0x1e0,&local_24);
        FUN_0100c42c(param_1,param_2,0x1e0,local_24 | 0x40);
        uVar1 = 1;
      }
      else {
        if ((&DAT_01049098)[iVar3] == 0) {
          FUN_010221a8(0xfd8a8000,1);
          FUN_0100c42c(param_1,param_2,0x30c,0);
          FUN_010221a8(0xfd8a8000,0);
        }
        if (((code *)(&DAT_010490a4)[iVar2 - iVar6] != (code *)0x0) &&
           (iVar3 = (*(code *)(&DAT_010490a4)[iVar2 - iVar6])(puVar8), iVar3 != 0)) {
          FUN_0100ceac(s__s__cfg_entry_>pre_flr_failed__01031338,s_pci_skip_dev_01031248);
          FUN_01010bfc(s__s__cfg_entry_>pre_flr_failed__01031338,s_pci_skip_dev_01031248);
        }
        al_pcie_write_config(param_1,param_2,0x48,&local_24);
        FUN_0100c42c(param_1,param_2,0x48,local_24 | 0x8000);
        udelay(1000);
        if ((&DAT_01049098)[iVar2 - iVar6] == 0) {
          al_pcie_write_config(param_1,param_2,0x30c,&local_24);
        }
        pcVar4 = *(code **)((iVar2 - iVar6) * 4 + 0x10490a8);
        if ((pcVar4 != (code *)0x0) && (iVar2 = (*pcVar4)(puVar8), iVar2 != 0)) {
          FUN_0100ceac(s__s__cfg_entry_>post_flr_failed__01031358,s_pci_skip_dev_01031248);
          FUN_01010bfc(s__s__cfg_entry_>post_flr_failed__01031358,s_pci_skip_dev_01031248);
        }
        uVar1 = 0;
      }
    }
    else {
      uVar1 = 1;
    }
  }
  else {
    uVar1 = 0;
    if ((param_2 & 0xf800) != 0) {
      uVar1 = 1;
    }
  }
  return uVar1;
}



/* @ 0x1003e70  FUN_01003e70 */

void pci_devices_init(void)

{
  int iVar1;
  undefined *puVar2;
  int iVar3;
  
  puVar2 = &DAT_01049074;
  iVar3 = 0;
  do {
    while( true ) {
      iVar1 = FUN_01025edc(puVar2,puVar2[0x18],0,&LAB_010039e4,&LAB_01003600,0,puVar2);
      if (iVar1 != 0) break;
      iVar3 = iVar3 + 1;
      puVar2 = puVar2 + 0x3c;
      if (iVar3 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
        return;
      }
    }
    FUN_0100ceac(s__s__al_unit_adapter_handle_init_f_01031300,s_pci_devices_init_01031258,iVar3);
    iVar1 = iVar3 + 1;
    FUN_01010bfc(s__s__al_unit_adapter_handle_init_f_01031300,s_pci_devices_init_01031258,iVar3);
    puVar2 = puVar2 + 0x3c;
    iVar3 = iVar1;
  } while (iVar1 != AL_I2C_INTR_MASK_START_DET_SHIFT);
  return;
}



/* @ 0x1003f30  FUN_01003f30 */

void FUN_01003f30(int param_1,undefined4 param_2)

{
  (&DAT_01049094)[param_1 * 0xf] = param_2;
  return;
}



/* @ 0x1003f48  FUN_01003f48 */

undefined4 FUN_01003f48(int param_1)

{
  return (&DAT_01049094)[param_1 * 0xf];
}



/* @ 0x1003f60  FUN_01003f60 */

void FUN_01003f60(int param_1,undefined4 param_2)

{
  (&DAT_01049098)[param_1 * 0xf] = param_2;
  return;
}



/* @ 0x1003f78  FUN_01003f78 */

void pll_fixup_crypto_boost(void)

{
  int iVar1;
  undefined1 uStack_21;
  uint local_20;
  undefined1 auStack_1c [24];
  
  iVar1 = al_pll_init(0xfd860b00,s_SB_PLL_010313cc,1,auStack_1c);
  if (iVar1 != 0) {
    FUN_0100ceac(s__s__al_pll_init_failed__010313d4,s_pll_fixup_crypto_boost_010313b4);
    FUN_01010bfc(s__s__al_pll_init_failed__010313d4,s_pll_fixup_crypto_boost_010313b4);
    return;
  }
  iVar1 = al_pll_freq_get(auStack_1c,&uStack_21,&local_20);
  if (iVar1 == 0) {
    if ((local_20 == (local_20 / 600000) * 600000) || (local_20 % 300000 == 0)) {
      iVar1 = FUN_01024218(auStack_1c);
      if (iVar1 != 0) {
        FUN_0100ceac(s__s__al_pll_channel_div_set_faile_0103143c,s_pll_fixup_crypto_boost_010313b4);
        FUN_01010bfc(s__s__al_pll_channel_div_set_faile_0103143c,s_pll_fixup_crypto_boost_010313b4);
      }
    }
    else {
      FUN_0100ceac(s_PLL_freq_not_suitable_for_600MHz_01031410,local_20 % 300000,local_20 / 600000,
                   300000);
      FUN_01010bfc(s_PLL_freq_not_suitable_for_600MHz_01031410);
    }
    return;
  }
  FUN_0100ceac(s__s__al_pll_freq_get_failed__010313f0,s_pll_fixup_crypto_boost_010313b4);
  FUN_01010bfc(s__s__al_pll_freq_get_failed__010313f0,s_pll_fixup_crypto_boost_010313b4);
  return;
}



/* @ 0x10040d4  FUN_010040d4 */

int iodma_pci_init(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 local_84;
  undefined1 local_80 [4];
  undefined4 local_7c;
  char *local_78;
  undefined1 local_74;
  undefined4 local_70;
  uint local_6c;
  uint local_68;
  int local_64;
  int local_60;
  undefined8 local_58;
  undefined4 local_50;
  uint local_48;
  uint local_44;
  uint local_40;
  int local_3c;
  uint local_38;
  uint local_30;
  int local_2c;
  undefined4 local_28;
  
  DAT_01049488 = 0x2800;
  local_84 = 0;
  FUN_0100c27c(0,0x2800,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,&DAT_01049a80);
  iVar2 = FUN_0100c818(0,DAT_01049488,0,&local_84);
  if (iVar2 == 0) {
    iVar2 = al_pcie_master_enable(0,DAT_01049488);
    if (iVar2 == 0) {
      iVar2 = al_pcie_memory_enable(0,DAT_01049488);
      uVar1 = DAT_01049488;
      if (iVar2 == 0) {
        al_pcie_write_config(0,DAT_01049488,0x110,&local_48);
        local_48 = local_48 | 3;
        FUN_0100c42c(0,uVar1,0x110);
        al_pcie_write_config(0,uVar1,0x220,&local_48);
        local_48 = ~(~(local_48 >> 10) << 10);
        FUN_0100c42c(0,uVar1,0x220);
        DAT_01049a98 = FUN_0100d2f4(0x1040);
        DAT_01049a9c = DAT_01049a98 + 0x3fU & 0xffffffc0;
        DAT_01049a8c = FUN_0100d2f4(0x1040);
        DAT_01049a90 = DAT_01049a8c + 0x3fU & 0xffffffc0;
        DAT_01049a84 = FUN_0100d2f4(0x1040);
        DAT_01049a88 = DAT_01049a84 + 0x3fU & 0xffffffc0;
        if ((DAT_01049a84 != 0 && DAT_01049a98 != 0) && (DAT_01049a8c != 0)) {
          FUN_01012ae0(DAT_01049a88,0,0x1000);
          local_80[0] = DAT_01049a80;
          local_7c = local_84;
          local_78 = s_RAID_UDMA_0103156c;
          local_74 = 4;
          local_70 = 0x100;
          local_6c = DAT_01049a9c;
          local_68 = DAT_01049a9c;
          local_58 = 0;
          local_50 = 0x10;
          local_48 = 0x100;
          local_44 = DAT_01049a90;
          local_40 = DAT_01049a90;
          local_38 = DAT_01049a88;
          local_30 = DAT_01049a88;
          local_28 = 0x10;
          local_64 = iVar2;
          local_60 = iVar2;
          local_3c = iVar2;
          local_2c = iVar2;
          FUN_01014188(&DAT_010494c0,local_80);
          FUN_01014230(&DAT_010494c0,0,&local_70,&local_48,3);
          FUN_010142b8(&DAT_010494c0,2);
          DAT_01049aa0 = FUN_01014318(&DAT_010494c0,0);
          DAT_01049a94 = FUN_01014364(&DAT_010494c0,0);
          return 0;
        }
        FUN_0100ceac(s_Failed_to_allocate_descriptors_0103154c);
        FUN_01010bfc(s_Failed_to_allocate_descriptors_0103154c);
        return -1;
      }
      FUN_0100ceac(s__s__failed_enabling_memory_regio_01031528,s_iodma_pci_init_0103147c);
      FUN_01010bfc(s__s__failed_enabling_memory_regio_01031528,s_iodma_pci_init_0103147c);
    }
    else {
      FUN_0100ceac(s__s__failed_enabling_bus_masterin_01031504,s_iodma_pci_init_0103147c);
      FUN_01010bfc(s__s__failed_enabling_bus_masterin_01031504,s_iodma_pci_init_0103147c);
    }
  }
  else {
    FUN_0100ceac(s__s__failed_getting_UDMA_BAR__010314e4,s_iodma_pci_init_0103147c);
    FUN_01010bfc(s__s__failed_getting_UDMA_BAR__010314e4,s_iodma_pci_init_0103147c);
  }
  FUN_0100ceac(s_Failed_to_get_BARs_01031578);
  FUN_01010bfc(s_Failed_to_get_BARs_01031578);
  return iVar2;
}



/* @ 0x1004430  FUN_01004430 */

void FUN_01004430(void)

{
  uint local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  al_pcie_write_config(0,DAT_01049488,4,&local_1c);
  al_pcie_write_config(0,DAT_01049488,AL_I2C_TAR_10BIT_ADDR_SHIFT,&local_18);
  al_pcie_write_config(0,DAT_01049488,0x10,&local_14);
  al_pcie_write_config(0,DAT_01049488,0x18,&local_10);
  al_pcie_write_config(0,DAT_01049488,0x20,&local_c);
  al_pcie_write_config(0,DAT_01049488,0x48,&local_20);
  FUN_0100c42c(0,DAT_01049488,0x48,local_20 | 0x8000);
  udelay(1000);
  FUN_0100c42c(0,DAT_01049488,4,local_1c);
  FUN_0100c42c(0,DAT_01049488,AL_I2C_TAR_10BIT_ADDR_SHIFT,local_18);
  FUN_0100c42c(0,DAT_01049488,0x10,local_14);
  FUN_0100c42c(0,DAT_01049488,0x18,local_10);
  FUN_0100c42c(0,DAT_01049488,0x20,local_c);
  FUN_0100d364(DAT_01049a98);
  FUN_0100d364(DAT_01049a8c);
  FUN_0100d364(DAT_01049a84);
  return;
}



/* @ 0x1004580  FUN_01004580 */

/* WARNING: Restarted to delay deadcode elimination for space: ram */

int al_udma_desc_action_add(uint param_1,uint param_2,undefined4 param_3,undefined4 param_4,uint param_5,
                int param_6)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint extraout_r2;
  int extraout_r3;
  uint uVar4;
  uint uVar5;
  ushort *puVar6;
  ushort *puVar7;
  uint *puVar8;
  bool bVar9;
  uint local_60;
  uint uStack_5c;
  undefined1 auStack_3c [4];
  undefined1 auStack_38 [12];
  undefined4 local_2c;
  
  FUN_01012ae0(auStack_3c,0,0x14);
  FUN_01012ae0(auStack_38,param_3,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  local_2c = 0xffc0;
  iVar1 = al_unit_adapter_handle_init(DAT_01049aa0,DAT_01049a94,auStack_3c);
  if (iVar1 != 0) {
    return iVar1;
  }
  FUN_01028d2c(param_5,param_6,0xffc0,0);
  uVar2 = (param_1 + param_5) - extraout_r2;
  uVar3 = (param_2 + param_6 + (uint)CARRY4(param_1,param_5)) -
          (extraout_r3 + (uint)(param_1 + param_5 < extraout_r2));
  bVar9 = uVar3 <= param_2;
  if (param_2 == uVar3) {
    bVar9 = uVar2 <= param_1;
  }
  local_60 = param_1;
  uStack_5c = param_2;
  if (bVar9) {
    puVar7 = DAT_01049a94;
    if (extraout_r2 == 0 && extraout_r3 == 0) {
      return 0;
    }
  }
  else {
    uVar5 = 0;
    do {
      puVar7 = DAT_01049a94;
      if (uVar5 < 0xff) {
        if (DAT_01049a94 == (ushort *)0x0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd
                      );
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd
                      );
          FUN_01000454(0);
          puVar7 = DAT_01049a94;
          uVar4 = (uint)_PrefetchAbort;
          _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
          puVar8 = (uint *)(_SupervisorCall + uVar4 * 0x10);
          if (DAT_01049a94 == (ushort *)0x0) {
            FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                         s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,
                         0x216);
            FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                         s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,
                         0x216);
            FUN_01000454(0);
          }
        }
        else {
          puVar8 = (uint *)(*(int *)(DAT_01049a94 + 4) + (uint)DAT_01049a94[6] * 0x10);
          DAT_01049a94[6] = *DAT_01049a94 & DAT_01049a94[6] + 1;
        }
        iVar1 = *(int *)(puVar7 + 8);
        if (puVar7[6] == 0) {
          *(uint *)(puVar7 + 8) = iVar1 + 1U & 3;
        }
        if (puVar8 == (uint *)0x0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                       s____HAL__include_udma_fast_al_hal_010315c0,
                       s_al_udma_fast_desc_flags_set_01031460,0x92);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                       s____HAL__include_udma_fast_al_hal_010315c0,
                       s_al_udma_fast_desc_flags_set_01031460,0x92);
          FUN_01000454(0);
        }
        puVar7 = DAT_01049a94;
        bVar9 = DAT_01049a94 == (ushort *)0x0;
        *puVar8 = iVar1 << 0x18 | *puVar8 & 0xfcffffff;
        puVar8[2] = local_60;
        puVar8[3] = uStack_5c;
        if (bVar9) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x230);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x230);
          FUN_01000454(0);
        }
        if (*(int *)(puVar7 + 0x26) == 0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                       s____num_>_0______num_<__udma_q_>s_010315f8,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x231);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                       s____num_>_0______num_<__udma_q_>s_010315f8,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x231);
          FUN_01000454(0);
        }
        DataMemoryBarrier(0xf);
        *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
        puVar7 = DAT_01049aa0;
        if (DAT_01049aa0 == (ushort *)0x0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd
                      );
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd
                      );
          FUN_01000454(0);
          puVar7 = DAT_01049aa0;
          uVar4 = (uint)_PrefetchAbort;
          _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
          puVar8 = (uint *)(_SupervisorCall + uVar4 * 0x10);
          if (DAT_01049aa0 == (ushort *)0x0) {
            FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                         s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,
                         0x216);
            FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                         s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,
                         0x216);
            FUN_01000454(0);
          }
        }
        else {
          puVar8 = (uint *)(*(int *)(DAT_01049aa0 + 4) + (uint)DAT_01049aa0[6] * 0x10);
          DAT_01049aa0[6] = *DAT_01049aa0 & DAT_01049aa0[6] + 1;
        }
        iVar1 = *(int *)(puVar7 + 8);
        if (puVar7[6] == 0) {
          *(uint *)(puVar7 + 8) = iVar1 + 1U & 3;
        }
        if (puVar8 == (uint *)0x0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                       s____HAL__include_udma_fast_al_hal_010315c0,
                       s_al_udma_fast_desc_flags_set_01031460,0x92);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                       s____HAL__include_udma_fast_al_hal_010315c0,
                       s_al_udma_fast_desc_flags_set_01031460,0x92);
          FUN_01000454(0);
        }
        puVar6 = DAT_01049aa0;
        bVar9 = DAT_01049aa0 == (ushort *)0x0;
        *puVar8 = iVar1 << 0x18 | *puVar8 & 0xfcffffff;
        if (bVar9) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x230);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x230);
          FUN_01000454(0);
        }
        if (*(int *)(puVar6 + 0x26) == 0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                       s____num_>_0______num_<__udma_q_>s_010315f8,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x231);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                       s____num_>_0______num_<__udma_q_>s_010315f8,1,
                       s____HAL__include_udma_al_hal_udma_01031590,
                       s_al_udma_desc_action_add_010314b4,0x231);
          FUN_01000454(0);
        }
        puVar7 = DAT_01049a94;
        DataMemoryBarrier(0xf);
        uVar5 = uVar5 + 1;
        *(undefined4 *)(*(int *)(puVar6 + 2) + 0x38) = 1;
        bVar9 = 0xffff003f < local_60;
        local_60 = local_60 + 0xffc0;
        uStack_5c = uStack_5c + bVar9;
      }
      if (puVar7 == (ushort *)0x0) {
        FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                     s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                     0x2ba);
        FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                     s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                     0x2ba);
        FUN_01000454(0);
      }
      uVar4 = *(uint *)(*(int *)(puVar7 + 2) + 0x4c);
      puVar7[0x12] = (ushort)uVar4;
      if (((uVar4 & 0xffff) - (uint)puVar7[0xe] & (uint)*puVar7) != 0) {
        uVar5 = uVar5 - 1;
        puVar7[0xe] = *puVar7 & puVar7[0xe] + 1;
      }
      bVar9 = uStack_5c <= uVar3;
      if (uVar3 == uStack_5c) {
        bVar9 = local_60 <= uVar2;
      }
    } while (bVar9 && (uVar3 != uStack_5c || uVar2 != local_60));
    puVar7 = DAT_01049a94;
    if (extraout_r2 == 0 && extraout_r3 == 0) {
      if (uVar5 == 0) {
        return 0;
      }
      goto joined_r0x01004b4c;
    }
    for (; uVar5 != 0; uVar5 = uVar5 - 1) {
      do {
        puVar6 = puVar7;
        puVar7 = puVar6;
        if (puVar6 == (ushort *)0x0) {
          FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                       0x2ba);
          FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                       s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                       0x2ba);
          FUN_01000454(0);
          puVar7 = DAT_01049a94;
        }
        uVar2 = *(uint *)(*(int *)(puVar6 + 2) + 0x4c);
        puVar6[0x12] = (ushort)uVar2;
      } while (((uVar2 & 0xffff) - (uint)puVar6[0xe] & (uint)*puVar6) == 0);
      puVar6[0xe] = puVar6[0xe] + 1 & *puVar6;
    }
  }
  if (puVar7 == (ushort *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd);
    FUN_01000454(0);
    puVar7 = DAT_01049a94;
    uVar2 = (uint)_PrefetchAbort;
    _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
    puVar8 = (uint *)(_SupervisorCall + uVar2 * 0x10);
    if (DAT_01049a94 == (ushort *)0x0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                   s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,0x216)
      ;
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                   s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,0x216)
      ;
      FUN_01000454(0);
    }
  }
  else {
    puVar8 = (uint *)(*(int *)(puVar7 + 4) + (uint)puVar7[6] * 0x10);
    puVar7[6] = *puVar7 & puVar7[6] + 1;
  }
  iVar1 = *(int *)(puVar7 + 8);
  if (puVar7[6] == 0) {
    *(uint *)(puVar7 + 8) = iVar1 + 1U & 3;
  }
  if (puVar8 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                 s____HAL__include_udma_fast_al_hal_010315c0,s_al_udma_fast_desc_flags_set_01031460,
                 0x92);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                 s____HAL__include_udma_fast_al_hal_010315c0,s_al_udma_fast_desc_flags_set_01031460,
                 0x92);
    FUN_01000454(0);
  }
  puVar7 = DAT_01049a94;
  bVar9 = DAT_01049a94 == (ushort *)0x0;
  puVar8[2] = local_60;
  puVar8[3] = uStack_5c;
  *puVar8 = *puVar8 & 0xfcff0000 | iVar1 << 0x18 | extraout_r2;
  if (bVar9) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x230);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x230);
    FUN_01000454(0);
  }
  if (*(int *)(puVar7 + 0x26) == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_>_0______num_<__udma_q_>s_010315f8,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x231);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_>_0______num_<__udma_q_>s_010315f8,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x231);
    FUN_01000454(0);
  }
  puVar6 = DAT_01049aa0;
  DataMemoryBarrier(0xf);
  *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
  if (puVar6 == (ushort *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_get_0103148c,0x1fd);
    FUN_01000454(0);
    puVar6 = DAT_01049aa0;
    uVar2 = (uint)_PrefetchAbort;
    _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
    puVar8 = (uint *)(_SupervisorCall + uVar2 * 0x10);
    if (DAT_01049aa0 == (ushort *)0x0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                   s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,0x216)
      ;
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                   s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_ring_id_get_010314a0,0x216)
      ;
      FUN_01000454(0);
    }
  }
  else {
    puVar8 = (uint *)(*(int *)(puVar6 + 4) + (uint)puVar6[6] * 0x10);
    puVar6[6] = *puVar6 & puVar6[6] + 1;
  }
  iVar1 = *(int *)(puVar6 + 8);
  if (puVar6[6] == 0) {
    *(uint *)(puVar6 + 8) = iVar1 + 1U & 3;
  }
  if (puVar8 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                 s____HAL__include_udma_fast_al_hal_010315c0,s_al_udma_fast_desc_flags_set_01031460,
                 0x92);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc__010315f0,1,
                 s____HAL__include_udma_fast_al_hal_010315c0,s_al_udma_fast_desc_flags_set_01031460,
                 0x92);
    FUN_01000454(0);
  }
  puVar7 = DAT_01049aa0;
  bVar9 = DAT_01049aa0 == (ushort *)0x0;
  *puVar8 = *puVar8 & 0xfcff0000 | iVar1 << 0x18 | extraout_r2;
  if (bVar9) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x230);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x230);
    FUN_01000454(0);
  }
  if (*(int *)(puVar7 + 0x26) == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_>_0______num_<__udma_q_>s_010315f8,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x231);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_>_0______num_<__udma_q_>s_010315f8,1,
                 s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_desc_action_add_010314b4,
                 0x231);
    FUN_01000454(0);
  }
  DataMemoryBarrier(0xf);
  uVar5 = 1;
  *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
joined_r0x01004b4c:
  do {
    do {
      puVar7 = DAT_01049a94;
      DAT_01049a94 = puVar7;
      if (puVar7 == (ushort *)0x0) {
        FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                     s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                     0x2ba);
        FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                     s____HAL__include_udma_al_hal_udma_01031590,s_al_udma_cdesc_get_all_010314cc,
                     0x2ba);
        FUN_01000454(0);
      }
      uVar2 = *(uint *)(*(int *)(puVar7 + 2) + 0x4c);
      puVar7[0x12] = (ushort)uVar2;
    } while (((uVar2 & 0xffff) - (uint)puVar7[0xe] & (uint)*puVar7) == 0);
    uVar5 = uVar5 - 1;
    puVar7[0xe] = puVar7[0xe] + 1 & *puVar7;
  } while (uVar5 != 0);
  return 0;
}



/* @ 0x100533c  FUN_0100533c */


void FUN_0100533c(void)

{
  if (DAT_01049acc == 0) {
    al_sys_fabric_handle_init(&DAT_01049ac4,0xf0070000);
    al_sys_fabric_cluster_handle_init(&DAT_01049aa4,&DAT_01049ac4,0);
    al_sys_fabric_cluster_handle_init(_DAT_010053dc,&DAT_01049ac4,0);
    al_sys_fabric_cluster_handle_init(FUN_010053de,&DAT_01049ac4,0);
    al_sys_fabric_cluster_handle_init(uRam010053e4,&DAT_01049ac4,0);
    DAT_01049acc = 1;
  }
  return;
}



/* @ 0x10053de  FUN_010053de */

int read_toc_obj_hdr(uint param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 in_stack_000002f0;
  int iStack_3c;
  undefined1 auStack_38 [16];
  undefined4 uStack_28;
  int iStack_18;
  
  iStack_18 = param_1 << 4;
  uVar1 = al_flash_obj_id_to_str(param_1,param_2,in_stack_000002f0);
  iVar2 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,param_1 | DAT_0104a504 << 0x1c,
                       param_1 | DAT_0104a508 << 0x1c,0,&iStack_3c,auStack_38);
  if (iVar2 == 0) {
    if (iStack_3c < 0) {
      iVar2 = -0x16;
      FUN_0100ceac(s__s___s_not_found_in_TOC__010316e8,s_read_toc_obj_hdr_01031620,uVar1);
      FUN_01010bfc(s__s___s_not_found_in_TOC__010316e8,s_read_toc_obj_hdr_01031620,uVar1);
    }
    else {
      iVar2 = al_flash_obj_header_read_and_validate(DAT_01049d8c,uStack_28,param_2);
    }
  }
  else {
    FUN_0100ceac(s__s__al_flash_toc_find_id_failed___010316b4,s_read_toc_obj_hdr_01031620,uVar1,
                 DAT_0104a504);
    FUN_01010bfc(s__s__al_flash_toc_find_id_failed___010316b4,s_read_toc_obj_hdr_01031620,uVar1,
                 DAT_0104a504);
  }
  return iVar2;
}



/* @ 0x10053e8  FUN_010053e8 */

int read_toc_obj_hdr(uint param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  int local_3c;
  undefined1 auStack_38 [16];
  undefined4 local_28;
  
  uVar1 = al_flash_obj_id_to_str();
  iVar2 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,param_1 | DAT_0104a504 << 0x1c,
                       param_1 | DAT_0104a508 << 0x1c,0,&local_3c,auStack_38);
  if (iVar2 == 0) {
    if (local_3c < 0) {
      iVar2 = -0x16;
      FUN_0100ceac(s__s___s_not_found_in_TOC__010316e8,s_read_toc_obj_hdr_01031620,uVar1);
      FUN_01010bfc(s__s___s_not_found_in_TOC__010316e8,s_read_toc_obj_hdr_01031620,uVar1);
    }
    else {
      iVar2 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_28,param_2);
    }
  }
  else {
    FUN_0100ceac(s__s__al_flash_toc_find_id_failed___010316b4,s_read_toc_obj_hdr_01031620,uVar1,
                 DAT_0104a504);
    FUN_01010bfc(s__s__al_flash_toc_find_id_failed___010316b4,s_read_toc_obj_hdr_01031620,uVar1,
                 DAT_0104a504);
  }
  return iVar2;
}



/* @ 0x1005500  FUN_01005500 */

undefined4 enabled(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint *puVar4;
  undefined1 *puVar5;
  uint uVar6;
  int local_5c;
  uint local_58 [3];
  undefined1 local_4c;
  undefined1 local_48;
  undefined1 local_44;
  
  iVar1 = FUN_01007f84(param_1,s__soc_general_shared_data_01031634);
  if (iVar1 < 0) {
    return 0;
  }
  uVar2 = FUN_01007eb8(param_1,iVar1,s_status_01030628,&local_5c);
  if (local_5c < 0) {
    return 0;
  }
  iVar3 = FUN_01012954(uVar2,s_enabled_01030630);
  if (iVar3 != 0) {
    return 0;
  }
  puVar4 = (uint *)FUN_01007eb8(param_1,iVar1,s__address_cells_01031650,local_58);
  if (puVar4 == (uint *)0x0) {
    puVar4 = (uint *)FUN_01007eb8(param_1,iVar1,&DAT_01031660,local_58);
    uVar6 = local_58[0];
joined_r0x010057c4:
    if (uVar6 == 2) {
      uVar6 = puVar4[1];
      DAT_01049ad0 = (undefined1 *)
                     (uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
                     uVar6 >> 0x18);
      goto LAB_010055d8;
    }
  }
  else {
    uVar6 = *puVar4;
    uVar6 = uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
            uVar6 >> 0x18;
    puVar4 = (uint *)FUN_01007eb8(param_1,iVar1,&DAT_01031660,local_58);
    if (uVar6 == local_58[0]) {
      if (uVar6 == 1) {
        uVar6 = *puVar4;
        DAT_01049ad0 = (undefined1 *)
                       (uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
                       uVar6 >> 0x18);
        goto LAB_010055d8;
      }
      goto joined_r0x010057c4;
    }
  }
  DAT_01049ad0 = (undefined1 *)0xfd8a4180;
LAB_010055d8:
  FUN_01012ae0(DAT_01049ad0,0,0x80);
  puVar5 = DAT_01049ad0;
  iVar1 = read_toc_obj_hdr(0x200,local_58);
  if (iVar1 == 0) {
    puVar5[1] = local_4c;
    puVar5[2] = local_48;
    puVar5[3] = local_44;
  }
  else {
    puVar5[1] = 0;
    puVar5[2] = 0;
    puVar5[3] = 0;
  }
  puVar5 = DAT_01049ad0;
  iVar1 = read_toc_obj_hdr(5,local_58);
  if (iVar1 == 0) {
    puVar5[4] = local_4c;
    puVar5[5] = local_48;
    puVar5[6] = local_44;
  }
  else {
    puVar5[4] = 0;
    puVar5[5] = 0;
    puVar5[6] = 0;
  }
  puVar5 = DAT_01049ad0;
  iVar1 = read_toc_obj_hdr(0x100,local_58);
  if (iVar1 == 0) {
    puVar5[0x14] = local_4c;
    puVar5[0x15] = local_48;
    puVar5[0x16] = local_44;
  }
  else {
    puVar5[0x14] = 0;
    puVar5[0x15] = 0;
    puVar5[0x16] = 0;
  }
  puVar5 = DAT_01049ad0;
  iVar1 = read_toc_obj_hdr(0x110,local_58);
  if (iVar1 == 0) {
    puVar5[0x17] = local_4c;
    puVar5[0x18] = local_48;
    puVar5[0x19] = local_44;
  }
  else {
    puVar5[0x17] = 0;
    puVar5[0x18] = 0;
    puVar5[0x19] = 0;
  }
  iVar1 = FUN_01003460(DAT_01049dbf,0x61d,2,DAT_01049ad0 + AL_I2C_TAR_10BIT_ADDR_SHIFT,4);
  puVar5 = DAT_01049ad0;
  if (iVar1 == 0) {
    uVar6 = *(uint *)(DAT_01049ad0 + 0xc);
    *(uint *)(DAT_01049ad0 + 0xc) =
         uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 | uVar6 >> 0x18;
  }
  else {
    FUN_0100ceac(s_Unable_to_read_Board_RevID_from_E_01031668);
    FUN_01010bfc(s_Unable_to_read_Board_RevID_from_E_01031668);
    puVar5 = DAT_01049ad0;
    *(undefined4 *)(DAT_01049ad0 + 0xc) = 0;
  }
  *puVar5 = 0xa7;
  FUN_0100ceac(s_Initialized_general_shared_data_01031690);
  FUN_01010bfc(s_Initialized_general_shared_data_01031690);
  return 0;
}



/* @ 0x10057e2  thunk_FUN_01005ac0 */

void thunk_FUN_01005ac0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                       undefined4 param_5,uint param_6,undefined4 param_7,undefined4 param_8,
                       undefined4 param_9,uint *param_10,undefined4 param_11,undefined4 param_12,
                       uint param_13,undefined4 param_14,int param_15)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  uint *puVar6;
  undefined4 uVar7;
  char cVar8;
  uint uVar9;
  int *piVar10;
  int iVar11;
  uint unaff_r5;
  uint uVar12;
  int *piVar13;
  int iVar14;
  undefined4 *puVar15;
  undefined1 *puVar16;
  uint *unaff_r7;
  undefined1 uVar17;
  undefined4 *unaff_r10;
  int *piVar18;
  int unaff_r11;
  bool bVar19;
  uint in_stack_00000044;
  int in_stack_00000048;
  byte in_stack_00000388;
  int in_stack_0000038c;
  uint in_stack_00000390;
  int in_stack_00000394;
  int in_stack_00000398;
  byte in_stack_000003d4;
  int in_stack_000003d8;
  int in_stack_000003dc;
  int in_stack_000003e0;
  int in_stack_000003e4;
  byte in_stack_00000420;
  int in_stack_00000424;
  int in_stack_00000428;
  uint in_stack_0000042c;
  int in_stack_00000430;
  byte in_stack_0000046c;
  int in_stack_00000470;
  int in_stack_00000474;
  int in_stack_00000478;
  int in_stack_0000047c;
  byte in_stack_000004b8;
  code *UNRECOVERED_JUMPTABLE;
  
  while( true ) {
    iVar14 = unaff_r5 * 0x4c;
    param_11 = unaff_r5;
    FUN_010129d8(&stack0x00000510,DAT_01006908,param_3);
    FUN_01010b84(&stack0x00000108,param_8,unaff_r5);
    iVar1 = FUN_01007f84(*unaff_r10,&stack0x00000108);
    iVar11 = unaff_r11 + iVar14 + 4;
    if (iVar1 < 0) break;
    uVar2 = FUN_01007eb8(*unaff_r10,iVar1,param_9,0);
    uVar3 = FUN_01007eb8(*unaff_r10,iVar1,s_ref_clock_01031c70,0);
    uVar4 = FUN_01007eb8(*unaff_r10,iVar1,&DAT_01031c7c,0);
    iVar5 = FUN_01007e64(*unaff_r10,iVar1,s_active_lanes_01031c80,&stack0x00000044);
    if (iVar5 == 0) {
      FUN_0100ceac(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      FUN_01010bfc(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      break;
    }
    in_stack_00000044 = in_stack_00000044 >> 2;
    if (in_stack_00000044 != 0) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        if (3 < uVar12) {
          iVar1 = -0x16;
          FUN_0100ceac(s_invalid_active_lane___d___01031cac,uVar12);
          FUN_01010bfc(s_invalid_active_lane___d___01031cac,uVar12);
          goto LAB_01005be8;
        }
        if (*(int *)(iVar11 + uVar12 * 4) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_lane_defined_active_multiple_tim_01031cc8,uVar12);
          FUN_01010bfc(s_lane_defined_active_multiple_tim_01031cc8,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(iVar11 + uVar12 * 4) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar5 = FUN_01012954(uVar2,s_40gbe_010321b8);
    if (iVar5 == 0) {
      piVar10 = (int *)(unaff_r11 + iVar14);
      do {
        piVar10 = piVar10 + 1;
        if (*piVar10 != 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          FUN_01010bfc(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          goto LAB_01005be8;
        }
      } while ((int *)(iVar11 + 0xc) != piVar10);
    }
    iVar5 = FUN_01007e64(*unaff_r10,iVar1,s_inv_tx_lanes_01031d2c,&stack0x00000044);
    if ((iVar5 != 0) && (in_stack_00000044 = in_stack_00000044 >> 2, in_stack_00000044 != 0)) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        iVar5 = iVar14 + 0x20 + uVar12 * 4;
        if (3 < uVar12) goto LAB_01005cb8;
        if (*(int *)(unaff_r11 + iVar5) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_tx_lane_defined_inverted_multipl_01031d64,uVar12);
          FUN_01010bfc(s_tx_lane_defined_inverted_multipl_01031d64,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(unaff_r11 + iVar5) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar1 = FUN_01007e64(*unaff_r10,iVar1,s_inv_rx_lanes_01031d3c,&stack0x00000044);
    if ((iVar1 != 0) && (in_stack_00000044 = in_stack_00000044 >> 2, in_stack_00000044 != 0)) {
      puVar6 = (uint *)(iVar1 + 8);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        iVar1 = iVar14 + 0x30 + uVar12 * 4;
        if (3 < uVar12) goto LAB_01005cb8;
        if (*(int *)(unaff_r11 + iVar1) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_rx_lane_defined_inverted_multipl_01031d94,uVar12);
          FUN_01010bfc(s_rx_lane_defined_inverted_multipl_01031d94,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(unaff_r11 + iVar1) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar1 = param_6 * 0x10;
    piVar10 = (int *)(&stack0x00000068 + iVar1);
    puVar15 = (undefined4 *)(&stack0x00000248 + param_6 * 0x40);
    do {
      iVar14 = *piVar10;
      if (((-1 < iVar14) &&
          (iVar5 = FUN_01007eb8(*unaff_r10,iVar14,s_override_01031dc4,0), iVar5 != 0)) &&
         (iVar5 = FUN_01012954(iVar5,s_enabled_01030630), iVar5 == 0)) {
        uVar7 = *unaff_r10;
        *puVar15 = 1;
        iVar5 = FUN_01007e64(uVar7,iVar14,s_dcgain_01031dd0,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 1) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_3db_freq_01031dd8,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 5) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_gain_01031de8,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 6) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_1st_tap_ctrl_01031df4,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 7) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_2nd_tap_ctrl_01031e08,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 2) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_3rd_tap_ctrl_01031e1c,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 9) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_4th_tap_ctrl_01031e30,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 10) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_low_freq_agc_gain_01031e44,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 0xb) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar14 = FUN_01007e64(*unaff_r10,iVar14,s_high_freq_agc_boost_01031e58,&stack0x00000048);
        if ((iVar14 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 0xd) = (char)((uint)*(undefined4 *)(iVar14 + 0xc) >> 0x18);
        }
      }
      piVar10 = piVar10 + 1;
      puVar15 = puVar15 + 4;
    } while (piVar10 != (int *)(&stack0x00000078 + iVar1));
    puVar15 = (undefined4 *)(&stack0x00000158 + param_6 * 0x30);
    piVar10 = (int *)(&stack0x000000b8 + iVar1);
    do {
      iVar14 = *piVar10;
      if (((-1 < iVar14) &&
          (iVar5 = FUN_01007eb8(*unaff_r10,iVar14,s_override_01031dc4,0), iVar5 != 0)) &&
         (iVar5 = FUN_01012954(iVar5,s_enabled_01030630), iVar5 == 0)) {
        uVar7 = *unaff_r10;
        *puVar15 = 1;
        iVar5 = FUN_01007e64(uVar7,iVar14,&DAT_01031e6c,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 1) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_total_driver_units_01031e70,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 5) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_post_emph_01031e84,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 6) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_pre_emph_01031e90,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 2) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar14 = FUN_01007e64(*unaff_r10,iVar14,s_slew_rate_01031e9c,&stack0x00000048);
        if ((iVar14 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 9) = (char)((uint)*(undefined4 *)(iVar14 + 0xc) >> 0x18);
        }
      }
      piVar10 = piVar10 + 1;
      puVar15 = puVar15 + 3;
    } while ((int *)(&stack0x000000c8 + iVar1) != piVar10);
    iVar14 = 0;
    iVar1 = param_11 * 0x10;
    (&stack0x0000004c)[param_11] = 0;
    puVar15 = (undefined4 *)(&stack0x00000510 + param_11 * 0xf0);
    while ((puVar15[1] != 1 || (iVar5 = FUN_01012954(uVar2,*puVar15), iVar5 != 0))) {
      uVar9 = param_11;
      iVar14 = iVar14 + 1;
      puVar15 = puVar15 + 4;
      if (iVar14 == 0xf) {
        iVar1 = -0x16;
        FUN_0100ceac(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,param_11);
        FUN_01010bfc(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,uVar9);
        goto LAB_01005be8;
      }
    }
    if ((*(int *)(&stack0x0000051c + ((iVar1 - param_11) + iVar14) * 0x10) == -1) ||
       (uVar17 = 1,
       *(int *)(DAT_0104a3a8 +
                *(int *)(&stack0x0000051c + ((iVar1 - param_11) + iVar14) * 0x10) * 0x14 + 0xc) == 0
       )) {
      uVar17 = (&stack0x00000518)[((iVar1 - param_11) + iVar14) * 0x10];
    }
    iVar1 = param_11 * 0x4c;
    (&stack0x00000388)[iVar1] = uVar17;
    iVar14 = FUN_01012954(uVar3,s_100Mhz_internal_01031ea8);
    if (iVar14 == 0) {
      iVar14 = 2;
      (&stack0x0000004c)[param_11] = 2;
    }
    else {
      iVar14 = FUN_01012954(uVar3,s_100Mhz_01031eb8);
      if (iVar14 == 0) {
        *(undefined2 *)(&stack0x0000039e + iVar1) = 0x101;
        iVar14 = 0;
      }
      else {
        iVar14 = FUN_01012954(uVar3,s_125Mhz_01031ec0);
        if (iVar14 == 0) {
          *(undefined2 *)(&stack0x0000039e + iVar1) = 0x201;
          iVar14 = 0;
        }
        else {
          iVar14 = FUN_01012954(uVar3,s_156_25Mhz_01031ec8);
          if (iVar14 == 0) {
            *(undefined2 *)(&stack0x0000039e + iVar1) = 0x301;
            iVar14 = 0;
          }
          else {
            iVar14 = FUN_01012954(uVar3,&DAT_01031ed4);
            if (iVar14 == 0) {
              iVar14 = 1;
              (&stack0x0000039d)[iVar1] = 4;
              (&stack0x0000039e)[iVar1] = 4;
              (&stack0x0000004c)[param_11] = 1;
            }
            else {
              iVar14 = FUN_01012954(uVar3,s_right_01031edc);
              if (iVar14 != 0) {
                iVar1 = -0x16;
                FUN_0100ceac(s_Not_supported_reference_clock____01031ee4,uVar3);
                FUN_01010bfc(s_Not_supported_reference_clock____01031ee4,uVar3);
                goto LAB_01005be8;
              }
              iVar14 = 2;
              (&stack0x0000039c)[iVar1] = 2;
              (&stack0x0000039e)[iVar1] = 2;
              (&stack0x0000004c)[param_11] = 2;
            }
          }
        }
      }
    }
    if (param_15 != 0) {
      iVar1 = param_11 * 0x4c;
      (&stack0x0000039d)[iVar1] = (&stack0x0000039e)[iVar1];
      (&stack0x000003a1)[iVar1] = (&stack0x0000039f)[iVar1];
    }
    uVar9 = (uint)(byte)(&stack0x00000388)[param_11 * 0x4c];
    bVar19 = 0x14 < uVar9;
    if (uVar9 != 0x15) {
      bVar19 = 1 < uVar9 - 0x10;
    }
    if (!bVar19 || (uVar9 == 0x15 || uVar9 - 0x10 == 2)) {
      iVar1 = 0;
      piVar10 = DAT_0100690c;
      do {
        if (((*piVar10 != 0) && (piVar10[4] == param_11)) &&
           (*(int *)(iVar11 + piVar10[8] * 4) == 1)) {
          FUN_01003f30(iVar1);
        }
        iVar1 = iVar1 + 1;
        piVar10 = piVar10 + 1;
      } while (iVar1 != 4);
    }
    iVar1 = param_11 * 0x4c;
    cVar8 = (&stack0x00000388)[iVar1];
    if (cVar8 == '\x13') {
      if ((DAT_0104a56c != 0) && (DAT_0104a57c == param_11)) {
        FUN_01003f30(0,1);
        cVar8 = (&stack0x00000388)[iVar1];
        goto LAB_01006384;
      }
    }
    else {
LAB_01006384:
      if (cVar8 == '\x14') {
        if ((DAT_0104a574 == 0) || (DAT_0104a584 != param_11)) goto LAB_010063b0;
        FUN_01003f30(2,1);
        cVar8 = (&stack0x00000388)[param_11 * 0x4c];
      }
      if (cVar8 == '\x0e') {
        FUN_01003f30(8,1);
        cVar8 = (&stack0x00000388)[param_11 * 0x4c];
      }
      if (cVar8 == '\x0f') {
        FUN_01003f30(9,1);
      }
    }
LAB_010063b0:
    iVar1 = FUN_01012954(uVar4,s_enabled_01030630);
    if (iVar1 == 0) {
      *(undefined4 *)(&stack0x000003a4 + param_11 * 0x4c) = 1;
    }
    if (iVar14 == 2) {
      if ((&stack0x0000004c)[param_13 & 0xff] != '\x02') {
        iVar1 = (param_13 & 0xff) * 0x4c;
        (&stack0x0000039c)[iVar1] = 1;
        (&stack0x000003a0)[iVar1] = (&stack0x0000039f)[iVar1];
      }
LAB_010063f4:
      param_15 = 0;
    }
    else {
      if (iVar14 != 1) goto LAB_010063f4;
      param_15 = 1;
    }
    if (param_10 == unaff_r7) {
      if ((DAT_0104a608 == 0) && ((in_stack_00000388 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_A_off_violation__01031f0c);
        FUN_01010bfc(s_group_A_off_violation__01031f0c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a60c == 0) && ((in_stack_000003d4 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_B_off_violation__01031f24);
        FUN_01010bfc(s_group_B_off_violation__01031f24);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a610 == 0) && ((in_stack_00000420 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_C_off_violation__01031f3c);
        FUN_01010bfc(s_group_C_off_violation__01031f3c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a614 == 0) && ((in_stack_0000046c & 0xfd) != 0)) {
        FUN_0100ceac(s_group_D_off_violation__01031f54);
        FUN_01010bfc(s_group_D_off_violation__01031f54);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a618 == 0) && (2 < in_stack_000004b8)) {
        FUN_0100ceac(s_group_E_off_violation__01031f6c);
        FUN_01010bfc(s_group_E_off_violation__01031f6c);
        DAT_0104a568 = 1;
      }
      if ((in_stack_000003d4 == 0xe) &&
         ((((DAT_0104a5c8 == 0 || ((in_stack_000003d8 != 0 && ((DAT_0104a5cc & 1) == 0)))) ||
           ((in_stack_000003dc != 0 && ((DAT_0104a5cc & 2) == 0)))) ||
          (((in_stack_000003e0 != 0 && ((DAT_0104a5cc & 4) == 0)) ||
           ((in_stack_000003e4 != 0 && ((DAT_0104a5cc & 8) == 0)))))))) {
        FUN_0100ceac(s_SATA_0_violation__01031f84);
        FUN_01010bfc(s_SATA_0_violation__01031f84);
        DAT_0104a568 = 1;
      }
      if (in_stack_00000420 == 0xf) {
        if ((((DAT_0104a5d0 == 0) || ((in_stack_00000424 != 0 && ((DAT_0104a5d4 & 1) == 0)))) ||
            ((in_stack_00000428 != 0 && ((DAT_0104a5d4 & 2) == 0)))) ||
           (((in_stack_0000042c != 0 && ((DAT_0104a5d4 & 4) == 0)) ||
            ((in_stack_00000430 != 0 && ((DAT_0104a5d4 & 8) == 0)))))) {
          FUN_0100ceac(s_SATA_1_violation__01031f98);
          FUN_01010bfc(s_SATA_1_violation__01031f98);
          DAT_0104a568 = 1;
          goto LAB_01006584;
        }
LAB_01006b0c:
        if ((in_stack_0000046c - 0x10 < 3) &&
           (((((in_stack_00000470 != 0 && (DAT_0104a5e4 == 0)) ||
              ((in_stack_00000474 != 0 && (DAT_0104a5e0 == 0)))) ||
             ((in_stack_00000478 != 0 && (DAT_0104a5dc == 0)))) ||
            ((in_stack_0000047c != 0 && (DAT_0104a5d8 == 0)))))) goto LAB_010065f0;
      }
      else {
LAB_01006584:
        if ((2 < in_stack_00000420 - 0x10) ||
           (((((in_stack_00000424 == 0 || (DAT_0104a5e4 != 0)) &&
              ((in_stack_00000428 == 0 || (DAT_0104a5e0 != 0)))) &&
             ((in_stack_0000042c == 0 || (DAT_0104a5dc != 0)))) &&
            ((in_stack_00000430 == 0 || (DAT_0104a5d8 != 0)))))) goto LAB_01006b0c;
LAB_010065f0:
        FUN_0100ceac(s_ETH_violation__01031fac);
        FUN_01010bfc(s_ETH_violation__01031fac);
        DAT_0104a568 = 1;
      }
      uVar9 = (uint)in_stack_00000388;
      if (*(int *)(DAT_0104a3a8 + 0xc) == 0) {
        if (uVar9 == 5) {
          uVar12 = 2;
LAB_01006650:
          if (in_stack_00000390 != 0) {
            if (in_stack_00000394 == 0) {
              in_stack_00000390 = 1;
            }
            else if (in_stack_00000398 == 0) {
              in_stack_00000390 = 1;
            }
            else {
              in_stack_00000390 = 2;
            }
          }
        }
        else {
          uVar12 = (uint)(uVar9 - 3 < 2);
          if (uVar9 == 3) goto LAB_01006bbc;
LAB_01006644:
          if (uVar9 != 6) goto LAB_01006650;
          in_stack_00000390 = 3;
        }
LAB_0100667c:
        if (DAT_0104a5e8 < uVar12) {
          FUN_0100ceac(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar12);
          FUN_01010bfc(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar12,DAT_0104a5e8);
          DAT_0104a568 = 1;
        }
        if ((uVar12 != 0) && (DAT_0104a5ec < in_stack_00000390)) {
          FUN_0100ceac(s_PCIe_0_width_violation___d_>__d__01031fe0,in_stack_00000390);
          FUN_01010bfc(s_PCIe_0_width_violation___d_>__d__01031fe0,in_stack_00000390,DAT_0104a5ec);
          DAT_0104a568 = 1;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (in_stack_000003d4 == 8) {
            uVar9 = 2;
          }
          else if (in_stack_000003d4 == 7) {
            uVar9 = 1;
          }
          else {
LAB_010066fc:
            uVar9 = (uint)(in_stack_00000388 == 3);
          }
        }
        else {
          uVar9 = 3;
        }
        if (in_stack_00000388 == 3) goto LAB_01006e2c;
        uVar12 = 0;
        if (in_stack_000003dc != 0) {
          if (in_stack_000003e0 == 0) {
            uVar12 = 1;
          }
          else if (in_stack_000003e4 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_0100673c:
        if (DAT_0104a5f0 < uVar9) {
          FUN_0100ceac(s_PCIe_1_speed_violation___d_>__d__01032004,uVar9);
          FUN_01010bfc(s_PCIe_1_speed_violation___d_>__d__01032004,uVar9,DAT_0104a5f0);
          DAT_0104a568 = 1;
        }
        if ((uVar9 != 0) && (DAT_0104a5f4 < uVar12)) {
          FUN_0100ceac(s_PCIe_1_width_violation___d_>__d__01032028,uVar12);
          FUN_01010bfc(s_PCIe_1_width_violation___d_>__d__01032028,uVar12,DAT_0104a5f4);
          DAT_0104a568 = 1;
        }
      }
      else {
        uVar12 = 3;
        if (uVar9 != 3) goto LAB_01006644;
LAB_01006bbc:
        if (in_stack_0000038c != 0) {
          if (in_stack_00000390 != 0) {
            in_stack_00000390 = 1;
          }
          goto LAB_0100667c;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (in_stack_000003d4 == 8) {
            uVar9 = 2;
            goto LAB_01006e2c;
          }
          if (in_stack_000003d4 == 7) {
            uVar9 = 1;
            goto LAB_01006e2c;
          }
          goto LAB_010066fc;
        }
LAB_01006e2c:
        if (in_stack_00000394 != 0) {
          uVar12 = (uint)(in_stack_00000398 != 0);
          goto LAB_0100673c;
        }
      }
      uVar9 = (uint)in_stack_00000420;
      if (*(int *)(DAT_0104a3a8 + 0x34) == 0) {
        if (uVar9 == 0xb) {
          uVar12 = 2;
        }
        else {
          uVar12 = (uint)(uVar9 - 9 < 2);
          if (uVar9 == 9) goto LAB_01006b90;
        }
LAB_010067c0:
        if (in_stack_0000042c != 0) {
          if (in_stack_00000428 == 0) {
            in_stack_0000042c = 1;
          }
          else if (in_stack_00000424 == 0) {
            in_stack_0000042c = 1;
          }
          else {
            in_stack_0000042c = 2;
          }
        }
LAB_010067ec:
        if (DAT_0104a5f8 < uVar12) {
          FUN_0100ceac(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar12);
          FUN_01010bfc(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar12,DAT_0104a5f8);
          DAT_0104a568 = 1;
        }
        if ((uVar12 != 0) && (DAT_0104a5fc < in_stack_0000042c)) {
          FUN_0100ceac(s_PCIe_2_width_violation___d_>__d__01032070,in_stack_0000042c);
          FUN_01010bfc(s_PCIe_2_width_violation___d_>__d__01032070,in_stack_0000042c,DAT_0104a5fc);
          DAT_0104a568 = 1;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (in_stack_0000046c == 0xd) {
            uVar9 = 2;
          }
          else if (in_stack_0000046c == 0xc) {
            uVar9 = 1;
          }
          else {
LAB_0100686c:
            uVar9 = (uint)(in_stack_00000420 == 9);
          }
        }
        else {
          uVar9 = 3;
        }
        if (in_stack_00000420 == 9) goto LAB_01006bf4;
        uVar12 = 0;
        if (in_stack_00000478 != 0) {
          if (in_stack_00000474 == 0) {
            uVar12 = 1;
          }
          else if (in_stack_00000470 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
        if (uVar9 <= DAT_0104a600) goto LAB_010068b8;
LAB_01006c18:
        FUN_0100ceac(s_PCIe_3_speed_violation___d_>__d__01032094,uVar9);
        FUN_01010bfc(s_PCIe_3_speed_violation___d_>__d__01032094,uVar9,DAT_0104a600);
        DAT_0104a568 = 1;
        if ((uVar9 != 0) && (DAT_0104a604 < uVar12)) goto LAB_01006c58;
LAB_01006c84:
        FUN_01003f30(0);
        FUN_01003f30(2,0);
        FUN_01003f30(3,0);
        FUN_01003f30(6,0);
        FUN_01003f30(7,0);
        FUN_01003f30(8,0);
        FUN_01003f30(9,0);
        in_stack_00000388 = 0;
      }
      else {
        uVar12 = 3;
        if (uVar9 != 9) goto LAB_010067c0;
LAB_01006b90:
        if (in_stack_00000430 != 0) {
          if (in_stack_0000042c != 0) {
            in_stack_0000042c = 1;
          }
          goto LAB_010067ec;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (in_stack_0000046c == 0xd) {
            uVar9 = 2;
            goto LAB_01006bf4;
          }
          if (in_stack_0000046c == 0xc) {
            uVar9 = 1;
            goto LAB_01006bf4;
          }
          goto LAB_0100686c;
        }
        uVar9 = 3;
LAB_01006bf4:
        if (in_stack_00000428 != 0) {
          uVar12 = (uint)(in_stack_00000424 != 0);
          if (DAT_0104a600 < uVar9) goto LAB_01006c18;
LAB_010068b8:
          if ((uVar9 != 0) && (DAT_0104a604 < uVar12)) {
LAB_01006c58:
            FUN_0100ceac(s_PCIe_3_width_violation___d_>__d__010320b8,uVar12);
            FUN_01010bfc(s_PCIe_3_width_violation___d_>__d__010320b8,uVar12,DAT_0104a604);
            DAT_0104a568 = 1;
            goto LAB_01006c84;
          }
        }
        if (DAT_0104a568 != 0) goto LAB_01006c84;
      }
      uVar2 = DAT_01006910;
      iVar1 = 1000;
      FUN_01024b8c(DAT_01006910,0xfd860a00,0xfd8a8000);
      al_thermal_sensor_enable_set(uVar2,1);
      goto LAB_01006928;
    }
    param_6 = unaff_r7[1];
    param_13 = *unaff_r7;
    unaff_r5 = param_6 & 0xff;
    param_3 = 0x4b0;
    unaff_r7 = unaff_r7 + 1;
  }
  iVar1 = -0x16;
  goto LAB_01005be8;
LAB_01005cb8:
  iVar1 = -0x16;
  FUN_0100ceac(s_invalid_lane___d___01031d4c,uVar12);
  FUN_01010bfc(s_invalid_lane___d___01031d4c,uVar12);
  goto LAB_01005be8;
  while( true ) {
    udelay(1);
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) break;
LAB_01006928:
    iVar11 = al_thermal_sensor_is_ready(uVar2);
    if (iVar11 != 0) {
      al_thermal_sensor_trigger_continuous(DAT_01006910);
      goto LAB_01006944;
    }
  }
  FUN_0100ceac(s_Thermal_sensor_failed_to_power_u_010320dc);
  FUN_01010bfc(s_Thermal_sensor_failed_to_power_u_010320dc);
LAB_01006944:
  iVar11 = DAT_01006918;
  uVar9 = 0;
  iVar1 = DAT_01006918;
  do {
    iVar14 = FUN_0102614c(uVar9 * 0x400 + -0x2740000,uVar9 & 0xff,iVar1);
    uVar9 = uVar9 + 1;
    if (iVar14 != 0) goto LAB_01006e50;
    FUN_01026178(iVar1,&DAT_010057e0);
    iVar1 = iVar1 + 0xfc;
  } while (uVar9 != 4);
  iVar1 = FUN_0102614c(0xfd8c2000,4,DAT_01006914);
  if (iVar1 == 0) {
    FUN_01026178(DAT_01006914,&DAT_010057e0);
    if ((DAT_01049b20 != 0) && (2 < in_stack_000004b8)) {
      FUN_0100ceac(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      FUN_01010bfc(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      iVar1 = al_flash_obj_data_load(DAT_01049d8c,DAT_01049b1c,0x1120000);
      if (iVar1 != 0) {
        FUN_0100ceac(s_al_flash_obj_data_load_failed__s_01032160);
        FUN_01010bfc(s_al_flash_obj_data_load_failed__s_01032160);
      }
    }
    iVar1 = al_serdes_init_cores(DAT_01006918,0xfd8a8000);
    if (iVar1 == 0) {
      puVar16 = &stack0x00000248;
      iVar14 = 0;
      piVar10 = (int *)&stack0x0000038c;
      while( true ) {
        if (in_stack_00000388 != 0) {
          piVar13 = (int *)(&stack0x00000158 + iVar14);
          uVar9 = 0;
          piVar18 = piVar10;
          do {
            if (*piVar18 != 0) {
              if (*(int *)(puVar16 + uVar9 * 0x10) == 1) {
                (**(code **)(iVar11 + 0xa4))(iVar11,uVar9 & 0xff,puVar16 + uVar9 * 0x10);
              }
              if (*piVar13 == 1) {
                (**(code **)(iVar11 + 0x9c))(iVar11,uVar9 & 0xff,piVar13);
              }
            }
            uVar9 = uVar9 + 1;
            piVar13 = piVar13 + 3;
            piVar18 = piVar18 + 1;
          } while (uVar9 != 4);
        }
        iVar14 = iVar14 + 0x30;
        iVar11 = iVar11 + 0xfc;
        puVar16 = puVar16 + 0x40;
        if (iVar14 == 0xf0) break;
        in_stack_00000388 = *(byte *)(piVar10 + 0x12);
        piVar10 = piVar10 + 0x13;
      }
    }
  }
  else {
LAB_01006e50:
    iVar1 = -1;
    FUN_0100ceac(s_al_serdes_handle_init_failed__01032110);
    FUN_01010bfc(s_al_serdes_handle_init_failed__01032110);
  }
LAB_01005be8:
                    /* WARNING: Could not recover jumptable at 0x01005c08. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(iVar1);
  return;
}



/* @ 0x10057e4  FUN_010057e4 */

int al_thermal_sensor_readout_get(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar3 = DAT_0100585c;
  iVar4 = 50000;
  do {
    iVar2 = al_thermal_sensor_readout_is_valid(iVar3);
    iVar1 = DAT_0100585c;
    if (iVar2 != 0) {
      if (DAT_0100585c == 0) {
        FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8
                     ,1,s____HAL__drivers_sys_services_al__010434a0,
                     s_al_thermal_sensor_readout_get_01043480,0x13b);
        FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8
                     ,1,s____HAL__drivers_sys_services_al__010434a0,
                     s_al_thermal_sensor_readout_get_01043480,0x13b);
        FUN_01000454(0);
      }
      iVar3 = *(int *)(iVar1 + 0xc) * *(int *)(iVar1 + 4);
      if (iVar3 < 0) {
        iVar3 = iVar3 + 0xfff;
      }
      return ((iVar3 >> 0xc) - *(int *)(iVar1 + 8)) / 10;
    }
    udelay(1);
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  FUN_0100ceac(s_Thermal_sensor_failed_to_measure_0103225c);
  FUN_01010bfc(s_Thermal_sensor_failed_to_measure_0103225c);
  return -0x3e;
}



/* @ 0x1005860  FUN_01005860 */

void FUN_01005860(void)

{
  int iVar1;
  int local_2c;
  undefined1 auStack_28 [16];
  undefined4 local_18;
  
  iVar1 = al_flash_toc_find_id_with_fallback(DAT_01049d8c,DAT_0104a500,DAT_0104a504 << 0x1c | 0x500,
                       DAT_0104a508 << 0x1c | 0x500,0,&local_2c,auStack_28);
  if (((iVar1 == 0) && (-1 < local_2c)) &&
     (iVar1 = al_flash_obj_header_read_and_validate(DAT_01049d8c,local_18,&DAT_01049ad4), iVar1 == 0)) {
    DAT_01049b1c = local_18;
    DAT_01049b20 = 1;
  }
  return;
}



/* @ 0x1005914  FUN_01005914 */

void dt_based_init_serdes_group(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  uint *puVar6;
  byte bVar7;
  uint uVar8;
  byte *pbVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  uint uVar13;
  int *piVar14;
  int *piVar15;
  undefined4 *puVar16;
  int *piVar17;
  uint *puVar18;
  code *UNRECOVERED_JUMPTABLE;
  bool bVar19;
  int *local_9e4;
  int *local_9e0;
  uint local_9c8;
  uint uStack_9a4;
  int iStack_9a0;
  char acStack_99c [8];
  uint local_994;
  undefined4 uStack_990;
  undefined4 local_98c;
  undefined4 uStack_988;
  uint local_984;
  int aiStack_980 [4];
  int aiStack_970 [16];
  int aiStack_930 [4];
  int aiStack_920 [16];
  undefined1 auStack_8e0 [80];
  int aiStack_890 [60];
  undefined4 auStack_7a0 [80];
  byte abStack_660 [4];
  int aiStack_65c [3];
  int iStack_650;
  undefined1 local_64c [2];
  undefined2 local_64a;
  undefined1 local_648 [4];
  undefined4 uStack_644;
  byte abStack_640 [16];
  byte abStack_630 [28];
  byte bStack_614;
  int iStack_610;
  int iStack_60c;
  int iStack_608;
  int iStack_604;
  undefined1 local_600;
  undefined1 local_5fe;
  byte bStack_5c8;
  int iStack_5c4;
  int iStack_5c0;
  uint uStack_5bc;
  int iStack_5b8;
  undefined1 local_5b4;
  undefined1 local_5b2;
  undefined1 local_5b0;
  byte bStack_57c;
  int iStack_578;
  int iStack_574;
  int iStack_570;
  int iStack_56c;
  undefined1 local_568;
  undefined1 local_566;
  undefined1 local_563;
  byte bStack_530;
  undefined1 local_51b;
  undefined1 local_51a;
  undefined1 local_517;
  undefined4 uStack_4e0;
  undefined4 uStack_4dc;
  undefined4 auStack_4d8 [2];
  byte abStack_4d0 [4];
  int aiStack_4cc [298];
  
  FUN_01012ae0(abStack_660,0,0x188);
  local_994 = DAT_01031704;
  uStack_990 = DAT_01031708;
  local_64c[0] = 2;
  local_64a._0_1_ = 2;
  local_600 = 2;
  local_5fe = 2;
  local_568 = 2;
  local_566 = 2;
  local_9e0 = aiStack_980;
  local_5b4 = 3;
  local_98c = DAT_0103170c;
  uStack_988 = DAT_01031710;
  local_984 = DAT_01031714;
  iVar11 = 0;
  local_648[0] = 1;
  local_5b2 = 3;
  local_5b0 = 1;
  local_563 = 1;
  local_51b = 4;
  local_51a = 4;
  local_517 = 1;
  FUN_01012ae0(auStack_7a0,0,0x140);
  FUN_01012ae0(aiStack_890,0,0xf0);
  local_9e4 = aiStack_930;
  do {
    iVar10 = 0;
    piVar14 = local_9e0;
    piVar17 = local_9e4;
    do {
      FUN_01010b84(auStack_4d8,s__soc_board_cfg_serdes_group_d_la_01031be4,iVar11,iVar10);
      iVar1 = FUN_01007f84(DAT_01049d88,auStack_4d8);
      *piVar14 = iVar1;
      FUN_01010b84(auStack_4d8,s__soc_board_cfg_serdes_group_d_la_01031c14,iVar11,iVar10);
      iVar10 = iVar10 + 1;
      iVar1 = FUN_01007f84(DAT_01049d88,auStack_4d8);
      *piVar17 = iVar1;
      piVar14 = piVar14 + 1;
      piVar17 = piVar17 + 1;
    } while (iVar10 != 4);
    iVar11 = iVar11 + 1;
    local_9e0 = local_9e0 + 4;
    local_9e4 = local_9e4 + 4;
  } while (iVar11 != 5);
  bVar19 = false;
  local_9e4 = (int *)local_994;
  local_9c8 = 0xffffffff;
  puVar18 = &local_994;
  while( true ) {
    uVar12 = (uint)local_9e4 & 0xff;
    iVar1 = uVar12 * 0x4c;
    FUN_010129d8(auStack_4d8,DAT_01006908,0x4b0);
    FUN_01010b84(auStack_8e0,s__soc_board_cfg_serdes_group_d_01031c44,uVar12);
    iVar10 = iVar1 + 4;
    iVar11 = FUN_01007f84(DAT_01049d88,auStack_8e0);
    if (iVar11 < 0) break;
    uVar2 = FUN_01007eb8(DAT_01049d88,iVar11,s_interface_01031c64,0);
    uVar3 = FUN_01007eb8(DAT_01049d88,iVar11,s_ref_clock_01031c70,0);
    uVar4 = FUN_01007eb8(DAT_01049d88,iVar11,&DAT_01031c7c,0);
    iVar5 = FUN_01007e64(DAT_01049d88,iVar11,s_active_lanes_01031c80,&uStack_9a4);
    if (iVar5 == 0) {
      FUN_0100ceac(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      FUN_01010bfc(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      break;
    }
    uStack_9a4 = uStack_9a4 >> 2;
    if (uStack_9a4 != 0) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        if (3 < uVar13) {
          iVar11 = -0x16;
          FUN_0100ceac(s_invalid_active_lane___d___01031cac,uVar13);
          FUN_01010bfc(s_invalid_active_lane___d___01031cac,uVar13);
          goto LAB_01005be8;
        }
        if (*(int *)(abStack_660 + uVar13 * 4 + iVar10) == 1) {
          iVar11 = -0x16;
          FUN_0100ceac(s_lane_defined_active_multiple_tim_01031cc8,uVar13);
          FUN_01010bfc(s_lane_defined_active_multiple_tim_01031cc8,uVar13);
          goto LAB_01005be8;
        }
        pbVar9 = abStack_660 + uVar13 * 4 + iVar10;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (uStack_9a4 != uVar8);
    }
    iVar5 = FUN_01012954(uVar2,s_40gbe_010321b8);
    if (iVar5 == 0) {
      pbVar9 = abStack_660 + iVar1;
      do {
        pbVar9 = pbVar9 + 4;
        if (*(int *)pbVar9 != 1) {
          iVar11 = -0x16;
          FUN_0100ceac(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          FUN_01010bfc(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          goto LAB_01005be8;
        }
      } while (local_64c + iVar1 + -4 != pbVar9);
    }
    iVar5 = FUN_01007e64(DAT_01049d88,iVar11,s_inv_tx_lanes_01031d2c,&uStack_9a4);
    if ((iVar5 != 0) && (uStack_9a4 = uStack_9a4 >> 2, uStack_9a4 != 0)) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        iVar5 = iVar1 + 0x20 + uVar13 * 4;
        if (3 < uVar13) goto LAB_01005cb8;
        if (*(int *)(abStack_660 + iVar5) == 1) {
          iVar11 = -0x16;
          FUN_0100ceac(s_tx_lane_defined_inverted_multipl_01031d64,uVar13);
          FUN_01010bfc(s_tx_lane_defined_inverted_multipl_01031d64,uVar13);
          goto LAB_01005be8;
        }
        pbVar9 = abStack_660 + iVar5;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (uStack_9a4 != uVar8);
    }
    iVar11 = FUN_01007e64(DAT_01049d88,iVar11,s_inv_rx_lanes_01031d3c,&uStack_9a4);
    if ((iVar11 != 0) && (uStack_9a4 = uStack_9a4 >> 2, uStack_9a4 != 0)) {
      puVar6 = (uint *)(iVar11 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        iVar11 = iVar1 + 0x30 + uVar13 * 4;
        if (3 < uVar13) goto LAB_01005cb8;
        if (*(int *)(abStack_660 + iVar11) == 1) {
          iVar11 = -0x16;
          FUN_0100ceac(s_rx_lane_defined_inverted_multipl_01031d94,uVar13);
          FUN_01010bfc(s_rx_lane_defined_inverted_multipl_01031d94,uVar13);
          goto LAB_01005be8;
        }
        pbVar9 = abStack_660 + iVar11;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (uStack_9a4 != uVar8);
    }
    piVar14 = aiStack_980 + (int)local_9e4 * 4;
    puVar16 = auStack_7a0 + (int)local_9e4 * 0x10;
    do {
      iVar11 = *piVar14;
      if (((-1 < iVar11) &&
          (iVar1 = FUN_01007eb8(DAT_01049d88,iVar11,s_override_01031dc4,0), iVar1 != 0)) &&
         (iVar1 = FUN_01012954(iVar1,s_enabled_01030630), iVar1 == 0)) {
        *puVar16 = 1;
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dcgain_01031dd0,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)(puVar16 + 1) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_3db_freq_01031dd8,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 5) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_gain_01031de8,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 6) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_1st_tap_ctrl_01031df4,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 7) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_2nd_tap_ctrl_01031e08,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)(puVar16 + 2) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_3rd_tap_ctrl_01031e1c,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 9) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_dfe_4th_tap_ctrl_01031e30,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 10) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_low_freq_agc_gain_01031e44,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 0xb) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar11 = FUN_01007e64(DAT_01049d88,iVar11,s_high_freq_agc_boost_01031e58,&iStack_9a0);
        if ((iVar11 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)puVar16 + 0xd) = (char)((uint)*(undefined4 *)(iVar11 + 0xc) >> 0x18);
        }
      }
      piVar14 = piVar14 + 1;
      puVar16 = puVar16 + 4;
    } while (piVar14 != aiStack_970 + (int)local_9e4 * 4);
    piVar17 = aiStack_890 + (int)local_9e4 * 0xc;
    piVar14 = aiStack_930 + (int)local_9e4 * 4;
    do {
      iVar11 = *piVar14;
      if (((-1 < iVar11) &&
          (iVar1 = FUN_01007eb8(DAT_01049d88,iVar11,s_override_01031dc4,0), iVar1 != 0)) &&
         (iVar1 = FUN_01012954(iVar1,s_enabled_01030630), iVar1 == 0)) {
        *piVar17 = 1;
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,&DAT_01031e6c,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)(piVar17 + 1) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_total_driver_units_01031e70,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)piVar17 + 5) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_post_emph_01031e84,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)piVar17 + 6) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e64(DAT_01049d88,iVar11,s_pre_emph_01031e90,&iStack_9a0);
        if ((iVar1 != 0) && (iStack_9a0 != 0)) {
          *(char *)(piVar17 + 2) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar11 = FUN_01007e64(DAT_01049d88,iVar11,s_slew_rate_01031e9c,&iStack_9a0);
        if ((iVar11 != 0) && (iStack_9a0 != 0)) {
          *(char *)((int)piVar17 + 9) = (char)((uint)*(undefined4 *)(iVar11 + 0xc) >> 0x18);
        }
      }
      piVar14 = piVar14 + 1;
      piVar17 = piVar17 + 3;
    } while (aiStack_920 + (int)local_9e4 * 4 != piVar14);
    iVar11 = 0;
    acStack_99c[uVar12] = '\0';
    puVar16 = auStack_4d8 + uVar12 * 0x3c;
    while ((puVar16[1] != 1 || (iVar1 = FUN_01012954(uVar2,*puVar16), iVar1 != 0))) {
      iVar11 = iVar11 + 1;
      puVar16 = puVar16 + 4;
      if (iVar11 == 0xf) {
        iVar11 = -0x16;
        FUN_0100ceac(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,uVar12);
        FUN_01010bfc(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,uVar12);
        goto LAB_01005be8;
      }
    }
    if ((aiStack_4cc[(uVar12 * 0xf + iVar11) * 4] == -1) ||
       (bVar7 = 1,
       *(int *)(DAT_0104a3a8 + aiStack_4cc[(uVar12 * 0xf + iVar11) * 4] * 0x14 + 0xc) == 0)) {
      bVar7 = abStack_4d0[(uVar12 * 0xf + iVar11) * 0x10];
    }
    iVar11 = uVar12 * 0x4c;
    abStack_660[iVar11] = bVar7;
    iVar1 = FUN_01012954(uVar3,s_100Mhz_internal_01031ea8);
    if (iVar1 == 0) {
      iVar1 = 2;
      acStack_99c[uVar12] = '\x02';
    }
    else {
      iVar1 = FUN_01012954(uVar3,s_100Mhz_01031eb8);
      if (iVar1 == 0) {
        *(undefined2 *)(local_64c + iVar11 + 2) = 0x101;
        iVar1 = 0;
      }
      else {
        iVar1 = FUN_01012954(uVar3,s_125Mhz_01031ec0);
        if (iVar1 == 0) {
          *(undefined2 *)(local_64c + iVar11 + 2) = 0x201;
          iVar1 = 0;
        }
        else {
          iVar1 = FUN_01012954(uVar3,s_156_25Mhz_01031ec8);
          if (iVar1 == 0) {
            *(undefined2 *)(local_64c + iVar11 + 2) = 0x301;
            iVar1 = 0;
          }
          else {
            iVar1 = FUN_01012954(uVar3,&DAT_01031ed4);
            if (iVar1 == 0) {
              iVar1 = 1;
              local_64c[iVar11 + 1] = 4;
              local_64c[iVar11 + 2] = 4;
              acStack_99c[uVar12] = '\x01';
            }
            else {
              iVar1 = FUN_01012954(uVar3,s_right_01031edc);
              if (iVar1 != 0) {
                iVar11 = -0x16;
                FUN_0100ceac(s_Not_supported_reference_clock____01031ee4,uVar3);
                FUN_01010bfc(s_Not_supported_reference_clock____01031ee4,uVar3);
                goto LAB_01005be8;
              }
              iVar1 = 2;
              local_64c[iVar11] = 2;
              local_64c[iVar11 + 2] = 2;
              acStack_99c[uVar12] = '\x02';
            }
          }
        }
      }
    }
    if (bVar19) {
      iVar11 = uVar12 * 0x4c;
      local_64c[iVar11 + 1] = local_64c[iVar11 + 2];
      local_64c[iVar11 + 5] = local_64c[iVar11 + 3];
    }
    uVar8 = (uint)abStack_660[uVar12 * 0x4c];
    bVar19 = 0x14 < uVar8;
    if (uVar8 != 0x15) {
      bVar19 = 1 < uVar8 - 0x10;
    }
    if (!bVar19 || (uVar8 == 0x15 || uVar8 - 0x10 == 2)) {
      iVar11 = 0;
      piVar14 = DAT_0100690c;
      do {
        if (((*piVar14 != 0) && (piVar14[4] == uVar12)) &&
           (*(int *)(abStack_660 + piVar14[8] * 4 + iVar10) == 1)) {
          FUN_01003f30(iVar11);
        }
        iVar11 = iVar11 + 1;
        piVar14 = piVar14 + 1;
      } while (iVar11 != 4);
    }
    bVar7 = abStack_660[uVar12 * 0x4c];
    if (bVar7 == 0x13) {
      if ((DAT_0104a56c != 0) && (DAT_0104a57c == uVar12)) {
        FUN_01003f30(0,1);
        bVar7 = abStack_660[uVar12 * 0x4c];
        goto LAB_01006384;
      }
    }
    else {
LAB_01006384:
      if (bVar7 == 0x14) {
        if ((DAT_0104a574 == 0) || (DAT_0104a584 != uVar12)) goto LAB_010063b0;
        FUN_01003f30(2,1);
        bVar7 = abStack_660[uVar12 * 0x4c];
      }
      if (bVar7 == 0xe) {
        FUN_01003f30(8,1);
        bVar7 = abStack_660[uVar12 * 0x4c];
      }
      if (bVar7 == 0xf) {
        FUN_01003f30(9,1);
      }
    }
LAB_010063b0:
    iVar11 = FUN_01012954(uVar4,s_enabled_01030630);
    if (iVar11 == 0) {
      *(undefined4 *)(local_64c + uVar12 * 0x4c + 8) = 1;
    }
    if (iVar1 == 2) {
      if (acStack_99c[local_9c8 & 0xff] != '\x02') {
        iVar11 = (local_9c8 & 0xff) * 0x4c;
        local_64c[iVar11] = 1;
        local_64c[iVar11 + 4] = local_64c[iVar11 + 3];
      }
LAB_010063f4:
      bVar19 = false;
    }
    else {
      if (iVar1 != 1) goto LAB_010063f4;
      bVar19 = true;
    }
    if (&local_984 == puVar18) {
      if ((DAT_0104a608 == 0) && ((abStack_660[0] & 0xfd) != 0)) {
        FUN_0100ceac(s_group_A_off_violation__01031f0c);
        FUN_01010bfc(s_group_A_off_violation__01031f0c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a60c == 0) && ((bStack_614 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_B_off_violation__01031f24);
        FUN_01010bfc(s_group_B_off_violation__01031f24);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a610 == 0) && ((bStack_5c8 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_C_off_violation__01031f3c);
        FUN_01010bfc(s_group_C_off_violation__01031f3c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a614 == 0) && ((bStack_57c & 0xfd) != 0)) {
        FUN_0100ceac(s_group_D_off_violation__01031f54);
        FUN_01010bfc(s_group_D_off_violation__01031f54);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a618 == 0) && (2 < bStack_530)) {
        FUN_0100ceac(s_group_E_off_violation__01031f6c);
        FUN_01010bfc(s_group_E_off_violation__01031f6c);
        DAT_0104a568 = 1;
      }
      if ((bStack_614 == 0xe) &&
         ((((DAT_0104a5c8 == 0 || ((iStack_610 != 0 && ((DAT_0104a5cc & 1) == 0)))) ||
           ((iStack_60c != 0 && ((DAT_0104a5cc & 2) == 0)))) ||
          (((iStack_608 != 0 && ((DAT_0104a5cc & 4) == 0)) ||
           ((iStack_604 != 0 && ((DAT_0104a5cc & 8) == 0)))))))) {
        FUN_0100ceac(s_SATA_0_violation__01031f84);
        FUN_01010bfc(s_SATA_0_violation__01031f84);
        DAT_0104a568 = 1;
      }
      uVar12 = (uint)bStack_5c8;
      if (uVar12 == 0xf) {
        if ((((DAT_0104a5d0 == 0) || ((iStack_5c4 != 0 && ((DAT_0104a5d4 & 1) == 0)))) ||
            ((iStack_5c0 != 0 && ((DAT_0104a5d4 & 2) == 0)))) ||
           (((uStack_5bc != 0 && ((DAT_0104a5d4 & 4) == 0)) ||
            ((iStack_5b8 != 0 && ((DAT_0104a5d4 & 8) == 0)))))) {
          FUN_0100ceac(s_SATA_1_violation__01031f98);
          FUN_01010bfc(s_SATA_1_violation__01031f98);
          uVar12 = (uint)bStack_5c8;
          DAT_0104a568 = 1;
          goto LAB_01006584;
        }
LAB_01006b0c:
        if ((bStack_57c - 0x10 < 3) &&
           (((((iStack_578 != 0 && (DAT_0104a5e4 == 0)) ||
              ((iStack_574 != 0 && (DAT_0104a5e0 == 0)))) ||
             ((iStack_570 != 0 && (DAT_0104a5dc == 0)))) ||
            ((iStack_56c != 0 && (DAT_0104a5d8 == 0)))))) goto LAB_010065f0;
      }
      else {
LAB_01006584:
        if ((2 < uVar12 - 0x10) ||
           (((((iStack_5c4 == 0 || (DAT_0104a5e4 != 0)) &&
              ((iStack_5c0 == 0 || (DAT_0104a5e0 != 0)))) &&
             ((uStack_5bc == 0 || (DAT_0104a5dc != 0)))) &&
            ((iStack_5b8 == 0 || (DAT_0104a5d8 != 0)))))) goto LAB_01006b0c;
LAB_010065f0:
        FUN_0100ceac(s_ETH_violation__01031fac);
        FUN_01010bfc(s_ETH_violation__01031fac);
        DAT_0104a568 = 1;
      }
      uVar8 = (uint)abStack_660[0];
      uVar12 = aiStack_65c[1];
      if (*(int *)(DAT_0104a3a8 + 0xc) == 0) {
        if (uVar8 == 5) {
          uVar13 = 2;
LAB_01006650:
          if (aiStack_65c[1] != 0) {
            if (aiStack_65c[2] == 0) {
              uVar12 = 1;
            }
            else if (iStack_650 == 0) {
              uVar12 = 1;
            }
            else {
              uVar12 = 2;
            }
          }
        }
        else {
          uVar13 = (uint)(uVar8 - 3 < 2);
          if (uVar8 == 3) goto LAB_01006bbc;
LAB_01006644:
          if (uVar8 != 6) goto LAB_01006650;
          uVar12 = 3;
        }
LAB_0100667c:
        if (DAT_0104a5e8 < uVar13) {
          FUN_0100ceac(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar13);
          FUN_01010bfc(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar13,DAT_0104a5e8);
          DAT_0104a568 = 1;
        }
        if ((uVar13 != 0) && (DAT_0104a5ec < uVar12)) {
          FUN_0100ceac(s_PCIe_0_width_violation___d_>__d__01031fe0,uVar12);
          FUN_01010bfc(s_PCIe_0_width_violation___d_>__d__01031fe0,uVar12,DAT_0104a5ec);
          DAT_0104a568 = 1;
        }
        uVar8 = (uint)abStack_660[0];
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (bStack_614 == 8) {
            uVar12 = 2;
          }
          else if (bStack_614 == 7) {
            uVar12 = 1;
          }
          else {
LAB_010066fc:
            uVar12 = (uint)(uVar8 == 3);
          }
        }
        else {
          uVar12 = 3;
        }
        bVar19 = uVar8 == 3;
        uVar8 = uVar12;
        if (bVar19) goto LAB_01006e2c;
        uVar12 = 0;
        if (iStack_60c != 0) {
          if (iStack_608 == 0) {
            uVar12 = 1;
          }
          else if (iStack_604 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_0100673c:
        if (DAT_0104a5f0 < uVar8) {
          FUN_0100ceac(s_PCIe_1_speed_violation___d_>__d__01032004,uVar8);
          FUN_01010bfc(s_PCIe_1_speed_violation___d_>__d__01032004,uVar8,DAT_0104a5f0);
          DAT_0104a568 = 1;
        }
        if ((uVar8 != 0) && (DAT_0104a5f4 < uVar12)) {
          FUN_0100ceac(s_PCIe_1_width_violation___d_>__d__01032028,uVar12);
          FUN_01010bfc(s_PCIe_1_width_violation___d_>__d__01032028,uVar12,DAT_0104a5f4);
          DAT_0104a568 = 1;
        }
      }
      else {
        uVar13 = 3;
        if (uVar8 != 3) goto LAB_01006644;
LAB_01006bbc:
        if (aiStack_65c[0] != 0) {
          if (aiStack_65c[1] != 0) {
            uVar12 = 1;
          }
          goto LAB_0100667c;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (bStack_614 == 8) {
            uVar8 = 2;
            goto LAB_01006e2c;
          }
          if (bStack_614 == 7) {
            uVar8 = 1;
            goto LAB_01006e2c;
          }
          goto LAB_010066fc;
        }
LAB_01006e2c:
        if (aiStack_65c[2] != 0) {
          uVar12 = (uint)(iStack_650 != 0);
          goto LAB_0100673c;
        }
      }
      uVar8 = (uint)bStack_5c8;
      uVar12 = uStack_5bc;
      if (*(int *)(DAT_0104a3a8 + 0x34) == 0) {
        if (uVar8 == 0xb) {
          uVar13 = 2;
        }
        else {
          uVar13 = (uint)(uVar8 - 9 < 2);
          if (uVar8 == 9) goto LAB_01006b90;
        }
LAB_010067c0:
        if (uStack_5bc != 0) {
          if (iStack_5c0 == 0) {
            uVar12 = 1;
          }
          else if (iStack_5c4 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_010067ec:
        if (DAT_0104a5f8 < uVar13) {
          FUN_0100ceac(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar13);
          FUN_01010bfc(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar13,DAT_0104a5f8);
          DAT_0104a568 = 1;
        }
        if ((uVar13 != 0) && (DAT_0104a5fc < uVar12)) {
          FUN_0100ceac(s_PCIe_2_width_violation___d_>__d__01032070,uVar12);
          FUN_01010bfc(s_PCIe_2_width_violation___d_>__d__01032070,uVar12,DAT_0104a5fc);
          DAT_0104a568 = 1;
        }
        uVar8 = (uint)bStack_5c8;
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (bStack_57c == 0xd) {
            uVar12 = 2;
          }
          else if (bStack_57c == 0xc) {
            uVar12 = 1;
          }
          else {
LAB_0100686c:
            uVar12 = (uint)(uVar8 == 9);
          }
        }
        else {
          uVar12 = 3;
        }
        if (uVar8 == 9) goto LAB_01006bf4;
        uVar8 = 0;
        if (iStack_570 != 0) {
          if (iStack_574 == 0) {
            uVar8 = 1;
          }
          else if (iStack_578 == 0) {
            uVar8 = 1;
          }
          else {
            uVar8 = 2;
          }
        }
        if (uVar12 <= DAT_0104a600) goto LAB_010068b8;
LAB_01006c18:
        FUN_0100ceac(s_PCIe_3_speed_violation___d_>__d__01032094,uVar12);
        FUN_01010bfc(s_PCIe_3_speed_violation___d_>__d__01032094,uVar12,DAT_0104a600);
        DAT_0104a568 = 1;
        if ((uVar12 != 0) && (DAT_0104a604 < uVar8)) goto LAB_01006c58;
LAB_01006c84:
        FUN_01003f30(0);
        FUN_01003f30(2,0);
        FUN_01003f30(3,0);
        FUN_01003f30(6,0);
        FUN_01003f30(7,0);
        FUN_01003f30(8,0);
        FUN_01003f30(9,0);
        abStack_660[0] = 0;
        bStack_614 = 0;
        bStack_5c8 = 0;
        bStack_57c = 0;
      }
      else {
        uVar13 = 3;
        if (uVar8 != 9) goto LAB_010067c0;
LAB_01006b90:
        if (iStack_5b8 != 0) {
          if (uStack_5bc != 0) {
            uVar12 = 1;
          }
          goto LAB_010067ec;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (bStack_57c == 0xd) {
            uVar12 = 2;
            goto LAB_01006bf4;
          }
          if (bStack_57c == 0xc) {
            uVar12 = 1;
            goto LAB_01006bf4;
          }
          goto LAB_0100686c;
        }
        uVar12 = 3;
LAB_01006bf4:
        if (iStack_5c0 != 0) {
          uVar8 = (uint)(iStack_5c4 != 0);
          if (DAT_0104a600 < uVar12) goto LAB_01006c18;
LAB_010068b8:
          if ((uVar12 != 0) && (DAT_0104a604 < uVar8)) {
LAB_01006c58:
            FUN_0100ceac(s_PCIe_3_width_violation___d_>__d__010320b8,uVar8);
            FUN_01010bfc(s_PCIe_3_width_violation___d_>__d__010320b8,uVar8,DAT_0104a604);
            DAT_0104a568 = 1;
            goto LAB_01006c84;
          }
        }
        if (DAT_0104a568 != 0) goto LAB_01006c84;
      }
      uVar2 = DAT_01006910;
      iVar11 = 1000;
      FUN_01024b8c(DAT_01006910,0xfd860a00,0xfd8a8000);
      al_thermal_sensor_enable_set(uVar2,1);
      goto LAB_01006928;
    }
    local_9e4 = (int *)puVar18[1];
    local_9c8 = *puVar18;
    puVar18 = puVar18 + 1;
  }
  iVar11 = -0x16;
  goto LAB_01005be8;
LAB_01005cb8:
  iVar11 = -0x16;
  FUN_0100ceac(s_invalid_lane___d___01031d4c,uVar13);
  FUN_01010bfc(s_invalid_lane___d___01031d4c,uVar13);
  goto LAB_01005be8;
  while( true ) {
    udelay(1);
    iVar11 = iVar11 + -1;
    if (iVar11 == 0) break;
LAB_01006928:
    iVar10 = al_thermal_sensor_is_ready(uVar2);
    if (iVar10 != 0) {
      al_thermal_sensor_trigger_continuous(DAT_01006910);
      goto LAB_01006944;
    }
  }
  FUN_0100ceac(s_Thermal_sensor_failed_to_power_u_010320dc);
  FUN_01010bfc(s_Thermal_sensor_failed_to_power_u_010320dc);
LAB_01006944:
  iVar10 = DAT_01006918;
  uVar12 = 0;
  iVar11 = DAT_01006918;
  do {
    iVar1 = FUN_0102614c(uVar12 * 0x400 + -0x2740000,uVar12 & 0xff,iVar11);
    uVar12 = uVar12 + 1;
    if (iVar1 != 0) goto LAB_01006e50;
    FUN_01026178(iVar11,&DAT_010057e0);
    iVar11 = iVar11 + 0xfc;
  } while (uVar12 != 4);
  iVar11 = FUN_0102614c(0xfd8c2000,4,DAT_01006914);
  if (iVar11 == 0) {
    FUN_01026178(DAT_01006914,&DAT_010057e0);
    if ((DAT_01049b20 != 0) && (2 < bStack_530)) {
      FUN_0100ceac(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      FUN_01010bfc(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      iVar11 = al_flash_obj_data_load(DAT_01049d8c,DAT_01049b1c,0x1120000);
      if (iVar11 == 0) {
        uStack_4e0 = 0x1120000;
        uStack_4dc = DAT_01049afc;
      }
      else {
        FUN_0100ceac(s_al_flash_obj_data_load_failed__s_01032160);
        FUN_01010bfc(s_al_flash_obj_data_load_failed__s_01032160);
      }
    }
    iVar11 = al_serdes_init_cores(DAT_01006918,0xfd8a8000,abStack_660);
    if (iVar11 == 0) {
      puVar16 = auStack_7a0;
      iVar1 = 0;
      piVar14 = aiStack_65c;
      bVar7 = abStack_660[0];
      while( true ) {
        if (bVar7 != 0) {
          piVar15 = (int *)((int)aiStack_890 + iVar1);
          uVar12 = 0;
          piVar17 = piVar14;
          do {
            if (*piVar17 != 0) {
              if (puVar16[uVar12 * 4] == 1) {
                (**(code **)(iVar10 + 0xa4))(iVar10,uVar12 & 0xff,puVar16 + uVar12 * 4);
              }
              if (*piVar15 == 1) {
                (**(code **)(iVar10 + 0x9c))(iVar10,uVar12 & 0xff,piVar15);
              }
            }
            uVar12 = uVar12 + 1;
            piVar15 = piVar15 + 3;
            piVar17 = piVar17 + 1;
          } while (uVar12 != 4);
        }
        iVar1 = iVar1 + 0x30;
        iVar10 = iVar10 + 0xfc;
        puVar16 = puVar16 + 0x10;
        if (iVar1 == 0xf0) break;
        bVar7 = *(byte *)(piVar14 + 0x12);
        piVar14 = piVar14 + 0x13;
      }
    }
  }
  else {
LAB_01006e50:
    iVar11 = -1;
    FUN_0100ceac(s_al_serdes_handle_init_failed__01032110);
    FUN_01010bfc(s_al_serdes_handle_init_failed__01032110);
  }
LAB_01005be8:
                    /* WARNING: Could not recover jumptable at 0x01005c08. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(iVar11);
  return;
}



/* @ 0x1005ac0  FUN_01005ac0 */

void dt_based_init_serdes_group(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,uint param_6,undefined4 param_7,undefined4 param_8,
                 undefined4 param_9,uint *param_10,undefined4 param_11,undefined4 param_12,
                 uint param_13,undefined4 param_14,int param_15)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  uint *puVar6;
  undefined4 uVar7;
  char cVar8;
  uint uVar9;
  int *piVar10;
  int iVar11;
  uint unaff_r5;
  uint uVar12;
  int *piVar13;
  int iVar14;
  undefined4 *puVar15;
  undefined1 *puVar16;
  uint *unaff_r7;
  undefined1 uVar17;
  undefined4 *unaff_r10;
  int *piVar18;
  int unaff_r11;
  bool bVar19;
  uint in_stack_00000044;
  int in_stack_00000048;
  byte in_stack_00000388;
  int in_stack_0000038c;
  uint in_stack_00000390;
  int in_stack_00000394;
  int in_stack_00000398;
  byte in_stack_000003d4;
  int in_stack_000003d8;
  int in_stack_000003dc;
  int in_stack_000003e0;
  int in_stack_000003e4;
  byte in_stack_00000420;
  int in_stack_00000424;
  int in_stack_00000428;
  uint in_stack_0000042c;
  int in_stack_00000430;
  byte in_stack_0000046c;
  int in_stack_00000470;
  int in_stack_00000474;
  int in_stack_00000478;
  int in_stack_0000047c;
  byte in_stack_000004b8;
  code *UNRECOVERED_JUMPTABLE;
  
  while( true ) {
    iVar14 = unaff_r5 * 0x4c;
    param_11 = unaff_r5;
    FUN_010129d8(&stack0x00000510,DAT_01006908,param_3);
    FUN_01010b84(&stack0x00000108,param_8,unaff_r5);
    iVar1 = FUN_01007f84(*unaff_r10,&stack0x00000108);
    iVar11 = unaff_r11 + iVar14 + 4;
    if (iVar1 < 0) break;
    uVar2 = FUN_01007eb8(*unaff_r10,iVar1,param_9,0);
    uVar3 = FUN_01007eb8(*unaff_r10,iVar1,s_ref_clock_01031c70,0);
    uVar4 = FUN_01007eb8(*unaff_r10,iVar1,&DAT_01031c7c,0);
    iVar5 = FUN_01007e64(*unaff_r10,iVar1,s_active_lanes_01031c80,&stack0x00000044);
    if (iVar5 == 0) {
      FUN_0100ceac(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      FUN_01010bfc(s__s__property___s__missing_01031c90,s_dt_based_init_serdes_group_01031bc8,
                   s_active_lanes_01031c80);
      break;
    }
    in_stack_00000044 = in_stack_00000044 >> 2;
    if (in_stack_00000044 != 0) {
      puVar6 = (uint *)(iVar5 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        if (3 < uVar12) {
          iVar1 = -0x16;
          FUN_0100ceac(s_invalid_active_lane___d___01031cac,uVar12);
          FUN_01010bfc(s_invalid_active_lane___d___01031cac,uVar12);
          goto LAB_01005be8;
        }
        if (*(int *)(iVar11 + uVar12 * 4) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_lane_defined_active_multiple_tim_01031cc8,uVar12);
          FUN_01010bfc(s_lane_defined_active_multiple_tim_01031cc8,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(iVar11 + uVar12 * 4) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar5 = FUN_01012954(uVar2,s_40gbe_010321b8);
    if (iVar5 == 0) {
      piVar10 = (int *)(unaff_r11 + iVar14);
      do {
        piVar10 = piVar10 + 1;
        if (*piVar10 != 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          FUN_01010bfc(s_When_serdes_is_40gbe__all_lanes_m_01031cf4);
          goto LAB_01005be8;
        }
      } while ((int *)(iVar11 + AL_I2C_TAR_10BIT_ADDR_SHIFT) != piVar10);
    }
    iVar5 = FUN_01007e64(*unaff_r10,iVar1,s_inv_tx_lanes_01031d2c,&stack0x00000044);
    if ((iVar5 != 0) && (in_stack_00000044 = in_stack_00000044 >> 2, in_stack_00000044 != 0)) {
      puVar6 = (uint *)(iVar5 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        iVar5 = iVar14 + 0x20 + uVar12 * 4;
        if (3 < uVar12) goto LAB_01005cb8;
        if (*(int *)(unaff_r11 + iVar5) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_tx_lane_defined_inverted_multipl_01031d64,uVar12);
          FUN_01010bfc(s_tx_lane_defined_inverted_multipl_01031d64,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(unaff_r11 + iVar5) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar1 = FUN_01007e64(*unaff_r10,iVar1,s_inv_rx_lanes_01031d3c,&stack0x00000044);
    if ((iVar1 != 0) && (in_stack_00000044 = in_stack_00000044 >> 2, in_stack_00000044 != 0)) {
      puVar6 = (uint *)(iVar1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
      uVar9 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar12 = *puVar6;
        uVar9 = uVar9 + 1;
        uVar12 = uVar12 << 0x18 | (uVar12 >> 8 & 0xff) << 0x10 | (uVar12 >> 0x10 & 0xff) << 8 |
                 uVar12 >> 0x18;
        iVar1 = iVar14 + 0x30 + uVar12 * 4;
        if (3 < uVar12) goto LAB_01005cb8;
        if (*(int *)(unaff_r11 + iVar1) == 1) {
          iVar1 = -0x16;
          FUN_0100ceac(s_rx_lane_defined_inverted_multipl_01031d94,uVar12);
          FUN_01010bfc(s_rx_lane_defined_inverted_multipl_01031d94,uVar12);
          goto LAB_01005be8;
        }
        *(undefined4 *)(unaff_r11 + iVar1) = 1;
      } while (in_stack_00000044 != uVar9);
    }
    iVar1 = param_6 * 0x10;
    piVar10 = (int *)(&stack0x00000068 + iVar1);
    puVar15 = (undefined4 *)(&stack0x00000248 + param_6 * 0x40);
    do {
      iVar14 = *piVar10;
      if (((-1 < iVar14) &&
          (iVar5 = FUN_01007eb8(*unaff_r10,iVar14,s_override_01031dc4,0), iVar5 != 0)) &&
         (iVar5 = FUN_01012954(iVar5,s_enabled_01030630), iVar5 == 0)) {
        uVar7 = *unaff_r10;
        *puVar15 = 1;
        iVar5 = FUN_01007e64(uVar7,iVar14,s_dcgain_01031dd0,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 1) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_3db_freq_01031dd8,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 5) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_gain_01031de8,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 6) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_1st_tap_ctrl_01031df4,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 7) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_2nd_tap_ctrl_01031e08,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 2) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_3rd_tap_ctrl_01031e1c,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 9) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_dfe_4th_tap_ctrl_01031e30,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 10) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_low_freq_agc_gain_01031e44,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 0xb) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar14 = FUN_01007e64(*unaff_r10,iVar14,s_high_freq_agc_boost_01031e58,&stack0x00000048);
        if ((iVar14 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 0xd) = (char)((uint)*(undefined4 *)(iVar14 + 0xc) >> 0x18);
        }
      }
      piVar10 = piVar10 + 1;
      puVar15 = puVar15 + 4;
    } while (piVar10 != (int *)(&stack0x00000078 + iVar1));
    puVar15 = (undefined4 *)(&stack0x00000158 + param_6 * 0x30);
    piVar10 = (int *)(&stack0x000000b8 + iVar1);
    do {
      iVar14 = *piVar10;
      if (((-1 < iVar14) &&
          (iVar5 = FUN_01007eb8(*unaff_r10,iVar14,s_override_01031dc4,0), iVar5 != 0)) &&
         (iVar5 = FUN_01012954(iVar5,s_enabled_01030630), iVar5 == 0)) {
        uVar7 = *unaff_r10;
        *puVar15 = 1;
        iVar5 = FUN_01007e64(uVar7,iVar14,&DAT_01031e6c,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 1) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_total_driver_units_01031e70,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 5) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_post_emph_01031e84,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 6) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar5 = FUN_01007e64(*unaff_r10,iVar14,s_pre_emph_01031e90,&stack0x00000048);
        if ((iVar5 != 0) && (in_stack_00000048 != 0)) {
          *(char *)(puVar15 + 2) = (char)((uint)*(undefined4 *)(iVar5 + 0xc) >> 0x18);
        }
        iVar14 = FUN_01007e64(*unaff_r10,iVar14,s_slew_rate_01031e9c,&stack0x00000048);
        if ((iVar14 != 0) && (in_stack_00000048 != 0)) {
          *(char *)((int)puVar15 + 9) = (char)((uint)*(undefined4 *)(iVar14 + 0xc) >> 0x18);
        }
      }
      piVar10 = piVar10 + 1;
      puVar15 = puVar15 + 3;
    } while ((int *)(&stack0x000000c8 + iVar1) != piVar10);
    iVar14 = 0;
    iVar1 = param_11 * 0x10;
    (&stack0x0000004c)[param_11] = 0;
    puVar15 = (undefined4 *)(&stack0x00000510 + param_11 * 0xf0);
    while ((puVar15[1] != 1 || (iVar5 = FUN_01012954(uVar2,*puVar15), iVar5 != 0))) {
      uVar9 = param_11;
      iVar14 = iVar14 + 1;
      puVar15 = puVar15 + 4;
      if (iVar14 == 0xf) {
        iVar1 = -0x16;
        FUN_0100ceac(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,param_11);
        FUN_01010bfc(s__s__s___d___not_supported_interf_01032190,
                     s_dt_based_init_serdes_group_01031bc8,uVar2,uVar9);
        goto LAB_01005be8;
      }
    }
    if ((*(int *)(&stack0x0000051c + ((iVar1 - param_11) + iVar14) * 0x10) == -1) ||
       (uVar17 = 1,
       *(int *)(DAT_0104a3a8 +
                *(int *)(&stack0x0000051c + ((iVar1 - param_11) + iVar14) * 0x10) * 0x14 + 0xc) == 0
       )) {
      uVar17 = (&stack0x00000518)[((iVar1 - param_11) + iVar14) * 0x10];
    }
    iVar1 = param_11 * 0x4c;
    (&stack0x00000388)[iVar1] = uVar17;
    iVar14 = FUN_01012954(uVar3,s_100Mhz_internal_01031ea8);
    if (iVar14 == 0) {
      iVar14 = 2;
      (&stack0x0000004c)[param_11] = 2;
    }
    else {
      iVar14 = FUN_01012954(uVar3,s_100Mhz_01031eb8);
      if (iVar14 == 0) {
        *(undefined2 *)(&stack0x0000039e + iVar1) = 0x101;
        iVar14 = 0;
      }
      else {
        iVar14 = FUN_01012954(uVar3,s_125Mhz_01031ec0);
        if (iVar14 == 0) {
          *(undefined2 *)(&stack0x0000039e + iVar1) = 0x201;
          iVar14 = 0;
        }
        else {
          iVar14 = FUN_01012954(uVar3,s_156_25Mhz_01031ec8);
          if (iVar14 == 0) {
            *(undefined2 *)(&stack0x0000039e + iVar1) = 0x301;
            iVar14 = 0;
          }
          else {
            iVar14 = FUN_01012954(uVar3,&DAT_01031ed4);
            if (iVar14 == 0) {
              iVar14 = 1;
              (&stack0x0000039d)[iVar1] = 4;
              (&stack0x0000039e)[iVar1] = 4;
              (&stack0x0000004c)[param_11] = 1;
            }
            else {
              iVar14 = FUN_01012954(uVar3,s_right_01031edc);
              if (iVar14 != 0) {
                iVar1 = -0x16;
                FUN_0100ceac(s_Not_supported_reference_clock____01031ee4,uVar3);
                FUN_01010bfc(s_Not_supported_reference_clock____01031ee4,uVar3);
                goto LAB_01005be8;
              }
              iVar14 = 2;
              (&stack0x0000039c)[iVar1] = 2;
              (&stack0x0000039e)[iVar1] = 2;
              (&stack0x0000004c)[param_11] = 2;
            }
          }
        }
      }
    }
    if (param_15 != 0) {
      iVar1 = param_11 * 0x4c;
      (&stack0x0000039d)[iVar1] = (&stack0x0000039e)[iVar1];
      (&stack0x000003a1)[iVar1] = (&stack0x0000039f)[iVar1];
    }
    uVar9 = (uint)(byte)(&stack0x00000388)[param_11 * 0x4c];
    bVar19 = 0x14 < uVar9;
    if (uVar9 != 0x15) {
      bVar19 = 1 < uVar9 - 0x10;
    }
    if (!bVar19 || (uVar9 == 0x15 || uVar9 - 0x10 == 2)) {
      iVar1 = 0;
      piVar10 = DAT_0100690c;
      do {
        if (((*piVar10 != 0) && (piVar10[4] == param_11)) &&
           (*(int *)(iVar11 + piVar10[8] * 4) == 1)) {
          FUN_01003f30(iVar1);
        }
        iVar1 = iVar1 + 1;
        piVar10 = piVar10 + 1;
      } while (iVar1 != 4);
    }
    iVar1 = param_11 * 0x4c;
    cVar8 = (&stack0x00000388)[iVar1];
    if (cVar8 == '\x13') {
      if ((DAT_0104a56c != 0) && (DAT_0104a57c == param_11)) {
        FUN_01003f30(0,1);
        cVar8 = (&stack0x00000388)[iVar1];
        goto LAB_01006384;
      }
    }
    else {
LAB_01006384:
      if (cVar8 == '\x14') {
        if ((DAT_0104a574 == 0) || (DAT_0104a584 != param_11)) goto LAB_010063b0;
        FUN_01003f30(2,1);
        cVar8 = (&stack0x00000388)[param_11 * 0x4c];
      }
      if (cVar8 == '\x0e') {
        FUN_01003f30(AL_I2C_INTR_MASK_ACTIVITY_SHIFT,1);
        cVar8 = (&stack0x00000388)[param_11 * 0x4c];
      }
      if (cVar8 == '\x0f') {
        FUN_01003f30(AL_I2C_INTR_MASK_STOP_DET_SHIFT,1);
      }
    }
LAB_010063b0:
    iVar1 = FUN_01012954(uVar4,s_enabled_01030630);
    if (iVar1 == 0) {
      *(undefined4 *)(&stack0x000003a4 + param_11 * 0x4c) = 1;
    }
    if (iVar14 == 2) {
      if ((&stack0x0000004c)[param_13 & 0xff] != '\x02') {
        iVar1 = (param_13 & 0xff) * 0x4c;
        (&stack0x0000039c)[iVar1] = 1;
        (&stack0x000003a0)[iVar1] = (&stack0x0000039f)[iVar1];
      }
LAB_010063f4:
      param_15 = 0;
    }
    else {
      if (iVar14 != 1) goto LAB_010063f4;
      param_15 = 1;
    }
    if (param_10 == unaff_r7) {
      if ((DAT_0104a608 == 0) && ((in_stack_00000388 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_A_off_violation__01031f0c);
        FUN_01010bfc(s_group_A_off_violation__01031f0c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a60c == 0) && ((in_stack_000003d4 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_B_off_violation__01031f24);
        FUN_01010bfc(s_group_B_off_violation__01031f24);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a610 == 0) && ((in_stack_00000420 & 0xfd) != 0)) {
        FUN_0100ceac(s_group_C_off_violation__01031f3c);
        FUN_01010bfc(s_group_C_off_violation__01031f3c);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a614 == 0) && ((in_stack_0000046c & 0xfd) != 0)) {
        FUN_0100ceac(s_group_D_off_violation__01031f54);
        FUN_01010bfc(s_group_D_off_violation__01031f54);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a618 == 0) && (2 < in_stack_000004b8)) {
        FUN_0100ceac(s_group_E_off_violation__01031f6c);
        FUN_01010bfc(s_group_E_off_violation__01031f6c);
        DAT_0104a568 = 1;
      }
      if ((in_stack_000003d4 == 0xe) &&
         ((((DAT_0104a5c8 == 0 || ((in_stack_000003d8 != 0 && ((DAT_0104a5cc & 1) == 0)))) ||
           ((in_stack_000003dc != 0 && ((DAT_0104a5cc & 2) == 0)))) ||
          (((in_stack_000003e0 != 0 && ((DAT_0104a5cc & 4) == 0)) ||
           ((in_stack_000003e4 != 0 && ((DAT_0104a5cc & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) == 0)))))))
         ) {
        FUN_0100ceac(s_SATA_0_violation__01031f84);
        FUN_01010bfc(s_SATA_0_violation__01031f84);
        DAT_0104a568 = 1;
      }
      if (in_stack_00000420 == 0xf) {
        if ((((DAT_0104a5d0 == 0) || ((in_stack_00000424 != 0 && ((DAT_0104a5d4 & 1) == 0)))) ||
            ((in_stack_00000428 != 0 && ((DAT_0104a5d4 & 2) == 0)))) ||
           (((in_stack_0000042c != 0 && ((DAT_0104a5d4 & 4) == 0)) ||
            ((in_stack_00000430 != 0 && ((DAT_0104a5d4 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) == 0))))))
        {
          FUN_0100ceac(s_SATA_1_violation__01031f98);
          FUN_01010bfc(s_SATA_1_violation__01031f98);
          DAT_0104a568 = 1;
          goto LAB_01006584;
        }
LAB_01006b0c:
        if ((in_stack_0000046c - 0x10 < 3) &&
           (((((in_stack_00000470 != 0 && (DAT_0104a5e4 == 0)) ||
              ((in_stack_00000474 != 0 && (DAT_0104a5e0 == 0)))) ||
             ((in_stack_00000478 != 0 && (DAT_0104a5dc == 0)))) ||
            ((in_stack_0000047c != 0 && (DAT_0104a5d8 == 0)))))) goto LAB_010065f0;
      }
      else {
LAB_01006584:
        if ((2 < in_stack_00000420 - 0x10) ||
           (((((in_stack_00000424 == 0 || (DAT_0104a5e4 != 0)) &&
              ((in_stack_00000428 == 0 || (DAT_0104a5e0 != 0)))) &&
             ((in_stack_0000042c == 0 || (DAT_0104a5dc != 0)))) &&
            ((in_stack_00000430 == 0 || (DAT_0104a5d8 != 0)))))) goto LAB_01006b0c;
LAB_010065f0:
        FUN_0100ceac(s_ETH_violation__01031fac);
        FUN_01010bfc(s_ETH_violation__01031fac);
        DAT_0104a568 = 1;
      }
      uVar9 = (uint)in_stack_00000388;
      if (*(int *)(DAT_0104a3a8 + 0xc) == 0) {
        if (uVar9 == 5) {
          uVar12 = 2;
LAB_01006650:
          if (in_stack_00000390 != 0) {
            if (in_stack_00000394 == 0) {
              in_stack_00000390 = 1;
            }
            else if (in_stack_00000398 == 0) {
              in_stack_00000390 = 1;
            }
            else {
              in_stack_00000390 = 2;
            }
          }
        }
        else {
          uVar12 = (uint)(uVar9 - 3 < 2);
          if (uVar9 == 3) goto LAB_01006bbc;
LAB_01006644:
          if (uVar9 != 6) goto LAB_01006650;
          in_stack_00000390 = 3;
        }
LAB_0100667c:
        if (DAT_0104a5e8 < uVar12) {
          FUN_0100ceac(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar12);
          FUN_01010bfc(s_PCIe_0_speed_violation___d_>__d__01031fbc,uVar12,DAT_0104a5e8);
          DAT_0104a568 = 1;
        }
        if ((uVar12 != 0) && (DAT_0104a5ec < in_stack_00000390)) {
          FUN_0100ceac(s_PCIe_0_width_violation___d_>__d__01031fe0,in_stack_00000390);
          FUN_01010bfc(s_PCIe_0_width_violation___d_>__d__01031fe0,in_stack_00000390,DAT_0104a5ec);
          DAT_0104a568 = 1;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (in_stack_000003d4 == AL_I2C_INTR_MASK_ACTIVITY_SHIFT) {
            uVar9 = 2;
          }
          else if (in_stack_000003d4 == 7) {
            uVar9 = 1;
          }
          else {
LAB_010066fc:
            uVar9 = (uint)(in_stack_00000388 == 3);
          }
        }
        else {
          uVar9 = 3;
        }
        if (in_stack_00000388 == 3) goto LAB_01006e2c;
        uVar12 = 0;
        if (in_stack_000003dc != 0) {
          if (in_stack_000003e0 == 0) {
            uVar12 = 1;
          }
          else if (in_stack_000003e4 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_0100673c:
        if (DAT_0104a5f0 < uVar9) {
          FUN_0100ceac(s_PCIe_1_speed_violation___d_>__d__01032004,uVar9);
          FUN_01010bfc(s_PCIe_1_speed_violation___d_>__d__01032004,uVar9,DAT_0104a5f0);
          DAT_0104a568 = 1;
        }
        if ((uVar9 != 0) && (DAT_0104a5f4 < uVar12)) {
          FUN_0100ceac(s_PCIe_1_width_violation___d_>__d__01032028,uVar12);
          FUN_01010bfc(s_PCIe_1_width_violation___d_>__d__01032028,uVar12,DAT_0104a5f4);
          DAT_0104a568 = 1;
        }
      }
      else {
        uVar12 = 3;
        if (uVar9 != 3) goto LAB_01006644;
LAB_01006bbc:
        if (in_stack_0000038c != 0) {
          if (in_stack_00000390 != 0) {
            in_stack_00000390 = 1;
          }
          goto LAB_0100667c;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (in_stack_000003d4 == AL_I2C_INTR_MASK_ACTIVITY_SHIFT) {
            uVar9 = 2;
            goto LAB_01006e2c;
          }
          if (in_stack_000003d4 == 7) {
            uVar9 = 1;
            goto LAB_01006e2c;
          }
          goto LAB_010066fc;
        }
LAB_01006e2c:
        if (in_stack_00000394 != 0) {
          uVar12 = (uint)(in_stack_00000398 != 0);
          goto LAB_0100673c;
        }
      }
      uVar9 = (uint)in_stack_00000420;
      if (*(int *)(DAT_0104a3a8 + 0x34) == 0) {
        if (uVar9 == AL_I2C_INTR_MASK_GEN_CALL_SHIFT) {
          uVar12 = 2;
        }
        else {
          uVar12 = (uint)(uVar9 + -AL_I2C_INTR_MASK_STOP_DET_SHIFT < 2);
          if (uVar9 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) goto LAB_01006b90;
        }
LAB_010067c0:
        if (in_stack_0000042c != 0) {
          if (in_stack_00000428 == 0) {
            in_stack_0000042c = 1;
          }
          else if (in_stack_00000424 == 0) {
            in_stack_0000042c = 1;
          }
          else {
            in_stack_0000042c = 2;
          }
        }
LAB_010067ec:
        if (DAT_0104a5f8 < uVar12) {
          FUN_0100ceac(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar12);
          FUN_01010bfc(s_PCIe_2_speed_violation___d_>__d__0103204c,uVar12,DAT_0104a5f8);
          DAT_0104a568 = 1;
        }
        if ((uVar12 != 0) && (DAT_0104a5fc < in_stack_0000042c)) {
          FUN_0100ceac(s_PCIe_2_width_violation___d_>__d__01032070,in_stack_0000042c);
          FUN_01010bfc(s_PCIe_2_width_violation___d_>__d__01032070,in_stack_0000042c,DAT_0104a5fc);
          DAT_0104a568 = 1;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (in_stack_0000046c == 0xd) {
            uVar9 = 2;
          }
          else if (in_stack_0000046c == AL_I2C_TAR_10BIT_ADDR_SHIFT) {
            uVar9 = 1;
          }
          else {
LAB_0100686c:
            uVar9 = (uint)(in_stack_00000420 == AL_I2C_INTR_MASK_STOP_DET_SHIFT);
          }
        }
        else {
          uVar9 = 3;
        }
        if (in_stack_00000420 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) goto LAB_01006bf4;
        uVar12 = 0;
        if (in_stack_00000478 != 0) {
          if (in_stack_00000474 == 0) {
            uVar12 = 1;
          }
          else if (in_stack_00000470 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
        if (uVar9 <= DAT_0104a600) goto LAB_010068b8;
LAB_01006c18:
        FUN_0100ceac(s_PCIe_3_speed_violation___d_>__d__01032094,uVar9);
        FUN_01010bfc(s_PCIe_3_speed_violation___d_>__d__01032094,uVar9,DAT_0104a600);
        DAT_0104a568 = 1;
        if ((uVar9 != 0) && (DAT_0104a604 < uVar12)) goto LAB_01006c58;
LAB_01006c84:
        FUN_01003f30(0);
        FUN_01003f30(2,0);
        FUN_01003f30(3,0);
        FUN_01003f30(6,0);
        FUN_01003f30(7,0);
        FUN_01003f30(AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0);
        FUN_01003f30(AL_I2C_INTR_MASK_STOP_DET_SHIFT,0);
        in_stack_00000388 = 0;
      }
      else {
        uVar12 = 3;
        if (uVar9 != AL_I2C_INTR_MASK_STOP_DET_SHIFT) goto LAB_010067c0;
LAB_01006b90:
        if (in_stack_00000430 != 0) {
          if (in_stack_0000042c != 0) {
            in_stack_0000042c = 1;
          }
          goto LAB_010067ec;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (in_stack_0000046c == 0xd) {
            uVar9 = 2;
            goto LAB_01006bf4;
          }
          if (in_stack_0000046c == AL_I2C_TAR_10BIT_ADDR_SHIFT) {
            uVar9 = 1;
            goto LAB_01006bf4;
          }
          goto LAB_0100686c;
        }
        uVar9 = 3;
LAB_01006bf4:
        if (in_stack_00000428 != 0) {
          uVar12 = (uint)(in_stack_00000424 != 0);
          if (DAT_0104a600 < uVar9) goto LAB_01006c18;
LAB_010068b8:
          if ((uVar9 != 0) && (DAT_0104a604 < uVar12)) {
LAB_01006c58:
            FUN_0100ceac(s_PCIe_3_width_violation___d_>__d__010320b8,uVar12);
            FUN_01010bfc(s_PCIe_3_width_violation___d_>__d__010320b8,uVar12,DAT_0104a604);
            DAT_0104a568 = 1;
            goto LAB_01006c84;
          }
        }
        if (DAT_0104a568 != 0) goto LAB_01006c84;
      }
      uVar2 = DAT_01006910;
      iVar1 = 1000;
      FUN_01024b8c(DAT_01006910,0xfd860a00,0xfd8a8000);
      al_thermal_sensor_enable_set(uVar2,1);
      goto LAB_01006928;
    }
    param_6 = unaff_r7[1];
    param_13 = *unaff_r7;
    unaff_r5 = param_6 & 0xff;
    param_3 = 0x4b0;
    unaff_r7 = unaff_r7 + 1;
  }
  iVar1 = -0x16;
  goto LAB_01005be8;
LAB_01005cb8:
  iVar1 = -0x16;
  FUN_0100ceac(s_invalid_lane___d___01031d4c,uVar12);
  FUN_01010bfc(s_invalid_lane___d___01031d4c,uVar12);
  goto LAB_01005be8;
  while( true ) {
    udelay(1);
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) break;
LAB_01006928:
    iVar11 = al_thermal_sensor_is_ready(uVar2);
    if (iVar11 != 0) {
      al_thermal_sensor_trigger_continuous(DAT_01006910);
      goto LAB_01006944;
    }
  }
  FUN_0100ceac(s_Thermal_sensor_failed_to_power_u_010320dc);
  FUN_01010bfc(s_Thermal_sensor_failed_to_power_u_010320dc);
LAB_01006944:
  iVar11 = DAT_01006918;
  uVar9 = 0;
  iVar1 = DAT_01006918;
  do {
    iVar14 = FUN_0102614c(uVar9 * 0x400 + -0x2740000,uVar9 & 0xff,iVar1);
    uVar9 = uVar9 + 1;
    if (iVar14 != 0) goto LAB_01006e50;
    FUN_01026178(iVar1,&DAT_010057e0);
    iVar1 = iVar1 + 0xfc;
  } while (uVar9 != 4);
  iVar1 = FUN_0102614c(0xfd8c2000,4,DAT_01006914);
  if (iVar1 == 0) {
    FUN_01026178(DAT_01006914,&DAT_010057e0);
    if ((DAT_01049b20 != 0) && (2 < in_stack_000004b8)) {
      FUN_0100ceac(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      FUN_01010bfc(s_Loading_SerDes_25G_FW_to__08X____01032130,0x1120000,DAT_01049afc);
      iVar1 = al_flash_obj_data_load(DAT_01049d8c,DAT_01049b1c,0x1120000);
      if (iVar1 != 0) {
        FUN_0100ceac(s_al_flash_obj_data_load_failed__s_01032160);
        FUN_01010bfc(s_al_flash_obj_data_load_failed__s_01032160);
      }
    }
    iVar1 = al_serdes_init_cores(DAT_01006918,0xfd8a8000);
    if (iVar1 == 0) {
      puVar16 = &stack0x00000248;
      iVar14 = 0;
      piVar10 = (int *)&stack0x0000038c;
      while( true ) {
        if (in_stack_00000388 != 0) {
          piVar13 = (int *)(&stack0x00000158 + iVar14);
          uVar9 = 0;
          piVar18 = piVar10;
          do {
            if (*piVar18 != 0) {
              if (*(int *)(puVar16 + uVar9 * 0x10) == 1) {
                (**(code **)(iVar11 + 0xa4))(iVar11,uVar9 & 0xff,puVar16 + uVar9 * 0x10);
              }
              if (*piVar13 == 1) {
                (**(code **)(iVar11 + 0x9c))(iVar11,uVar9 & 0xff,piVar13);
              }
            }
            uVar9 = uVar9 + 1;
            piVar13 = piVar13 + 3;
            piVar18 = piVar18 + 1;
          } while (uVar9 != 4);
        }
        iVar14 = iVar14 + 0x30;
        iVar11 = iVar11 + 0xfc;
        puVar16 = puVar16 + 0x40;
        if (iVar14 == 0xf0) break;
        in_stack_00000388 = *(byte *)(piVar10 + 0x12);
        piVar10 = piVar10 + 0x13;
      }
    }
  }
  else {
LAB_01006e50:
    iVar1 = -1;
    FUN_0100ceac(s_al_serdes_handle_init_failed__01032110);
    FUN_01010bfc(s_al_serdes_handle_init_failed__01032110);
  }
LAB_01005be8:
                    /* WARNING: Could not recover jumptable at 0x01005c08. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(iVar1);
  return;
}



/* @ 0x1007270  FUN_01007270 */

undefined4 FUN_01007270(int param_1,int param_2,int param_3)

{
  int *piVar1;
  
  if (param_1 < 0x11) {
    piVar1 = (int *)&DAT_f0200f00;
    if (param_2 != 0) {
      piVar1 = (int *)&DAT_f0009f00;
    }
    *piVar1 = param_1 + param_3 * 0x10000;
    return 0;
  }
  return 0xffffffea;
}



/* @ 0x10072a8  FUN_010072a8 */

uint FUN_010072a8(void)

{
  uint uVar1;
  
  if (DAT_01049b30 == '\0') {
    uVar1 = FUN_01000280();
    return uVar1;
  }
  if (DAT_01049b30 == '\x01') {
    return *(uint *)(DAT_010492dc + 0x1008);
  }
  uVar1 = al_timer_value_get(&DAT_01049b38);
  return ~uVar1;
}



/* @ 0x1007318  FUN_01007318 */

ulonglong FUN_01007318(void)

{
  uint uVar1;
  ulonglong uVar2;
  
  if (DAT_01049b30 == '\0') {
    uVar2 = FUN_01000280();
    return uVar2;
  }
  if (DAT_01049b30 != '\x01') {
    uVar1 = al_timer_value_get(&DAT_01049b38);
    return (ulonglong)~uVar1;
  }
  do {
  } while (*(int *)(DAT_010492dc + 0x100c) != *(int *)(DAT_010492dc + 0x100c));
  return *(ulonglong *)(DAT_010492dc + 0x1008);
}



/* @ 0x10073a8  FUN_010073a8 */

void time_init(int param_1,undefined4 param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  
  if (DAT_01049b2c != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___ref_initialized____0__010322d4,1,
                 s_services_basic_src_time_c_010322b8,s_time_init_010322a4,0xe3);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___ref_initialized____0__010322d4,1,
                 s_services_basic_src_time_c_010322b8,s_time_init_010322a4,0xe3);
    FUN_01000454(0);
  }
  DAT_01049b28 = param_3 >> 4;
  DAT_01049b30 = (undefined1)param_1;
  DAT_01049b2c = 1;
  DAT_01049b24 = param_2;
  DAT_01049b34 = param_4;
  if (param_1 == 2) {
    iVar1 = al_timer_init(&DAT_01049b38,0xfd890000,0);
    if (iVar1 != 0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                   s_al_timer_init__sb_timer___void___010322ec,iVar1,
                   s_services_basic_src_time_c_010322b8,s_sb_timer_init_01032294,0x7e);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                   s_al_timer_init__sb_timer___void___010322ec,iVar1,
                   s_services_basic_src_time_c_010322b8,s_sb_timer_init_01032294,0x7e);
      FUN_01000454(0);
    }
    al_timer_enable(&DAT_01049b38,0);
    al_timer_config_set(&DAT_01049b38,1,0);
    al_timer_load_set(&DAT_01049b38,0xffffffff);
    al_timer_int_enable(&DAT_01049b38,0);
    al_timer_enable(&DAT_01049b38,1);
  }
  DAT_01049b2c = 1;
  return;
}



/* @ 0x100758c  FUN_0100758c */

undefined4 udelay(uint param_1)

{
  uint uVar1;
  uint *puVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  
  if (DAT_01049b2c != 1) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___ref_initialized____1__01032328,1,
                 s_services_basic_src_time_c_010322b8,s_udelay_010322b0,0xfb);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___ref_initialized____1__01032328,1,
                 s_services_basic_src_time_c_010322b8,s_udelay_010322b0,0xfb);
    FUN_01000454(0);
  }
  if (DAT_01049b30 == '\x02') {
    puVar2 = &DAT_01049b34;
  }
  else {
    puVar2 = &DAT_01049b28;
  }
  uVar3 = *puVar2;
  if (param_1 != 0) {
    do {
      uVar5 = param_1;
      if (9999 < param_1) {
        uVar5 = 10000;
      }
      uVar1 = FUN_010072a8();
      uVar6 = uVar5 * (uVar3 / 1000);
      uVar4 = uVar6 / 1000;
      if (999 < uVar6) {
        do {
          uVar6 = FUN_010072a8();
          if (uVar6 < uVar1) {
            uVar4 = (uVar4 + 1 + uVar1) - uVar6;
          }
          else {
            uVar4 = (uVar4 + uVar1) - uVar6;
          }
          uVar1 = uVar6;
        } while (0 < (int)uVar4);
      }
      param_1 = param_1 - uVar5;
    } while (param_1 != 0);
  }
  return 0;
}



/* @ 0x10076f8  FUN_010076f8 */

void FUN_010076f8(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined8 uVar5;
  
  uVar5 = FUN_01007318();
  iVar2 = (int)((ulonglong)uVar5 >> 0x20);
  uVar1 = (uint)uVar5;
  iVar3 = (((iVar2 << 5 | uVar1 >> 0x1b) - (iVar2 + (uint)(uVar1 * 0x20 < uVar1))) * 0x200 |
          uVar1 * 0x1f >> 0x17) + iVar2 + (uint)CARRY4(uVar1 * 0x3e00,uVar1);
  if (DAT_01049b30 == '\x02') {
    puVar4 = &DAT_01049b34;
  }
  else {
    puVar4 = &DAT_01049b28;
  }
  FUN_01028d2c(uVar1 * 1000000,
               iVar2 + ((iVar3 * 0x40 | uVar1 * 0x3e01 >> 0x1a) -
                       (iVar3 + (uint)(uVar1 * 0xf8040 < uVar1 * 0x3e01))) +
               (uint)CARRY4(uVar1,uVar1 * 999999),*puVar4,0);
  return;
}



/* @ 0x10077a8  FUN_010077a8 */

void FUN_010077a8(uint param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_01000084();
  uart_write_byte(uVar1,param_1 & 0xff);
  if (param_1 != AL_I2C_INTR_MASK_START_DET_SHIFT) {
    return;
  }
  uart_write_byte(uVar1,0xd);
  return;
}



/* @ 0x10077fc  FUN_010077fc */

void FUN_010077fc(void)

{
  return;
}



/* @ 0x1007800  FUN_01007800 */

undefined4 FUN_01007800(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  
  uVar2 = *param_1;
  uVar2 = uVar2 << 0x18 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 | uVar2 >> 0x18;
  if (uVar2 != 0xd00dfeed) {
    if (uVar2 != 0x2ff20112) {
      return ~AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
    if (param_1[9] == 0) {
      uVar1 = 0xfffffff9;
    }
    else {
      uVar1 = 0;
    }
    return uVar1;
  }
  uVar2 = param_1[5];
  if ((uVar2 << 0x18 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 | uVar2 >> 0x18) <
      0x10) {
    return ~AL_I2C_INTR_MASK_STOP_DET_SHIFT;
  }
  uVar2 = param_1[6];
  if ((uVar2 << 0x18 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 | uVar2 >> 0x18) <
      0x12) {
    uVar1 = 0;
  }
  else {
    uVar1 = ~AL_I2C_INTR_MASK_STOP_DET_SHIFT;
  }
  return uVar1;
}



/* @ 0x1007874  FUN_01007874 */

int FUN_01007874(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  
  uVar1 = *(uint *)(param_1 + 0x14);
  if (0x10 < (uVar1 << 0x18 | (uVar1 >> 8 & 0xff) << 0x10 | (uVar1 >> 0x10 & 0xff) << 8 |
             uVar1 >> 0x18)) {
    if (CARRY4(param_2,param_3)) {
      return 0;
    }
    uVar1 = *(uint *)(param_1 + 0x24);
    if ((uVar1 << 0x18 | (uVar1 >> 8 & 0xff) << 0x10 | (uVar1 >> 0x10 & 0xff) << 8 | uVar1 >> 0x18)
        < param_2 + param_3) {
      return 0;
    }
  }
  uVar1 = *(uint *)(param_1 + 8);
  if ((int)param_3 < 0) {
    param_1 = 0;
  }
  else {
    param_1 = param_1 + param_2 + (uVar1 << 0x18 | (uVar1 >> 8 & 0xff) << 0x10 |
                                   (uVar1 >> 0x10 & 0xff) << 8 | uVar1 >> 0x18);
  }
  return param_1;
}



/* @ 0x10078d0  FUN_010078d0 */

uint FUN_010078d0(undefined4 param_1,int param_2,uint *param_3)

{
  uint *puVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  
  *param_3 = 0xfffffff8;
  puVar1 = (uint *)FUN_01007874(param_1,param_2,4);
  if (puVar1 != (uint *)0x0) {
    uVar6 = *puVar1;
    iVar4 = param_2 + 4;
    *param_3 = ~AL_I2C_INTR_MASK_START_DET_SHIFT;
    uVar6 = uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
            uVar6 >> 0x18;
    iVar3 = iVar4;
    switch(uVar6) {
    case 1:
      do {
        iVar4 = iVar3 + 1;
        pcVar2 = (char *)FUN_01007874(param_1,iVar3);
        if (pcVar2 == (char *)0x0) {
          return AL_I2C_INTR_MASK_STOP_DET_SHIFT;
        }
        iVar3 = iVar4;
      } while (*pcVar2 != '\0');
      break;
    case 2:
    case 4:
    case 9:
      break;
    case 3:
      puVar1 = (uint *)FUN_01007874(param_1,iVar4,4);
      if (puVar1 == (uint *)0x0) {
        return AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      }
      uVar5 = *puVar1;
      iVar4 = param_2 + AL_I2C_TAR_10BIT_ADDR_SHIFT +
              (uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 |
              uVar5 >> 0x18);
      break;
    default:
      goto LAB_010079c8;
    }
    iVar3 = FUN_01007874(param_1,param_2,iVar4 - param_2);
    if (iVar3 != 0) {
      *param_3 = iVar4 + 3U & 0xfffffffc;
      return uVar6;
    }
  }
LAB_010079c8:
  return AL_I2C_INTR_MASK_STOP_DET_SHIFT;
}



/* @ 0x10079e0  FUN_010079e0 */

uint FUN_010079e0(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_c [2];
  
  if ((((int)param_2 < 0) || ((param_2 & 3) != 0)) ||
     (local_c[0] = param_2, iVar1 = FUN_010078d0(param_1,param_2,local_c), iVar1 != 1)) {
    local_c[0] = 0xfffffffc;
  }
  return local_c[0];
}



/* @ 0x1007a20  FUN_01007a20 */

uint FUN_01007a20(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_c [2];
  
  if ((((int)param_2 < 0) || ((param_2 & 3) != 0)) ||
     (local_c[0] = param_2, iVar1 = FUN_010078d0(param_1,param_2,local_c), iVar1 != 3)) {
    local_c[0] = 0xfffffffc;
  }
  return local_c[0];
}



/* @ 0x1007a60  FUN_01007a60 */

int FUN_01007a60(undefined4 param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int local_14;
  
  local_14 = 0;
  if ((param_2 < 0) || (iVar2 = FUN_010079e0(), local_14 = iVar2, -1 < iVar2)) {
    do {
      while (iVar2 = local_14, iVar1 = FUN_010078d0(param_1,local_14,&local_14), iVar1 == 2) {
        if ((param_3 != (int *)0x0) && (iVar2 = *param_3, *param_3 = iVar2 + -1, iVar2 + -1 < 0)) {
          return local_14;
        }
      }
      if (iVar1 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) {
        if (-1 < local_14) {
          return -1;
        }
        if (param_3 != (int *)0x0 || local_14 != -AL_I2C_INTR_MASK_ACTIVITY_SHIFT) {
          return local_14;
        }
        return -1;
      }
    } while (iVar1 != 1);
    if (param_3 != (int *)0x0) {
      *param_3 = *param_3 + 1;
    }
  }
  return iVar2;
}



/* @ 0x1007bbc  FUN_01007bbc */

int FUN_01007bbc(undefined4 param_1,int param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int local_24 [2];
  
  iVar1 = FUN_01007800();
  if ((iVar1 == 0) && (iVar1 = param_2, -1 < param_2)) {
    local_24[0] = 0;
    while (param_2 = FUN_01007a60(param_1,param_2,local_24), -1 < param_2 && -1 < local_24[0]) {
      if (((local_24[0] == 1) && (iVar1 = FUN_01007874(param_1,param_2 + 4,param_4 + 1), iVar1 != 0)
          ) && (iVar2 = FUN_01012b04(iVar1,param_3,param_4), iVar2 == 0)) {
        if (*(char *)(iVar1 + param_4) == '\0') {
          return param_2;
        }
        iVar2 = FUN_01012b3c(param_3,0x40,param_4);
        if ((iVar2 == 0) && (*(char *)(iVar1 + param_4) == '@')) {
          return param_2;
        }
      }
    }
    iVar1 = param_2;
    if (local_24[0] < 0) {
      iVar1 = -1;
    }
  }
  return iVar1;
}



/* @ 0x1007cb0  FUN_01007cb0 */

int FUN_01007cb0(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int local_14 [3];
  
  iVar2 = FUN_010079e0();
  if (iVar2 < 0) {
    return iVar2;
  }
  while( true ) {
    iVar1 = FUN_010078d0(param_1,iVar2,local_14);
    if (iVar1 == 3) {
      return iVar2;
    }
    if (iVar1 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) break;
    iVar2 = local_14[0];
    if (iVar1 != 4) {
      return -1;
    }
  }
  if (local_14[0] < 0) {
    return local_14[0];
  }
  return ~AL_I2C_INTR_MASK_START_DET_SHIFT;
}



/* @ 0x1007ce8  FUN_01007ce8 */

int FUN_01007ce8(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int aiStack_14 [3];
  
  iVar2 = FUN_01007a20();
  if (iVar2 < 0) {
    return iVar2;
  }
  while( true ) {
    iVar1 = FUN_010078d0(param_1,iVar2,aiStack_14);
    if (iVar1 == 3) {
      return iVar2;
    }
    if (iVar1 == 9) break;
    iVar2 = aiStack_14[0];
    if (iVar1 != 4) {
      return -1;
    }
  }
  if (aiStack_14[0] < 0) {
    return aiStack_14[0];
  }
  return -0xb;
}



/* @ 0x1007d20  FUN_01007d20 */

int FUN_01007d20(int param_1,int param_2,uint *param_3)

{
  uint uVar1;
  
  uVar1 = FUN_01007a20();
  if ((int)uVar1 < 0) {
    if (param_3 == (uint *)0x0) {
      param_1 = 0;
    }
    else {
      param_1 = 0;
      *param_3 = uVar1;
    }
  }
  else {
    uVar1 = *(uint *)(param_1 + 8);
    param_1 = param_1 + param_2 + (uVar1 << 0x18 | (uVar1 >> 8 & 0xff) << 0x10 |
                                   (uVar1 >> 0x10 & 0xff) << 8 | uVar1 >> 0x18);
    if (param_3 != (uint *)0x0) {
      uVar1 = *(uint *)(param_1 + 4);
      *param_3 = uVar1 << 0x18 | (uVar1 >> 8 & 0xff) << 0x10 | (uVar1 >> 0x10 & 0xff) << 8 |
                 uVar1 >> 0x18;
    }
  }
  return param_1;
}



/* @ 0x1007d90  FUN_01007d90 */

int FUN_01007d90(int param_1,undefined4 param_2,undefined4 param_3,int param_4,int *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  iVar1 = FUN_01007cb0();
  do {
    if (iVar1 < 0) {
LAB_01007e50:
      if (param_5 != (int *)0x0) {
        *param_5 = iVar1;
      }
      return 0;
    }
    iVar2 = FUN_01007d20(param_1,iVar1,param_5);
    if (iVar2 == 0) {
      iVar1 = ~AL_I2C_TAR_10BIT_ADDR_SHIFT;
      goto LAB_01007e50;
    }
    uVar5 = *(uint *)(iVar2 + 8);
    uVar4 = *(uint *)(param_1 + 0xc);
    iVar6 = param_1 + (uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 |
                      uVar5 >> 0x18) +
                      (uVar4 << 0x18 | (uVar4 >> 8 & 0xff) << 0x10 | (uVar4 >> 0x10 & 0xff) << 8 |
                      uVar4 >> 0x18);
    iVar3 = FUN_0101297c(iVar6);
    if ((param_4 == iVar3) && (iVar3 = FUN_01012b04(iVar6,param_3,param_4), iVar3 == 0)) {
      return iVar2;
    }
    iVar1 = FUN_01007ce8(param_1,iVar1);
  } while( true );
}



/* @ 0x1007e64  FUN_01007e64 */

void FUN_01007e64(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  
  uVar1 = FUN_0101297c(param_3);
  FUN_01007d90(param_1,param_2,param_3,uVar1,param_4);
  return;
}



/* @ 0x1007eb8  FUN_01007eb8 */

int FUN_01007eb8(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_0101297c(param_3);
  iVar2 = FUN_01007d90(param_1,param_2,param_3,uVar1,param_4);
  if (iVar2 != 0) {
    iVar2 = iVar2 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
  }
  return iVar2;
}



/* @ 0x1007f14  FUN_01007f14 */

int FUN_01007f14(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  
  iVar1 = FUN_01007f84(param_1,s__aliases_01032340);
  if ((-1 < iVar1) && (iVar1 = FUN_01007d90(param_1,iVar1,param_2,param_3,0), iVar1 != 0)) {
    return iVar1 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
  }
  return 0;
}



/* @ 0x1007f84  FUN_01007f84 */

int FUN_01007f84(undefined4 param_1,char *param_2)

{
  int iVar1;
  int iVar2;
  char cVar3;
  char *pcVar4;
  char *pcVar5;
  
  iVar1 = FUN_0101297c(param_2);
  iVar2 = FUN_01007800(param_1);
  if (iVar2 == 0) {
    cVar3 = *param_2;
    pcVar4 = param_2;
    if (cVar3 != '/') {
      pcVar4 = (char *)FUN_010129a8(param_2,0x2f);
      if (pcVar4 == (char *)0x0) {
        pcVar4 = param_2 + iVar1;
      }
      iVar2 = FUN_01007f14(param_1,param_2,(int)pcVar4 - (int)param_2);
      if (iVar2 == 0) {
        return -5;
      }
      iVar2 = FUN_01007f84(param_1);
      cVar3 = *pcVar4;
    }
    if (cVar3 != '\0') {
      while( true ) {
        pcVar5 = pcVar4;
        if (cVar3 == '/') {
          do {
            pcVar4 = pcVar4 + 1;
          } while (*pcVar4 == '/');
          pcVar5 = pcVar4;
          if (*pcVar4 == '\0') {
            return iVar2;
          }
        }
        pcVar4 = (char *)FUN_010129a8(pcVar5,0x2f);
        if (pcVar4 == (char *)0x0) {
          pcVar4 = param_2 + iVar1;
        }
        iVar2 = FUN_01007bbc(param_1,iVar2,pcVar5,(int)pcVar4 - (int)pcVar5);
        if (iVar2 < 0) break;
        cVar3 = *pcVar4;
        if (cVar3 == '\0') {
          return iVar2;
        }
      }
      return iVar2;
    }
  }
  return iVar2;
}



/* @ 0x1008098  FUN_01008098 */

int FUN_01008098(undefined4 param_1,undefined4 param_2,int param_3,uint *param_4,undefined4 param_5,
                undefined4 param_6,undefined4 param_7,undefined4 param_8)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  undefined1 auStack_48 [16];
  undefined1 auStack_38 [16];
  undefined1 auStack_28 [16];
  
  iVar1 = FUN_0100aae4(auStack_48,auStack_38,auStack_28,0);
  if (iVar1 == 0) {
    iVar1 = FUN_0100b898(auStack_48,param_1,param_2);
    if (((iVar1 == 0) && (iVar1 = FUN_0100b898(auStack_38,param_5,param_6), iVar1 == 0)) &&
       (iVar1 = FUN_0100b898(auStack_28,param_7,param_8), iVar1 == 0)) {
      iVar1 = FUN_01009fa4(auStack_28,auStack_48);
      if (iVar1 == -1) {
        iVar1 = 0x16;
      }
      else {
        iVar1 = FUN_0100a6a8(auStack_48,auStack_38,auStack_28,auStack_48);
        if (iVar1 == 0) {
          uVar2 = FUN_0100bb80(auStack_28);
          if (*param_4 < uVar2) {
            iVar1 = 6;
            *param_4 = uVar2;
          }
          else {
            iVar1 = FUN_0100bb80(auStack_48);
            iVar3 = FUN_0100bb80(auStack_28);
            if (iVar3 < iVar1) {
              iVar1 = 1;
            }
            else {
              *param_4 = uVar2;
              FUN_010081f8(param_3,uVar2);
              iVar1 = FUN_0100bb80(auStack_48);
              iVar1 = FUN_0100bae8(auStack_48,param_3 + (uVar2 - iVar1));
            }
          }
        }
      }
    }
    FUN_01009f50(auStack_48,auStack_38,auStack_28,0);
  }
  return iVar1;
}



/* @ 0x10081f8  FUN_010081f8 */

void FUN_010081f8(undefined1 *param_1,int param_2)

{
  undefined1 *puVar1;
  undefined1 *puVar2;
  
  if (param_2 != 0) {
    puVar2 = param_1;
    do {
      puVar1 = puVar2 + 1;
      *puVar2 = 0;
      puVar2 = puVar1;
    } while (puVar1 != param_1 + param_2);
  }
  return;
}



/* @ 0x1008218  FUN_01008218 */

undefined4 FUN_01008218(int param_1,byte *param_2)

{
  int *piVar1;
  uint *puVar2;
  uint *puVar3;
  int *piVar4;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  int iVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  uint uVar15;
  int local_14c;
  uint local_148 [5];
  uint local_134;
  uint local_130;
  uint local_12c;
  uint local_128 [65];
  int *piVar5;
  
  piVar1 = &local_14c;
  piVar4 = (int *)(param_1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  do {
    piVar5 = piVar4 + 1;
    piVar1 = piVar1 + 1;
    *piVar1 = *piVar4;
    piVar4 = piVar5;
  } while ((int *)(param_1 + 0x28) != piVar5);
  puVar2 = &local_12c;
  do {
    puVar2 = puVar2 + 1;
    *puVar2 = (uint)param_2[1] << 0x10 | (uint)*param_2 << 0x18 | (uint)param_2[3] |
              (uint)param_2[2] << 8;
    param_2 = param_2 + 4;
  } while (local_128 + 0xf != puVar2);
  puVar2 = local_128;
  do {
    uVar9 = puVar2[0xe];
    puVar3 = puVar2 + 1;
    uVar7 = *puVar3;
    puVar2[0x10] = ((uVar9 >> 0x13 | uVar9 << 0xd) ^ (uVar9 >> 0x11 | uVar9 << 0xf) ^ uVar9 >> 10) +
                   puVar2[9] + *puVar2 +
                   ((uVar7 >> 0x12 | uVar7 << 0xe) ^ (uVar7 >> 7 | uVar7 << 0x19) ^ uVar7 >> 3);
    puVar2 = puVar3;
  } while (local_128 + 0x30 != puVar3);
  iVar10 = ((local_148[4] >> 0xb | local_148[4] << 0x15) ^
            (local_148[4] >> 6 | local_148[4] << 0x1a) ^ (local_148[4] >> 0x19 | local_148[4] << 7))
           + local_12c + local_128[0] + 0x428a2f98 +
           ((local_130 ^ local_134) & local_148[4] ^ local_130);
  uVar14 = iVar10 + local_148[3];
  uVar11 = ((local_148[0] >> 0xd | local_148[0] << 0x13) ^
            (local_148[0] >> 2 | local_148[0] << 0x1e) ^ (local_148[0] >> 0x16 | local_148[0] << 10)
           ) + ((local_148[0] | local_148[1]) & local_148[2] | local_148[0] & local_148[1]) + iVar10
  ;
  iVar10 = ((local_148[4] ^ local_134) & uVar14 ^ local_134) + local_130 + local_128[1] + 0x71374491
           + ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
             (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar13 = local_148[2] + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) +
          ((local_148[0] | uVar11) & local_148[1] | local_148[0] & uVar11) + iVar10;
  iVar10 = ((local_148[4] ^ uVar14) & uVar13 ^ local_148[4]) +
           local_134 + local_128[2] + -0x4a3f0431 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar7 = local_148[1] + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & local_148[0] | uVar11 & uVar6) +
           iVar10;
  iVar10 = ((uVar14 ^ uVar13) & uVar7 ^ uVar14) + local_148[4] + local_128[3] + -0x164a245b +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar9 = local_148[0] + iVar10;
  uVar8 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
          (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) + iVar10
  ;
  iVar10 = ((uVar13 ^ uVar7) & uVar9 ^ uVar13) + local_128[4] + 0x3956c25b + uVar14 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
           (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar12 | uVar8) & uVar6 | uVar12 & uVar8) + iVar10;
  iVar10 = ((uVar7 ^ uVar9) & uVar11 ^ uVar7) + local_128[5] + 0x59f111f1 + uVar13 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar15 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
           (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar8 | uVar14) & uVar12 | uVar8 & uVar14) +
           iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[6] + -0x6dc07d5c + uVar7 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar13 = ((uVar15 >> 0xd | uVar15 * 0x80000) ^ (uVar15 >> 2 | uVar15 * 0x40000000) ^
           (uVar15 >> 0x16 | uVar15 * 0x400)) + ((uVar14 | uVar15) & uVar8 | uVar14 & uVar15) +
           iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[7] + -0x54e3a12b + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar7 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
          (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar15 | uVar13) & uVar14 | uVar15 & uVar13) +
          iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar8 ^ uVar6) + local_128[8] + -0x27f85568 + uVar11 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
           (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar13 | uVar7) & uVar15 | uVar13 & uVar7) + iVar10;
  iVar10 = ((uVar12 ^ uVar8) & uVar14 ^ uVar12) + local_128[9] + 0x12835b01 + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar15 = uVar15 + iVar10;
  uVar9 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar7 | uVar11) & uVar13 | uVar7 & uVar11) + iVar10
  ;
  iVar10 = ((uVar8 ^ uVar14) & uVar15 ^ uVar8) + local_128[10] + 0x243185be + uVar12 +
           ((uVar15 >> 0xb | uVar15 * 0x200000) ^ (uVar15 >> 6 | uVar15 * 0x4000000) ^
           (uVar15 >> 0x19 | uVar15 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar12 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar11 | uVar9) & uVar7 | uVar11 & uVar9) + iVar10;
  iVar10 = ((uVar14 ^ uVar15) & uVar13 ^ uVar14) + local_128[0xb] + 0x550c7dc3 + uVar8 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
          (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar9 | uVar12) & uVar11 | uVar9 & uVar12) + iVar10
  ;
  iVar10 = ((uVar15 ^ uVar13) & uVar7 ^ uVar15) + local_128[0xc] + 0x72be5d74 + uVar14 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar12 | uVar6) & uVar9 | uVar12 & uVar6) + iVar10;
  iVar10 = ((uVar13 ^ uVar7) & uVar11 ^ uVar13) + local_128[0xd] + -0x7f214e02 + uVar15 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar15 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
           (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar6 | uVar14) & uVar12 | uVar6 & uVar14) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar11) & uVar9 ^ uVar7) + local_128[0xe] + -0x6423f959 + uVar13 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar13 = ((uVar15 >> 0xd | uVar15 * 0x80000) ^ (uVar15 >> 2 | uVar15 * 0x40000000) ^
           (uVar15 >> 0x16 | uVar15 * 0x400)) + ((uVar14 | uVar15) & uVar6 | uVar14 & uVar15) +
           iVar10;
  iVar10 = ((uVar11 ^ uVar9) & uVar12 ^ uVar11) + local_128[0xf] + -0x3e640e8c + uVar7 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
          (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar15 | uVar13) & uVar14 | uVar15 & uVar13) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar12) & uVar6 ^ uVar9) + local_128[0x10] + -0x1b64963f + uVar11 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar13 | uVar7) & uVar15 | uVar13 & uVar7) + iVar10;
  iVar10 = ((uVar12 ^ uVar6) & uVar14 ^ uVar12) + local_128[0x11] + -0x1041b87a + uVar9 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar15 = uVar15 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar13 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar14) & uVar15 ^ uVar6) + local_128[0x12] + 0xfc19dc6 + uVar12 +
           ((uVar15 >> 0xb | uVar15 * 0x200000) ^ (uVar15 >> 6 | uVar15 * 0x4000000) ^
           (uVar15 >> 0x19 | uVar15 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar14 ^ uVar15) & uVar13 ^ uVar14) + local_128[0x13] + 0x240ca1cc + uVar6 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar15 ^ uVar13) & uVar7 ^ uVar15) + local_128[0x14] + 0x2de92c6f + uVar14 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) + iVar10;
  iVar10 = ((uVar13 ^ uVar7) & uVar8 ^ uVar13) + local_128[0x15] + 0x4a7484aa + uVar15 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar15 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
           (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar8) & uVar9 ^ uVar7) + local_128[0x16] + 0x5cb0a9dc + uVar13 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar15 >> 0xd | uVar15 * 0x80000) ^ (uVar15 >> 2 | uVar15 * 0x40000000) ^
           (uVar15 >> 0x16 | uVar15 * 0x400)) + ((uVar12 | uVar15) & uVar6 | uVar12 & uVar15) +
           iVar10;
  iVar10 = ((uVar8 ^ uVar9) & uVar11 ^ uVar8) + local_128[0x17] + 0x76f988da + uVar7 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
          (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar15 | uVar14) & uVar12 | uVar15 & uVar14) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[0x18] + -0x67c1aeae + uVar8 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar14 | uVar7) & uVar15 | uVar14 & uVar7) + iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[0x19] + -0x57ce3993 + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar15 = uVar15 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar14 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar15 ^ uVar6) + local_128[0x1a] + -0x4ffcd838 + uVar11 +
           ((uVar15 >> 0xb | uVar15 * 0x200000) ^ (uVar15 >> 6 | uVar15 * 0x4000000) ^
           (uVar15 >> 0x19 | uVar15 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar12 ^ uVar15) & uVar14 ^ uVar12) + local_128[0x1b] + -0x40a68039 + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar15 ^ uVar14) & uVar7 ^ uVar15) + local_128[0x1c] + -0x391ff40d + uVar12 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) + iVar10;
  iVar10 = ((uVar14 ^ uVar7) & uVar8 ^ uVar14) + local_128[0x1d] + -0x2a586eb9 + uVar15 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar13 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
           (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar8) & uVar9 ^ uVar7) + local_128[0x1e] + 0x6ca6351 + uVar14 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
           (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar12 | uVar13) & uVar6 | uVar12 & uVar13) +
           iVar10;
  iVar10 = ((uVar8 ^ uVar9) & uVar11 ^ uVar8) + local_128[0x1f] + 0x14292967 + uVar7 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
          (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar13 | uVar14) & uVar12 | uVar13 & uVar14) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[0x20] + 0x27b70a85 + uVar8 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar14 | uVar7) & uVar13 | uVar14 & uVar7) + iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[0x21] + 0x2e1b2138 + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar14 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar13 ^ uVar6) + local_128[0x22] + 0x4d2c6dfc + uVar11 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar12 ^ uVar13) & uVar14 ^ uVar12) + local_128[0x23] + 0x53380d13 + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar13 ^ uVar14) & uVar7 ^ uVar13) + local_128[0x24] + 0x650a7354 + uVar12 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) + iVar10;
  iVar10 = ((uVar14 ^ uVar7) & uVar8 ^ uVar14) + local_128[0x25] + 0x766a0abb + uVar13 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar13 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
           (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar8) & uVar9 ^ uVar7) + local_128[0x26] + -0x7e3d36d2 + uVar14 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
           (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar12 | uVar13) & uVar6 | uVar12 & uVar13) +
           iVar10;
  iVar10 = ((uVar8 ^ uVar9) & uVar11 ^ uVar8) + local_128[0x27] + -0x6d8dd37b + uVar7 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
          (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar13 | uVar14) & uVar12 | uVar13 & uVar14) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[0x28] + -0x5d40175f + uVar8 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar14 | uVar7) & uVar13 | uVar14 & uVar7) + iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[0x29] + -0x57e599b5 + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar14 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar13 ^ uVar6) + local_128[0x2a] + -0x3db47490 + uVar11 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar12 ^ uVar13) & uVar14 ^ uVar12) + local_128[0x2b] + -0x3893ae5d + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar13 ^ uVar14) & uVar7 ^ uVar13) + local_128[0x2c] + -0x2e6d17e7 + uVar12 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) + iVar10;
  iVar10 = ((uVar14 ^ uVar7) & uVar8 ^ uVar14) + local_128[0x2d] + -0x2966f9dc + uVar13 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar13 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
           (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar8) & uVar9 ^ uVar7) + local_128[0x2e] + -0xbf1ca7b + uVar14 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
           (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar12 | uVar13) & uVar6 | uVar12 & uVar13) +
           iVar10;
  iVar10 = ((uVar8 ^ uVar9) & uVar11 ^ uVar8) + local_128[0x2f] + 0x106aa070 + uVar7 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
          (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar13 | uVar14) & uVar12 | uVar13 & uVar14) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[0x30] + 0x19a4c116 + uVar8 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar14 | uVar7) & uVar13 | uVar14 & uVar7) + iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[0x31] + 0x1e376c08 + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar14 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar13 ^ uVar6) + local_128[0x32] + 0x2748774c + uVar11 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar12 ^ uVar13) & uVar14 ^ uVar12) + local_128[0x33] + 0x34b0bcb5 + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar13 ^ uVar14) & uVar7 ^ uVar13) + local_128[0x34] + 0x391c0cb3 + uVar12 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  uVar8 = uVar8 + iVar10;
  uVar12 = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
           (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) + iVar10;
  iVar10 = ((uVar14 ^ uVar7) & uVar8 ^ uVar14) + local_128[0x35] + 0x4ed8aa4a + uVar13 +
           ((uVar8 >> 0xb | uVar8 * 0x200000) ^ (uVar8 >> 6 | uVar8 * 0x4000000) ^
           (uVar8 >> 0x19 | uVar8 * 0x80));
  uVar9 = uVar9 + iVar10;
  uVar13 = ((uVar12 >> 0xd | uVar12 * 0x80000) ^ (uVar12 >> 2 | uVar12 * 0x40000000) ^
           (uVar12 >> 0x16 | uVar12 * 0x400)) + ((uVar6 | uVar12) & uVar11 | uVar6 & uVar12) +
           iVar10;
  iVar10 = ((uVar7 ^ uVar8) & uVar9 ^ uVar7) + local_128[0x36] + 0x5b9cca4f + uVar14 +
           ((uVar9 >> 0xb | uVar9 * 0x200000) ^ (uVar9 >> 6 | uVar9 * 0x4000000) ^
           (uVar9 >> 0x19 | uVar9 * 0x80));
  uVar11 = uVar11 + iVar10;
  uVar14 = ((uVar13 >> 0xd | uVar13 * 0x80000) ^ (uVar13 >> 2 | uVar13 * 0x40000000) ^
           (uVar13 >> 0x16 | uVar13 * 0x400)) + ((uVar12 | uVar13) & uVar6 | uVar12 & uVar13) +
           iVar10;
  iVar10 = ((uVar8 ^ uVar9) & uVar11 ^ uVar8) + local_128[0x37] + 0x682e6ff3 + uVar7 +
           ((uVar11 >> 0xb | uVar11 * 0x200000) ^ (uVar11 >> 6 | uVar11 * 0x4000000) ^
           (uVar11 >> 0x19 | uVar11 * 0x80));
  uVar6 = uVar6 + iVar10;
  uVar7 = ((uVar14 >> 0xd | uVar14 * 0x80000) ^ (uVar14 >> 2 | uVar14 * 0x40000000) ^
          (uVar14 >> 0x16 | uVar14 * 0x400)) + ((uVar13 | uVar14) & uVar12 | uVar13 & uVar14) +
          iVar10;
  iVar10 = ((uVar9 ^ uVar11) & uVar6 ^ uVar9) + local_128[0x38] + 0x748f82ee + uVar8 +
           ((uVar6 >> 0xb | uVar6 * 0x200000) ^ (uVar6 >> 6 | uVar6 * 0x4000000) ^
           (uVar6 >> 0x19 | uVar6 * 0x80));
  uVar12 = uVar12 + iVar10;
  uVar8 = ((uVar7 >> 0xd | uVar7 * 0x80000) ^ (uVar7 >> 2 | uVar7 * 0x40000000) ^
          (uVar7 >> 0x16 | uVar7 * 0x400)) + ((uVar14 | uVar7) & uVar13 | uVar14 & uVar7) + iVar10;
  iVar10 = ((uVar11 ^ uVar6) & uVar12 ^ uVar11) + local_128[0x39] + 0x78a5636f + uVar9 +
           ((uVar12 >> 0xb | uVar12 * 0x200000) ^ (uVar12 >> 6 | uVar12 * 0x4000000) ^
           (uVar12 >> 0x19 | uVar12 * 0x80));
  uVar13 = uVar13 + iVar10;
  uVar9 = ((uVar8 >> 0xd | uVar8 * 0x80000) ^ (uVar8 >> 2 | uVar8 * 0x40000000) ^
          (uVar8 >> 0x16 | uVar8 * 0x400)) + ((uVar7 | uVar8) & uVar14 | uVar7 & uVar8) + iVar10;
  iVar10 = ((uVar6 ^ uVar12) & uVar13 ^ uVar6) + local_128[0x3a] + -0x7b3787ec + uVar11 +
           ((uVar13 >> 0xb | uVar13 * 0x200000) ^ (uVar13 >> 6 | uVar13 * 0x4000000) ^
           (uVar13 >> 0x19 | uVar13 * 0x80));
  uVar14 = uVar14 + iVar10;
  uVar11 = ((uVar9 >> 0xd | uVar9 * 0x80000) ^ (uVar9 >> 2 | uVar9 * 0x40000000) ^
           (uVar9 >> 0x16 | uVar9 * 0x400)) + ((uVar8 | uVar9) & uVar7 | uVar8 & uVar9) + iVar10;
  iVar10 = ((uVar12 ^ uVar13) & uVar14 ^ uVar12) + local_128[0x3b] + -0x7338fdf8 + uVar6 +
           ((uVar14 >> 0xb | uVar14 * 0x200000) ^ (uVar14 >> 6 | uVar14 * 0x4000000) ^
           (uVar14 >> 0x19 | uVar14 * 0x80));
  uVar7 = uVar7 + iVar10;
  uVar6 = ((uVar11 >> 0xd | uVar11 * 0x80000) ^ (uVar11 >> 2 | uVar11 * 0x40000000) ^
          (uVar11 >> 0x16 | uVar11 * 0x400)) + ((uVar9 | uVar11) & uVar8 | uVar9 & uVar11) + iVar10;
  iVar10 = ((uVar13 ^ uVar14) & uVar7 ^ uVar13) + local_128[0x3c] + -0x6f410006 + uVar12 +
           ((uVar7 >> 0xb | uVar7 * 0x200000) ^ (uVar7 >> 6 | uVar7 * 0x4000000) ^
           (uVar7 >> 0x19 | uVar7 * 0x80));
  local_12c = uVar8 + iVar10;
  local_148[3] = ((uVar6 >> 0xd | uVar6 * 0x80000) ^ (uVar6 >> 2 | uVar6 * 0x40000000) ^
                 (uVar6 >> 0x16 | uVar6 * 0x400)) + ((uVar11 | uVar6) & uVar9 | uVar11 & uVar6) +
                 iVar10;
  iVar10 = ((uVar14 ^ uVar7) & local_12c ^ uVar14) + local_128[0x3d] + -0x5baf9315 + uVar13 +
           ((local_12c >> 0xb | local_12c * 0x200000) ^ (local_12c >> 6 | local_12c * 0x4000000) ^
           (local_12c >> 0x19 | local_12c * 0x80));
  local_130 = uVar9 + iVar10;
  local_148[2] = ((local_148[3] >> 0xd | local_148[3] * 0x80000) ^
                  (local_148[3] >> 2 | local_148[3] * 0x40000000) ^
                 (local_148[3] >> 0x16 | local_148[3] * 0x400)) +
                 ((uVar6 | local_148[3]) & uVar11 | uVar6 & local_148[3]) + iVar10;
  iVar10 = ((uVar7 ^ local_12c) & local_130 ^ uVar7) + local_128[0x3e] + -0x41065c09 + uVar14 +
           ((local_130 >> 0xb | local_130 * 0x200000) ^ (local_130 >> 6 | local_130 * 0x4000000) ^
           (local_130 >> 0x19 | local_130 * 0x80));
  local_134 = uVar11 + iVar10;
  local_148[1] = ((local_148[2] >> 0xd | local_148[2] * 0x80000) ^
                  (local_148[2] >> 2 | local_148[2] * 0x40000000) ^
                 (local_148[2] >> 0x16 | local_148[2] * 0x400)) +
                 ((local_148[3] | local_148[2]) & uVar6 | local_148[3] & local_148[2]) + iVar10;
  iVar10 = (local_12c ^ (local_12c ^ local_130) & local_134) + local_128[0x3f] + -0x398e870e + uVar7
           + ((local_134 >> 0xb | local_134 * 0x200000) ^ (local_134 >> 6 | local_134 * 0x4000000) ^
             (local_134 >> 0x19 | local_134 * 0x80));
  uVar7 = ((local_148[1] >> 0xd | local_148[1] * 0x80000) ^
           (local_148[1] >> 2 | local_148[1] * 0x40000000) ^
          (local_148[1] >> 0x16 | local_148[1] * 0x400)) +
          (local_148[3] & (local_148[2] | local_148[1]) | local_148[2] & local_148[1]) + iVar10;
  puVar2 = local_148;
  local_148[4] = uVar6 + iVar10;
  piVar4 = (int *)(param_1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  while( true ) {
    puVar2 = puVar2 + 1;
    *piVar4 = uVar7 + *piVar4;
    if ((int *)(param_1 + 0x28) == piVar4 + 1) break;
    uVar7 = *puVar2;
    piVar4 = piVar4 + 1;
  }
  return 0;
}



/* @ 0x1009a54  FUN_01009a54 */

undefined4 FUN_01009a54(undefined8 *param_1)

{
  *param_1 = 0;
  *(undefined4 *)(param_1 + 1) = 0x6a09e667;
  *(undefined4 *)((int)param_1 + 0xc) = 0xbb67ae85;
  *(undefined4 *)(param_1 + 2) = 0x3c6ef372;
  *(undefined4 *)((int)param_1 + 0x14) = 0xa54ff53a;
  *(undefined4 *)(param_1 + 3) = 0x510e527f;
  *(undefined4 *)((int)param_1 + 0x1c) = 0x9b05688c;
  *(undefined4 *)(param_1 + 4) = 0x1f83d9ab;
  *(undefined4 *)((int)param_1 + 0x24) = 0x5be0cd19;
  *(undefined4 *)(param_1 + 5) = 0;
  return 0;
}



/* @ 0x1009ae0  FUN_01009ae0 */

undefined4 FUN_01009ae0(uint *param_1,undefined1 *param_2)

{
  uint uVar1;
  undefined1 *puVar2;
  uint *puVar3;
  uint *puVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  
  uVar7 = param_1[10];
  if (0x3f < uVar7) {
    return 0x10;
  }
  uVar1 = uVar7 + 1;
  uVar5 = *param_1 + uVar7 * 8;
  param_1[10] = uVar1;
  uVar6 = param_1[1] + (uint)CARRY4(*param_1,uVar7 * 8);
  *param_1 = uVar5;
  param_1[1] = uVar6;
  *(undefined1 *)((int)param_1 + uVar7 + 0x2c) = 0x80;
  if (uVar1 < 0x39) {
    if (uVar1 == 0x38) goto LAB_01009ba8;
  }
  else {
    if (uVar1 != 0x40) {
      puVar2 = (undefined1 *)((int)param_1 + uVar7 + 0x2c);
      do {
        puVar2 = puVar2 + 1;
        *puVar2 = 0;
      } while (puVar2 != (undefined1 *)((int)param_1 + 0x6b));
      param_1[10] = 0x40;
    }
    FUN_01008218(param_1,param_1 + 0xb);
    uVar1 = 0;
    uVar5 = *param_1;
    uVar6 = param_1[1];
  }
  puVar2 = (undefined1 *)((int)param_1 + uVar1 + 0x2c);
  do {
    uVar1 = uVar1 + 1;
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  } while (uVar1 != 0x38);
  param_1[10] = 0x38;
LAB_01009ba8:
  *(char *)(param_1 + 0x19) = (char)(uVar6 >> 0x18);
  *(char *)((int)param_1 + 0x65) = (char)(uVar6 >> 0x10);
  *(char *)((int)param_1 + 0x66) = (char)(uVar6 >> 8);
  *(char *)((int)param_1 + 0x67) = (char)uVar6;
  param_1[0x1a] =
       uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 | uVar5 >> 0x18;
  FUN_01008218(param_1,param_1 + 0xb);
  puVar3 = param_1 + 2;
  do {
    *param_2 = *(undefined1 *)((int)puVar3 + 3);
    puVar4 = puVar3 + 1;
    param_2[1] = (char)(*puVar3 >> 0x10);
    param_2[2] = (char)(*puVar3 >> 8);
    param_2[3] = (char)*puVar3;
    param_2 = param_2 + 4;
    puVar3 = puVar4;
  } while (param_1 + 10 != puVar4);
  return 0;
}



/* @ 0x1009c40  FUN_01009c40 */

int FUN_01009c40(uint *param_1,int param_2,uint param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  uVar2 = param_1[10];
  if (0x40 < uVar2) {
    return 0x10;
  }
  if (param_3 != 0) {
    while( true ) {
      if (uVar2 == 0 && 0x3f < param_3) {
        iVar1 = FUN_01008218(param_1);
        param_3 = param_3 - 0x40;
        if (iVar1 != 0) {
          return iVar1;
        }
        uVar2 = *param_1;
        *param_1 = uVar2 + 0x200;
        param_1[1] = param_1[1] + (uint)(0xfffffdff < uVar2);
        param_2 = param_2 + 0x40;
      }
      else {
        uVar3 = 0x40 - uVar2;
        if (param_3 <= 0x40 - uVar2) {
          uVar3 = param_3;
        }
        iVar1 = param_2 + uVar3;
        FUN_010129d8((int)(param_1 + 0xb) + uVar2,param_2,uVar3);
        uVar2 = param_1[10];
        param_3 = param_3 - uVar3;
        param_1[10] = uVar3 + uVar2;
        param_2 = iVar1;
        if (uVar3 + uVar2 == 0x40) {
          iVar1 = FUN_01008218(param_1,param_1 + 0xb);
          if (iVar1 != 0) {
            return iVar1;
          }
          uVar2 = *param_1;
          param_1[10] = 0;
          *param_1 = uVar2 + 0x200;
          param_1[1] = param_1[1] + (uint)(0xfffffdff < uVar2);
        }
      }
      if (param_3 == 0) break;
      uVar2 = param_1[10];
    }
  }
  return 0;
}



/* @ 0x1009d54  FUN_01009d54 */

void FUN_01009d54(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  
  FUN_0100bbb0();
  iVar2 = param_2 / 0x1c + 1;
  iVar1 = FUN_0100a9b8(param_1,iVar2,param_2 * -0x6db6db6d);
  if (iVar1 == 0) {
    *param_1 = iVar2;
    *(int *)(param_1[3] + (param_2 / 0x1c) * 4) = 1 << (param_2 % 0x1c & 0xffU);
  }
  return;
}



/* @ 0x1009dd0  FUN_01009dd0 */

int FUN_01009dd0(int param_1,int param_2)

{
  int iVar1;
  
  if ((param_2 == param_1) || (iVar1 = FUN_0100a0bc(), iVar1 == 0)) {
    iVar1 = 0;
    *(undefined4 *)(param_2 + 8) = 0;
  }
  return iVar1;
}



/* @ 0x1009e08  FUN_01009e08 */

int FUN_01009e08(int *param_1,int *param_2,int *param_3)

{
  int iVar1;
  uint *puVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int *piVar10;
  uint *puVar11;
  
  iVar7 = param_1[2];
  iVar8 = param_2[2];
  if (iVar7 != iVar8) {
    iVar1 = FUN_0100a030();
    if (iVar1 == -1) {
      param_3[2] = iVar8;
      param_1 = param_2;
      param_2 = param_1;
    }
    else {
      param_3[2] = iVar7;
    }
    iVar7 = al_pcie_read_config(param_1,param_2,param_3);
    return iVar7;
  }
  param_3[2] = iVar7;
  iVar1 = *param_2;
  iVar8 = *param_1;
  iVar7 = iVar1;
  piVar5 = param_1;
  if (iVar8 <= iVar1) {
    iVar7 = iVar8;
    iVar8 = iVar1;
    piVar5 = param_2;
  }
  iVar1 = iVar8 + 1;
  if ((iVar8 < param_3[1]) || (iVar3 = FUN_0100a9b8(param_3,iVar1), iVar3 == 0)) {
    iVar3 = 0;
    iVar9 = *param_3;
    piVar4 = (int *)param_1[3];
    if (iVar7 < 1) {
      param_1 = (int *)0x0;
    }
    piVar10 = (int *)param_2[3];
    *param_3 = iVar1;
    puVar2 = (uint *)param_3[3];
    if (0 < iVar7) {
      param_1 = (int *)0x0;
      puVar11 = puVar2;
      do {
        iVar3 = iVar3 + 1;
        uVar6 = *piVar4 + *piVar10 + (int)param_1;
        param_1 = (int *)(uVar6 >> 0x1c);
        *puVar11 = uVar6 & 0xfffffff;
        piVar4 = piVar4 + 1;
        piVar10 = piVar10 + 1;
        puVar11 = puVar11 + 1;
      } while (iVar7 != iVar3);
      puVar2 = puVar2 + iVar7;
    }
    if ((iVar7 != iVar8) && (iVar3 < iVar8)) {
      iVar7 = piVar5[3];
      piVar5 = (int *)(iVar7 + iVar3 * 4);
      puVar11 = puVar2;
      do {
        piVar4 = piVar5 + 1;
        uVar6 = *piVar5 + (int)param_1;
        param_1 = (int *)(uVar6 >> 0x1c);
        *puVar11 = uVar6 & 0xfffffff;
        piVar5 = piVar4;
        puVar11 = puVar11 + 1;
      } while ((int *)(iVar7 + iVar8 * 4) != piVar4);
      puVar2 = puVar2 + (iVar8 - iVar3);
    }
    *puVar2 = (uint)param_1;
    if (iVar1 < iVar9) {
      do {
        puVar2 = puVar2 + 1;
        iVar1 = iVar1 + 1;
        *puVar2 = 0;
      } while (iVar9 != iVar1);
    }
    FUN_01009e90(param_3);
    iVar3 = 0;
  }
  return iVar3;
}



/* @ 0x1009e90  FUN_01009e90 */

void FUN_01009e90(int *param_1)

{
  int *piVar1;
  int iVar2;
  
  iVar2 = *param_1;
  if (iVar2 < 1) {
    if (iVar2 != 0) {
      return;
    }
  }
  else {
    piVar1 = (int *)(param_1[3] + (iVar2 + 0x3fffffff) * 4);
    if (*(int *)(param_1[3] + (iVar2 + 0x3fffffff) * 4) != 0) {
      return;
    }
    while (iVar2 = iVar2 + -1, iVar2 != 0) {
      piVar1 = piVar1 + -1;
      if (*piVar1 != 0) {
        *param_1 = iVar2;
        return;
      }
    }
    *param_1 = 0;
  }
  param_1[2] = 0;
  return;
}



/* @ 0x1009eec  FUN_01009eec */

void FUN_01009eec(int *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar4;
  undefined4 *puVar3;
  
  puVar4 = (undefined4 *)param_1[3];
  if (puVar4 == (undefined4 *)0x0) {
    return;
  }
  iVar1 = *param_1;
  if (0 < iVar1) {
    puVar2 = puVar4;
    do {
      puVar3 = puVar2 + 1;
      *puVar2 = 0;
      puVar2 = puVar3;
    } while (puVar3 != puVar4 + iVar1);
  }
  FUN_0100d364(puVar4);
  *param_1 = 0;
  param_1[1] = 0;
  param_1[2] = 0;
  param_1[3] = 0;
  return;
}



/* @ 0x1009f50  FUN_01009f50 */

void FUN_01009f50(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int local_c [3];
  
  piVar1 = local_c;
  local_c[2] = param_4;
  local_c[1] = param_3;
  local_c[0] = param_2;
  while (param_1 != 0) {
    FUN_01009eec();
    param_1 = *piVar1;
    piVar1 = piVar1 + 1;
  }
  return;
}



/* @ 0x1009fa4  FUN_01009fa4 */

undefined4 FUN_01009fa4(int param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = *(int *)(param_1 + 8);
  if (iVar2 != *(int *)(param_2 + 8)) {
    if (iVar2 == 1) {
      uVar1 = 0xffffffff;
    }
    else {
      uVar1 = 1;
    }
    return uVar1;
  }
  if (iVar2 == 1) {
    uVar1 = FUN_0100a030(param_2,param_1);
    return uVar1;
  }
  uVar1 = FUN_0100a030();
  return uVar1;
}



/* @ 0x1009fec  FUN_01009fec */

undefined4 FUN_01009fec(int *param_1,uint param_2)

{
  undefined4 uVar1;
  
  if (param_1[2] == 1) {
    return 0xffffffff;
  }
  if ((*param_1 < 2) && (*(uint *)param_1[3] <= param_2)) {
    if (*(uint *)param_1[3] < param_2) {
      uVar1 = 0xffffffff;
    }
    else {
      uVar1 = 0;
    }
    return uVar1;
  }
  return 1;
}



/* @ 0x100a030  FUN_0100a030 */

undefined4 FUN_0100a030(int *param_1,int *param_2)

{
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  iVar6 = *param_1;
  if (*param_2 < iVar6) {
    return 1;
  }
  if (*param_2 <= iVar6) {
    iVar3 = iVar6 + 0x3fffffff;
    puVar1 = (uint *)(param_1[3] + iVar3 * 4);
    puVar2 = (uint *)(param_2[3] + iVar3 * 4);
    if (iVar6 < 1) {
      return 0;
    }
    uVar5 = *(uint *)(param_1[3] + iVar3 * 4);
    uVar4 = *(uint *)(param_2[3] + iVar3 * 4);
    if (uVar4 < uVar5) {
      return 1;
    }
    if (uVar4 <= uVar5) {
      iVar3 = 0;
      do {
        iVar3 = iVar3 + 1;
        if (iVar6 == iVar3) {
          return 0;
        }
        puVar1 = puVar1 + -1;
        puVar2 = puVar2 + -1;
        if (*puVar2 < *puVar1) {
          return 1;
        }
      } while (*puVar2 <= *puVar1);
    }
  }
  return 0xffffffff;
}



/* @ 0x100a0bc  FUN_0100a0bc */

int FUN_0100a0bc(int *param_1,int *param_2)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined4 *puVar6;
  
  if (param_1 == param_2) {
    return 0;
  }
  iVar1 = *param_1;
  if (param_2[1] < iVar1) {
    iVar1 = FUN_0100a9b8(param_2);
    if (iVar1 != 0) {
      return iVar1;
    }
    iVar1 = *param_1;
  }
  puVar2 = (undefined4 *)param_2[3];
  if (iVar1 < 1) {
    iVar3 = 0;
  }
  else {
    puVar6 = puVar2 + -1;
    iVar3 = 0;
    puVar4 = (undefined4 *)param_1[3];
    do {
      iVar3 = iVar3 + 1;
      puVar6 = puVar6 + 1;
      *puVar6 = *puVar4;
      puVar4 = puVar4 + 1;
    } while (iVar1 != iVar3);
    puVar2 = puVar2 + iVar1;
  }
  iVar5 = *param_2;
  if (iVar3 < iVar5) {
    do {
      iVar3 = iVar3 + 1;
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    } while (iVar3 != iVar5);
  }
  iVar3 = param_1[2];
  *param_2 = iVar1;
  param_2[2] = iVar3;
  return 0;
}



/* @ 0x100a180  FUN_0100a180 */

int FUN_0100a180(int *param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  iVar2 = *param_1;
  if (iVar2 == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = (iVar2 + -1) * 0x1c;
    uVar3 = *(uint *)(param_1[3] + (iVar2 + 0x3fffffff) * 4);
    if (uVar3 != 0) {
      do {
        uVar3 = uVar3 >> 1;
        iVar1 = iVar1 + 1;
      } while (uVar3 != 0);
      return iVar1;
    }
  }
  return iVar1;
}



/* @ 0x100a1c4  FUN_0100a1c4 */

int FUN_0100a1c4(int *param_1,int *param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  uint extraout_r3;
  int iVar7;
  int iVar8;
  uint *puVar9;
  int iVar10;
  uint *puVar11;
  
  iVar4 = *param_1;
  uVar5 = param_2[1];
  if ((int)uVar5 < iVar4) {
    iVar4 = FUN_0100a9b8(param_2);
    if (iVar4 != 0) {
      return iVar4;
    }
    iVar4 = *param_1;
    uVar5 = extraout_r3;
  }
  iVar10 = param_1[3];
  iVar3 = iVar4 + -1;
  iVar1 = (iVar4 + 0x3fffffff) * 4;
  if (-1 < iVar3) {
    uVar5 = 0;
  }
  iVar8 = *param_2;
  iVar7 = param_2[3];
  *param_2 = iVar4;
  puVar9 = (uint *)(iVar7 + iVar1);
  puVar11 = (uint *)(iVar10 + iVar1);
  if (-1 < iVar3) {
    do {
      uVar2 = uVar5 << 0x1b;
      iVar3 = iVar3 + -1;
      uVar5 = *puVar11 & 1;
      *puVar9 = uVar2 | *puVar11 >> 1;
      puVar9 = puVar9 + -1;
      puVar11 = puVar11 + -1;
    } while (iVar3 != -1);
  }
  puVar6 = (undefined4 *)(iVar7 + iVar1 + 4);
  if (iVar4 < iVar8) {
    do {
      iVar4 = iVar4 + 1;
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    } while (iVar8 != iVar4);
  }
  param_2[2] = param_1[2];
  FUN_01009e90(param_2);
  return 0;
}



/* @ 0x100a2a4  FUN_0100a2a4 */

int FUN_0100a2a4(undefined4 param_1,int param_2,int *param_3,int param_4)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  uint uVar5;
  uint uVar6;
  undefined1 auStack_28 [16];
  
  if (param_2 < 1) {
    iVar2 = FUN_0100a0bc(param_1,param_3);
    if (param_4 != 0) {
      FUN_0100bbb0(param_4);
    }
  }
  else {
    iVar2 = FUN_0100aa40(auStack_28);
    if (iVar2 == 0) {
      if (((param_4 == 0) || (iVar2 = FUN_0100b124(param_1,param_2,auStack_28), iVar2 == 0)) &&
         (iVar2 = FUN_0100a0bc(param_1,param_3), iVar2 == 0)) {
        if (0x1b < param_2) {
          FUN_0100b960(param_3,param_2 / 0x1c);
        }
        uVar5 = param_2 % 0x1c;
        if (uVar5 != 0) {
          iVar3 = *param_3 + -1;
          if (-1 < iVar3) {
            uVar6 = 0;
            puVar4 = (uint *)(param_3[3] + (*param_3 + 0x3fffffff) * 4);
            do {
              iVar3 = iVar3 + -1;
              uVar1 = uVar6 << (0x1c - uVar5 & 0xff);
              uVar6 = *puVar4 & ~(-1 << (uVar5 & 0xff));
              *puVar4 = uVar1 | *puVar4 >> (uVar5 & 0xff);
              puVar4 = puVar4 + -1;
            } while (iVar3 != -1);
          }
        }
        FUN_01009e90(param_3);
        if (param_4 != 0) {
          FUN_0100a668(auStack_28,param_4);
        }
      }
      FUN_01009eec(auStack_28);
    }
  }
  return iVar2;
}



/* @ 0x100a408  FUN_0100a408 */

int FUN_0100a408(int param_1,int *param_2,int *param_3,int *param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  undefined1 auStack_60 [16];
  undefined1 auStack_50 [16];
  undefined1 auStack_40 [16];
  undefined1 auStack_30 [16];
  
  if (*param_2 == 0) {
    iVar1 = -3;
  }
  else {
    iVar1 = FUN_0100a030();
    if (iVar1 == -1) {
      if (param_4 == (int *)0x0) {
        iVar1 = 0;
      }
      else {
        iVar1 = FUN_0100a0bc(param_1,param_4);
      }
      if (param_3 != (int *)0x0) {
        FUN_0100bbb0(param_3);
      }
    }
    else {
      iVar1 = FUN_0100aae4(auStack_60,auStack_50,auStack_40,auStack_30,0);
      if (iVar1 == 0) {
        FUN_0100b9ec(auStack_40,1);
        iVar2 = FUN_0100a180(param_1);
        iVar3 = FUN_0100a180(param_2);
        iVar1 = FUN_01009dd0(param_1,auStack_60);
        if ((iVar1 == 0) && (iVar1 = FUN_01009dd0(param_2,auStack_50), iVar1 == 0)) {
          iVar2 = iVar2 - iVar3;
          iVar1 = FUN_0100b684(auStack_50,iVar2);
          if ((iVar1 == 0) && (iVar3 = FUN_0100b684(auStack_40,iVar2), iVar1 = iVar3, iVar3 == 0)) {
            while( true ) {
              bVar5 = iVar2 < 0;
              iVar2 = iVar2 + -1;
              if (bVar5) break;
              iVar1 = FUN_01009fa4(auStack_50,auStack_60);
              if (iVar1 != 1) {
                iVar1 = FUN_0100ba50(auStack_60,auStack_50);
                if ((iVar1 != 0) || (iVar1 = FUN_01009e08(auStack_30,auStack_40), iVar1 != 0))
                goto LAB_0100a4c8;
              }
              iVar1 = FUN_0100a2a4(auStack_50,1,auStack_50,0);
              if ((iVar1 != 0) || (iVar1 = FUN_0100a2a4(auStack_40,1,auStack_40,0), iVar1 != 0))
              goto LAB_0100a4c8;
            }
            iVar2 = *(int *)(param_1 + 8);
            if (param_3 != (int *)0x0) {
              iVar4 = param_2[2];
              FUN_0100a668(param_3,auStack_30);
              iVar1 = 0;
              if ((*param_3 != 0) && (iVar1 = 0, iVar4 != iVar2)) {
                iVar1 = 1;
              }
              param_3[2] = iVar1;
            }
            iVar1 = iVar3;
            if (param_4 != (int *)0x0) {
              FUN_0100a668(param_4,auStack_60);
              if (*param_4 == 0) {
                iVar2 = 0;
              }
              param_4[2] = iVar2;
            }
          }
        }
LAB_0100a4c8:
        FUN_01009f50(auStack_60,auStack_50,auStack_40,auStack_30,0);
      }
      else {
        iVar1 = 1;
      }
    }
  }
  return iVar1;
}



/* @ 0x100a668  FUN_0100a668 */

void FUN_0100a668(undefined4 *param_1,undefined4 *param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  uVar3 = *param_1;
  uVar4 = param_1[1];
  uVar5 = param_2[1];
  uVar1 = param_1[2];
  uVar2 = param_1[3];
  *param_1 = *param_2;
  param_1[1] = uVar5;
  uVar5 = param_2[3];
  param_1[2] = param_2[2];
  param_1[3] = uVar5;
  *param_2 = uVar3;
  param_2[1] = uVar4;
  param_2[2] = uVar1;
  param_2[3] = uVar2;
  return;
}



/* @ 0x100a6a8  FUN_0100a6a8 */

int FUN_0100a6a8(undefined4 param_1,int param_2,int *param_3,undefined4 param_4)

{
  int iVar1;
  undefined1 auStack_38 [16];
  undefined1 auStack_28 [16];
  
  if (param_3[2] != 1) {
    if (*(int *)(param_2 + 8) == 1) {
      iVar1 = FUN_0100aa40(auStack_38);
      if (iVar1 != 0) {
        return iVar1;
      }
      iVar1 = FUN_0100ac14(param_1,param_3,auStack_38);
      if ((iVar1 == 0) && (iVar1 = FUN_0100aa40(auStack_28), iVar1 == 0)) {
        iVar1 = FUN_01009dd0(param_2,auStack_28);
        if (iVar1 == 0) {
          iVar1 = FUN_0100a6a8(auStack_38,auStack_28,param_3,param_4);
        }
        FUN_01009f50(auStack_38,auStack_28,0);
        return iVar1;
      }
      FUN_01009eec(auStack_38);
      return iVar1;
    }
    if ((0 < *param_3) && ((*(uint *)param_3[3] & 1) != 0)) {
      iVar1 = FUN_0100a7bc();
      return iVar1;
    }
  }
  return -3;
}



/* @ 0x100a7bc  FUN_0100a7bc */

int FUN_0100a7bc(undefined4 param_1,int *param_2,undefined4 param_3,undefined4 param_4,int param_5)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  undefined4 local_5c;
  undefined1 auStack_58 [16];
  undefined1 auStack_48 [16];
  undefined1 auStack_38 [20];
  
  FUN_0100a180(param_2);
  iVar1 = FUN_0100aa40(auStack_38);
  if (iVar1 == 0) {
    iVar1 = FUN_0100aa40(auStack_38);
    if (iVar1 == 0) {
      if (param_5 == 0) {
        iVar1 = FUN_0100b530(param_3,&local_5c);
        if ((iVar1 == 0) && (iVar1 = FUN_0100aa40(auStack_58), iVar1 == 0)) {
          iVar1 = FUN_0100b2b8(auStack_58,param_3);
          if ((iVar1 == 0) &&
             ((iVar1 = FUN_0100b810(param_1,auStack_58,param_3,auStack_38), iVar1 == 0 &&
              (iVar1 = FUN_0100a0bc(auStack_38), iVar1 == 0)))) {
            iVar2 = *param_2;
            uVar3 = 0;
            while (iVar2 = iVar2 + -1, iVar2 != -1) {
              iVar5 = 0x1c;
              uVar7 = *(uint *)(param_2[3] + iVar2 * 4);
              uVar4 = uVar3;
              do {
                uVar6 = uVar7 >> 0x1b & 1;
                uVar3 = uVar4 | uVar6;
                if (uVar3 != 0) {
                  if ((uVar4 & ~(uVar7 >> 0x1b)) == 0) {
                    iVar1 = FUN_0100ba2c(auStack_58);
                    if ((iVar1 != 0) ||
                       (iVar1 = FUN_0100b3ac(auStack_58,param_3,local_5c), iVar1 != 0))
                    goto LAB_0100a99c;
                    iVar1 = FUN_0100b7b0(auStack_58,auStack_48 + uVar6 * 0x10);
                  }
                  else {
                    iVar1 = FUN_0100ba2c(auStack_58);
                  }
                  if ((iVar1 != 0) ||
                     (iVar1 = FUN_0100b3ac(auStack_58,param_3,local_5c), iVar1 != 0))
                  goto LAB_0100a99c;
                }
                iVar5 = iVar5 + -1;
                uVar7 = uVar7 << 1;
                uVar4 = uVar3;
              } while (iVar5 != 0);
            }
            iVar1 = FUN_0100b3ac(auStack_58,param_3,local_5c);
            if (iVar1 == 0) {
              FUN_0100a668(auStack_58,param_4);
            }
          }
LAB_0100a99c:
          FUN_01009eec(auStack_58);
        }
      }
      else {
        iVar1 = -3;
      }
      FUN_01009eec(auStack_38);
    }
    FUN_01009eec(auStack_38);
  }
  return iVar1;
}



/* @ 0x100a9b8  FUN_0100a9b8 */

undefined4 FUN_0100a9b8(int param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  
  if (param_2 <= *(int *)(param_1 + 4)) {
    return 0;
  }
  param_2 = param_2 + 2;
  iVar1 = mspace_realloc(*(undefined4 *)(param_1 + 0xc),param_2 * 4);
  if (iVar1 == 0) {
    uVar2 = 0xfffffffe;
  }
  else {
    iVar3 = *(int *)(param_1 + 4);
    *(int *)(param_1 + 4) = param_2;
    *(int *)(param_1 + 0xc) = iVar1;
    if (iVar3 < param_2) {
      puVar4 = (undefined4 *)(iVar1 + iVar3 * 4);
      do {
        puVar5 = puVar4 + 1;
        *puVar4 = 0;
        puVar4 = puVar5;
      } while (puVar5 != (undefined4 *)(iVar1 + param_2 * 4));
    }
    uVar2 = 0;
  }
  return uVar2;
}



/* @ 0x100aa40  FUN_0100aa40 */

undefined4 FUN_0100aa40(undefined4 *param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_0100d2f4(4);
  param_1[3] = puVar1;
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0;
    *param_1 = 0;
    param_1[1] = 1;
    param_1[2] = 0;
    return 0;
  }
  return 0xfffffffe;
}



/* @ 0x100aa98  FUN_0100aa98 */

void FUN_0100aa98(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = FUN_0100aa40();
  if (iVar1 != 0) {
    return;
  }
  FUN_0100a0bc(param_2,param_1);
  return;
}



/* @ 0x100aae4  FUN_0100aae4 */

undefined4 FUN_0100aae4(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int *local_28;
  int local_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  local_28 = &local_c;
  if (param_1 != 0) {
    iVar2 = 0;
    iVar1 = param_1;
    local_c = param_2;
    uStack_8 = param_3;
    uStack_4 = param_4;
    do {
      iVar1 = FUN_0100aa40(iVar1);
      if (iVar1 != 0) {
        iVar1 = iVar2 + -1;
        if (iVar2 != 0) {
          piVar3 = &local_c;
          do {
            iVar1 = iVar1 + -1;
            FUN_01009eec(param_1);
            param_1 = *piVar3;
            piVar3 = piVar3 + 1;
          } while (iVar1 != -1);
        }
        return 0xfffffffe;
      }
      iVar2 = iVar2 + 1;
      iVar1 = *local_28;
      local_28 = local_28 + 1;
    } while (iVar1 != 0);
  }
  return 0;
}



/* @ 0x100aba0  FUN_0100aba0 */

undefined4 FUN_0100aba0(undefined4 *param_1,int param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *extraout_r3;
  undefined4 *puVar3;
  int iVar4;
  
  iVar4 = param_2 + 2;
  puVar1 = (undefined4 *)FUN_0100d2f4(iVar4 * 4);
  param_1[3] = puVar1;
  if (puVar1 == (undefined4 *)0x0) {
    uVar2 = 0xfffffffe;
  }
  else {
    puVar3 = extraout_r3;
    if (0 < iVar4) {
      puVar3 = puVar1 + -1;
      puVar1 = puVar1 + param_2 + 1;
    }
    *param_1 = 0;
    param_1[1] = iVar4;
    param_1[2] = 0;
    if (0 < iVar4) {
      do {
        puVar3 = puVar3 + 1;
        *puVar3 = 0;
      } while (puVar3 != puVar1);
    }
    uVar2 = 0;
  }
  return uVar2;
}



/* @ 0x100ac14  FUN_0100ac14 */

int FUN_0100ac14(undefined4 param_1,int *param_2,undefined4 param_3)

{
  int iVar1;
  int local_98 [3];
  uint *local_8c;
  int local_88 [3];
  uint *local_7c;
  int local_78 [3];
  uint *local_6c;
  int local_68 [3];
  uint *local_5c;
  int local_58 [3];
  uint *local_4c;
  int local_48 [3];
  uint *local_3c;
  int local_38 [3];
  uint *local_2c;
  int local_28 [3];
  uint *local_1c;
  
  if ((param_2[2] == 1) || (*param_2 == 0)) {
    return -3;
  }
  if ((param_2[2] == 1) || (*param_2 == 0)) {
    return -3;
  }
  iVar1 = FUN_0100aae4(local_98,local_88,local_78,local_68,local_58,local_48,local_38,local_28,0);
  if (iVar1 != 0) {
    return iVar1;
  }
  iVar1 = FUN_0100b218(param_1,param_2,local_98);
  if ((iVar1 == 0) && (iVar1 = FUN_0100a0bc(param_2,local_88), iVar1 == 0)) {
    if ((local_98[0] < 1) ||
       ((((*local_8c & 1) != 0 || (local_88[0] < 1)) || ((*local_7c & 1) != 0)))) {
      iVar1 = FUN_0100a0bc(local_98,local_78);
      if ((iVar1 == 0) && (iVar1 = FUN_0100a0bc(local_88,local_68), iVar1 == 0)) {
        FUN_0100b9ec(local_58,1);
        FUN_0100b9ec(local_28,1);
        do {
          while ((0 < local_78[0] && ((*local_6c & 1) == 0))) {
            iVar1 = FUN_0100a1c4(local_78);
            if (((iVar1 != 0) ||
                ((((0 < local_58[0] && ((*local_4c & 1) != 0)) ||
                  ((0 < local_48[0] && ((*local_3c & 1) != 0)))) &&
                 ((iVar1 = FUN_01009e08(local_58,local_88), iVar1 != 0 ||
                  (iVar1 = FUN_0100ba50(local_48,local_98), iVar1 != 0)))))) ||
               ((iVar1 = FUN_0100a1c4(local_58), iVar1 != 0 ||
                (iVar1 = FUN_0100a1c4(local_48), iVar1 != 0)))) goto LAB_0100ad30;
          }
          while ((0 < local_68[0] && ((*local_5c & 1) == 0))) {
            iVar1 = FUN_0100a1c4(local_68);
            if ((iVar1 != 0) ||
               (((((0 < local_38[0] && ((*local_2c & 1) != 0)) ||
                  ((0 < local_28[0] && ((*local_1c & 1) != 0)))) &&
                 ((iVar1 = FUN_01009e08(local_38,local_88), iVar1 != 0 ||
                  (iVar1 = FUN_0100ba50(local_28,local_98), iVar1 != 0)))) ||
                ((iVar1 = FUN_0100a1c4(local_38), iVar1 != 0 ||
                 (iVar1 = FUN_0100a1c4(local_28), iVar1 != 0)))))) goto LAB_0100ad30;
          }
          iVar1 = FUN_01009fa4(local_78);
          if (iVar1 == -1) {
            iVar1 = FUN_0100ba50(local_68,local_78);
            if ((iVar1 != 0) || (iVar1 = FUN_0100ba50(local_38,local_58), iVar1 != 0)) break;
            iVar1 = FUN_0100ba50(local_28,local_48);
          }
          else {
            iVar1 = FUN_0100ba50(local_78,local_68);
            if ((iVar1 != 0) || (iVar1 = FUN_0100ba50(local_58,local_38), iVar1 != 0)) break;
            iVar1 = FUN_0100ba50(local_48,local_28);
          }
          if (iVar1 != 0) break;
          if (local_78[0] == 0) {
            iVar1 = FUN_01009fec(local_68,1);
            if (iVar1 != 0) goto LAB_0100ad2c;
            goto LAB_0100afd4;
          }
        } while( true );
      }
    }
    else {
LAB_0100ad2c:
      iVar1 = -3;
    }
  }
  goto LAB_0100ad30;
  while (iVar1 = FUN_01009e08(local_38,param_2), iVar1 == 0) {
LAB_0100afd4:
    iVar1 = FUN_01009fec(local_38,0);
    if (iVar1 != -1) goto LAB_0100b004;
  }
  goto LAB_0100ad30;
  while (iVar1 = FUN_0100ba50(local_38,param_2), iVar1 == 0) {
LAB_0100b004:
    iVar1 = FUN_0100a030(local_38,param_2);
    if (iVar1 == -1) {
      iVar1 = 0;
      FUN_0100a668(local_38,param_3);
      break;
    }
  }
LAB_0100ad30:
  FUN_01009f50(local_98,local_88,local_78,local_68,local_58,local_48,local_38,local_28,0);
  return iVar1;
}



/* @ 0x100b080  FUN_0100b080 */

int FUN_0100b080(int *param_1,int param_2)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined4 *puVar6;
  int unaff_lr;
  
  if (0 < param_2) {
    iVar3 = *param_1;
    iVar1 = iVar3 + param_2;
    if (param_1[1] < iVar1) {
      unaff_lr = 0x100b110;
      iVar1 = FUN_0100a9b8();
      if (iVar1 != 0) {
        return iVar1;
      }
      iVar3 = *param_1;
      iVar1 = param_2 + iVar3;
    }
    puVar6 = (undefined4 *)param_1[3];
    iVar5 = iVar1 + -1;
    *param_1 = iVar1;
    if (param_2 <= iVar5) {
      unaff_lr = param_2 + -1;
    }
    if (param_2 <= iVar5) {
      puVar2 = puVar6 + iVar1;
      puVar4 = puVar6 + iVar3 + 0x3fffffff;
      do {
        iVar5 = iVar5 + -1;
        puVar2 = puVar2 + -1;
        *puVar2 = *puVar4;
        puVar4 = puVar4 + -1;
      } while (iVar5 != unaff_lr);
    }
    iVar1 = 0;
    do {
      iVar1 = iVar1 + 1;
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    } while (param_2 != iVar1);
  }
  return 0;
}



/* @ 0x100b124  FUN_0100b124 */

int FUN_0100b124(int *param_1,uint param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar7;
  undefined4 *puVar6;
  
  if ((int)param_2 < 1) {
    iVar2 = 0;
    FUN_0100bbb0(param_3);
  }
  else {
    if (*param_1 * 0x1c <= (int)param_2) {
      iVar2 = FUN_0100a0bc(param_1,param_3);
      return iVar2;
    }
    iVar2 = FUN_0100a0bc(param_1,param_3);
    if (iVar2 == 0) {
      iVar3 = *param_3;
      iVar7 = param_3[3];
      iVar1 = (int)param_2 / 0x1c;
      iVar4 = iVar1;
      if (iVar1 * 0x1c - param_2 != 0) {
        iVar4 = iVar1 + 1;
      }
      if (iVar4 < iVar3) {
        puVar5 = (undefined4 *)(iVar7 + iVar4 * 4);
        do {
          puVar6 = puVar5 + 1;
          *puVar5 = 0;
          puVar5 = puVar6;
        } while (puVar6 != (undefined4 *)(iVar7 + iVar3 * 4));
      }
      *(uint *)(iVar7 + iVar1 * 4) = *(uint *)(iVar7 + iVar1 * 4) & ~(-1 << param_2 % 0x1c);
      FUN_01009e90(param_3);
    }
  }
  return iVar2;
}



/* @ 0x100b218  FUN_0100b218 */

int FUN_0100b218(undefined4 param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  undefined1 auStack_28 [8];
  int local_20;
  
  iVar1 = FUN_0100aa40(auStack_28);
  if (iVar1 == 0) {
    iVar1 = FUN_0100a408(param_1,param_2,0,auStack_28);
    if (iVar1 == 0) {
      if (local_20 == *(int *)(param_2 + 8)) {
        FUN_0100a668(auStack_28,param_3);
      }
      else {
        iVar1 = FUN_01009e08(param_2,auStack_28,param_3);
      }
    }
    FUN_01009eec(auStack_28);
  }
  return iVar1;
}



/* @ 0x100b2b8  FUN_0100b2b8 */

int FUN_0100b2b8(undefined4 param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_0100a180(param_2);
  if (*param_2 < 2) {
    iVar1 = 0;
    FUN_0100b9ec(param_1,1);
  }
  else {
    iVar2 = FUN_01009d54(param_1,iVar1 % 0x1c + (*param_2 + -1) * 0x1c + -1);
    if (iVar2 != 0) {
      return iVar2;
    }
    iVar1 = iVar1 % 0x1c + -1;
  }
  while( true ) {
    iVar2 = FUN_0100b58c(param_1,param_1);
    iVar1 = iVar1 + 1;
    if (iVar2 != 0) {
      return iVar2;
    }
    iVar2 = FUN_0100a030(param_1,param_2);
    if ((iVar2 != -1) && (iVar2 = al_pcie_read_config(param_1,param_2,param_1), iVar2 != 0)) break;
    if (iVar1 == 0x1c) {
      return 0;
    }
  }
  return iVar2;
}



/* @ 0x100b3ac  FUN_0100b3ac */

int FUN_0100b3ac(int *param_1,int *param_2,int param_3)

{
  longlong lVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  uint *puVar7;
  uint *puVar8;
  uint *puVar9;
  uint *puVar10;
  int iVar11;
  undefined8 uVar12;
  
  iVar4 = *param_2 * 2 + 1;
  if ((iVar4 <= param_1[1]) || (iVar11 = FUN_0100a9b8(param_1,iVar4), iVar11 == 0)) {
    *param_1 = iVar4;
    iVar4 = *param_2;
    if (0 < iVar4) {
      puVar7 = (uint *)param_1[3];
      puVar10 = (uint *)param_2[3];
      puVar8 = puVar7;
      do {
        puVar9 = puVar8 + 1;
        uVar3 = *puVar8;
        uVar2 = 0;
        iVar11 = 0;
        uVar6 = param_3 * uVar3;
        puVar5 = puVar10;
        while( true ) {
          iVar11 = iVar11 + 1;
          uVar12 = VectorShiftRight(CONCAT44(uVar3,uVar3),0x20);
          lVar1 = (ulonglong)*puVar5 * (ulonglong)(uVar6 & 0xfffffff) +
                  CONCAT44((int)((ulonglong)uVar12 >> 0x20) + (uint)CARRY4((uint)uVar12,uVar2),
                           (uint)uVar12 + uVar2);
          uVar3 = (uint)lVar1;
          uVar2 = uVar3 >> 0x1c | (int)((ulonglong)lVar1 >> 0x20) << 4;
          *puVar8 = uVar3 & 0xfffffff;
          if (iVar4 == iVar11) break;
          uVar3 = puVar8[1];
          puVar5 = puVar5 + 1;
          puVar8 = puVar8 + 1;
        }
        puVar8 = puVar9 + iVar4 + -1;
        for (; uVar2 != 0; uVar2 = uVar2 + uVar3 >> 0x1c) {
          uVar3 = *puVar8;
          *puVar8 = uVar2 + uVar3 & 0xfffffff;
          puVar8 = puVar8 + 1;
        }
        puVar8 = puVar9;
      } while (puVar7 + iVar4 != puVar9);
    }
    FUN_01009e90(param_1);
    FUN_0100b960(param_1,*param_2);
    iVar4 = FUN_0100a030(param_1,param_2);
    if (iVar4 != -1) {
      iVar4 = al_pcie_read_config(param_1,param_2);
      return iVar4;
    }
    iVar11 = 0;
  }
  return iVar11;
}



/* @ 0x100b530  FUN_0100b530 */

undefined4 FUN_0100b530(int param_1,uint *param_2)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = **(uint **)(param_1 + 0xc);
  if ((uVar2 & 1) != 0) {
    iVar1 = ((uVar2 + 2) * 2 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) + uVar2;
    iVar1 = iVar1 * (2 - iVar1 * uVar2);
    iVar1 = iVar1 * (2 - iVar1 * uVar2);
    *param_2 = (iVar1 * uVar2 + -2) * iVar1 & 0xfffffff;
    return 0;
  }
  return 0xfffffffd;
}



/* @ 0x100b58c  FUN_0100b58c */

int FUN_0100b58c(int *param_1,int *param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  int iVar6;
  int iVar7;
  uint *puVar8;
  uint uVar9;
  
  iVar2 = *param_1;
  if (param_2[1] <= iVar2) {
    iVar2 = FUN_0100a9b8(param_2,iVar2 + 1);
    if (iVar2 != 0) {
      return iVar2;
    }
    iVar2 = *param_1;
  }
  iVar7 = *param_2;
  puVar4 = (uint *)param_1[3];
  *param_2 = iVar2;
  puVar8 = (uint *)param_2[3];
  if (0 < iVar2) {
    puVar5 = puVar8;
    iVar3 = 0;
    uVar9 = 0;
    do {
      iVar6 = iVar3;
      iVar3 = iVar6 + 1;
      uVar1 = *puVar4 >> 0x1b;
      *puVar5 = uVar9 | (*puVar4 & 0x7ffffff) << 1;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
      uVar9 = uVar1;
    } while (iVar2 != iVar3);
    iVar2 = iVar3;
    if (uVar1 != 0) {
      puVar8[iVar3] = 1;
      iVar2 = iVar6 + 2;
      *param_2 = iVar2;
    }
  }
  puVar8 = puVar8 + iVar2;
  if (iVar2 < iVar7) {
    do {
      iVar2 = iVar2 + 1;
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    } while (iVar7 != iVar2);
  }
  param_2[2] = param_1[2];
  return 0;
}



/* @ 0x100b684  FUN_0100b684 */

int FUN_0100b684(int *param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint *puVar7;
  uint *puVar8;
  
  if ((param_3 == param_1) || (iVar1 = FUN_0100a0bc(param_1,param_3), iVar1 == 0)) {
    iVar1 = param_2 / 0x1c + *param_3;
    if (((iVar1 < param_3[1]) ||
        (iVar1 = FUN_0100a9b8(param_3,iVar1 + 1,param_3[1],param_2 * -0x6db6db6d), iVar1 == 0)) &&
       ((param_2 < 0x1c || (iVar1 = FUN_0100b080(param_3,param_2 / 0x1c), iVar1 == 0)))) {
      uVar4 = param_2 % 0x1c;
      if (uVar4 != 0) {
        iVar1 = *param_3;
        puVar7 = (uint *)param_3[3];
        if (0 < iVar1) {
          uVar5 = 0;
          iVar3 = 0;
          puVar8 = puVar7;
          do {
            iVar2 = iVar3;
            iVar3 = iVar2 + 1;
            uVar6 = uVar5 | *puVar8 << (uVar4 & 0xff);
            uVar5 = ~(-1 << (uVar4 & 0xff)) & *puVar8 >> (0x1c - uVar4 & 0xff);
            *puVar8 = uVar6 & 0xfffffff;
            puVar8 = puVar8 + 1;
          } while (iVar3 != iVar1);
          if (uVar5 != 0) {
            *param_3 = iVar2 + 2;
            puVar7[iVar3] = uVar5;
          }
        }
      }
      FUN_01009e90(param_3);
      iVar1 = 0;
    }
  }
  return iVar1;
}



/* @ 0x100b7b0  FUN_0100b7b0 */

void FUN_0100b7b0(int param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = *(int *)(param_1 + 8);
  iVar3 = *(int *)(param_2 + 8);
  FUN_0100bd90();
  if (*param_3 < 1) {
    iVar1 = 0;
  }
  else {
    iVar1 = 0;
    if (iVar2 != iVar3) {
      iVar1 = 1;
    }
  }
  param_3[2] = iVar1;
  return;
}



/* @ 0x100b810  FUN_0100b810 */

int FUN_0100b810(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined1 auStack_28 [16];
  
  iVar1 = FUN_0100aa40(auStack_28);
  if (iVar1 == 0) {
    iVar1 = FUN_0100b7b0(param_1,param_2,auStack_28);
    if (iVar1 == 0) {
      iVar1 = FUN_0100b218(auStack_28,param_3,param_4);
    }
    FUN_01009eec(auStack_28);
  }
  return iVar1;
}



/* @ 0x100b898  FUN_0100b898 */

int FUN_0100b898(int *param_1,byte *param_2,int param_3)

{
  int iVar1;
  byte *pbVar2;
  byte *pbVar3;
  
  if ((param_1[1] < 2) && (iVar1 = FUN_0100a9b8(param_1,2), iVar1 != 0)) {
    return iVar1;
  }
  FUN_0100bbb0(param_1);
  if (0 < param_3) {
    pbVar3 = param_2;
    do {
      iVar1 = FUN_0100b684(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,param_1);
      if (iVar1 != 0) {
        return iVar1;
      }
      pbVar2 = pbVar3 + 1;
      iVar1 = *param_1;
      *(uint *)param_1[3] = *(uint *)param_1[3] | (uint)*pbVar3;
      *param_1 = iVar1 + 1;
      pbVar3 = pbVar2;
    } while (param_2 + param_3 != pbVar2);
  }
  FUN_01009e90(param_1);
  return 0;
}



/* @ 0x100b960  FUN_0100b960 */

void FUN_0100b960(int *param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  
  if (param_2 < 1) {
    return;
  }
  iVar5 = *param_1;
  if (iVar5 <= param_2) {
    FUN_0100bbb0();
    return;
  }
  iVar2 = param_1[3];
  iVar6 = iVar5 - param_2;
  iVar4 = 0;
  puVar1 = (undefined4 *)(iVar2 + -4);
  puVar3 = (undefined4 *)(iVar2 + param_2 * 4);
  do {
    iVar4 = iVar4 + 1;
    puVar1 = puVar1 + 1;
    *puVar1 = *puVar3;
    puVar3 = puVar3 + 1;
  } while (iVar4 != iVar6);
  puVar3 = (undefined4 *)(iVar2 + iVar6 * 4);
  iVar2 = iVar6;
  if (iVar6 < iVar5) {
    do {
      iVar2 = iVar2 + 1;
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    } while (iVar5 != iVar2);
  }
  *param_1 = iVar6;
  return;
}



/* @ 0x100b9ec  FUN_0100b9ec */

void FUN_0100b9ec(uint *param_1,uint param_2)

{
  FUN_0100bbb0();
  *(uint *)param_1[3] = param_2 & 0xfffffff;
  *param_1 = (uint)((param_2 & 0xfffffff) != 0);
  return;
}



/* @ 0x100ba2c  FUN_0100ba2c */

void FUN_0100ba2c(undefined4 param_1,int param_2)

{
  FUN_0100bedc();
  *(undefined4 *)(param_2 + 8) = 0;
  return;
}



/* @ 0x100ba50  FUN_0100ba50 */

int FUN_0100ba50(int *param_1,int *param_2,int *param_3)

{
  int *piVar1;
  uint *puVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int *piVar10;
  uint *puVar11;
  
  iVar6 = param_1[2];
  if (iVar6 != param_2[2]) {
    param_3[2] = iVar6;
    iVar7 = *param_2;
    iVar8 = *param_1;
    iVar6 = iVar7;
    piVar1 = param_1;
    if (iVar8 <= iVar7) {
      iVar6 = iVar8;
      iVar8 = iVar7;
      piVar1 = param_2;
    }
    iVar7 = iVar8 + 1;
    if ((iVar8 < param_3[1]) || (iVar3 = FUN_0100a9b8(param_3,iVar7), iVar3 == 0)) {
      iVar3 = 0;
      iVar9 = *param_3;
      piVar4 = (int *)param_1[3];
      if (iVar6 < 1) {
        param_1 = (int *)0x0;
      }
      piVar10 = (int *)param_2[3];
      *param_3 = iVar7;
      puVar2 = (uint *)param_3[3];
      if (0 < iVar6) {
        param_1 = (int *)0x0;
        puVar11 = puVar2;
        do {
          iVar3 = iVar3 + 1;
          uVar5 = *piVar4 + *piVar10 + (int)param_1;
          param_1 = (int *)(uVar5 >> 0x1c);
          *puVar11 = uVar5 & 0xfffffff;
          piVar4 = piVar4 + 1;
          piVar10 = piVar10 + 1;
          puVar11 = puVar11 + 1;
        } while (iVar6 != iVar3);
        puVar2 = puVar2 + iVar6;
      }
      if ((iVar6 != iVar8) && (iVar3 < iVar8)) {
        iVar6 = piVar1[3];
        piVar1 = (int *)(iVar6 + iVar3 * 4);
        puVar11 = puVar2;
        do {
          piVar4 = piVar1 + 1;
          uVar5 = *piVar1 + (int)param_1;
          param_1 = (int *)(uVar5 >> 0x1c);
          *puVar11 = uVar5 & 0xfffffff;
          piVar1 = piVar4;
          puVar11 = puVar11 + 1;
        } while ((int *)(iVar6 + iVar8 * 4) != piVar4);
        puVar2 = puVar2 + (iVar8 - iVar3);
      }
      *puVar2 = (uint)param_1;
      if (iVar7 < iVar9) {
        do {
          puVar2 = puVar2 + 1;
          iVar7 = iVar7 + 1;
          *puVar2 = 0;
        } while (iVar9 != iVar7);
      }
      FUN_01009e90(param_3);
      iVar3 = 0;
    }
    return iVar3;
  }
  iVar8 = FUN_0100a030();
  if (iVar8 == -1) {
    param_3[2] = (uint)(iVar6 == 0);
    piVar1 = param_2;
  }
  else {
    param_3[2] = iVar6;
    piVar1 = param_1;
    param_1 = param_2;
  }
  iVar6 = al_pcie_read_config(piVar1,param_1,param_3);
  return iVar6;
}



/* @ 0x100bae8  FUN_0100bae8 */

int FUN_0100bae8(undefined4 param_1,int param_2)

{
  int iVar1;
  undefined1 *puVar2;
  int local_28 [3];
  undefined4 *local_1c;
  
  iVar1 = FUN_0100aa98(local_28,param_1);
  if (iVar1 == 0) {
    puVar2 = (undefined1 *)(param_2 + -1);
    do {
      if (local_28[0] == 0) {
        FUN_0100bbe4(param_2,puVar2 + (1 - param_2));
        iVar1 = 0;
        break;
      }
      puVar2 = puVar2 + 1;
      *puVar2 = (char)*local_1c;
      iVar1 = FUN_0100a2a4(local_28,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,local_28);
    } while (iVar1 == 0);
    FUN_01009eec(local_28);
  }
  return iVar1;
}



/* @ 0x100bb80  FUN_0100bb80 */

int FUN_0100bb80(void)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = FUN_0100a180();
  uVar2 = uVar1;
  if ((int)uVar1 < 0) {
    uVar2 = uVar1 + 7;
  }
  return (uint)((uVar1 & 7) != 0) + ((int)uVar2 >> 3);
}



/* @ 0x100bbb0  FUN_0100bbb0 */

void FUN_0100bbb0(undefined4 *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  
  iVar1 = param_1[1];
  iVar3 = 0;
  puVar2 = (undefined4 *)param_1[3];
  *param_1 = 0;
  param_1[2] = 0;
  if (0 < iVar1) {
    do {
      iVar3 = iVar3 + 1;
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    } while (iVar3 != iVar1);
  }
  return;
}



/* @ 0x100bbe4  FUN_0100bbe4 */

void FUN_0100bbe4(int param_1,int param_2)

{
  undefined1 uVar1;
  undefined1 *puVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  
  if (param_2 < 2) {
    return;
  }
  puVar5 = (undefined1 *)(param_1 + param_2);
  puVar2 = (undefined1 *)(param_1 + -1);
  do {
    puVar5 = puVar5 + -1;
    puVar3 = puVar2 + 1;
    uVar1 = *puVar3;
    puVar4 = puVar2 + (2 - param_1);
    *puVar3 = *puVar5;
    *puVar5 = uVar1;
    puVar2 = puVar3;
  } while ((int)puVar4 < (-2 - (int)puVar3) + param_1 + param_2);
  return;
}



/* @ 0x100bd90  FUN_0100bd90 */

int FUN_0100bd90(int *param_1,int *param_2,undefined4 param_3,int param_4)

{
  longlong lVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  uint *puVar8;
  uint *puVar9;
  uint uVar10;
  int iVar11;
  int iVar12;
  uint *puVar13;
  int iVar14;
  undefined8 uVar15;
  int local_38 [3];
  uint *local_2c;
  
  iVar2 = FUN_0100aba0(local_38,param_4);
  if (iVar2 == 0) {
    iVar7 = *param_1;
    if (0 < iVar7) {
      iVar12 = *param_2;
      puVar8 = (uint *)param_2[3];
      puVar13 = (uint *)(param_1[3] + -4);
      iVar11 = 0;
      do {
        puVar13 = puVar13 + 1;
        uVar10 = *puVar13;
        iVar14 = param_4 - iVar11;
        if (iVar12 <= param_4 - iVar11) {
          iVar14 = iVar12;
        }
        if (iVar14 < 1) {
          uVar5 = 0;
          puVar9 = local_2c;
          iVar14 = iVar11;
        }
        else {
          iVar4 = 0;
          puVar3 = local_2c + -1;
          uVar5 = 0;
          puVar9 = puVar8;
          do {
            iVar4 = iVar4 + 1;
            puVar3 = puVar3 + 1;
            uVar15 = VectorShiftRight(CONCAT44(uVar5,uVar5),0x20);
            lVar1 = (ulonglong)*puVar9 * (ulonglong)uVar10 +
                    CONCAT44((int)((ulonglong)uVar15 >> 0x20) + (uint)CARRY4((uint)uVar15,*puVar3),
                             (uint)uVar15 + *puVar3);
            uVar6 = (uint)lVar1;
            uVar5 = uVar6 >> 0x1c | (int)((ulonglong)lVar1 >> 0x20) << 4;
            *puVar3 = uVar6 & 0xfffffff;
            puVar9 = puVar9 + 1;
          } while (iVar14 != iVar4);
          puVar9 = local_2c + iVar14;
          iVar14 = iVar14 + iVar11;
        }
        iVar11 = iVar11 + 1;
        local_2c = local_2c + 1;
        if (iVar14 < param_4) {
          *puVar9 = uVar5;
        }
      } while (iVar7 != iVar11);
    }
    local_38[0] = param_4;
    FUN_01009e90(local_38);
    FUN_0100a668(local_38,param_3);
    FUN_01009eec(local_38);
  }
  return iVar2;
}



/* @ 0x100bedc  FUN_0100bedc */

int FUN_0100bedc(int *param_1,undefined4 param_2)

{
  longlong lVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint *puVar9;
  uint *puVar10;
  int iVar11;
  uint *puVar12;
  int iVar13;
  uint uVar14;
  uint *puVar15;
  uint *puVar16;
  uint *puVar17;
  undefined8 uVar18;
  int local_38 [3];
  int local_2c;
  
  iVar13 = *param_1;
  iVar8 = iVar13 * 2 + 1;
  iVar2 = FUN_0100aba0(local_38,iVar8);
  if (iVar2 == 0) {
    if (0 < iVar13) {
      puVar12 = (uint *)(param_1[3] + -4);
      puVar9 = (uint *)(local_2c + iVar13 * 4);
      puVar10 = (uint *)(local_2c + 4);
      iVar11 = 0;
      do {
        puVar12 = puVar12 + 1;
        iVar11 = iVar11 + 1;
        lVar1 = (ulonglong)*puVar12 * (ulonglong)*puVar12 + (ulonglong)puVar10[-1];
        uVar5 = (uint)lVar1;
        uVar6 = uVar5 >> 0x1c | (int)((ulonglong)lVar1 >> 0x20) << 4;
        puVar10[-1] = uVar5 & 0xfffffff;
        uVar5 = *puVar12;
        puVar15 = puVar10;
        puVar17 = puVar12;
        puVar16 = puVar10;
        if (iVar11 < iVar13) {
          do {
            lVar1 = (ulonglong)puVar17[1] * (ulonglong)uVar5;
            uVar3 = (uint)lVar1;
            uVar4 = uVar3 * 2 + uVar6;
            uVar14 = *puVar15;
            uVar7 = uVar4 + uVar14;
            puVar16 = puVar15 + 1;
            *puVar15 = uVar7 & 0xfffffff;
            uVar6 = uVar7 >> 0x1c |
                    ((int)((ulonglong)lVar1 >> 0x20) * 2 + (uint)CARRY4(uVar3,uVar3) +
                     (uint)CARRY4(uVar3 * 2,uVar6) + (uint)CARRY4(uVar4,uVar14)) * 0x10;
            puVar15 = puVar16;
            puVar17 = puVar17 + 1;
          } while (puVar16 != puVar9);
        }
        while (uVar6 != 0) {
          uVar5 = *puVar16;
          uVar18 = VectorShiftRight(CONCAT44(uVar6,uVar6),0x20);
          *puVar16 = uVar5 + uVar6 & 0xfffffff;
          puVar16 = puVar16 + 1;
          uVar6 = (uint)uVar18 + uVar5 >> 0x1c |
                  ((int)((ulonglong)uVar18 >> 0x20) + (uint)CARRY4((uint)uVar18,uVar5)) * 0x10;
        }
        puVar10 = puVar10 + 2;
        puVar9 = puVar9 + 1;
      } while (iVar13 != iVar11);
    }
    local_38[0] = iVar8;
    FUN_01009e90(local_38);
    FUN_0100a668(local_38,param_2);
    FUN_01009eec(local_38);
  }
  return iVar2;
}



/* @ 0x100c044  FUN_0100c044 */

int al_pcie_read_config(uint *param_1,uint *param_2,uint *param_3)

{
  uint *puVar1;
  uint *puVar2;
  int *piVar3;
  uint uVar4;
  uint *puVar5;
  uint *puVar6;
  uint *puVar7;
  uint *puVar8;
  int *piVar9;
  uint *puVar10;
  int *piVar11;
  undefined8 uVar12;
  
  puVar5 = (uint *)*param_1;
  puVar6 = (uint *)*param_2;
  puVar2 = param_2;
  if ((int)param_3[1] < (int)puVar5) {
    uVar12 = FUN_0100a9b8(param_3,puVar5);
    puVar2 = (uint *)((ulonglong)uVar12 >> 0x20);
    if ((int)uVar12 != 0) {
      return (int)uVar12;
    }
  }
  puVar8 = (uint *)*param_3;
  puVar7 = puVar6;
  if ((int)puVar6 < 1) {
    puVar7 = (uint *)0x0;
  }
  piVar11 = (int *)param_1[3];
  if ((int)puVar6 < 1) {
    puVar2 = puVar7;
  }
  piVar3 = (int *)param_2[3];
  *param_3 = (uint)puVar5;
  puVar10 = (uint *)param_3[3];
  if (0 < (int)puVar6) {
    puVar1 = (uint *)0x0;
    puVar2 = (uint *)0x0;
    piVar9 = piVar11;
    puVar6 = puVar10;
    do {
      puVar1 = (uint *)((int)puVar1 + 1);
      uVar4 = (*piVar9 - (int)puVar2) - *piVar3;
      puVar2 = (uint *)(uVar4 >> 0x1f);
      *puVar6 = uVar4 & 0xfffffff;
      piVar3 = piVar3 + 1;
      piVar9 = piVar9 + 1;
      puVar6 = puVar6 + 1;
    } while (puVar7 != puVar1);
    piVar11 = piVar11 + (int)puVar7;
    puVar10 = puVar10 + (int)puVar7;
  }
  puVar6 = puVar10;
  puVar1 = puVar7;
  if ((int)puVar7 < (int)puVar5) {
    do {
      puVar1 = (uint *)((int)puVar1 + 1);
      uVar4 = *piVar11 - (int)puVar2;
      puVar2 = (uint *)(uVar4 >> 0x1f);
      *puVar6 = uVar4 & 0xfffffff;
      puVar6 = puVar6 + 1;
      piVar11 = piVar11 + 1;
    } while (puVar5 != puVar1);
    puVar10 = puVar10 + ((int)puVar5 - (int)puVar7);
  }
  if ((int)puVar5 < (int)puVar8) {
    do {
      puVar5 = (uint *)((int)puVar5 + 1);
      *puVar10 = 0;
      puVar10 = puVar10 + 1;
    } while (puVar8 != puVar5);
  }
  FUN_01009e90(param_3);
  return 0;
}



/* @ 0x100c170  FUN_0100c170 */

undefined4
FUN_0100c170(int param_1,int param_2,int param_3,int param_4,uint param_5,char param_6,
            undefined4 *param_7)

{
  param_5 = param_4 << 0xc | param_3 << 0xf | param_2 << 0x14 | param_5;
  if (param_6 == '\x02') {
    *(undefined2 *)param_7 = *(undefined2 *)(param_5 + param_1);
  }
  else if (param_6 == '\x04') {
    *param_7 = *(undefined4 *)(param_5 + param_1);
  }
  else if (param_6 == '\x01') {
    *(undefined1 *)param_7 = *(undefined1 *)(param_5 + param_1);
  }
  else {
    FUN_0100ceac(s__s__illegal_size__u__010323b0,s_al_pcie_read_config_0103234c,param_6);
    FUN_01010bfc(s__s__illegal_size__u__010323b0,s_al_pcie_read_config_0103234c,param_6);
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_read_config_0103234c,0x4d);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_read_config_0103234c,0x4d);
    FUN_01000454(0);
  }
  return 0;
}



/* @ 0x100c27c  FUN_0100c27c */

void FUN_0100c27c(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c170(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,1,param_4);
  return;
}



/* @ 0x100c2b4  FUN_0100c2b4 */

void FUN_0100c2b4(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c170(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,2,param_4);
  return;
}



/* @ 0x100c2ec  FUN_0100c2ec */

void al_pcie_write_config(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c170(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,4,param_4);
  return;
}



/* @ 0x100c324  FUN_0100c324 */

undefined4
FUN_0100c324(int param_1,int param_2,int param_3,int param_4,uint param_5,char param_6,
            undefined4 param_7)

{
  param_5 = param_4 << 0xc | param_3 << 0xf | param_2 << 0x14 | param_5;
  if (param_6 == '\x02') {
    *(short *)(param_5 + param_1) = (short)param_7;
  }
  else if (param_6 == '\x04') {
    *(undefined4 *)(param_5 + param_1) = param_7;
  }
  else if (param_6 == '\x01') {
    *(char *)(param_5 + param_1) = (char)param_7;
  }
  else {
    FUN_0100ceac(s__s__illegal_size__u__010323b0,s_al_pcie_write_config_01032360,param_6);
    FUN_01010bfc(s__s__illegal_size__u__010323b0,s_al_pcie_write_config_01032360,param_6);
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_write_config_01032360,0x85);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_write_config_01032360,0x85);
    FUN_01000454(0);
  }
  return 0;
}



/* @ 0x100c42c  FUN_0100c42c */

void FUN_0100c42c(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c324(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,4,param_4);
  return;
}



/* @ 0x100c714  FUN_0100c714 */

int al_pcie_master_enable(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint *puVar3;
  uint local_24 [2];
  
  uVar2 = *(undefined4 *)(&DAT_010323f8 + param_1 * 4);
  puVar3 = local_24;
  iVar1 = FUN_0100c170();
  if (iVar1 != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_master_enable_01032390,0xbf,
                 puVar3);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_master_enable_01032390,0xbf);
    FUN_01000454(0);
  }
  local_24[0] = local_24[0] | 4;
  iVar1 = FUN_0100c324(uVar2,(param_2 & 0xffffff) >> 0x10,(param_2 & 0xffff) >> 0xb,
                       (param_2 & 0x7ff) >> 8,4,2,local_24[0]);
  if (iVar1 != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_master_enable_01032390,0xc2);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_master_enable_01032390,0xc2);
    FUN_01000454(0);
  }
  return iVar1;
}



/* @ 0x100c730  FUN_0100c730 */

int al_pcie_memory_enable(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint *puVar3;
  uint local_24 [2];
  
  uVar2 = *(undefined4 *)(&DAT_010323f8 + param_1 * 4);
  puVar3 = local_24;
  iVar1 = FUN_0100c170();
  if (iVar1 != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_memory_enable_01032378,0xb1,
                 puVar3);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_memory_enable_01032378,0xb1);
    FUN_01000454(0);
  }
  local_24[0] = local_24[0] | 2;
  iVar1 = FUN_0100c324(uVar2,(param_2 & 0xffffff) >> 0x10,(param_2 & 0xffff) >> 0xb,
                       (param_2 & 0x7ff) >> 8,4,2,local_24[0]);
  if (iVar1 != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_memory_enable_01032378,0xb4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____ret__010323a8,1,
                 s_services_pcie_al_pci_lib_src_pci_010323c8,s_al_pcie_memory_enable_01032378,0xb4);
    FUN_01000454(0);
  }
  return iVar1;
}



/* @ 0x100c74c  FUN_0100c74c */

byte FUN_0100c74c(undefined4 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  byte bVar2;
  byte local_1c;
  byte local_1b;
  ushort local_1a [3];
  
  local_1b = 0x34;
  FUN_0100c2b4(param_1,param_2,6,local_1a);
  if ((local_1a[0] & 0x10) != 0) {
    iVar1 = 0x100;
    do {
      FUN_0100c27c(param_1,param_2,local_1b,&local_1b);
      bVar2 = local_1b & 0xfc;
      if (local_1b < 0x40) {
        return 0;
      }
      local_1b = bVar2;
      FUN_0100c27c(param_1,param_2,bVar2,&local_1c);
      if (local_1c == 0xff) {
        return 0;
      }
      if (local_1c == param_3) {
        return local_1b;
      }
      local_1b = local_1b + 1;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
  }
  return 0;
}



/* @ 0x100c818  FUN_0100c818 */

undefined4 FUN_0100c818(int param_1,uint param_2,int param_3,int *param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  
  if ((param_2 & 0xffffff) >> 0x10 != 0 || param_1 != 0) {
    return 0xffffffea;
  }
  uVar2 = param_2 >> 8;
  uVar4 = uVar2 & 7;
  uVar6 = (uint)(uVar4 != 0);
  uVar1 = uVar6;
  if (param_3 == 0) {
    uVar1 = 0;
  }
  if (uVar1 == 0) {
    uVar1 = param_2 >> 0xb;
    if ((uVar1 & 0x1c) == 0) {
      if (((param_2 & 0x400) == 0) && ((uVar1 & uVar6) == 0)) {
        if (param_3 == 2) {
          if ((param_2 & 0x800) == 0) {
            iVar3 = 0x18000;
          }
          else {
            iVar3 = 0x9000;
          }
          *param_4 = iVar3 + ((uVar1 & 0x1f) + 0xfc0) * 0x100000;
          return 0;
        }
        if (param_3 == 4) {
          *param_4 = (uVar1 & 0x1f) * 0x100000 + -0x3ffb000;
          return 0;
        }
        if (param_3 == 0) {
          *param_4 = (uVar4 + (uVar1 & 0x1f) * 8 + 0x7e01) * 0x20000;
          return 0;
        }
      }
    }
    else {
      uVar5 = uVar1 & 0x1f;
      if (uVar5 == 4) {
        if ((uVar2 & 6) == 0) {
          if (param_3 == 0) {
            *param_4 = (uVar4 + 0x7e21) * 0x20000;
          }
          else {
            if (param_3 != 4) {
              return 0xffffffea;
            }
            *param_4 = -0x3bfb000;
          }
          return 0;
        }
      }
      else if (uVar5 == 5) {
        if ((uVar2 & 6) == 0) {
          if (param_3 == 0) {
            *param_4 = (uVar4 + 0x7e29) * 0x20000;
            return 0;
          }
          if (param_3 == 4) {
            *param_4 = -0x3afb000;
            return 0;
          }
          return 0xffffffea;
        }
      }
      else {
        uVar2 = (uint)(uVar5 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1);
        if ((uVar1 & 0x18) == 0) {
          uVar2 = 0;
        }
        uVar2 = (uVar6 | uVar2 ^ 1) ^ 1;
        if (param_3 != 5) {
          uVar2 = 0;
        }
        if (uVar2 != 0) {
          *param_4 = (uVar5 + 0xfc0) * 0x100000;
          return 0;
        }
      }
    }
  }
  return 0xffffffea;
}



/* @ 0x100ca08  FUN_0100ca08 */

void FUN_0100ca08(void)

{
  return;
}



/* @ 0x100ca0c  FUN_0100ca0c */

undefined4 FUN_0100ca0c(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short local_20;
  undefined1 auStack_1e [2];
  undefined4 local_1c;
  
  if (param_1 == 0 && param_2 != 0) {
    iVar3 = 0;
    iVar4 = DAT_0100cb70;
    do {
      FUN_0100ca08(0,iVar3);
      iVar1 = pci_skip_dev(0,iVar3);
      if ((iVar1 == 0) && (FUN_0100c2b4(0,iVar3,0,&local_20), (ushort)(local_20 - 1U) < 0xfffe)) {
        *(undefined4 *)(iVar4 + -0x14) = 1;
        *(int *)(iVar4 + -0x10) = iVar3 * 0x10 + -0x4400000;
        FUN_0100c2b4(0,iVar3,2,iVar4 + -4);
        FUN_0100c2b4(0,iVar3,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,iVar4 + -2);
        iVar1 = 0;
        do {
          iVar2 = FUN_0100c818(0,iVar3,iVar1,&local_1c);
          if (iVar2 == 0) {
            *(undefined4 *)(iVar4 + iVar1 * 4) = local_1c;
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 != 6);
        *(undefined4 *)(iVar4 + -8) = 1;
        iVar1 = FUN_0100c74c(0,iVar3,0x10);
        if (iVar1 != 0) {
          FUN_0100c2b4(0,iVar3,0x30e,iVar4 + -AL_I2C_TAR_10BIT_ADDR_SHIFT);
          FUN_0100c2b4(0,iVar3,0x310,auStack_1e);
        }
      }
      iVar3 = iVar3 + 0x800;
      iVar4 = iVar4 + 0x30;
    } while (iVar3 != 0x5000);
  }
  return 0;
}



/* @ 0x100cb74  FUN_0100cb74 */


void __wd0_board_reset(void)

{
  FUN_0100ceac(s__s__Resetting_the_board__01032420,s___wd0_board_reset_0103240c);
  FUN_01010bfc(s__s__Resetting_the_board__01032420,s___wd0_board_reset_0103240c);
  udelay(10000);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x100cbc8  FUN_0100cbc8 */

undefined4 FUN_0100cbc8(void)

{
  exec_via_agent();
  FUN_01000454(s_test_main_returned_without_execu_0103243c);
  return 0;
}



/* @ 0x100ccdc  FUN_0100ccdc */

void FUN_0100ccdc(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(&DAT_010492e0 + param_1 * 4);
  if (iVar1 == 0) {
    return;
  }
  FUN_010281ec(&DAT_01049d20 + param_1 * 0x24,iVar1,*(int *)(&DAT_010492f0 + param_1 * 4) - iVar1,0,
               &LAB_0100cbf0);
  return;
}



/* @ 0x100cd38  FUN_0100cd38 */

void FUN_0100cd38(int param_1,int param_2,int param_3)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 local_44;
  undefined4 uStack_40;
  undefined8 local_3c;
  undefined4 local_34;
  undefined4 uStack_30;
  undefined4 local_2c;
  undefined4 uStack_28;
  undefined4 local_24;
  
  iVar1 = param_1 * 0x24;
  puVar2 = (undefined4 *)(&DAT_01049d20 + iVar1);
  local_44 = *puVar2;
  uStack_40 = *(undefined4 *)(&DAT_01049d24 + iVar1);
  local_24 = *(undefined4 *)(&DAT_01049d40 + iVar1);
  local_3c = *(undefined8 *)(&DAT_01049d28 + iVar1);
  iVar3 = *(int *)(&DAT_010492e0 + param_1 * 4);
  *(int *)(&DAT_010492e0 + param_1 * 4) = param_2;
  local_34 = *(undefined4 *)(&DAT_01049d30 + iVar1);
  uStack_30 = *(undefined4 *)(&DAT_01049d34 + iVar1);
  local_2c = *(undefined4 *)(&DAT_01049d38 + iVar1);
  uStack_28 = *(undefined4 *)(&DAT_01049d3c + iVar1);
  *(int *)(&DAT_010492f0 + param_1 * 4) = param_3;
  FUN_010281ec(puVar2,param_2,param_3 - param_2,1,&LAB_0100cbf0);
  if (iVar3 != 0) {
    FUN_01028410(puVar2,&local_44);
  }
  return;
}



/* @ 0x100cdf4  FUN_0100cdf4 */

void FUN_0100cdf4(int param_1,undefined4 param_2)

{
  if (*(int *)(&DAT_010492e0 + param_1 * 4) != 0) {
    *(undefined4 *)(&DAT_01049d28 + param_1 * 0x24) = param_2;
    return;
  }
  return;
}



/* @ 0x100ce20  FUN_0100ce20 */

undefined4 FUN_0100ce20(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_01000084();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) == 0) {
    uVar2 = 0;
  }
  else {
    if (*(int *)(&DAT_01049d30 + iVar1 * 0x24) != 0) {
      uVar2 = FUN_01010be0(&LAB_0100cc90,0,param_1,param_2);
      return uVar2;
    }
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



/* @ 0x100ceac  FUN_0100ceac */

void FUN_0100ceac(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  iVar1 = FUN_01000084();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) != 0) {
    FUN_0100ce20(param_1,&uStack_c);
  }
  return;
}



/* @ 0x100cef4  FUN_0100cef4 */

void FUN_0100cef4(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  undefined4 uVar6;
  char *pcVar7;
  char local_74 [88];
  
  iVar1 = FUN_01000084();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) == 0) {
    return;
  }
  iVar1 = iVar1 * 0x24;
  uVar6 = *(undefined4 *)(&DAT_01049d28 + iVar1);
  *(undefined4 *)(&DAT_01049d28 + iVar1) = 0;
  FUN_0100ceac(s__________________________________0104407c);
  FUN_01010bfc(s__________________________________0104407c);
  FUN_0100ceac(s___TRACE_DUMP__BEGIN_010440b8);
  FUN_01010bfc(s___TRACE_DUMP__BEGIN_010440b8);
  FUN_0100ceac(s__________________________________0104407c);
  FUN_01010bfc(s__________________________________0104407c);
  FUN_0100ceac(s___base____p_010440d0,*(undefined4 *)(&DAT_01049d20 + iVar1));
  FUN_01010bfc(s___base____p_010440d0,*(undefined4 *)(&DAT_01049d20 + iVar1));
  FUN_0100ceac(s___size____x_010440e0,*(undefined4 *)(&DAT_01049d24 + iVar1));
  FUN_01010bfc(s___size____x_010440e0,*(undefined4 *)(&DAT_01049d24 + iVar1));
  FUN_0100ceac(s___en____d_010440f0,uVar6);
  FUN_01010bfc(s___en____d_010440f0,uVar6);
  FUN_0100ceac(s___head____p_010440fc,*(undefined4 *)(&DAT_01049d30 + iVar1));
  FUN_01010bfc(s___head____p_010440fc,*(undefined4 *)(&DAT_01049d30 + iVar1));
  FUN_0100ceac(s___start____p_0104410c,*(undefined4 *)(&DAT_01049d34 + iVar1));
  FUN_01010bfc(s___start____p_0104410c,*(undefined4 *)(&DAT_01049d34 + iVar1));
  FUN_0100ceac(s___end____p_0104411c,*(undefined4 *)(&DAT_01049d38 + iVar1));
  FUN_01010bfc(s___end____p_0104411c,*(undefined4 *)(&DAT_01049d38 + iVar1));
  FUN_0100ceac(s___add_timestamp____d_01044128,*(undefined4 *)(&DAT_01049d3c + iVar1));
  FUN_01010bfc(s___add_timestamp____d_01044128,*(undefined4 *)(&DAT_01049d3c + iVar1));
  FUN_0100ceac(s___ctrl____p_01044140,*(undefined4 *)(&DAT_01049d40 + iVar1));
  FUN_01010bfc(s___ctrl____p_01044140,*(undefined4 *)(&DAT_01049d40 + iVar1));
  FUN_0100ceac(s___ctrl_>head____x_01044150,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x10))
  ;
  FUN_01010bfc(s___ctrl_>head____x_01044150,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x10))
  ;
  FUN_0100ceac(s___ctrl_>tail____x_01044164,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x14))
  ;
  FUN_01010bfc(s___ctrl_>tail____x_01044164,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x14))
  ;
  FUN_0100ceac(s___ctrl_>reset_request____d_01044178,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x18));
  FUN_01010bfc(s___ctrl_>reset_request____d_01044178,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x18));
  FUN_0100ceac(s___ctrl_>wrap_around____d_01044194,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c));
  FUN_01010bfc(s___ctrl_>wrap_around____d_01044194,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c));
  FUN_0100ceac(s__________________________________0104407c);
  FUN_01010bfc(s__________________________________0104407c);
  pcVar7 = *(char **)(&DAT_01049d30 + iVar1);
  if ((*(int *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c) == 0) ||
     (pcVar7 == *(char **)(&DAT_01049d38 + iVar1))) {
    pcVar5 = *(char **)(&DAT_01049d34 + iVar1);
  }
  else {
    pcVar5 = pcVar7 + 1;
  }
  if (param_1 != 0) {
    iVar2 = *(int *)(&DAT_01049d34 + iVar1);
    if (param_1 < (uint)(*(int *)(&DAT_01049d38 + iVar1) - iVar2)) {
      if (*(int *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c) == 0) {
        if ((uint)((int)pcVar7 - (int)pcVar5) <= param_1) goto LAB_01028be4;
      }
      else if ((uint)((int)pcVar7 - iVar2) <= param_1) {
        pcVar5 = pcVar5 + *(int *)(&DAT_01049d38 + iVar1) + (-param_1 - iVar2);
        goto LAB_01028be4;
      }
      pcVar5 = pcVar7 + -param_1;
    }
  }
LAB_01028be4:
  uVar3 = 0;
  if (pcVar7 != pcVar5) {
    uVar4 = uVar3;
    do {
      if (*pcVar5 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
        local_74[uVar4] = '\0';
        FUN_0100ceac(s_The_test_s_final_words_are___s_01028f04 + 0x1c,local_74);
        FUN_01010bfc(s_The_test_s_final_words_are___s_01028f04 + 0x1c,local_74);
        uVar3 = 0;
      }
      else {
        uVar3 = uVar4;
        if (uVar4 < 0x50) {
          uVar3 = uVar4 + 1;
          local_74[uVar4] = *pcVar5;
        }
      }
      pcVar5 = pcVar5 + 1;
      if (*(char **)(&DAT_01049d38 + iVar1) == pcVar5) {
        pcVar5 = *(char **)(&DAT_01049d34 + iVar1);
      }
      uVar4 = uVar3;
    } while (pcVar7 != pcVar5);
  }
  local_74[uVar3] = '\0';
  FUN_0100ceac(&DAT_01028ee4,local_74);
  FUN_01010bfc(&DAT_01028ee4,local_74);
  FUN_0100ceac(s__________________________________010441b0);
  FUN_01010bfc(s__________________________________010441b0);
  *(undefined4 *)(&DAT_01049d28 + iVar1) = uVar6;
  return;
}



/* @ 0x100cf48  FUN_0100cf48 */

undefined4 uart_init_soft(int param_1,undefined4 param_2,undefined4 param_3)

{
  if (3 < param_1) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_init_soft_01032478,0x70);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_init_soft_01032478,0x70);
    FUN_01000454(0);
  }
  al_uart_handle_init(&DAT_0104a690 + param_1 * 8,*(undefined4 *)(&DAT_010324c0 + param_1 * 4));
  *(undefined4 *)(&DAT_0104a634 + param_1 * 0x18) = param_2;
  *(undefined4 *)(&DAT_0104a638 + param_1 * 0x18) = param_3;
  return 0;
}



/* @ 0x100d020  FUN_0100d020 */

void uart_write_byte(int param_1,undefined1 param_2)

{
  undefined1 local_11;
  
  local_11 = param_2;
  if (3 < param_1) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_write_byte_01032488,0x7e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_write_byte_01032488,0x7e);
    FUN_01000454(0);
  }
  if (*(int *)(&DAT_0104a690 + param_1 * 8) != 0) {
    al_uart_write(&DAT_0104a690 + param_1 * 8,1,&local_11,0xffffffff);
  }
  return;
}



/* @ 0x100d0dc  FUN_0100d0dc */

undefined1 uart_read_byte(int param_1)

{
  undefined1 local_19;
  
  if (3 < param_1) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_read_byte_01032498,0x8b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_read_byte_01032498,0x8b);
    FUN_01000454(0);
  }
  if (*(int *)(&DAT_0104a690 + param_1 * 8) == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____uart_if_port__regs__010324fc,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_read_byte_01032498,0x8c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____uart_if_port__regs__010324fc,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_read_byte_01032498,0x8c);
    FUN_01000454(0);
  }
  al_uart_read(&DAT_0104a690 + param_1 * 8,1,&local_19,0xffffffff);
  return local_19;
}



/* @ 0x100d200  FUN_0100d200 */

uint uart_is_input_available(int param_1)

{
  undefined4 unaff_r4;
  undefined4 unaff_r5;
  undefined4 unaff_r6;
  undefined4 unaff_lr;
  
  if (3 < param_1) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_is_input_available_010324a8,0x96)
    ;
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___port_<_4)_010324f0,1,
                 s_services_uart_alpine_src_uart_c_010324d0,s_uart_is_input_available_010324a8,0x96)
    ;
    FUN_01000454(0);
  }
  if (*(int *)(&DAT_0104a690 + param_1 * 8) != 0) {
    if ((int *)(&DAT_0104a690 + param_1 * 8) == (int *)0x0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                   s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_is_input_available_010429d8
                   ,0x1c9,unaff_r4,unaff_r5,unaff_r6,unaff_lr);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                   s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_is_input_available_010429d8
                   ,0x1c9);
      FUN_01000454(0);
    }
    return *(uint *)(*(int *)(&DAT_0104a690 + param_1 * 8) + 0x14) & 1;
  }
  return 0;
}



/* @ 0x100d2c0  FUN_0100d2c0 */

void FUN_0100d2c0(void)

{
  DAT_01049d44 = FUN_0100ea60(0x1812000,0x40000,0);
  return;
}



/* @ 0x100d2f4  FUN_0100d2f4 */

void FUN_0100d2f4(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_01000084();
  sys_alloc((&DAT_01049d44)[iVar1],param_1);
  return;
}



/* @ 0x100d324  FUN_0100d324 */

int mspace_realloc(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 unaff_r4;
  undefined4 unaff_r5;
  undefined4 unaff_r6;
  
  iVar1 = FUN_01000084();
  uVar2 = (&DAT_01049d44)[iVar1];
  if (param_1 == 0) {
    iVar1 = sys_alloc(uVar2,param_2);
    return iVar1;
  }
  if (param_2 < 0xffffffc0) {
    if (param_2 < AL_I2C_INTR_MASK_START_DET_SHIFT + 1) {
      uVar3 = 0x10;
    }
    else {
      uVar3 = param_2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT & 0xfffffff8;
    }
    iVar1 = FUN_0100e5a8(uVar2,param_1 + -AL_I2C_INTR_MASK_ACTIVITY_SHIFT,uVar3);
    if (iVar1 == 0) {
      iVar1 = sys_alloc(uVar2,param_2);
      if (iVar1 != 0) {
        if ((*(uint *)(param_1 + -4) & 3) == 0) {
          iVar4 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
        }
        else {
          iVar4 = 4;
        }
        uVar3 = (*(uint *)(param_1 + -4) & 0xfffffff8) - iVar4;
        if (param_2 <= uVar3) {
          uVar3 = param_2;
        }
        FUN_010129d8(iVar1,param_1,uVar3);
        FUN_010101f4(uVar2,param_1);
      }
    }
    else {
      iVar1 = iVar1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
  }
  else {
    FUN_01010bfc(s__s__Malloc_failed_01032628,s_mspace_realloc_01032618,param_2,&DAT_01049d44,
                 unaff_r4,unaff_r5,unaff_r6);
    iVar1 = 0;
  }
  return iVar1;
}



/* @ 0x100d364  FUN_0100d364 */

void FUN_0100d364(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_01000084();
  FUN_010101f4((&DAT_01049d44)[iVar1],param_1);
  return;
}



/* @ 0x100d394  FUN_0100d394 */

void abort(void)

{
  FUN_01010bfc(s_Received_ABORT_call_0103252c);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,s_abort_01032514,0x3e);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,s_abort_01032514,0x3e);
  FUN_01000454(0);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x100d40c  FUN_0100d40c */

undefined4
FUN_0100d40c(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6)

{
  FUN_01010bfc(s_Received_MMAP_call__1_start____p_01032568,param_1,param_2,param_3,param_4,param_5,
               param_6);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,&DAT_0103251c,0x59);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,&DAT_0103251c,0x59);
  FUN_01000454(0);
  return 0;
}



/* @ 0x100d4c0  FUN_0100d4c0 */

undefined4 munmap(undefined4 param_1,undefined4 param_2)

{
  FUN_01010bfc(s_Received_MUNMAP_call__1_start_____010325d8,param_1,param_2);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,s_munmap_01032524,99);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s_stdlib_malloc_src_malloc_porting_01032544,s_munmap_01032524,99);
  FUN_01000454(0);
  return 0;
}



/* @ 0x100d560  FUN_0100d560 */

uint FUN_0100d560(int param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  
  uVar8 = param_1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  iVar7 = param_1;
  if ((uVar8 & 7) != 0) {
    iVar7 = param_1 + (-uVar8 & 7);
    uVar8 = iVar7 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  }
  FUN_01012ae0(uVar8,0,0x1e0);
  uVar1 = DAT_01049d48;
  *(undefined4 *)(iVar7 + 4) = 0x1e3;
  uVar4 = DAT_01049d5c;
  *(int *)(iVar7 + 0x18) = param_1;
  *(undefined4 *)(iVar7 + 0x28) = 0xfff;
  iVar5 = iVar7 + 0x30;
  *(int *)(iVar7 + 0x1b8) = param_2;
  *(undefined4 *)(iVar7 + 0x2c) = uVar1;
  *(int *)(iVar7 + 0x1bc) = param_2;
  *(int *)(iVar7 + 0x1c8) = param_1;
  *(uint *)(iVar7 + 0x1c4) = uVar4 | 4;
  *(int *)(iVar7 + 0x1cc) = param_2;
  *(undefined4 *)(iVar7 + 0x1d8) = 0;
  *(undefined4 *)(iVar7 + 0x1dc) = 0;
  do {
    *(int *)(iVar5 + 8) = iVar5;
    *(int *)(iVar5 + 0xc) = iVar5;
    uVar1 = DAT_01049d58;
    iVar5 = iVar5 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  } while (iVar7 + 0x130 != iVar5);
  iVar2 = iVar7 + 0x1e0;
  iVar6 = (param_1 + param_2) - iVar2;
  uVar4 = iVar6 - 0x28;
  iVar5 = iVar2;
  if ((iVar7 + 0x1e8U & 7) != 0) {
    uVar3 = -(iVar7 + 0x1e8U) & 7;
    uVar4 = (iVar6 + -0x28) - uVar3;
    iVar5 = iVar2 + uVar3;
  }
  *(uint *)(iVar7 + 0x14) = uVar4;
  *(int *)(iVar7 + 0x20) = iVar5;
  *(uint *)(iVar5 + 4) = uVar4 | 1;
  *(undefined4 *)(iVar2 + iVar6 + -0x24) = 0x28;
  *(undefined4 *)(iVar7 + 0x24) = uVar1;
  return uVar8;
}



/* @ 0x100d680  FUN_0100d680 */

int FUN_0100d680(int param_1)

{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint *puVar6;
  uint *puVar7;
  uint uVar8;
  uint *puVar9;
  uint uVar10;
  uint *puVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  uint uVar15;
  int iVar16;
  int iVar17;
  bool bVar18;
  bool bVar19;
  int local_3c;
  
  if (*(uint **)(param_1 + 0x1c8) == (uint *)0x0) {
    uVar12 = 0xfff;
    local_3c = 0;
  }
  else {
    uVar12 = 0;
    puVar7 = (uint *)(param_1 + 0x1c0);
    local_3c = 0;
    puVar9 = *(uint **)(param_1 + 0x1c8);
    do {
      uVar12 = uVar12 + 1;
      puVar11 = (uint *)puVar9[2];
      if ((puVar9[3] & AL_I2C_INTR_MASK_STOP_DET_SHIFT) == 1) {
        uVar13 = *puVar9;
        uVar10 = uVar13;
        if ((uVar13 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT & 7) != 0) {
          uVar10 = uVar13 + (-(uVar13 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT) & 7);
        }
        uVar14 = *(uint *)(uVar10 + 4);
        if ((uVar14 & 3) == 1) {
          uVar8 = puVar9[1];
          uVar15 = uVar14 & 0xfffffff8;
          if (uVar13 + (uVar8 - 0x28) <= uVar10 + uVar15) {
            if (*(uint *)(param_1 + 0x14) == uVar10) {
              *(undefined4 *)(param_1 + 8) = 0;
              *(undefined4 *)(param_1 + 0x14) = 0;
            }
            else {
              uVar2 = *(uint *)(uVar10 + 0xc);
              uVar3 = *(uint *)(uVar10 + 0x18);
              if (uVar2 == uVar10) {
                if (*(uint *)(uVar10 + 0x14) != 0) {
                  puVar1 = (uint *)(uVar10 + 0x14);
                  uVar4 = *(uint *)(uVar10 + 0x14);
LAB_0100da5c:
                  do {
                    do {
                      uVar2 = uVar4;
                      puVar6 = puVar1;
                      uVar4 = *(uint *)(uVar2 + 0x14);
                      puVar1 = (uint *)(uVar2 + 0x14);
                    } while (uVar4 != 0);
                    uVar4 = *(uint *)(uVar2 + 0x10);
                    puVar1 = (uint *)(uVar2 + 0x10);
                  } while (uVar4 != 0);
                  if (puVar6 < *(uint **)(param_1 + 0x10)) {
                    abort();
                  }
                  else {
                    *puVar6 = 0;
                  }
                  goto LAB_0100d78c;
                }
                uVar2 = *(uint *)(uVar10 + 0x10);
                if (uVar2 != 0) {
                  puVar1 = (uint *)(uVar10 + 0x10);
                  uVar4 = uVar2;
                  goto LAB_0100da5c;
                }
                if (uVar3 == 0) goto LAB_0100d82c;
                uVar4 = *(uint *)(uVar10 + 0x1c);
                iVar16 = param_1 + uVar4 * 4;
                if (*(uint *)(iVar16 + 0x130) == uVar10) {
                  *(undefined4 *)(iVar16 + 0x130) = 0;
                  goto LAB_0100da44;
                }
LAB_0100d7ac:
                if (uVar3 < *(uint *)(param_1 + 0x10)) {
                  abort();
                }
                else if (*(uint *)(uVar3 + 0x10) == uVar10) {
                  *(uint *)(uVar3 + 0x10) = uVar2;
                }
                else {
                  *(uint *)(uVar3 + 0x14) = uVar2;
                }
                if (uVar2 != 0) {
LAB_0100d7dc:
                  uVar4 = *(uint *)(param_1 + 0x10);
                  if (uVar2 < uVar4) {
LAB_0100dae0:
                    abort();
                  }
                  else {
                    uVar5 = *(uint *)(uVar10 + 0x10);
                    *(uint *)(uVar2 + 0x18) = uVar3;
                    if (uVar5 != 0) {
                      if (uVar5 < uVar4) {
                        abort();
                      }
                      else {
                        *(uint *)(uVar2 + 0x10) = uVar5;
                        *(uint *)(uVar5 + 0x18) = uVar2;
                      }
                    }
                    uVar3 = *(uint *)(uVar10 + 0x14);
                    if (uVar3 != 0) {
                      if (uVar3 < *(uint *)(param_1 + 0x10)) goto LAB_0100dae0;
                      *(uint *)(uVar2 + 0x14) = uVar3;
                      *(uint *)(uVar3 + 0x18) = uVar2;
                    }
                  }
                }
              }
              else {
                uVar4 = *(uint *)(uVar10 + 8);
                if (((uVar4 < *(uint *)(param_1 + 0x10)) || (*(uint *)(uVar4 + 0xc) != uVar10)) ||
                   (*(uint *)(uVar2 + 8) != uVar10)) {
                  abort();
                }
                else {
                  *(uint *)(uVar4 + 0xc) = uVar2;
                  *(uint *)(uVar2 + 8) = uVar4;
                }
LAB_0100d78c:
                if (uVar3 != 0) {
                  uVar4 = *(uint *)(uVar10 + 0x1c);
                  iVar16 = param_1 + uVar4 * 4;
                  if (*(uint *)(iVar16 + 0x130) != uVar10) goto LAB_0100d7ac;
                  *(uint *)(iVar16 + 0x130) = uVar2;
                  if (uVar2 == 0) {
LAB_0100da44:
                    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & ~(1 << (uVar4 & 0xff));
                    goto LAB_0100d82c;
                  }
                  goto LAB_0100d7dc;
                }
              }
            }
LAB_0100d82c:
            iVar16 = munmap(uVar13,uVar8);
            if (iVar16 == 0) {
              local_3c = local_3c + uVar8;
              *(uint *)(param_1 + 0x1b0) = *(int *)(param_1 + 0x1b0) - uVar8;
              puVar7[2] = (uint)puVar11;
              puVar9 = puVar7;
            }
            else {
              uVar14 = uVar14 >> 8;
              if (uVar14 == 0) {
                iVar16 = 0x130;
                iVar17 = 0x4c;
                uVar13 = 1;
                uVar14 = 0;
              }
              else if (uVar14 < 0x10000) {
                uVar8 = uVar14 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
                iVar16 = uVar14 << uVar8;
                uVar13 = iVar16 - 0x1000U >> 0x10 & 4;
                iVar16 = iVar16 << uVar13;
                uVar14 = iVar16 - 0x4000U >> 0x10 & 2;
                iVar16 = ((uint)(iVar16 << uVar14) >> 0xf) - ((uVar8 | uVar13) + uVar14);
                uVar14 = (uVar15 >> (iVar16 + 0x15U & 0xff) & 1) + (iVar16 + 0xe) * 2;
                iVar17 = uVar14 + 0x4c;
                uVar13 = 1 << (uVar14 & 0xff);
                iVar16 = iVar17 * 4;
              }
              else {
                iVar16 = 0x1ac;
                uVar13 = 0x80000000;
                uVar14 = 0x1f;
                iVar17 = 0x6b;
              }
              uVar8 = *(uint *)(param_1 + 4);
              *(undefined4 *)(uVar10 + 0x10) = 0;
              *(undefined4 *)(uVar10 + 0x14) = 0;
              *(uint *)(uVar10 + 0x1c) = uVar14;
              if ((uVar8 & uVar13) == 0) {
                *(uint *)(param_1 + 4) = uVar8 | uVar13;
                *(uint *)(param_1 + iVar17 * 4) = uVar10;
                *(uint *)(uVar10 + 8) = uVar10;
                *(uint *)(uVar10 + 0xc) = uVar10;
                *(int *)(uVar10 + 0x18) = param_1 + iVar16;
              }
              else {
                uVar8 = *(uint *)(param_1 + iVar17 * 4);
                uVar13 = uVar15;
                if (uVar14 != 0x1f) {
                  uVar13 = uVar15 << (0x19 - (uVar14 >> 1) & 0xff);
                }
                do {
                  uVar14 = uVar8;
                  iVar16 = (int)uVar13 >> 0x1f;
                  uVar13 = uVar13 << 1;
                  iVar16 = 4 - iVar16;
                  if ((*(uint *)(uVar14 + 4) & 0xfffffff8) == uVar15) {
                    uVar8 = *(uint *)(uVar14 + 8);
                    uVar13 = *(uint *)(param_1 + 0x10);
                    bVar19 = uVar14 <= uVar13;
                    bVar18 = uVar13 == uVar14;
                    if (!bVar19 || bVar18) {
                      bVar19 = uVar8 <= uVar13;
                      bVar18 = uVar13 == uVar8;
                    }
                    if (bVar19 && !bVar18) goto LAB_0100db30;
                    *(uint *)(uVar8 + 0xc) = uVar10;
                    *(uint *)(uVar14 + 8) = uVar10;
                    *(uint *)(uVar10 + 8) = uVar8;
                    *(uint *)(uVar10 + 0xc) = uVar14;
                    *(undefined4 *)(uVar10 + 0x18) = 0;
                    goto joined_r0x0100d6c4;
                  }
                  uVar8 = *(uint *)(uVar14 + iVar16 * 4);
                } while (uVar8 != 0);
                if (uVar14 + iVar16 * 4 < *(uint *)(param_1 + 0x10)) {
LAB_0100db30:
                  abort();
                }
                else {
                  *(uint *)(uVar14 + iVar16 * 4) = uVar10;
                  *(uint *)(uVar10 + 8) = uVar10;
                  *(uint *)(uVar10 + 0xc) = uVar10;
                  *(uint *)(uVar10 + 0x18) = uVar14;
                }
              }
            }
          }
        }
      }
joined_r0x0100d6c4:
      puVar7 = puVar9;
      puVar9 = puVar11;
    } while (puVar11 != (uint *)0x0);
    if (uVar12 < 0xfff) {
      uVar12 = 0xfff;
    }
  }
  *(uint *)(param_1 + 0x20) = uVar12;
  return local_3c;
}



/* @ 0x100db5c  FUN_0100db5c */

void FUN_0100db5c(uint *param_1,uint *param_2,uint param_3)

{
  uint *puVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  uint uVar5;
  int iVar6;
  uint *puVar7;
  uint *puVar8;
  int iVar9;
  uint *puVar10;
  uint *puVar11;
  uint *puVar12;
  uint uVar13;
  uint uVar14;
  bool bVar15;
  bool bVar16;
  
  puVar10 = param_2 + 1;
  puVar8 = (uint *)((int)param_2 + param_3);
  if ((*puVar10 & 1) != 0) {
    puVar10 = (uint *)param_1[4];
    goto LAB_0100dc8c;
  }
  uVar13 = *param_2;
  param_2 = (uint *)((int)param_2 - uVar13);
  if ((*puVar10 & 3) == 0) {
    iVar9 = param_3 + 0x10 + uVar13;
    iVar6 = munmap(param_2,iVar9);
    if (iVar6 != 0) {
      return;
    }
    param_1[0x6c] = param_1[0x6c] - iVar9;
    return;
  }
  puVar10 = (uint *)param_1[4];
  param_3 = param_3 + uVar13;
  if (param_2 < puVar10) goto LAB_0100e00c;
  puVar3 = (uint *)param_1[5];
  if (puVar3 == param_2) {
    uVar13 = puVar8[1];
    if ((uVar13 & 3) == 3) {
      param_1[2] = param_3;
      puVar8[1] = uVar13 & 0xfffffffe;
      puVar3[1] = param_3 | 1;
      *puVar8 = param_3;
      return;
    }
    goto LAB_0100dc8c;
  }
  uVar13 = uVar13 >> 3;
  puVar3 = (uint *)param_2[3];
  if (uVar13 < 0x20) {
    puVar11 = (uint *)param_2[2];
    if (param_1 + uVar13 * 2 + 10 == puVar11) {
      if (puVar11 == puVar3) {
LAB_0100df4c:
        *param_1 = *param_1 & ~(1 << (uVar13 & 0xff));
        goto LAB_0100dc8c;
      }
LAB_0100e1ec:
      if ((puVar10 <= puVar3) && ((uint *)puVar3[2] == param_2)) {
LAB_0100e268:
        puVar11[3] = (uint)puVar3;
        puVar3[2] = (uint)puVar11;
        goto LAB_0100dc8c;
      }
    }
    else if ((puVar10 <= puVar11) && ((uint *)puVar11[3] == param_2)) {
      if (puVar11 != puVar3) {
        if (param_1 + uVar13 * 2 + 10 != puVar3) goto LAB_0100e1ec;
        goto LAB_0100e268;
      }
      goto LAB_0100df4c;
    }
    goto LAB_0100e200;
  }
  puVar11 = (uint *)param_2[6];
  if (puVar3 == param_2) {
    if ((uint *)param_2[5] != (uint *)0x0) {
      puVar4 = (uint *)param_2[5];
      puVar12 = param_2 + 5;
LAB_0100e130:
      do {
        do {
          puVar7 = puVar12;
          puVar3 = puVar4;
          puVar4 = (uint *)puVar3[5];
          puVar12 = puVar3 + 5;
        } while (puVar4 != (uint *)0x0);
        puVar4 = (uint *)puVar3[4];
        puVar12 = puVar3 + 4;
      } while (puVar4 != (uint *)0x0);
      if (puVar7 < puVar10) {
        abort();
        puVar10 = (uint *)param_1[4];
      }
      else {
        *puVar7 = 0;
      }
      goto LAB_0100dbfc;
    }
    if ((uint *)param_2[4] != (uint *)0x0) {
      puVar4 = (uint *)param_2[4];
      puVar12 = param_2 + 4;
      goto LAB_0100e130;
    }
    if (puVar11 == (uint *)0x0) goto LAB_0100dc8c;
    uVar13 = param_2[7];
    if ((uint *)param_1[uVar13 + 0x4c] == param_2) {
      param_1[uVar13 + 0x4c] = 0;
      goto LAB_0100e1a0;
    }
    puVar3 = (uint *)0x0;
LAB_0100dc20:
    if (puVar11 < puVar10) {
      abort();
      puVar10 = (uint *)param_1[4];
    }
    else if ((uint *)puVar11[4] == param_2) {
      puVar11[4] = (uint)puVar3;
    }
    else {
      puVar11[5] = (uint)puVar3;
    }
    if (puVar3 != (uint *)0x0) {
LAB_0100dc40:
      if (puVar3 < puVar10) {
LAB_0100e200:
        abort();
        puVar10 = (uint *)param_1[4];
      }
      else {
        puVar4 = (uint *)param_2[4];
        puVar3[6] = (uint)puVar11;
        if (puVar4 != (uint *)0x0) {
          if (puVar4 < puVar10) {
            abort();
            puVar10 = (uint *)param_1[4];
          }
          else {
            puVar3[4] = (uint)puVar4;
            puVar4[6] = (uint)puVar3;
          }
        }
        puVar11 = (uint *)param_2[5];
        if (puVar11 != (uint *)0x0) {
          if (puVar11 < puVar10) goto LAB_0100e200;
          puVar3[5] = (uint)puVar11;
          puVar11[6] = (uint)puVar3;
        }
      }
    }
  }
  else {
    puVar4 = (uint *)param_2[2];
    if (((puVar4 < puVar10) || ((uint *)puVar4[3] != param_2)) || ((uint *)puVar3[2] != param_2)) {
      abort();
      puVar10 = (uint *)param_1[4];
    }
    else {
      puVar4[3] = (uint)puVar3;
      puVar3[2] = (uint)puVar4;
    }
LAB_0100dbfc:
    if (puVar11 != (uint *)0x0) {
      uVar13 = param_2[7];
      if ((uint *)param_1[uVar13 + 0x4c] != param_2) goto LAB_0100dc20;
      param_1[uVar13 + 0x4c] = (uint)puVar3;
      if (puVar3 == (uint *)0x0) {
LAB_0100e1a0:
        param_1[1] = param_1[1] & ~(1 << (uVar13 & 0xff));
        goto LAB_0100dc8c;
      }
      goto LAB_0100dc40;
    }
  }
LAB_0100dc8c:
  if (puVar8 < puVar10) goto LAB_0100e00c;
  uVar13 = puVar8[1];
  if ((uVar13 & 2) == 0) {
    puVar3 = (uint *)param_1[5];
    if ((uint *)param_1[6] == puVar8) {
      uVar13 = param_1[3];
      param_1[6] = (uint)param_2;
      param_1[3] = param_3 + uVar13;
      param_2[1] = param_3 + uVar13 | 1;
      if (puVar3 != param_2) {
        return;
      }
      param_1[2] = 0;
      param_1[5] = 0;
      return;
    }
    if (puVar3 == puVar8) {
      param_1[5] = (uint)param_2;
      param_3 = param_3 + param_1[2];
      param_1[2] = param_3;
      param_2[1] = param_3 | 1;
      *(uint *)((int)param_2 + param_3) = param_3;
      return;
    }
    puVar11 = (uint *)puVar8[3];
    uVar5 = uVar13 >> 3;
    param_3 = param_3 + (uVar13 & 0xfffffff8);
    if (uVar5 < 0x20) {
      puVar4 = (uint *)puVar8[2];
      if (param_1 + uVar5 * 2 + 10 == puVar4) {
        if (puVar11 == puVar4) {
LAB_0100df1c:
          *param_1 = *param_1 & ~(1 << (uVar5 & 0xff));
        }
        else {
LAB_0100e2ec:
          if ((puVar11 < puVar10) || ((uint *)puVar11[2] != puVar8)) goto LAB_0100e300;
LAB_0100e238:
          puVar4[3] = (uint)puVar11;
          puVar11[2] = (uint)puVar4;
        }
      }
      else {
        if ((puVar10 <= puVar4) && ((uint *)puVar4[3] == puVar8)) {
          if (puVar11 != puVar4) {
            if (param_1 + uVar5 * 2 + 10 != puVar11) goto LAB_0100e2ec;
            goto LAB_0100e238;
          }
          goto LAB_0100df1c;
        }
LAB_0100e300:
        abort();
        puVar3 = (uint *)param_1[5];
      }
      goto LAB_0100dd9c;
    }
    puVar4 = (uint *)puVar8[6];
    if (puVar11 == puVar8) {
      puVar12 = (uint *)puVar8[5];
      if (puVar12 != (uint *)0x0) {
        puVar3 = puVar8 + 5;
LAB_0100e0d0:
        do {
          puVar1 = puVar12 + 5;
          puVar2 = (uint *)puVar12[5];
          puVar7 = puVar3;
          puVar11 = puVar12;
          while (puVar12 = puVar2, puVar3 = puVar1, puVar12 != (uint *)0x0) {
            puVar1 = puVar12 + 5;
            puVar7 = puVar3;
            puVar11 = puVar12;
            puVar2 = (uint *)puVar12[5];
          }
          puVar3 = puVar11 + 4;
          puVar12 = (uint *)*puVar3;
        } while (puVar12 != (uint *)0x0);
        if (puVar7 < puVar10) {
          abort();
        }
        else {
          *puVar7 = 0;
        }
        goto LAB_0100dd0c;
      }
      puVar12 = (uint *)puVar8[4];
      if (puVar12 != (uint *)0x0) {
        puVar3 = puVar8 + 4;
        goto LAB_0100e0d0;
      }
      if (puVar4 == (uint *)0x0) goto LAB_0100dd9c;
      uVar13 = puVar8[7];
      if ((uint *)param_1[uVar13 + 0x4c] == puVar8) {
        param_1[uVar13 + 0x4c] = 0;
        goto LAB_0100e178;
      }
      puVar11 = (uint *)0x0;
LAB_0100dd2c:
      if (puVar4 < puVar10) {
        abort();
      }
      else if ((uint *)puVar4[4] == puVar8) {
        puVar4[4] = (uint)puVar11;
      }
      else {
        puVar4[5] = (uint)puVar11;
      }
      if (puVar11 == (uint *)0x0) goto LAB_0100df8c;
LAB_0100dd4c:
      puVar10 = (uint *)param_1[4];
      if (puVar11 < puVar10) {
        abort();
        puVar3 = (uint *)param_1[5];
      }
      else {
        puVar3 = (uint *)puVar8[4];
        puVar11[6] = (uint)puVar4;
        if (puVar3 != (uint *)0x0) {
          if (puVar3 < puVar10) {
            abort();
          }
          else {
            puVar11[4] = (uint)puVar3;
            puVar3[6] = (uint)puVar11;
          }
        }
        uVar13 = puVar8[5];
        if (uVar13 == 0) goto LAB_0100df8c;
        if (uVar13 < param_1[4]) goto LAB_0100e300;
        puVar3 = (uint *)param_1[5];
        puVar11[5] = uVar13;
        *(uint **)(uVar13 + 0x18) = puVar11;
      }
    }
    else {
      puVar3 = (uint *)puVar8[2];
      if (((puVar3 < puVar10) || ((uint *)puVar3[3] != puVar8)) || ((uint *)puVar11[2] != puVar8)) {
        abort();
      }
      else {
        puVar3[3] = (uint)puVar11;
        puVar11[2] = (uint)puVar3;
      }
LAB_0100dd0c:
      if (puVar4 != (uint *)0x0) {
        uVar13 = puVar8[7];
        if ((uint *)param_1[uVar13 + 0x4c] != puVar8) {
          puVar10 = (uint *)param_1[4];
          goto LAB_0100dd2c;
        }
        param_1[uVar13 + 0x4c] = (uint)puVar11;
        if (puVar11 == (uint *)0x0) {
          puVar3 = (uint *)param_1[5];
LAB_0100e178:
          param_1[1] = param_1[1] & ~(1 << (uVar13 & 0xff));
          goto LAB_0100dd9c;
        }
        goto LAB_0100dd4c;
      }
LAB_0100df8c:
      puVar3 = (uint *)param_1[5];
    }
LAB_0100dd9c:
    param_2[1] = param_3 | 1;
    *(uint *)((int)param_2 + param_3) = param_3;
    if (puVar3 == param_2) {
      param_1[2] = param_3;
      return;
    }
  }
  else {
    puVar8[1] = uVar13 & 0xfffffffe;
    param_2[1] = param_3 | 1;
    *(uint *)((int)param_2 + param_3) = param_3;
  }
  uVar13 = param_3 >> 3;
  if (uVar13 < 0x20) {
    uVar5 = 1 << (uVar13 & 0xff);
    puVar10 = param_1 + uVar13 * 2 + 10;
    if ((*param_1 & uVar5) == 0) {
      *param_1 = *param_1 | uVar5;
      puVar8 = puVar10;
    }
    else {
      puVar8 = (uint *)param_1[uVar13 * 2 + 0xc];
      if ((uint *)param_1[uVar13 * 2 + 0xc] < (uint *)param_1[4]) {
        abort();
        puVar8 = puVar10;
      }
    }
    param_1[uVar13 * 2 + 0xc] = (uint)param_2;
    puVar8[3] = (uint)param_2;
    param_2[2] = (uint)puVar8;
    param_2[3] = (uint)puVar10;
  }
  else {
    uVar13 = param_3 >> 8;
    if (uVar13 < 0x10000) {
      uVar14 = uVar13 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      iVar6 = uVar13 << uVar14;
      uVar13 = iVar6 - 0x1000U >> 0x10 & 4;
      iVar6 = iVar6 << uVar13;
      uVar5 = iVar6 - 0x4000U >> 0x10 & 2;
      iVar6 = ((uint)(iVar6 << uVar5) >> 0xf) - ((uVar14 | uVar13) + uVar5);
      uVar5 = (param_3 >> (iVar6 + 0x15U & 0xff) & 1) + (iVar6 + 0xe) * 2;
      iVar9 = uVar5 + 0x4c;
      uVar13 = 1 << (uVar5 & 0xff);
      iVar6 = iVar9 * 4;
    }
    else {
      uVar13 = 0x80000000;
      iVar6 = 0x1ac;
      uVar5 = 0x1f;
      iVar9 = 0x6b;
    }
    uVar14 = param_1[1];
    param_2[4] = 0;
    param_2[5] = 0;
    param_2[7] = uVar5;
    if ((uVar14 & uVar13) == 0) {
      param_1[1] = uVar14 | uVar13;
      param_1[iVar9] = (uint)param_2;
      param_2[2] = (uint)param_2;
      param_2[3] = (uint)param_2;
      param_2[6] = (int)param_1 + iVar6;
    }
    else {
      uVar13 = param_3;
      uVar14 = param_1[iVar9];
      if (uVar5 != 0x1f) {
        uVar13 = param_3 << (0x19 - (uVar5 >> 1) & 0xff);
      }
      do {
        uVar5 = uVar14;
        iVar6 = (int)uVar13 >> 0x1f;
        uVar13 = uVar13 << 1;
        iVar6 = 4 - iVar6;
        if ((*(uint *)(uVar5 + 4) & 0xfffffff8) == param_3) {
          uVar14 = *(uint *)(uVar5 + 8);
          uVar13 = param_1[4];
          bVar16 = uVar5 <= uVar13;
          bVar15 = uVar13 == uVar5;
          if (!bVar16 || bVar15) {
            bVar16 = uVar14 <= uVar13;
            bVar15 = uVar13 == uVar14;
          }
          if (!bVar16 || bVar15) {
            *(uint **)(uVar14 + 0xc) = param_2;
            *(uint **)(uVar5 + 8) = param_2;
            param_2[2] = uVar14;
            param_2[3] = uVar5;
            param_2[6] = 0;
            return;
          }
          goto LAB_0100e00c;
        }
        uVar14 = *(uint *)(uVar5 + iVar6 * 4);
      } while (uVar14 != 0);
      if (uVar5 + iVar6 * 4 < param_1[4]) {
LAB_0100e00c:
        abort();
        return;
      }
      *(uint **)(uVar5 + iVar6 * 4) = param_2;
      param_2[2] = (uint)param_2;
      param_2[3] = (uint)param_2;
      param_2[6] = uVar5;
    }
  }
  return;
}



/* @ 0x100e394  FUN_0100e394 */

void FUN_0100e394(void)

{
  undefined1 auStack_10 [12];
  
  DAT_01049d48 = (uint)auStack_10 ^ 0x55555550 | AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  DAT_01049d4c = 0x1000;
  DAT_01049d50 = 0x10000;
  DAT_01049d54 = 0x40000;
  DAT_01049d58 = 0x200000;
  DAT_01049d5c = 5;
  return;
}



/* @ 0x100e5a8  FUN_0100e5a8 */

uint * FUN_0100e5a8(uint *param_1,uint *param_2,uint param_3)

{
  uint *puVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  uint *puVar6;
  uint uVar7;
  uint *puVar8;
  uint *puVar9;
  uint uVar10;
  uint *puVar11;
  bool bVar12;
  
  uVar10 = param_2[1];
  puVar11 = (uint *)param_1[4];
  uVar4 = uVar10 & 0xfffffff8;
  puVar8 = (uint *)((int)param_2 + uVar4);
  if (param_2 < puVar11) {
LAB_0100e6e4:
    abort();
    return (uint *)0x0;
  }
  uVar2 = uVar10 & 3;
  bVar12 = uVar2 != 0;
  if (uVar2 != 1) {
    bVar12 = puVar8 <= param_2;
  }
  if ((bVar12) || (uVar7 = puVar8[1], (uVar7 & 1) == 0)) goto LAB_0100e6e4;
  if (uVar2 == 0) {
    if (param_3 < 0x100) {
      return (uint *)0x0;
    }
    if (uVar4 < param_3 + 4) {
      return (uint *)0x0;
    }
    if ((uint)(DAT_01049d50 * 2) < uVar4 - param_3) {
      return (uint *)0x0;
    }
    return param_2;
  }
  if (param_3 <= uVar4) {
    uVar4 = uVar4 - param_3;
    if (0xf < uVar4) {
      param_2[1] = uVar10 & 1 | param_3 | 2;
      *(uint *)((int)param_2 + param_3 + 4) = uVar4 | 3;
      puVar8[1] = puVar8[1] | 1;
      FUN_0100db5c(param_1,(int)param_2 + param_3,uVar4);
      return param_2;
    }
    return param_2;
  }
  if (puVar8 == (uint *)param_1[6]) {
    if (uVar4 + param_1[3] <= param_3) {
      return (uint *)0x0;
    }
    uVar4 = (uVar4 + param_1[3]) - param_3;
    param_2[1] = uVar10 & 1 | param_3 | 2;
    *(uint *)((int)param_2 + param_3 + 4) = uVar4 | 1;
    param_1[3] = uVar4;
    param_1[6] = (int)param_2 + param_3;
    return param_2;
  }
  if (puVar8 == (uint *)param_1[5]) {
    uVar4 = uVar4 + param_1[2];
    if (uVar4 < param_3) {
      return (uint *)0x0;
    }
    uVar2 = uVar4 - param_3;
    if (uVar2 < 0x10) {
      param_2[1] = uVar4 | uVar10 & 1 | 2;
      *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) | 1;
      param_1[2] = 0;
      param_1[5] = 0;
      return param_2;
    }
    param_2[1] = param_3 | uVar10 & 1 | 2;
    *(uint *)((int)param_2 + param_3 + 4) = uVar2 | 1;
    *(uint *)((int)param_2 + uVar4) = uVar2;
    *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) & 0xfffffffe;
    param_1[2] = uVar2;
    param_1[5] = (int)param_2 + param_3;
    return param_2;
  }
  if ((uVar7 & 2) != 0) {
    return (uint *)0x0;
  }
  uVar4 = uVar4 + (uVar7 & 0xfffffff8);
  if (uVar4 < param_3) {
    return (uint *)0x0;
  }
  uVar7 = uVar7 >> 3;
  puVar9 = (uint *)puVar8[3];
  uVar2 = uVar4 - param_3;
  if (uVar7 < 0x20) {
    puVar3 = (uint *)puVar8[2];
    if ((puVar3 == param_1 + uVar7 * 2 + 10) ||
       ((puVar11 <= puVar3 && (puVar8 == (uint *)puVar3[3])))) {
      if (puVar3 == puVar9) {
        *param_1 = *param_1 & ~(1 << (uVar7 & 0xff));
        goto LAB_0100e6b4;
      }
      if ((param_1 + uVar7 * 2 + 10 == puVar9) ||
         ((puVar11 <= puVar9 && (puVar8 == (uint *)puVar9[2])))) {
        puVar3[3] = (uint)puVar9;
        puVar9[2] = (uint)puVar3;
        goto LAB_0100e6b4;
      }
    }
    goto LAB_0100e9dc;
  }
  puVar3 = (uint *)puVar8[6];
  if (puVar8 == puVar9) {
    if ((uint *)puVar8[5] != (uint *)0x0) {
      puVar5 = (uint *)puVar8[5];
      puVar1 = puVar8 + 5;
LAB_0100e944:
      do {
        do {
          puVar6 = puVar1;
          puVar9 = puVar5;
          puVar5 = (uint *)puVar9[5];
          puVar1 = puVar9 + 5;
        } while (puVar5 != (uint *)0x0);
        puVar5 = (uint *)puVar9[4];
        puVar1 = puVar9 + 4;
      } while (puVar5 != (uint *)0x0);
      if (puVar6 < puVar11) goto LAB_0100e990;
      *puVar6 = 0;
      goto LAB_0100e830;
    }
    puVar9 = (uint *)puVar8[4];
    if (puVar9 != (uint *)0x0) {
      puVar5 = puVar9;
      puVar1 = puVar8 + 4;
      goto LAB_0100e944;
    }
    if (puVar3 == (uint *)0x0) goto LAB_0100e6b4;
    uVar7 = puVar8[7];
    if (puVar8 == (uint *)param_1[uVar7 + 0x4c]) {
      param_1[uVar7 + 0x4c] = 0;
      goto LAB_0100e9b0;
    }
LAB_0100e850:
    if (puVar3 < puVar11) {
      abort();
    }
    else if (puVar8 == (uint *)puVar3[4]) {
      puVar3[4] = (uint)puVar9;
    }
    else {
      puVar3[5] = (uint)puVar9;
    }
    if (puVar9 != (uint *)0x0) {
LAB_0100e870:
      puVar11 = (uint *)param_1[4];
      if (puVar11 <= puVar9) {
        puVar5 = (uint *)puVar8[4];
        puVar9[6] = (uint)puVar3;
        if (puVar5 != (uint *)0x0) {
          if (puVar5 < puVar11) {
            abort();
          }
          else {
            puVar9[4] = (uint)puVar5;
            puVar5[6] = (uint)puVar9;
          }
        }
        uVar7 = puVar8[5];
        if (uVar7 == 0) goto LAB_0100e92c;
        if (param_1[4] <= uVar7) {
          uVar10 = param_2[1];
          puVar9[5] = uVar7;
          *(uint **)(uVar7 + 0x18) = puVar9;
          goto LAB_0100e6b4;
        }
      }
LAB_0100e9dc:
      abort();
      uVar10 = param_2[1];
      goto LAB_0100e6b4;
    }
  }
  else {
    puVar5 = (uint *)puVar8[2];
    if (((puVar5 < puVar11) || (puVar8 != (uint *)puVar5[3])) || (puVar8 != (uint *)puVar9[2])) {
LAB_0100e990:
      abort();
    }
    else {
      puVar5[3] = (uint)puVar9;
      puVar9[2] = (uint)puVar5;
    }
LAB_0100e830:
    if (puVar3 != (uint *)0x0) {
      uVar7 = puVar8[7];
      if (puVar8 != (uint *)param_1[uVar7 + 0x4c]) {
        puVar11 = (uint *)param_1[4];
        goto LAB_0100e850;
      }
      param_1[uVar7 + 0x4c] = (uint)puVar9;
      if (puVar9 == (uint *)0x0) {
        uVar10 = param_2[1];
LAB_0100e9b0:
        param_1[1] = param_1[1] & ~(1 << (uVar7 & 0xff));
        goto LAB_0100e6b4;
      }
      goto LAB_0100e870;
    }
  }
LAB_0100e92c:
  uVar10 = param_2[1];
LAB_0100e6b4:
  if (0xf < uVar2) {
    param_2[1] = param_3 | uVar10 & 1 | 2;
    *(uint *)((int)param_2 + param_3 + 4) = uVar2 | 3;
    *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) | 1;
    FUN_0100db5c(param_1,(int)param_2 + param_3,uVar2);
    return param_2;
  }
  param_2[1] = uVar4 | uVar10 & 1 | 2;
  *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) | 1;
  return param_2;
}



/* @ 0x100ea60  FUN_0100ea60 */

int FUN_0100ea60(undefined4 param_1,uint param_2)

{
  int iVar1;
  
  if (DAT_01049d48 == 0) {
    FUN_0100e394();
  }
  if (0x208 < param_2) {
    if (param_2 < -DAT_01049d4c - 0x208U) {
      iVar1 = FUN_0100d560(param_1,param_2);
      *(undefined4 *)(iVar1 + 0x1cc) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      return iVar1;
    }
  }
  return 0;
}



/* @ 0x100eae8  FUN_0100eae8 */

/* WARNING: Type propagation algorithm not settling */

uint * sys_alloc(uint *param_1,uint param_2)

{
  code *pcVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  uint uVar8;
  int iVar9;
  uint *puVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  undefined4 *puVar14;
  uint *puVar15;
  undefined4 *puVar16;
  uint *puVar17;
  uint uVar18;
  undefined4 *puVar19;
  uint uVar20;
  uint *puVar21;
  undefined4 *puVar22;
  uint *puVar23;
  undefined4 *puVar24;
  undefined4 *puVar25;
  uint uVar26;
  int iVar27;
  bool bVar28;
  undefined8 uVar29;
  
  if (param_2 < 0xf5) {
    if (param_2 < AL_I2C_INTR_MASK_START_DET_SHIFT + 1) {
      uVar8 = 2;
      uVar18 = 0x10;
    }
    else {
      uVar18 = param_2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT & 0xfffffff8;
      uVar8 = param_2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT >> 3;
    }
    uVar4 = *param_1;
    uVar13 = uVar4 >> (uVar8 & 0xff);
    if ((uVar13 & 3) != 0) {
      uVar8 = (~uVar13 & 1) + uVar8;
      uVar26 = param_1[uVar8 * 2 + 0xc];
      puVar3 = *(uint **)(uVar26 + 8);
      if (param_1 + uVar8 * 2 + 10 == puVar3) {
        *param_1 = uVar4 & ~(1 << (uVar8 & 0xff));
      }
      else if ((puVar3 < (uint *)param_1[4]) || (puVar3[3] != uVar26)) {
        abort();
      }
      else {
        puVar3[3] = (uint)(param_1 + uVar8 * 2 + 10);
        param_1[uVar8 * 2 + 0xc] = (uint)puVar3;
      }
      iVar9 = uVar26 + uVar8 * 8;
      *(uint *)(uVar26 + 4) = uVar8 * 8 | 3;
      *(uint *)(iVar9 + 4) = *(uint *)(iVar9 + 4) | 1;
      return (uint *)(uVar26 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
    }
    uVar26 = param_1[2];
    if (uVar18 <= uVar26) {
LAB_0100eb44:
      uVar8 = param_1[5];
      uVar13 = uVar26 - uVar18;
      iVar9 = uVar8 + uVar26;
      if (0xf < uVar13) {
        param_1[2] = uVar13;
        param_1[5] = uVar8 + uVar18;
        *(uint *)(uVar8 + uVar18 + 4) = uVar13 | 1;
        *(uint *)(uVar8 + uVar26) = uVar13;
        *(uint *)(uVar8 + 4) = uVar18 | 3;
        goto LAB_0100eb78;
      }
      uVar26 = uVar26 | 3;
LAB_0100ef9c:
      param_1[2] = 0;
      param_1[5] = 0;
      *(uint *)(uVar8 + 4) = uVar26;
      *(uint *)(iVar9 + 4) = *(uint *)(iVar9 + 4) | 1;
LAB_0100eb78:
      return (uint *)(uVar8 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
    }
    if (uVar13 != 0) {
      iVar9 = 1 << (uVar8 & 0xff);
      uVar8 = (iVar9 * -2 | iVar9 * 2) & uVar13 << (uVar8 & 0xff);
      uVar8 = (-uVar8 & uVar8) - 1;
      uVar11 = uVar8 >> 0xc & 0x10;
      uVar8 = uVar8 >> uVar11;
      uVar13 = uVar8 >> 5 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      uVar8 = uVar8 >> uVar13;
      uVar5 = uVar8 >> 2 & 4;
      uVar8 = uVar8 >> uVar5;
      uVar6 = uVar8 >> 1 & 2;
      uVar8 = uVar8 >> uVar6;
      uVar20 = -((int)(uVar8 << 0x1e) >> 0x1f);
      uVar13 = (uVar11 | uVar13) + uVar5 + uVar6 + uVar20 + (uVar8 >> (uVar20 & 0xff));
      uVar8 = param_1[uVar13 * 2 + 0xc];
      puVar3 = *(uint **)(uVar8 + 8);
      if (param_1 + uVar13 * 2 + 10 == puVar3) {
        *param_1 = uVar4 & ~(1 << (uVar13 & 0xff));
      }
      else if ((puVar3 < (uint *)param_1[4]) || (puVar3[3] != uVar8)) {
        abort();
        uVar26 = param_1[2];
      }
      else {
        puVar3[3] = (uint)(param_1 + uVar13 * 2 + 10);
        param_1[uVar13 * 2 + 0xc] = (uint)puVar3;
      }
      uVar4 = uVar13 * 8 - uVar18;
      *(uint *)(uVar8 + 4) = uVar18 | 3;
      *(uint *)(uVar8 + uVar18 + 4) = uVar4 | 1;
      *(uint *)(uVar8 + uVar13 * 8) = uVar4;
      if (uVar26 != 0) {
        uVar26 = uVar26 >> 3;
        uVar5 = param_1[5];
        uVar13 = 1 << (uVar26 & 0xff);
        puVar3 = param_1 + uVar26 * 2 + 10;
        if ((*param_1 & uVar13) == 0) {
          *param_1 = *param_1 | uVar13;
          puVar21 = puVar3;
        }
        else {
          puVar21 = (uint *)param_1[uVar26 * 2 + 0xc];
          if ((uint *)param_1[uVar26 * 2 + 0xc] < (uint *)param_1[4]) {
            abort();
            puVar21 = puVar3;
          }
        }
        param_1[uVar26 * 2 + 0xc] = uVar5;
        puVar21[3] = uVar5;
        *(uint **)(uVar5 + 8) = puVar21;
        *(uint **)(uVar5 + 0xc) = puVar3;
      }
      param_1[2] = uVar4;
      param_1[5] = uVar8 + uVar18;
      return (uint *)(uVar8 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
    }
    uVar26 = param_1[1];
    if (uVar26 != 0) {
      uVar8 = (-uVar26 & uVar26) - 1;
      uVar11 = uVar8 >> 0xc & 0x10;
      uVar8 = uVar8 >> uVar11;
      uVar13 = uVar8 >> 5 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      uVar8 = uVar8 >> uVar13;
      uVar4 = uVar8 >> 2 & 4;
      uVar8 = uVar8 >> uVar4;
      uVar5 = uVar8 >> 1 & 2;
      uVar8 = uVar8 >> uVar5;
      uVar6 = -((int)(uVar8 << 0x1e) >> 0x1f);
      puVar7 = (undefined4 *)
               param_1[(uVar11 | uVar13) + uVar4 + uVar5 + uVar6 + (uVar8 >> (uVar6 & 0xff)) + 0x4c]
      ;
      puVar19 = puVar7;
      uVar5 = (puVar7[1] & 0xfffffff8) - uVar18;
      while ((puVar25 = (undefined4 *)puVar7[4], (undefined4 *)puVar7[4] != (undefined4 *)0x0 ||
             (puVar25 = (undefined4 *)puVar7[5], (undefined4 *)puVar7[5] != (undefined4 *)0x0))) {
        puVar7 = puVar25;
        uVar8 = (puVar7[1] & 0xfffffff8) - uVar18;
        if (uVar8 < uVar5) {
          puVar19 = puVar7;
          uVar5 = uVar8;
        }
      }
      puVar7 = (undefined4 *)param_1[4];
      if ((puVar19 < puVar7) ||
         (puVar25 = (undefined4 *)((int)puVar19 + uVar18), puVar25 <= puVar19)) {
LAB_0100f9bc:
        abort();
      }
      else {
        puVar22 = (undefined4 *)puVar19[3];
        puVar24 = (undefined4 *)puVar19[6];
        if (puVar22 == puVar19) {
          if ((undefined4 *)puVar19[5] != (undefined4 *)0x0) {
            puVar14 = (undefined4 *)puVar19[5];
            puVar2 = puVar19 + 5;
LAB_0100fb94:
            do {
              do {
                puVar16 = puVar2;
                puVar22 = puVar14;
                puVar14 = (undefined4 *)puVar22[5];
                puVar2 = puVar22 + 5;
              } while (puVar14 != (undefined4 *)0x0);
              puVar14 = (undefined4 *)puVar22[4];
              puVar2 = puVar22 + 4;
            } while (puVar14 != (undefined4 *)0x0);
            if (puVar16 < puVar7) goto LAB_0100fc20;
            *puVar16 = 0;
            goto LAB_0100ee94;
          }
          puVar22 = (undefined4 *)puVar19[4];
          if (puVar22 != (undefined4 *)0x0) {
            puVar14 = puVar22;
            puVar2 = puVar19 + 4;
            goto LAB_0100fb94;
          }
          if (puVar24 != (undefined4 *)0x0) {
            uVar8 = puVar19[7];
            if (puVar19 == (undefined4 *)param_1[uVar8 + 0x4c]) {
              param_1[uVar8 + 0x4c] = 0;
              goto LAB_0100fc38;
            }
            goto LAB_0100eeb4;
          }
        }
        else {
          puVar14 = (undefined4 *)puVar19[2];
          if (((puVar14 < puVar7) || ((undefined4 *)puVar14[3] != puVar19)) ||
             ((undefined4 *)puVar22[2] != puVar19)) {
LAB_0100fc20:
            abort();
          }
          else {
            puVar14[3] = puVar22;
            puVar22[2] = puVar14;
          }
LAB_0100ee94:
          if (puVar24 != (undefined4 *)0x0) {
            uVar8 = puVar19[7];
            if ((undefined4 *)param_1[uVar8 + 0x4c] == puVar19) {
              param_1[uVar8 + 0x4c] = (uint)puVar22;
              if (puVar22 != (undefined4 *)0x0) goto LAB_0100eed4;
              uVar26 = param_1[1];
LAB_0100fc38:
              param_1[1] = uVar26 & ~(1 << (uVar8 & 0xff));
            }
            else {
              puVar7 = (undefined4 *)param_1[4];
LAB_0100eeb4:
              if (puVar24 < puVar7) {
                abort();
              }
              else if ((undefined4 *)puVar24[4] == puVar19) {
                puVar24[4] = puVar22;
              }
              else {
                puVar24[5] = puVar22;
              }
              if (puVar22 != (undefined4 *)0x0) {
LAB_0100eed4:
                puVar7 = (undefined4 *)param_1[4];
                if (puVar22 < puVar7) {
LAB_0100fdc8:
                  abort();
                }
                else {
                  puVar14 = (undefined4 *)puVar19[4];
                  puVar22[6] = puVar24;
                  if (puVar14 != (undefined4 *)0x0) {
                    if (puVar14 < puVar7) {
                      abort();
                    }
                    else {
                      puVar22[4] = puVar14;
                      puVar14[6] = puVar22;
                    }
                  }
                  uVar26 = puVar19[5];
                  if (uVar26 != 0) {
                    if (uVar26 < param_1[4]) goto LAB_0100fdc8;
                    puVar22[5] = uVar26;
                    *(undefined4 **)(uVar26 + 0x18) = puVar22;
                  }
                }
              }
            }
          }
        }
        if (uVar5 < 0x10) {
LAB_0100f8d4:
          uVar5 = uVar18 + uVar5;
          uVar26 = *(uint *)((int)puVar19 + uVar5 + 4);
          puVar19[1] = uVar5 | 3;
          *(uint *)((int)puVar19 + uVar5 + 4) = uVar26 | 1;
        }
        else {
          uVar26 = param_1[2];
          puVar19[1] = uVar18 | 3;
          puVar25[1] = uVar5 | 1;
          *(uint *)((int)puVar25 + uVar5) = uVar5;
          if (uVar26 != 0) {
            uVar26 = uVar26 >> 3;
            uVar13 = param_1[5];
            uVar8 = 1 << (uVar26 & 0xff);
            puVar3 = param_1 + uVar26 * 2 + 10;
            if ((*param_1 & uVar8) == 0) {
              *param_1 = *param_1 | uVar8;
              puVar21 = puVar3;
            }
            else {
              puVar21 = (uint *)param_1[uVar26 * 2 + 0xc];
              if ((uint *)param_1[uVar26 * 2 + 0xc] < (uint *)param_1[4]) {
                abort();
                puVar21 = puVar3;
              }
            }
            param_1[uVar26 * 2 + 0xc] = uVar13;
            puVar21[3] = uVar13;
            *(uint **)(uVar13 + 8) = puVar21;
            *(uint **)(uVar13 + 0xc) = puVar3;
          }
          param_1[2] = uVar5;
          param_1[5] = (uint)puVar25;
        }
LAB_0100f248:
        if (puVar19 + 2 != (uint *)0x0) {
          return puVar19 + 2;
        }
      }
      uVar26 = param_1[2];
      goto LAB_0100ecc8;
    }
LAB_0100ecd0:
    uVar26 = param_1[3];
    if (uVar18 < uVar26) goto LAB_0100ecdc;
  }
  else {
    uVar26 = param_1[2];
    if (param_2 < 0xffffffc0) {
      uVar8 = param_1[1];
      uVar18 = param_2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT & 0xfffffff8;
      if (uVar8 != 0) {
        uVar4 = param_2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT >> 8;
        uVar5 = -uVar18;
        uVar13 = uVar18;
        if (uVar4 < 0x10000) {
          uVar11 = uVar4 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
          iVar9 = uVar4 << uVar11;
          uVar4 = iVar9 - 0x1000U >> 0x10 & 4;
          iVar9 = iVar9 << uVar4;
          uVar6 = iVar9 - 0x4000U >> 0x10 & 2;
          iVar9 = ((uint)(iVar9 << uVar6) >> 0xf) - ((uVar11 | uVar4) + uVar6);
          uVar4 = (uVar18 >> (iVar9 + 0x15U & 0xff) & 1) + (iVar9 + 0xe) * 2;
          puVar7 = (undefined4 *)param_1[uVar4 + 0x4c];
          if (puVar7 == (undefined4 *)0x0) {
LAB_0100f850:
            uVar13 = uVar8 & -2 << (uVar4 & 0xff);
            if ((uVar13 == 0) ||
               (uVar13 = (-uVar13 & uVar13) - 1, uVar12 = uVar13 >> 0xc & 0x10,
               uVar13 = uVar13 >> uVar12, uVar4 = uVar13 >> 5 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT,
               uVar13 = uVar13 >> uVar4, uVar6 = uVar13 >> 2 & 4, uVar13 = uVar13 >> uVar6,
               uVar11 = uVar13 >> 1 & 2, uVar13 = uVar13 >> uVar11,
               uVar20 = -((int)(uVar13 << 0x1e) >> 0x1f),
               puVar7 = (undefined4 *)
                        param_1[(uVar12 | uVar4) + uVar6 + uVar11 + uVar20 +
                                (uVar13 >> (uVar20 & 0xff)) + 0x4c], puVar7 == (undefined4 *)0x0))
            goto LAB_0100ecc8;
            uVar11 = puVar7[1] & 0xfffffff8;
            puVar19 = (undefined4 *)0x0;
LAB_0100f0cc:
            while( true ) {
              puVar25 = (undefined4 *)puVar7[4];
              if (uVar11 - uVar18 < uVar5) {
                puVar19 = puVar7;
                uVar5 = uVar11 - uVar18;
              }
              if ((puVar25 == (undefined4 *)0x0) &&
                 (puVar25 = (undefined4 *)puVar7[5], puVar25 == (undefined4 *)0x0)) break;
              uVar11 = puVar25[1] & 0xfffffff8;
              puVar7 = puVar25;
            }
          }
          else {
            if (uVar4 != 0x1f) {
              uVar13 = uVar18 << (0x19 - (uVar4 >> 1) & 0xff);
            }
LAB_0100f040:
            puVar25 = (undefined4 *)0x0;
            puVar19 = (undefined4 *)0x0;
            uVar6 = uVar5;
            while( true ) {
              uVar11 = puVar7[1] & 0xfffffff8;
              uVar5 = uVar11 - uVar18;
              if ((uVar5 < uVar6) && (puVar19 = puVar7, uVar6 = uVar5, uVar5 == 0))
              goto LAB_0100f0cc;
              uVar5 = uVar6;
              puVar22 = (undefined4 *)puVar7[5];
              puVar7 = (undefined4 *)puVar7[4 - ((int)uVar13 >> 0x1f)];
              if (puVar22 != (undefined4 *)0x0 && puVar22 != puVar7) {
                puVar25 = puVar22;
              }
              if (puVar7 == (undefined4 *)0x0) break;
              uVar13 = uVar13 << 1;
              uVar6 = uVar5;
            }
            if (puVar19 == (undefined4 *)0x0 && puVar25 == (undefined4 *)0x0) goto LAB_0100f850;
            if (puVar25 != (undefined4 *)0x0) {
              uVar11 = puVar25[1] & 0xfffffff8;
              puVar7 = puVar25;
              goto LAB_0100f0cc;
            }
          }
          if ((puVar19 != (undefined4 *)0x0) && (uVar5 < uVar26 - uVar18)) {
            puVar7 = (undefined4 *)param_1[4];
            if ((puVar19 < puVar7) ||
               (puVar25 = (undefined4 *)((int)puVar19 + uVar18), puVar25 <= puVar19))
            goto LAB_0100f9bc;
            puVar22 = (undefined4 *)puVar19[3];
            puVar24 = (undefined4 *)puVar19[6];
            if (puVar19 == puVar22) {
              if ((undefined4 *)puVar19[5] != (undefined4 *)0x0) {
                puVar14 = (undefined4 *)puVar19[5];
                puVar2 = puVar19 + 5;
LAB_0100fa9c:
                do {
                  do {
                    puVar16 = puVar2;
                    puVar22 = puVar14;
                    puVar14 = (undefined4 *)puVar22[5];
                    puVar2 = puVar22 + 5;
                  } while (puVar14 != (undefined4 *)0x0);
                  puVar14 = (undefined4 *)puVar22[4];
                  puVar2 = puVar22 + 4;
                } while (puVar14 != (undefined4 *)0x0);
                if (puVar16 < puVar7) goto LAB_0100fb7c;
                *puVar16 = 0;
                goto LAB_0100f15c;
              }
              puVar22 = (undefined4 *)puVar19[4];
              if (puVar22 != (undefined4 *)0x0) {
                puVar14 = puVar22;
                puVar2 = puVar19 + 4;
                goto LAB_0100fa9c;
              }
              if (puVar24 != (undefined4 *)0x0) {
                uVar26 = puVar19[7];
                if (puVar19 == (undefined4 *)param_1[uVar26 + 0x4c]) {
                  param_1[uVar26 + 0x4c] = 0;
                  goto LAB_0100fbdc;
                }
                goto LAB_0100f17c;
              }
            }
            else {
              puVar14 = (undefined4 *)puVar19[2];
              if (((puVar14 < puVar7) || (puVar19 != (undefined4 *)puVar14[3])) ||
                 (puVar19 != (undefined4 *)puVar22[2])) {
LAB_0100fb7c:
                abort();
              }
              else {
                puVar14[3] = puVar22;
                puVar22[2] = puVar14;
              }
LAB_0100f15c:
              if (puVar24 != (undefined4 *)0x0) {
                uVar26 = puVar19[7];
                if (puVar19 == (undefined4 *)param_1[uVar26 + 0x4c]) {
                  param_1[uVar26 + 0x4c] = (uint)puVar22;
                  if (puVar22 != (undefined4 *)0x0) goto LAB_0100f19c;
                  uVar8 = param_1[1];
LAB_0100fbdc:
                  param_1[1] = uVar8 & ~(1 << (uVar26 & 0xff));
                }
                else {
                  puVar7 = (undefined4 *)param_1[4];
LAB_0100f17c:
                  if (puVar24 < puVar7) {
                    abort();
                  }
                  else if (puVar19 == (undefined4 *)puVar24[4]) {
                    puVar24[4] = puVar22;
                  }
                  else {
                    puVar24[5] = puVar22;
                  }
                  if (puVar22 != (undefined4 *)0x0) {
LAB_0100f19c:
                    puVar7 = (undefined4 *)param_1[4];
                    if (puVar22 < puVar7) {
LAB_0100fd44:
                      abort();
                    }
                    else {
                      puVar14 = (undefined4 *)puVar19[4];
                      puVar22[6] = puVar24;
                      if (puVar14 != (undefined4 *)0x0) {
                        if (puVar14 < puVar7) {
                          abort();
                        }
                        else {
                          puVar22[4] = puVar14;
                          puVar14[6] = puVar22;
                        }
                      }
                      uVar26 = puVar19[5];
                      if (uVar26 != 0) {
                        if (uVar26 < param_1[4]) goto LAB_0100fd44;
                        puVar22[5] = uVar26;
                        *(undefined4 **)(uVar26 + 0x18) = puVar22;
                      }
                    }
                  }
                }
              }
            }
            if (uVar5 < 0x10) goto LAB_0100f8d4;
            uVar26 = uVar5 >> 3;
            puVar19[1] = uVar18 | 3;
            puVar25[1] = uVar5 | 1;
            *(uint *)((int)puVar25 + uVar5) = uVar5;
            if (uVar26 < 0x20) {
              uVar8 = 1 << (uVar26 & 0xff);
              puVar3 = param_1 + uVar26 * 2 + 10;
              if ((*param_1 & uVar8) == 0) {
                *param_1 = *param_1 | uVar8;
                puVar21 = puVar3;
              }
              else {
                puVar21 = (uint *)param_1[uVar26 * 2 + 0xc];
                if ((uint *)param_1[uVar26 * 2 + 0xc] < (uint *)param_1[4]) {
                  abort();
                  puVar21 = puVar3;
                }
              }
              param_1[uVar26 * 2 + 0xc] = (uint)puVar25;
              puVar21[3] = (uint)puVar25;
              puVar25[2] = puVar21;
              puVar25[3] = puVar3;
            }
            else {
              uVar26 = uVar5 >> 8;
              if (uVar26 < 0x10000) {
                uVar8 = uVar26 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
                iVar9 = uVar26 << uVar8;
                uVar13 = iVar9 - 0x1000U >> 0x10 & 4;
                iVar9 = iVar9 << uVar13;
                uVar26 = iVar9 - 0x4000U >> 0x10 & 2;
                iVar9 = ((uint)(iVar9 << uVar26) >> 0xf) - ((uVar8 | uVar13) + uVar26);
                uVar8 = (uVar5 >> (iVar9 + 0x15U & 0xff) & 1) + (iVar9 + 0xe) * 2;
                iVar27 = uVar8 + 0x4c;
                uVar26 = 1 << (uVar8 & 0xff);
                iVar9 = iVar27 * 4;
              }
              else {
                iVar9 = 0x1ac;
                uVar26 = 0x80000000;
                uVar8 = 0x1f;
                iVar27 = 0x6b;
              }
              uVar13 = param_1[1];
              puVar25[4] = 0;
              puVar25[5] = 0;
              puVar25[7] = uVar8;
              if ((uVar13 & uVar26) == 0) {
                param_1[1] = uVar13 | uVar26;
                param_1[iVar27] = (uint)puVar25;
                puVar25[2] = puVar25;
                puVar25[3] = puVar25;
                puVar25[6] = (int)param_1 + iVar9;
              }
              else {
                uVar13 = param_1[iVar27];
                uVar26 = uVar5;
                if (uVar8 != 0x1f) {
                  uVar26 = uVar5 << (0x19 - (uVar8 >> 1) & 0xff);
                }
                do {
                  uVar8 = uVar13;
                  if (uVar5 == (*(uint *)(uVar8 + 4) & 0xfffffff8)) {
                    uVar26 = *(uint *)(uVar8 + 8);
                    if (uVar26 < param_1[4] || uVar8 < param_1[4]) goto LAB_0100fed4;
                    *(undefined4 **)(uVar26 + 0xc) = puVar25;
                    *(undefined4 **)(uVar8 + 8) = puVar25;
                    puVar25[2] = uVar26;
                    puVar25[3] = uVar8;
                    puVar25[6] = 0;
                    goto LAB_0100f248;
                  }
                  iVar9 = (int)uVar26 >> 0x1f;
                  uVar26 = uVar26 << 1;
                  iVar9 = 4 - iVar9;
                  uVar13 = *(uint *)(uVar8 + iVar9 * 4);
                } while (uVar13 != 0);
                if (uVar8 + iVar9 * 4 < param_1[4]) {
LAB_0100fed4:
                  abort();
                }
                else {
                  *(undefined4 **)(uVar8 + iVar9 * 4) = puVar25;
                  puVar25[2] = puVar25;
                  puVar25[3] = puVar25;
                  puVar25[6] = uVar8;
                }
              }
            }
            goto LAB_0100f248;
          }
        }
        else {
          puVar7 = (undefined4 *)param_1[0x6b];
          if (puVar7 != (undefined4 *)0x0) {
            uVar4 = 0x1f;
            goto LAB_0100f040;
          }
        }
      }
LAB_0100ecc8:
      if (uVar18 <= uVar26) goto LAB_0100eb44;
      goto LAB_0100ecd0;
    }
    if (uVar26 == 0xffffffff) {
      uVar8 = param_1[5];
      iVar9 = uVar8 - 1;
      uVar26 = 0xffffffff;
      goto LAB_0100ef9c;
    }
    uVar18 = 0xffffffff;
  }
  if (DAT_01049d48 == 0) {
    FUN_0100e394();
  }
  if ((((param_1[0x6f] & 1) == 0) || (uVar18 < DAT_01049d54)) || (param_1[3] == 0)) {
LAB_0100ebd0:
    uVar8 = DAT_01049d50 + 0x2f + uVar18 & -DAT_01049d50;
    if (uVar8 <= uVar18) {
      return (uint *)0x0;
    }
    uVar26 = param_1[0x6e];
    if (uVar26 != 0) {
      uVar13 = param_1[0x6c];
      goto LAB_0100ebfc;
    }
  }
  else {
    uVar26 = param_1[0x6e];
    uVar8 = DAT_01049d4c + 0x1e + uVar18 & -DAT_01049d4c;
    if ((uVar26 == 0) ||
       (uVar13 = param_1[0x6c], uVar8 + uVar13 <= uVar26 && uVar13 < uVar8 + uVar13)) {
      if ((uVar18 < uVar8) &&
         (puVar3 = (uint *)FUN_0100d40c(0,uVar8,3,2,0xffffffff,0), puVar3 != (uint *)0xffffffff)) {
        puVar21 = puVar3 + 2;
        if (((uint)puVar21 & 7) == 0) {
          uVar26 = 0;
          puVar10 = puVar3;
          uVar13 = uVar8;
        }
        else {
          uVar26 = -(int)puVar21 & 7;
          puVar10 = (uint *)((int)puVar3 + uVar26);
          uVar13 = uVar8 - uVar26;
          puVar21 = puVar10 + 2;
        }
        puVar15 = (uint *)param_1[4];
        *puVar10 = uVar26;
        puVar10[1] = uVar13 - 0x10;
        *(undefined4 *)((int)puVar10 + (uVar13 - 0xc)) = 7;
        *(undefined4 *)((int)puVar10 + (uVar13 - 8)) = 0;
        if (puVar15 == (uint *)0x0 || puVar3 < puVar15) {
          param_1[4] = (uint)puVar3;
        }
        uVar8 = uVar8 + param_1[0x6c];
        param_1[0x6c] = uVar8;
        if (param_1[0x6d] < uVar8) {
          param_1[0x6d] = uVar8;
        }
        if (puVar21 != (uint *)0x0) {
          return puVar21;
        }
      }
      goto LAB_0100ebd0;
    }
    uVar8 = DAT_01049d50 + 0x2f + uVar18 & -DAT_01049d50;
    if (uVar8 <= uVar18) {
      return (uint *)0x0;
    }
LAB_0100ebfc:
    uVar5 = uVar8 + uVar13;
    uVar4 = uVar26;
    if (uVar5 <= uVar26) {
      uVar4 = uVar13;
    }
    if ((uVar5 > uVar26 || uVar5 < uVar13) || uVar4 == uVar5) {
      return (uint *)0x0;
    }
  }
  puVar3 = (uint *)FUN_0100d40c(0,uVar8,3,2,0xffffffff,0);
  if (puVar3 == (uint *)0xffffffff) goto LAB_0100f434;
  puVar21 = (uint *)param_1[6];
  uVar26 = uVar8 + param_1[0x6c];
  param_1[0x6c] = uVar26;
  if (param_1[0x6d] < uVar26) {
    param_1[0x6d] = uVar26;
  }
  uVar26 = DAT_01049d48;
  if (puVar21 != (uint *)0x0) {
    puVar15 = param_1 + 0x70;
    puVar10 = puVar15;
    do {
      if (puVar3 == (uint *)((int)*puVar10 + puVar10[1])) {
        if (((((puVar10[3] & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) == 0) && ((puVar10[3] & 1) != 0)) &&
            ((uint *)*puVar10 <= puVar21)) && (puVar21 < puVar3)) {
          uVar26 = param_1[3];
          bVar28 = ((uint)(puVar21 + 2) & 7) == 0;
          puVar3 = puVar21 + 2;
          if (bVar28) {
            puVar3 = puVar21;
          }
          puVar10[1] = uVar8 + puVar10[1];
          uVar13 = DAT_01049d58;
          uVar8 = uVar8 + uVar26;
          uVar26 = uVar8;
          if (!bVar28) {
            uVar26 = -(int)puVar3;
            puVar3 = (uint *)((int)puVar21 + (uVar26 & 7));
            uVar26 = uVar8 - (uVar26 & 7);
          }
          param_1[3] = uVar26;
          param_1[6] = (uint)puVar3;
          puVar3[1] = uVar26 | 1;
          *(undefined4 *)((int)puVar21 + uVar8 + 4) = 0x28;
          param_1[7] = uVar13;
          goto LAB_0100f42c;
        }
        break;
      }
      puVar10 = (uint *)puVar10[2];
    } while (puVar10 != (uint *)0x0);
    puVar23 = (uint *)((int)puVar3 + uVar8);
    puVar10 = puVar15;
    if (puVar3 < (uint *)param_1[4]) {
      param_1[4] = (uint)puVar3;
    }
    while ((uint *)*puVar10 != puVar23) {
      puVar10 = (uint *)puVar10[2];
      if (puVar10 == (uint *)0x0) goto LAB_0100f26c;
    }
    if (((puVar10[3] & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) != 0) || ((puVar10[3] & 1) == 0))
    goto LAB_0100f26c;
    puVar15 = puVar3 + 2;
    *puVar10 = (uint)puVar3;
    puVar10[1] = puVar10[1] + uVar8;
    if (((uint)puVar15 & 7) != 0) {
      puVar3 = (uint *)((int)puVar3 + (-(int)puVar15 & 7U));
      puVar15 = puVar3 + 2;
    }
    if (((uint)(puVar23 + 2) & 7) != 0) {
      puVar23 = (uint *)((int)puVar23 + (-(int)(puVar23 + 2) & 7U));
    }
    uVar8 = (int)puVar23 + (-uVar18 - (int)puVar3);
    uVar26 = (int)puVar3 + uVar18;
    puVar3[1] = uVar18 | 3;
    if (puVar21 == puVar23) {
      uVar18 = param_1[3];
      param_1[6] = uVar26;
      param_1[3] = uVar8 + uVar18;
      *(uint *)(uVar26 + 4) = uVar8 + uVar18 | 1;
      return puVar15;
    }
    if ((uint *)param_1[5] == puVar23) {
      param_1[5] = uVar26;
      uVar8 = uVar8 + param_1[2];
      param_1[2] = uVar8;
      *(uint *)(uVar26 + 4) = uVar8 | 1;
      *(uint *)(uVar26 + uVar8) = uVar8;
      return puVar15;
    }
    uVar18 = puVar23[1];
    if ((uVar18 & 3) != 1) goto LAB_0100f4dc;
    puVar3 = (uint *)puVar23[3];
    uVar13 = uVar18 >> 3;
    if (uVar13 < 0x20) {
      puVar21 = (uint *)puVar23[2];
      if ((puVar21 != param_1 + uVar13 * 2 + 10) &&
         ((puVar21 < (uint *)param_1[4] || ((uint *)puVar21[3] != puVar23)))) goto LAB_0101017c;
      if (puVar21 == puVar3) {
        *param_1 = *param_1 & ~(1 << (uVar13 & 0xff));
        goto LAB_0100ff1c;
      }
      if ((param_1 + uVar13 * 2 + 10 != puVar3) &&
         ((puVar3 < (uint *)param_1[4] || ((uint *)puVar3[2] != puVar23)))) goto LAB_0101017c;
      puVar21[3] = (uint)puVar3;
      puVar3[2] = (uint)puVar21;
      goto LAB_0100ff1c;
    }
    uVar13 = puVar23[6];
    if (puVar23 == puVar3) {
      if ((uint *)puVar23[5] == (uint *)0x0) {
        puVar3 = (uint *)puVar23[4];
        if (puVar3 == (uint *)0x0) goto LAB_01010008;
        puVar21 = puVar3;
        puVar10 = puVar23 + 4;
      }
      else {
        puVar21 = (uint *)puVar23[5];
        puVar10 = puVar23 + 5;
      }
      do {
        do {
          puVar17 = puVar10;
          puVar3 = puVar21;
          puVar21 = (uint *)puVar3[5];
          puVar10 = puVar3 + 5;
        } while (puVar21 != (uint *)0x0);
        puVar21 = (uint *)puVar3[4];
        puVar10 = puVar3 + 4;
      } while (puVar21 != (uint *)0x0);
      if (puVar17 < (uint *)param_1[4]) {
        abort();
      }
      else {
        *puVar17 = 0;
      }
    }
    else {
      uVar4 = puVar23[2];
      if (((uVar4 < param_1[4]) || (*(uint **)(uVar4 + 0xc) != puVar23)) ||
         ((uint *)puVar3[2] != puVar23)) {
        abort();
      }
      else {
        *(uint **)(uVar4 + 0xc) = puVar3;
        puVar3[2] = uVar4;
      }
    }
LAB_01010008:
    if (uVar13 == 0) goto LAB_0100ff1c;
    uVar4 = puVar23[7];
    if ((uint *)param_1[uVar4 + 0x4c] == puVar23) {
      param_1[uVar4 + 0x4c] = (uint)puVar3;
      if (puVar3 == (uint *)0x0) {
        param_1[1] = param_1[1] & ~(1 << (uVar4 & 0xff));
        goto LAB_0100ff1c;
      }
    }
    else {
      if (uVar13 < param_1[4]) {
        abort();
      }
      else if (*(uint **)(uVar13 + 0x10) == puVar23) {
        *(uint **)(uVar13 + 0x10) = puVar3;
      }
      else {
        *(uint **)(uVar13 + 0x14) = puVar3;
      }
      if (puVar3 == (uint *)0x0) goto LAB_0100ff1c;
    }
    puVar21 = (uint *)param_1[4];
    if (puVar21 <= puVar3) {
      puVar10 = (uint *)puVar23[4];
      puVar3[6] = uVar13;
      if (puVar10 != (uint *)0x0) {
        if (puVar10 < puVar21) {
          abort();
        }
        else {
          puVar3[4] = (uint)puVar10;
          puVar10[6] = (uint)puVar3;
        }
      }
      uVar13 = puVar23[5];
      if (uVar13 == 0) goto LAB_0100ff1c;
      if (param_1[4] <= uVar13) {
        puVar3[5] = uVar13;
        *(uint **)(uVar13 + 0x18) = puVar3;
        goto LAB_0100ff1c;
      }
    }
LAB_0101017c:
    abort();
LAB_0100ff1c:
    puVar23 = (uint *)((int)puVar23 + (uVar18 & 0xfffffff8));
    uVar8 = uVar8 + (uVar18 & 0xfffffff8);
    uVar18 = puVar23[1];
LAB_0100f4dc:
    uVar13 = uVar8 >> 3;
    puVar23[1] = uVar18 & 0xfffffffe;
    *(uint *)(uVar26 + 4) = uVar8 | 1;
    *(uint *)(uVar26 + uVar8) = uVar8;
    if (uVar13 < 0x20) {
      uVar18 = 1 << (uVar13 & 0xff);
      puVar3 = param_1 + uVar13 * 2 + 10;
      if ((*param_1 & uVar18) == 0) {
        *param_1 = *param_1 | uVar18;
        puVar21 = puVar3;
      }
      else {
        puVar21 = (uint *)param_1[uVar13 * 2 + 0xc];
        if ((uint *)param_1[uVar13 * 2 + 0xc] < (uint *)param_1[4]) {
          abort();
          puVar21 = puVar3;
        }
      }
      param_1[uVar13 * 2 + 0xc] = uVar26;
      puVar21[3] = uVar26;
      *(uint **)(uVar26 + 8) = puVar21;
      *(uint **)(uVar26 + 0xc) = puVar3;
      return puVar15;
    }
    uVar18 = uVar8 >> 8;
    if (uVar18 < 0x10000) {
      uVar4 = uVar18 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      iVar9 = uVar18 << uVar4;
      uVar18 = iVar9 - 0x1000U >> 0x10 & 4;
      iVar9 = iVar9 << uVar18;
      uVar13 = iVar9 - 0x4000U >> 0x10 & 2;
      iVar9 = ((uint)(iVar9 << uVar13) >> 0xf) - ((uVar4 | uVar18) + uVar13);
      uVar18 = (uVar8 >> (iVar9 + 0x15U & 0xff) & 1) + (iVar9 + 0xe) * 2;
    }
    else {
      uVar18 = 0x1f;
    }
    uVar5 = param_1[1];
    *(uint *)(uVar26 + 0x1c) = uVar18;
    uVar13 = 0;
    uVar4 = 1 << (uVar18 & 0xff);
    *(undefined4 *)(uVar26 + 0x10) = 0;
    *(undefined4 *)(uVar26 + 0x14) = 0;
    if ((uVar5 & uVar4) == 0) {
      param_1[1] = uVar5 | uVar4;
      param_1[uVar18 + 0x4c] = uVar26;
      *(uint *)(uVar26 + 8) = uVar26;
      *(uint *)(uVar26 + 0xc) = uVar26;
      *(uint **)(uVar26 + 0x18) = param_1 + uVar18 + 0x4c;
      return puVar15;
    }
    if (uVar18 != 0x1f) {
      uVar13 = uVar18 >> 1;
    }
    if (uVar18 != 0x1f) {
      uVar13 = 0x19 - uVar13;
    }
    iVar9 = uVar8 << (uVar13 & 0xff);
    uVar18 = param_1[uVar18 + 0x4c];
    do {
      uVar13 = uVar18;
      if (uVar8 == (*(uint *)(uVar13 + 4) & 0xfffffff8)) {
        uVar18 = *(uint *)(uVar13 + 8);
        if (param_1[4] <= uVar13 && param_1[4] <= uVar18) {
          *(uint *)(uVar18 + 0xc) = uVar26;
          *(uint *)(uVar13 + 8) = uVar26;
          *(uint *)(uVar26 + 8) = uVar18;
          *(uint *)(uVar26 + 0xc) = uVar13;
          *(undefined4 *)(uVar26 + 0x18) = 0;
          return puVar15;
        }
        goto LAB_01010140;
      }
      iVar27 = iVar9 >> 0x1f;
      iVar9 = iVar9 << 1;
      iVar27 = 4 - iVar27;
      uVar18 = *(uint *)(uVar13 + iVar27 * 4);
    } while (uVar18 != 0);
    if (param_1[4] <= uVar13 + iVar27 * 4) {
      *(uint *)(uVar13 + iVar27 * 4) = uVar26;
      *(uint *)(uVar26 + 8) = uVar26;
      *(uint *)(uVar26 + 0xc) = uVar26;
      *(uint *)(uVar26 + 0x18) = uVar13;
      return puVar15;
    }
LAB_01010140:
    abort();
    return puVar15;
  }
  param_1[8] = 0xfff;
  param_1[0x70] = (uint)puVar3;
  param_1[9] = uVar26;
  param_1[0x71] = uVar8;
  puVar21 = param_1 + 10;
  if ((uint *)param_1[4] == (uint *)0x0 || puVar3 < (uint *)param_1[4]) {
    param_1[4] = (uint)puVar3;
  }
  param_1[0x73] = 1;
  do {
    puVar21[2] = (uint)puVar21;
    puVar21[3] = (uint)puVar21;
    uVar13 = DAT_01049d58;
    puVar21 = puVar21 + 2;
  } while (param_1 + 0x4a != puVar21);
  uVar26 = (int)param_1 + (param_1[-1] & 0xfffffff8);
  uVar4 = (int)param_1 + (param_1[-1] & 0xfffffff8) + -AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  iVar9 = uVar8 - uVar4;
  iVar27 = iVar9 + -0x28;
  uVar8 = uVar4;
  if ((uVar26 & 7) != 0) {
    uVar26 = -uVar26 & 7;
    iVar27 = (iVar9 + -0x28) - uVar26;
    uVar8 = uVar4 + uVar26;
  }
  uVar26 = (int)puVar3 + iVar27;
  param_1[3] = uVar26;
  param_1[6] = uVar8;
  *(uint *)(uVar8 + 4) = uVar26 | 1;
  *(undefined4 *)((int)puVar3 + uVar4 + iVar9 + -0x24) = 0x28;
  param_1[7] = uVar13;
  goto LAB_0100f42c;
LAB_0100f26c:
  while ((puVar21 < (uint *)*puVar15 ||
         (puVar10 = (uint *)((int)*puVar15 + puVar15[1]), puVar10 <= puVar21))) {
    puVar15 = (uint *)puVar15[2];
    if (puVar15 == (uint *)0x0) {
                    /* WARNING: Does not return */
      pcVar1 = (code *)software_udf(0,0x10101f0);
      (*pcVar1)();
    }
  }
  uVar26 = uVar8 - 0x28;
  if (((int)puVar10 - 0x27U & 7) == 0) {
    iVar9 = -0x2f;
  }
  else {
    iVar9 = (-((int)puVar10 - 0x27U) & 7) - 0x2f;
  }
  puVar15 = (uint *)((int)puVar10 + iVar9);
  if ((uint *)((int)puVar10 + iVar9) < puVar21 + 4) {
    puVar15 = puVar21;
  }
  puVar23 = puVar3;
  if (((uint)(puVar3 + 2) & 7) != 0) {
    uVar13 = -(int)(puVar3 + 2) & 7;
    uVar26 = uVar26 - uVar13;
    puVar23 = (uint *)((int)puVar3 + uVar13);
  }
  uVar29 = *(undefined8 *)(param_1 + 0x70);
  param_1[3] = uVar26;
  uVar13 = DAT_01049d58;
  param_1[6] = (uint)puVar23;
  puVar23[1] = uVar26 | 1;
  *(undefined4 *)((int)puVar3 + (uVar8 - 0x24)) = 0x28;
  param_1[7] = uVar13;
  *(undefined8 *)(puVar15 + 2) = uVar29;
  uVar26 = param_1[0x72];
  uVar13 = param_1[0x73];
  puVar15[1] = 0x1b;
  puVar15[4] = uVar26;
  puVar15[5] = uVar13;
  param_1[0x70] = (uint)puVar3;
  param_1[0x71] = uVar8;
  param_1[0x72] = (uint)(puVar15 + 2);
  param_1[0x73] = 1;
  puVar3 = puVar15 + 7;
  do {
    puVar23 = puVar3 + 1;
    *puVar3 = 7;
    puVar3 = puVar23;
  } while (puVar23 < puVar10);
  if (puVar21 == puVar15) {
LAB_0100f9a4:
    uVar26 = param_1[3];
  }
  else {
    uVar26 = (int)puVar15 - (int)puVar21;
    uVar8 = uVar26 >> 3;
    puVar15[1] = puVar15[1] & 0xfffffffe;
    puVar21[1] = uVar26 | 1;
    *puVar15 = uVar26;
    if (uVar8 < 0x20) {
      uVar26 = 1 << (uVar8 & 0xff);
      puVar3 = param_1 + uVar8 * 2 + 10;
      if ((*param_1 & uVar26) == 0) {
        *param_1 = *param_1 | uVar26;
        puVar10 = puVar3;
      }
      else {
        puVar10 = (uint *)param_1[uVar8 * 2 + 0xc];
        if ((uint *)param_1[uVar8 * 2 + 0xc] < (uint *)param_1[4]) {
          abort();
          puVar10 = puVar3;
        }
      }
      uVar26 = param_1[3];
      param_1[uVar8 * 2 + 0xc] = (uint)puVar21;
      puVar10[3] = (uint)puVar21;
      puVar21[2] = (uint)puVar10;
      puVar21[3] = (uint)puVar3;
    }
    else {
      uVar8 = uVar26 >> 8;
      if (uVar8 < 0x10000) {
        uVar13 = uVar8 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
        iVar9 = uVar8 << uVar13;
        uVar4 = iVar9 - 0x1000U >> 0x10 & 4;
        iVar9 = iVar9 << uVar4;
        uVar8 = iVar9 - 0x4000U >> 0x10 & 2;
        iVar9 = ((uint)(iVar9 << uVar8) >> 0xf) - ((uVar13 | uVar4) + uVar8);
        uVar13 = (uVar26 >> (iVar9 + 0x15U & 0xff) & 1) + (iVar9 + 0xe) * 2;
        iVar27 = uVar13 + 0x4c;
        uVar8 = 1 << (uVar13 & 0xff);
        iVar9 = iVar27 * 4;
      }
      else {
        uVar8 = 0x80000000;
        iVar9 = 0x1ac;
        uVar13 = 0x1f;
        iVar27 = 0x6b;
      }
      uVar4 = param_1[1];
      puVar21[4] = 0;
      puVar21[5] = 0;
      puVar21[7] = uVar13;
      if ((uVar4 & uVar8) == 0) {
        uVar26 = param_1[3];
        param_1[1] = uVar4 | uVar8;
        param_1[iVar27] = (uint)puVar21;
        puVar21[2] = (uint)puVar21;
        puVar21[3] = (uint)puVar21;
        puVar21[6] = (int)param_1 + iVar9;
      }
      else {
        uVar4 = param_1[iVar27];
        uVar8 = uVar26;
        if (uVar13 != 0x1f) {
          uVar8 = uVar26 << (0x19 - (uVar13 >> 1) & 0xff);
        }
        do {
          uVar13 = uVar4;
          if (uVar26 == (*(uint *)(uVar13 + 4) & 0xfffffff8)) {
            uVar8 = *(uint *)(uVar13 + 8);
            if (uVar13 < param_1[4] || uVar8 < param_1[4]) goto LAB_0100f9a0;
            uVar26 = param_1[3];
            *(uint **)(uVar8 + 0xc) = puVar21;
            *(uint **)(uVar13 + 8) = puVar21;
            puVar21[2] = uVar8;
            puVar21[3] = uVar13;
            puVar21[6] = 0;
            goto LAB_0100f42c;
          }
          iVar9 = (int)uVar8 >> 0x1f;
          uVar8 = uVar8 << 1;
          iVar9 = 4 - iVar9;
          uVar4 = *(uint *)(uVar13 + iVar9 * 4);
        } while (uVar4 != 0);
        if (uVar13 + iVar9 * 4 < param_1[4]) {
LAB_0100f9a0:
          abort();
          goto LAB_0100f9a4;
        }
        *(uint **)(uVar13 + iVar9 * 4) = puVar21;
        uVar26 = param_1[3];
        puVar21[2] = (uint)puVar21;
        puVar21[3] = (uint)puVar21;
        puVar21[6] = uVar13;
      }
    }
  }
LAB_0100f42c:
  if (uVar18 < uVar26) {
LAB_0100ecdc:
    uVar8 = param_1[6];
    param_1[3] = uVar26 - uVar18;
    param_1[6] = uVar8 + uVar18;
    *(uint *)(uVar8 + uVar18 + 4) = uVar26 - uVar18 | 1;
    *(uint *)(uVar8 + 4) = uVar18 | 3;
    return (uint *)(uVar8 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  }
LAB_0100f434:
  FUN_01010bfc(s__s__Malloc_failed_01032628,s_sys_alloc_0103260c);
  return (uint *)0x0;
}



/* @ 0x10101f4  FUN_010101f4 */

void FUN_010101f4(uint *param_1,int param_2)

{
  code *pcVar1;
  uint *puVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint *puVar6;
  uint *puVar7;
  int iVar8;
  uint uVar9;
  uint *puVar10;
  undefined4 unaff_r4;
  uint *puVar11;
  undefined4 unaff_r5;
  uint uVar12;
  uint uVar13;
  undefined4 unaff_r6;
  undefined4 unaff_r7;
  uint *puVar14;
  undefined4 unaff_r8;
  uint *puVar15;
  uint uVar16;
  uint *puVar17;
  uint uVar18;
  code *UNRECOVERED_JUMPTABLE;
  bool bVar19;
  bool bVar20;
  
  if (param_2 == 0) {
    return;
  }
  puVar6 = (uint *)param_1[4];
  puVar11 = (uint *)(param_2 + -AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  if (puVar11 < puVar6) goto LAB_010103a0;
  uVar12 = *(uint *)(param_2 + -4);
  if ((uVar12 & 3) == 1) goto LAB_010103a0;
  uVar13 = uVar12 & 0xfffffff8;
  puVar14 = (uint *)((int)puVar11 + uVar13);
  if ((uVar12 & 1) != 0) goto LAB_01010244;
  uVar9 = *(uint *)(param_2 + -8);
  puVar11 = (uint *)((int)puVar11 - uVar9);
  if ((uVar12 & 3) == 0) {
    iVar4 = uVar9 + 0x10 + uVar13;
    iVar8 = munmap(puVar11,iVar4);
    if (iVar8 != 0) {
      return;
    }
    param_1[0x6c] = param_1[0x6c] - iVar4;
    return;
  }
  uVar13 = uVar13 + uVar9;
  if (puVar11 < puVar6) goto LAB_010103a0;
  puVar15 = (uint *)param_1[5];
  if (puVar15 == puVar11) {
    uVar12 = puVar14[1];
    if ((uVar12 & 3) == 3) {
      param_1[2] = uVar13;
      puVar14[1] = uVar12 & 0xfffffffe;
      puVar15[1] = uVar13 | 1;
      *puVar14 = uVar13;
      return;
    }
    goto LAB_01010244;
  }
  uVar9 = uVar9 >> 3;
  puVar15 = (uint *)puVar11[3];
  if (uVar9 < 0x20) {
    puVar17 = (uint *)puVar11[2];
    if (param_1 + uVar9 * 2 + 10 == puVar17) {
      if (puVar15 != puVar17) {
LAB_0101092c:
        if ((puVar6 <= puVar15) && ((uint *)puVar15[2] == puVar11)) {
LAB_010109a8:
          puVar17[3] = (uint)puVar15;
          puVar15[2] = (uint)puVar17;
          goto LAB_01010244;
        }
        goto LAB_01010940;
      }
    }
    else {
      if ((puVar17 < puVar6) || ((uint *)puVar17[3] != puVar11)) goto LAB_01010940;
      if (puVar15 != puVar17) {
        if (param_1 + uVar9 * 2 + 10 != puVar15) goto LAB_0101092c;
        goto LAB_010109a8;
      }
    }
    *param_1 = *param_1 & ~(1 << (uVar9 & 0xff));
  }
  else {
    puVar17 = (uint *)puVar11[6];
    if (puVar15 == puVar11) {
      if ((uint *)puVar11[5] != (uint *)0x0) {
        puVar7 = (uint *)puVar11[5];
        puVar2 = puVar11 + 5;
LAB_0101077c:
        do {
          do {
            puVar10 = puVar2;
            puVar15 = puVar7;
            puVar7 = (uint *)puVar15[5];
            puVar2 = puVar15 + 5;
          } while (puVar7 != (uint *)0x0);
          puVar7 = (uint *)puVar15[4];
          puVar2 = puVar15 + 4;
        } while (puVar7 != (uint *)0x0);
        if (puVar10 < puVar6) goto LAB_010108c8;
        *puVar10 = 0;
        goto LAB_010105e8;
      }
      puVar15 = (uint *)puVar11[4];
      if (puVar15 != (uint *)0x0) {
        puVar7 = puVar15;
        puVar2 = puVar11 + 4;
        goto LAB_0101077c;
      }
      if (puVar17 == (uint *)0x0) goto LAB_01010244;
      uVar12 = puVar11[7];
      if ((uint *)param_1[uVar12 + 0x4c] == puVar11) {
        param_1[uVar12 + 0x4c] = 0;
        goto LAB_010108dc;
      }
LAB_01010608:
      if (puVar17 < puVar6) {
        abort();
      }
      else if ((uint *)puVar17[4] == puVar11) {
        puVar17[4] = (uint)puVar15;
      }
      else {
        puVar17[5] = (uint)puVar15;
      }
      if (puVar15 != (uint *)0x0) {
LAB_01010628:
        puVar6 = (uint *)param_1[4];
        if (puVar15 < puVar6) {
LAB_01010940:
          abort();
        }
        else {
          puVar7 = (uint *)puVar11[4];
          puVar15[6] = (uint)puVar17;
          if (puVar7 != (uint *)0x0) {
            if (puVar7 < puVar6) {
              abort();
            }
            else {
              puVar15[4] = (uint)puVar7;
              puVar7[6] = (uint)puVar15;
            }
          }
          uVar12 = puVar11[5];
          if (uVar12 != 0) {
            if (uVar12 < param_1[4]) goto LAB_01010940;
            puVar15[5] = uVar12;
            *(uint **)(uVar12 + 0x18) = puVar15;
          }
        }
      }
    }
    else {
      puVar7 = (uint *)puVar11[2];
      if (((puVar7 < puVar6) || ((uint *)puVar7[3] != puVar11)) || ((uint *)puVar15[2] != puVar11))
      {
LAB_010108c8:
        abort();
      }
      else {
        puVar7[3] = (uint)puVar15;
        puVar15[2] = (uint)puVar7;
      }
LAB_010105e8:
      if (puVar17 != (uint *)0x0) {
        uVar12 = puVar11[7];
        if ((uint *)param_1[uVar12 + 0x4c] != puVar11) {
          puVar6 = (uint *)param_1[4];
          goto LAB_01010608;
        }
        param_1[uVar12 + 0x4c] = (uint)puVar15;
        if (puVar15 == (uint *)0x0) {
LAB_010108dc:
          param_1[1] = param_1[1] & ~(1 << (uVar12 & 0xff));
          goto LAB_01010244;
        }
        goto LAB_01010628;
      }
    }
  }
LAB_01010244:
  if ((puVar14 <= puVar11) || (uVar12 = puVar14[1], (uVar12 & 1) == 0)) {
LAB_010103a0:
    abort();
    return;
  }
  if ((uVar12 & 2) != 0) {
    puVar14[1] = uVar12 & 0xfffffffe;
    puVar11[1] = uVar13 | 1;
    *(uint *)((int)puVar11 + uVar13) = uVar13;
    goto joined_r0x01010404;
  }
  puVar6 = (uint *)param_1[5];
  if ((uint *)param_1[6] == puVar14) {
    param_1[6] = (uint)puVar11;
    uVar13 = uVar13 + param_1[3];
    param_1[3] = uVar13;
    puVar11[1] = uVar13 | 1;
    if (puVar6 == puVar11) {
      param_1[2] = 0;
      param_1[5] = 0;
    }
    if (uVar13 <= param_1[7]) {
      return;
    }
    if (DAT_01049d48 == 0) {
      FUN_0100e394();
    }
    puVar6 = (uint *)param_1[6];
    if (puVar6 != (uint *)0x0) {
      if (0x28 < param_1[3]) {
        puVar14 = param_1 + 0x70;
        puVar11 = puVar14;
        do {
          puVar15 = (uint *)*puVar11;
          if (puVar15 <= puVar6) {
            uVar12 = puVar11[1];
            if (puVar6 < (uint *)((int)puVar15 + uVar12)) {
              if ((((puVar11[3] & 8) != 0) || ((puVar11[3] & 1) == 0)) ||
                 (uVar13 = DAT_01049d50 * (((DAT_01049d50 + param_1[3]) - 0x29) / DAT_01049d50 - 1),
                 uVar12 <= uVar13 && uVar13 - uVar12 != 0)) break;
              goto LAB_0100e4f8;
            }
          }
          puVar11 = (uint *)puVar11[2];
          if (puVar11 == (uint *)0x0) {
                    /* WARNING: Does not return */
            pcVar1 = (code *)software_udf(0,0x100e5a4);
            (*pcVar1)();
          }
        } while( true );
      }
      goto LAB_0100e47c;
    }
    goto LAB_0100e4ac;
  }
  if (puVar6 == puVar14) {
    param_1[5] = (uint)puVar11;
    uVar13 = uVar13 + param_1[2];
    param_1[2] = uVar13;
    puVar11[1] = uVar13 | 1;
    *(uint *)((int)puVar11 + uVar13) = uVar13;
    return;
  }
  puVar15 = (uint *)puVar14[3];
  uVar9 = uVar12 >> 3;
  uVar13 = uVar13 + (uVar12 & 0xfffffff8);
  if (uVar9 < 0x20) {
    puVar17 = (uint *)puVar14[2];
    if (param_1 + uVar9 * 2 + 10 != puVar17) {
      puVar7 = (uint *)param_1[4];
      if ((puVar7 <= puVar17) && ((uint *)puVar17[3] == puVar14)) {
        if (puVar15 != puVar17) {
          if (param_1 + uVar9 * 2 + 10 != puVar15) goto LAB_010107bc;
          goto LAB_01010920;
        }
        goto LAB_010102b4;
      }
      goto LAB_010107d0;
    }
    if (puVar15 == puVar17) {
LAB_010102b4:
      *param_1 = *param_1 & ~(1 << (uVar9 & 0xff));
    }
    else {
      puVar7 = (uint *)param_1[4];
LAB_010107bc:
      if ((puVar15 < puVar7) || ((uint *)puVar15[2] != puVar14)) goto LAB_010107d0;
LAB_01010920:
      puVar17[3] = (uint)puVar15;
      puVar15[2] = (uint)puVar17;
    }
  }
  else {
    uVar12 = puVar14[6];
    if (puVar15 == puVar14) {
      if ((uint *)puVar14[5] != (uint *)0x0) {
        puVar6 = (uint *)puVar14[5];
        puVar17 = puVar14 + 5;
LAB_01010864:
        do {
          do {
            puVar7 = puVar17;
            puVar15 = puVar6;
            puVar6 = (uint *)puVar15[5];
            puVar17 = puVar15 + 5;
          } while (puVar6 != (uint *)0x0);
          puVar6 = (uint *)puVar15[4];
          puVar17 = puVar15 + 4;
        } while (puVar6 != (uint *)0x0);
        if (puVar7 < (uint *)param_1[4]) goto LAB_010107b4;
        *puVar7 = 0;
        goto LAB_0101051c;
      }
      if ((uint *)puVar14[4] != (uint *)0x0) {
        puVar6 = (uint *)puVar14[4];
        puVar17 = puVar14 + 4;
        goto LAB_01010864;
      }
      if (uVar12 == 0) goto LAB_010102c4;
      uVar9 = puVar14[7];
      if ((uint *)param_1[uVar9 + 0x4c] == puVar14) {
        param_1[uVar9 + 0x4c] = 0;
        goto LAB_010108b0;
      }
      puVar15 = (uint *)0x0;
LAB_01010538:
      if (uVar12 < param_1[4]) {
        abort();
      }
      else if (*(uint **)(uVar12 + 0x10) == puVar14) {
        *(uint **)(uVar12 + 0x10) = puVar15;
      }
      else {
        *(uint **)(uVar12 + 0x14) = puVar15;
      }
      if (puVar15 != (uint *)0x0) {
LAB_0101055c:
        puVar6 = (uint *)param_1[4];
        if (puVar6 <= puVar15) {
          puVar17 = (uint *)puVar14[4];
          puVar15[6] = uVar12;
          if (puVar17 != (uint *)0x0) {
            if (puVar17 < puVar6) {
              abort();
            }
            else {
              puVar15[4] = (uint)puVar17;
              puVar17[6] = (uint)puVar15;
            }
          }
          uVar12 = puVar14[5];
          if (uVar12 == 0) goto LAB_010107d4;
          if (param_1[4] <= uVar12) {
            puVar6 = (uint *)param_1[5];
            puVar15[5] = uVar12;
            *(uint **)(uVar12 + 0x18) = puVar15;
            goto LAB_010102c4;
          }
        }
LAB_010107d0:
        abort();
      }
    }
    else {
      uVar9 = puVar14[2];
      if (((uVar9 < param_1[4]) || (*(uint **)(uVar9 + 0xc) != puVar14)) ||
         ((uint *)puVar15[2] != puVar14)) {
LAB_010107b4:
        abort();
      }
      else {
        *(uint **)(uVar9 + 0xc) = puVar15;
        puVar15[2] = uVar9;
      }
LAB_0101051c:
      if (uVar12 != 0) {
        uVar9 = puVar14[7];
        if ((uint *)param_1[uVar9 + 0x4c] != puVar14) goto LAB_01010538;
        param_1[uVar9 + 0x4c] = (uint)puVar15;
        if (puVar15 == (uint *)0x0) {
          puVar6 = (uint *)param_1[5];
LAB_010108b0:
          param_1[1] = param_1[1] & ~(1 << (uVar9 & 0xff));
          goto LAB_010102c4;
        }
        goto LAB_0101055c;
      }
    }
LAB_010107d4:
    puVar6 = (uint *)param_1[5];
  }
LAB_010102c4:
  puVar11[1] = uVar13 | 1;
  *(uint *)((int)puVar11 + uVar13) = uVar13;
  if (puVar6 == puVar11) {
    param_1[2] = uVar13;
    return;
  }
joined_r0x01010404:
  uVar12 = uVar13 >> 3;
  if (uVar12 < 0x20) {
    uVar13 = 1 << (uVar12 & 0xff);
    puVar6 = param_1 + uVar12 * 2 + 10;
    if ((*param_1 & uVar13) == 0) {
      *param_1 = *param_1 | uVar13;
      puVar14 = puVar6;
    }
    else {
      puVar14 = (uint *)param_1[uVar12 * 2 + 0xc];
      if ((uint *)param_1[uVar12 * 2 + 0xc] < (uint *)param_1[4]) {
        abort();
        puVar14 = puVar6;
      }
    }
    param_1[uVar12 * 2 + 0xc] = (uint)puVar11;
    puVar14[3] = (uint)puVar11;
    puVar11[2] = (uint)puVar14;
    puVar11[3] = (uint)puVar6;
  }
  else {
    uVar12 = uVar13 >> 8;
    if (uVar12 < 0x10000) {
      uVar9 = uVar12 - 0x100 >> 0x10 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      iVar8 = uVar12 << uVar9;
      uVar18 = iVar8 - 0x1000U >> 0x10 & 4;
      iVar8 = iVar8 << uVar18;
      uVar12 = iVar8 - 0x4000U >> 0x10 & 2;
      iVar8 = ((uint)(iVar8 << uVar12) >> 0xf) - ((uVar9 | uVar18) + uVar12);
      uVar9 = (uVar13 >> (iVar8 + 0x15U & 0xff) & 1) + (iVar8 + 0xe) * 2;
      iVar4 = uVar9 + 0x4c;
      uVar12 = 1 << (uVar9 & 0xff);
      iVar8 = iVar4 * 4;
    }
    else {
      uVar12 = 0x80000000;
      iVar8 = 0x1ac;
      uVar9 = 0x1f;
      iVar4 = 0x6b;
    }
    uVar18 = param_1[1];
    puVar11[4] = 0;
    puVar11[5] = 0;
    puVar11[7] = uVar9;
    if ((uVar12 & uVar18) == 0) {
      param_1[1] = uVar12 | uVar18;
      param_1[iVar4] = (uint)puVar11;
      puVar11[2] = (uint)puVar11;
      puVar11[3] = (uint)puVar11;
      puVar11[6] = (int)param_1 + iVar8;
    }
    else {
      uVar18 = param_1[iVar4];
      uVar12 = uVar13;
      if (uVar9 != 0x1f) {
        uVar12 = uVar13 << (0x19 - (uVar9 >> 1) & 0xff);
      }
      do {
        uVar9 = uVar18;
        iVar8 = (int)uVar12 >> 0x1f;
        uVar12 = uVar12 << 1;
        iVar8 = 4 - iVar8;
        if ((*(uint *)(uVar9 + 4) & 0xfffffff8) == uVar13) {
          uVar13 = *(uint *)(uVar9 + 8);
          uVar12 = param_1[4];
          bVar20 = uVar9 <= uVar12;
          bVar19 = uVar12 == uVar9;
          if (!bVar20 || bVar19) {
            bVar20 = uVar13 <= uVar12;
            bVar19 = uVar12 == uVar13;
          }
          if (bVar20 && !bVar19) goto LAB_010108f0;
          *(uint **)(uVar13 + 0xc) = puVar11;
          *(uint **)(uVar9 + 8) = puVar11;
          puVar11[2] = uVar13;
          puVar11[3] = uVar9;
          puVar11[6] = 0;
          goto LAB_010104ac;
        }
        uVar18 = *(uint *)(uVar9 + iVar8 * 4);
      } while (uVar18 != 0);
      if (uVar9 + iVar8 * 4 < param_1[4]) {
LAB_010108f0:
        abort();
      }
      else {
        *(uint **)(uVar9 + iVar8 * 4) = puVar11;
        puVar11[2] = (uint)puVar11;
        puVar11[3] = (uint)puVar11;
        puVar11[6] = uVar9;
      }
    }
LAB_010104ac:
    uVar12 = param_1[8];
    param_1[8] = uVar12 - 1;
    if (uVar12 - 1 == 0) {
      FUN_0100d680(param_1);
      return;
    }
  }
  return;
  while (puVar14 = (uint *)puVar14[2], puVar14 != (uint *)0x0) {
LAB_0100e4f8:
    if ((puVar15 <= puVar14) && (puVar14 < (uint *)((int)puVar15 + uVar12))) goto LAB_0100e47c;
  }
  iVar8 = munmap((int)puVar15 + (uVar12 - uVar13),uVar13,DAT_01049d50,puVar15,unaff_r4,
                       unaff_r5,unaff_r6,unaff_r7,unaff_r8);
  if (iVar8 == 0 && uVar13 != 0) {
    uVar9 = param_1[3];
    uVar18 = param_1[6];
    uVar16 = param_1[0x6c];
    puVar11[1] = puVar11[1] - uVar13;
    uVar12 = 0;
    if ((uVar18 + 8 & 7) != 0) {
      uVar12 = -(uVar18 + 8) & 7;
    }
    param_1[0x6c] = uVar16 - uVar13;
    uVar16 = DAT_01049d58;
    uVar5 = (uVar9 - uVar13) - uVar12;
    param_1[3] = uVar5;
    param_1[6] = uVar18 + uVar12;
    *(uint *)(uVar18 + uVar12 + 4) = uVar5 | 1;
    *(undefined4 *)(uVar18 + (uVar9 - uVar13) + 4) = 0x28;
    param_1[7] = uVar16;
  }
  else {
LAB_0100e47c:
    uVar13 = 0;
  }
  iVar8 = FUN_0100d680(param_1);
  if (uVar13 + iVar8 != 0) {
    uVar3 = 1;
    goto LAB_0100e4b0;
  }
  if (param_1[7] < param_1[3]) {
    param_1[7] = 0xffffffff;
    uVar3 = 0;
    goto LAB_0100e4b0;
  }
LAB_0100e4ac:
  uVar3 = 0;
LAB_0100e4b0:
                    /* WARNING: Could not recover jumptable at 0x0100e4c0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(uVar3);
  return;
}



/* @ 0x1010b18  FUN_01010b18 */

void FUN_01010b18(undefined1 *param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  undefined1 *local_1c;
  undefined4 *local_18;
  uint local_14;
  undefined4 uStack_4;
  
  local_18 = &uStack_4;
  local_1c = param_1;
  local_14 = param_2;
  uStack_4 = param_4;
  uVar1 = FUN_01011308(0,&local_1c,param_3,local_18,&local_14);
  if (param_2 <= uVar1) {
    local_1c = local_1c + -1;
  }
  *local_1c = 0;
  return;
}



/* @ 0x1010b84  FUN_01010b84 */

int FUN_01010b84(undefined1 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined1 *local_24 [2];
  undefined4 *local_1c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  local_1c = &uStack_8;
  local_24[0] = param_1;
  uStack_8 = param_3;
  uStack_4 = param_4;
  iVar1 = FUN_01011308(0,local_24,param_2,local_1c,0);
  *local_24[0] = 0;
  return iVar1 + 1;
}



/* @ 0x1010be0  FUN_01010be0 */

void FUN_01010be0(void)

{
  FUN_01011308();
  return;
}



/* @ 0x1010bfc  FUN_01010bfc */

void FUN_01010bfc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_01011308(&LAB_01010af4,0,param_1,&uStack_c,0);
  return;
}



/* @ 0x1010c40  FUN_01010c40 */

void FUN_01010c40(code *UNRECOVERED_JUMPTABLE,int *param_2,undefined4 param_3,int *param_4)

{
  int iVar1;
  
  if (param_2 == (int *)0x0) {
                    /* WARNING: Could not recover jumptable at 0x01010c9c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*UNRECOVERED_JUMPTABLE)(param_3);
    return;
  }
  if (param_4 != (int *)0x0) {
    if (*param_4 == 0) {
      return;
    }
    if (*param_4 == 1) {
      param_3 = 0;
    }
    *(char *)*param_2 = (char)param_3;
    iVar1 = *param_4;
    *param_2 = *param_2 + 1;
    *param_4 = iVar1 + -1;
    return;
  }
  *(char *)*param_2 = (char)param_3;
  *param_2 = *param_2 + 1;
  return;
}



/* @ 0x1010cb8  FUN_01010cb8 */

undefined4 FUN_01010cb8(byte *param_1)

{
  bool bVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  undefined1 uVar8;
  undefined1 *puVar9;
  byte *pbVar10;
  uint uVar11;
  int iVar12;
  uint uVar13;
  int iVar14;
  byte *pbVar15;
  uint uVar16;
  int iVar17;
  undefined1 auStack_28 [8];
  
  FUN_01012ae0(auStack_28,0,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  iVar12 = 1;
LAB_01010cf8:
  do {
    pbVar10 = param_1;
    param_1 = pbVar10 + 1;
    if (*pbVar10 == 0) {
      return 0;
    }
  } while (*pbVar10 != 0x25);
  uVar11 = (uint)pbVar10[1];
  bVar7 = false;
  bVar6 = false;
  bVar5 = false;
  bVar3 = false;
  bVar2 = false;
  bVar4 = false;
  bVar1 = false;
  iVar17 = iVar12;
LAB_01010d20:
  param_1 = param_1 + 1;
LAB_01010d24:
  switch(uVar11) {
  case 0x20:
  case 0x23:
  case 0x27:
  case 0x2b:
  case 0x2d:
  case 0x30:
    uVar11 = (uint)*param_1;
    goto LAB_01010d20;
  default:
    iVar12 = iVar17;
    if (uVar11 == 0) {
      return 0;
    }
    goto LAB_01010cf8;
  case 0x2a:
    uVar11 = (uint)*param_1;
    uVar13 = uVar11 - 0x30;
    pbVar10 = param_1;
    if (uVar13 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
      iVar14 = 0;
      while( true ) {
        pbVar10 = pbVar10 + 1;
        uVar16 = (uint)*pbVar10;
        iVar12 = uVar13 + iVar14;
        iVar14 = iVar12 * 10;
        uVar13 = uVar16 - 0x30;
        if (AL_I2C_INTR_MASK_STOP_DET_SHIFT < uVar13) break;
        if ((0xccccccc < iVar12) || ((int)(0x7fffffff - uVar13) < iVar14)) {
          return 0xffffffff;
        }
      }
    }
    else {
      iVar12 = 0;
      uVar16 = uVar11;
    }
    if (uVar16 == 0x24) {
      param_1 = pbVar10 + 1;
      uVar11 = (uint)pbVar10[1];
      auStack_28[iVar12] = 4;
      goto LAB_01010d20;
    }
    break;
  case 0x2e:
    pbVar10 = param_1 + 1;
    uVar11 = (uint)*param_1;
    if (uVar11 != 0x2a) {
      while (param_1 = pbVar10, uVar11 - 0x30 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
        uVar11 = (uint)*pbVar10;
        pbVar10 = pbVar10 + 1;
      }
      goto LAB_01010d24;
    }
    uVar11 = (uint)param_1[1];
    uVar13 = uVar11 - 0x30;
    pbVar15 = pbVar10;
    if (uVar13 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
      iVar14 = 0;
      while( true ) {
        pbVar15 = pbVar15 + 1;
        uVar16 = (uint)*pbVar15;
        iVar12 = iVar14 + uVar13;
        iVar14 = iVar12 * 10;
        uVar13 = uVar16 - 0x30;
        if (AL_I2C_INTR_MASK_STOP_DET_SHIFT < uVar13) break;
        if (0xccccccc < iVar12) {
          return 0xffffffff;
        }
        if ((int)(0x7fffffff - uVar13) < iVar14) {
          return 0xffffffff;
        }
      }
    }
    else {
      iVar12 = 0;
      uVar16 = uVar11;
    }
    param_1 = pbVar10;
    if (uVar16 == 0x24) {
      param_1 = pbVar15 + 1;
      uVar11 = (uint)pbVar15[1];
      auStack_28[iVar12] = 4;
      goto LAB_01010d20;
    }
    break;
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x39:
    goto switchD_01010d2c_caseD_31;
  case 0x44:
    bVar5 = true;
  case 100:
  case 0x69:
    iVar12 = iVar17 + 1;
    puVar9 = auStack_28 + iVar17;
    if (bVar1) {
      auStack_28[iVar17] = 0x16;
    }
    else if (bVar2) {
LAB_010110fc:
      iVar12 = iVar17 + 1;
      auStack_28[iVar17] = 0x11;
    }
    else if (bVar3) {
      *puVar9 = 0x14;
    }
    else if (bVar4) {
      *puVar9 = AL_I2C_INTR_MASK_START_DET_SHIFT;
    }
    else if (bVar5) {
      *puVar9 = 7;
    }
    else if (bVar6) {
      *puVar9 = 1;
    }
    else {
      if (bVar7) {
        uVar8 = 0x19;
      }
      else {
        uVar8 = 4;
      }
      *puVar9 = uVar8;
    }
    goto LAB_01010cf8;
  case 0x4f:
  case 0x55:
    bVar5 = true;
  case 0x58:
  case 0x6f:
  case 0x75:
  case 0x78:
    iVar12 = iVar17 + 1;
    puVar9 = auStack_28 + iVar17;
    if (bVar1) {
      auStack_28[iVar17] = 0x17;
    }
    else {
      if (bVar2) goto LAB_010110fc;
      if (bVar3) {
        *puVar9 = 0x13;
      }
      else if (bVar4) {
        *puVar9 = AL_I2C_INTR_MASK_GEN_CALL_SHIFT;
      }
      else if (bVar5) {
        *puVar9 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      }
      else if (bVar6) {
        *puVar9 = 2;
      }
      else {
        if (bVar7) {
          uVar8 = 0x1a;
        }
        else {
          uVar8 = 5;
        }
        *puVar9 = uVar8;
      }
    }
    goto LAB_01010cf8;
  case 99:
    iVar12 = iVar17 + 1;
    auStack_28[iVar17] = 4;
    goto LAB_01010cf8;
  case 0x68:
    uVar11 = (uint)*param_1;
    if (uVar11 == 0x68) {
      uVar11 = (uint)param_1[1];
      bVar7 = true;
      param_1 = param_1 + 1;
    }
    else {
      bVar6 = true;
    }
    goto LAB_01010d20;
  case 0x6a:
    bVar1 = true;
    uVar11 = (uint)*param_1;
    goto LAB_01010d20;
  case 0x6c:
    uVar11 = (uint)*param_1;
    if (uVar11 == 0x6c) {
      uVar11 = (uint)param_1[1];
      bVar4 = true;
      param_1 = param_1 + 1;
    }
    else {
      bVar5 = true;
    }
    goto LAB_01010d20;
  case 0x6e:
    iVar12 = iVar17 + 1;
    puVar9 = auStack_28 + iVar17;
    if (bVar4) {
      auStack_28[iVar17] = AL_I2C_TAR_10BIT_ADDR_SHIFT;
    }
    else if (bVar5) {
      *puVar9 = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
    }
    else if (bVar6) {
      *puVar9 = 3;
    }
    else if (bVar2) {
      *puVar9 = 0x12;
    }
    else if (bVar3) {
      *puVar9 = 0x15;
    }
    else {
      if (bVar1) {
        uVar8 = 0x18;
      }
      else {
        uVar8 = 6;
      }
      *puVar9 = uVar8;
    }
    goto LAB_01010cf8;
  case 0x70:
    iVar12 = iVar17 + 1;
    auStack_28[iVar17] = 0x10;
    goto LAB_01010cf8;
  case 0x71:
    bVar4 = true;
    uVar11 = (uint)*param_1;
    goto LAB_01010d20;
  case 0x73:
    iVar12 = iVar17 + 1;
    auStack_28[iVar17] = 0xf;
    goto LAB_01010cf8;
  case 0x74:
    bVar2 = true;
    uVar11 = (uint)*param_1;
    goto LAB_01010d20;
  case 0x7a:
    goto switchD_01010d2c_caseD_7a;
  }
  auStack_28[iVar17] = 4;
  iVar17 = iVar17 + 1;
  goto LAB_01010d20;
switchD_01010d2c_caseD_7a:
  bVar3 = true;
  uVar11 = (uint)*param_1;
  goto LAB_01010d20;
switchD_01010d2c_caseD_31:
  uVar13 = uVar11 - 0x30;
  iVar12 = 0;
  pbVar10 = param_1;
  while( true ) {
    iVar14 = uVar13 + iVar12;
    param_1 = pbVar10 + 1;
    uVar11 = (uint)*pbVar10;
    iVar12 = iVar14 * 10;
    uVar13 = uVar11 - 0x30;
    if (AL_I2C_INTR_MASK_STOP_DET_SHIFT < uVar13) break;
    if (0xccccccc < iVar14) {
      return 0xffffffff;
    }
    pbVar10 = param_1;
    if ((int)(0x7fffffff - uVar13) < iVar12) {
      return 0xffffffff;
    }
  }
  if (uVar11 == 0x24) goto code_r0x010111bc;
  goto LAB_01010d24;
code_r0x010111bc:
  uVar11 = (uint)pbVar10[1];
  iVar17 = iVar14;
  goto LAB_01010d20;
}



/* @ 0x1011308  FUN_01011308 */

/* WARNING: Type propagation algorithm not settling */

char * FUN_01011308(undefined4 param_1,undefined4 param_2,byte *param_3,uint *param_4,
                   undefined4 param_5)

{
  bool bVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  undefined8 *puVar8;
  uint *puVar9;
  undefined2 *puVar10;
  undefined4 *puVar11;
  undefined1 uVar12;
  char cVar13;
  char extraout_r2;
  byte *pbVar14;
  uint uVar15;
  byte *pbVar16;
  uint uVar17;
  char *pcVar18;
  undefined1 *puVar19;
  undefined1 *puVar20;
  uint uVar21;
  undefined1 *puVar22;
  char *pcVar23;
  char *pcVar24;
  int iVar25;
  int iVar26;
  byte *pbVar27;
  bool bVar28;
  undefined8 uVar29;
  uint local_118;
  char *local_114;
  char *local_110;
  char *local_10c;
  uint *local_108;
  int local_104;
  undefined1 *local_100;
  char *local_fc;
  undefined1 *local_f8;
  int local_f4;
  char *local_f0;
  int local_e8;
  char *local_dc;
  undefined1 auStack_d8 [4];
  uint *local_d4;
  undefined1 auStack_d0 [68];
  char local_8c [98];
  char local_2a [6];
  
  local_114 = (char *)0x0;
  local_110 = (char *)0x1;
  local_f8 = (undefined1 *)0x0;
  local_d4 = param_4;
  FUN_01012ae0(auStack_d8,0,4);
  pbVar27 = param_3;
  local_108 = param_4;
  pbVar16 = param_3;
LAB_01011370:
  pbVar14 = pbVar27;
  if (*pbVar14 != 0) goto code_r0x01011384;
  bVar1 = false;
  if (pbVar16 == pbVar14) {
    return local_114;
  }
  goto LAB_010117cc;
code_r0x01011384:
  pbVar27 = pbVar14 + 1;
  if (*pbVar14 == 0x25) {
    if (pbVar16 != pbVar14) {
      bVar1 = true;
LAB_010117cc:
      iVar26 = (int)pbVar14 - (int)pbVar16;
      if ((iVar26 < 0) || (0x7fffffff - (int)local_114 < iVar26)) {
        return (char *)0xffffffff;
      }
      do {
        pbVar27 = pbVar16 + 1;
        FUN_01010c40(param_1,param_2,*pbVar16,param_5);
        pbVar16 = pbVar27;
      } while (pbVar14 != pbVar27);
      local_114 = local_114 + iVar26;
      if (!bVar1) {
        return local_114;
      }
    }
    local_100 = (undefined1 *)(uint)pbVar14[1];
    local_f0 = (char *)0xffffffff;
    local_118 = 0;
    bVar7 = false;
    bVar6 = false;
    bVar5 = false;
    bVar4 = false;
    bVar3 = false;
    bVar2 = false;
    bVar28 = false;
    bVar1 = false;
    local_104 = 0;
    local_fc = (char *)0x0;
    pbVar16 = pbVar14 + 1;
LAB_010113b0:
    pcVar23 = local_f0;
    pbVar16 = pbVar16 + 1;
    pcVar18 = local_fc;
LAB_010113b8:
    local_fc = pcVar18;
    local_f0 = pcVar23;
    pbVar27 = pbVar16;
    switch(local_100) {
    case (undefined1 *)0x20:
      local_100 = (undefined1 *)(uint)*pbVar16;
      if (local_104 == 0) {
        local_104 = 0x20;
      }
      goto LAB_010113b0;
    default:
      if (local_100 == (undefined1 *)0x0) {
        return local_114;
      }
      pcVar23 = local_8c;
      local_10c = (char *)0x1;
      local_f4 = -1;
      local_f0 = (char *)0x1;
      local_8c[0] = (char)local_100;
LAB_01011bcc:
      local_104 = 0;
      local_100 = (undefined1 *)0x0;
      goto LAB_010116ec;
    case (undefined1 *)0x23:
      bVar1 = true;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x27:
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x2a:
      uVar17 = (uint)*pbVar16;
      uVar21 = uVar17 - 0x30;
      if (uVar21 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
        iVar26 = 0;
        while( true ) {
          pbVar27 = pbVar27 + 1;
          uVar17 = (uint)*pbVar27;
          iVar25 = iVar26 + uVar21;
          iVar26 = iVar25 * 10;
          uVar21 = uVar17 - 0x30;
          if (AL_I2C_INTR_MASK_STOP_DET_SHIFT < uVar21) break;
          if (0xccccccc < iVar25) {
            return (char *)0xffffffff;
          }
          if ((int)(0x7fffffff - uVar21) < iVar26) {
            return (char *)0xffffffff;
          }
        }
      }
      else {
        iVar25 = 0;
      }
      if (uVar17 == 0x24) {
        if (local_f8 == (undefined1 *)0x0) {
          local_f8 = auStack_d0;
          FUN_01010cb8(param_3);
        }
        pbVar16 = pbVar27 + 1;
        local_fc = *(char **)(local_f8 + iVar25 * 8);
      }
      else if (local_f8 == (undefined1 *)0x0) {
        local_fc = (char *)*local_108;
        local_110 = local_110 + 1;
        local_108 = local_108 + 1;
      }
      else {
        local_fc = *(char **)(local_f8 + (int)local_110 * 8);
        local_110 = local_110 + 1;
      }
      if ((int)local_fc < 0) {
        if (local_fc == (char *)0x80000000) {
          return (char *)0xffffffff;
        }
        local_fc = (char *)-(int)local_fc;
switchD_010113c0_caseD_2d:
        local_118 = local_118 | 4;
        local_100 = (undefined1 *)(uint)*pbVar16;
      }
      else {
        local_100 = (undefined1 *)(uint)*pbVar16;
      }
      goto LAB_010113b0;
    case (undefined1 *)0x2b:
      local_100 = (undefined1 *)(uint)*pbVar16;
      local_104 = 0x2b;
      goto LAB_010113b0;
    case (undefined1 *)0x2d:
      goto switchD_010113c0_caseD_2d;
    case (undefined1 *)0x2e:
      pbVar27 = pbVar16 + 1;
      local_100 = (undefined1 *)(uint)*pbVar16;
      if (local_100 != (undefined1 *)0x2a) {
        puVar20 = local_100 + -0x30;
        if (puVar20 < (undefined1 *)(AL_I2C_INTR_MASK_STOP_DET_SHIFT + 0x1)) {
          iVar26 = 0;
          pbVar16 = pbVar27;
          while( true ) {
            pbVar27 = pbVar16 + 1;
            local_100 = (undefined1 *)(uint)*pbVar16;
            pcVar23 = puVar20 + iVar26;
            iVar26 = (int)pcVar23 * 10;
            puVar20 = local_100 + -0x30;
            if ((undefined1 *)AL_I2C_INTR_MASK_STOP_DET_SHIFT < puVar20) break;
            if (0xccccccc < (int)pcVar23) {
              return (char *)0xffffffff;
            }
            pbVar16 = pbVar27;
            if (0x7fffffff - (int)puVar20 < iVar26) {
              return (char *)0xffffffff;
            }
          }
        }
        else {
          pcVar23 = (char *)0x0;
        }
        pbVar16 = pbVar27;
        pcVar18 = local_fc;
        if (local_100 == (undefined1 *)0x24) {
          local_110 = pcVar23;
          if (local_f8 == (undefined1 *)0x0) {
            FUN_01010cb8(param_3);
            local_f8 = auStack_d0;
            local_100 = (undefined1 *)(uint)*pbVar27;
          }
          else {
            local_100 = (undefined1 *)(uint)*pbVar27;
          }
          goto LAB_010113b0;
        }
        goto LAB_010113b8;
      }
      local_100 = (undefined1 *)(uint)pbVar16[1];
      puVar20 = local_100 + -0x30;
      pbVar16 = pbVar27;
      if (puVar20 < (undefined1 *)(AL_I2C_INTR_MASK_STOP_DET_SHIFT + 0x1)) {
        iVar26 = 0;
        while( true ) {
          pbVar16 = pbVar16 + 1;
          puVar19 = (undefined1 *)(uint)*pbVar16;
          puVar22 = puVar20 + iVar26;
          iVar26 = (int)puVar22 * 10;
          puVar20 = puVar19 + -0x30;
          if ((undefined1 *)AL_I2C_INTR_MASK_STOP_DET_SHIFT < puVar20) break;
          if (0xccccccc < (int)puVar22) {
            return (char *)0xffffffff;
          }
          if (0x7fffffff - (int)puVar20 < iVar26) {
            return (char *)0xffffffff;
          }
        }
      }
      else {
        puVar22 = (undefined1 *)0x0;
        puVar19 = local_100;
      }
      if (puVar19 == (undefined1 *)0x24) {
        if (local_f8 == (undefined1 *)0x0) {
          local_f8 = auStack_d0;
          FUN_01010cb8(param_3);
        }
        pbVar27 = pbVar16 + 1;
        local_100 = (undefined1 *)(uint)pbVar16[1];
        uVar17 = *(uint *)(local_f8 + (int)puVar22 * 8);
      }
      else if (local_f8 == (undefined1 *)0x0) {
        uVar17 = *local_108;
        local_110 = local_110 + 1;
        local_108 = local_108 + 1;
      }
      else {
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        local_110 = local_110 + 1;
      }
      local_f0 = (char *)(uVar17 | (int)uVar17 >> 0x1f);
      pbVar16 = pbVar27;
      goto LAB_010113b0;
    case (undefined1 *)0x30:
      local_118 = local_118 | 0x80;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x31:
    case (undefined1 *)0x32:
    case (undefined1 *)0x33:
    case (undefined1 *)0x34:
    case (undefined1 *)0x35:
    case (undefined1 *)0x36:
    case (undefined1 *)0x37:
    case (undefined1 *)0x38:
    case (undefined1 *)0x39:
      goto switchD_010113c0_caseD_31;
    case (undefined1 *)0x44:
      bVar3 = true;
    case (undefined1 *)0x64:
    case (undefined1 *)0x69:
      if (bVar28) {
        if (local_f8 != (undefined1 *)0x0) {
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
          goto LAB_0101160c;
        }
LAB_0101235c:
        puVar9 = (uint *)((int)local_108 + 7U & 0xfffffff8);
        uVar17 = *puVar9;
        uVar21 = puVar9[1];
        local_108 = puVar9 + 2;
LAB_0101160c:
        if (-1 < (int)uVar21) goto LAB_01011618;
LAB_010119c0:
        bVar28 = uVar17 != 0;
        uVar17 = -uVar17;
        uVar21 = -(uVar21 + bVar28);
        local_100 = (undefined1 *)0x0;
        local_104 = 0x2d;
        puVar20 = (undefined1 *)0x1;
LAB_010115c4:
        if (local_f0 != (char *)0xffffffff) goto LAB_01011630;
LAB_01011658:
        if (puVar20 == (undefined1 *)0x1) goto LAB_0101203c;
        if (puVar20 == (undefined1 *)0x2) {
          pcVar23 = local_2a + 2;
          do {
            uVar15 = uVar17 & 0xf;
            uVar17 = uVar17 >> 4 | uVar21 << 0x1c;
            uVar21 = uVar21 >> 4;
            pcVar23 = pcVar23 + -1;
            *pcVar23 = local_dc[uVar15];
          } while (uVar17 != 0 || uVar21 != 0);
        }
        else {
          pcVar23 = local_2a + 2;
          do {
            pcVar18 = pcVar23;
            iVar26 = (uVar17 & 7) + 0x30;
            uVar17 = uVar17 >> 3 | uVar21 << 0x1d;
            uVar21 = uVar21 >> 3;
            pcVar18[-1] = (char)iVar26;
            pcVar23 = pcVar18 + -1;
          } while (uVar17 != 0 || uVar21 != 0);
          if (iVar26 == 0x30) {
            bVar1 = false;
          }
          if (bVar1) {
            pcVar18[-2] = '0';
            pcVar23 = pcVar18 + -2;
          }
        }
      }
      else {
        if (!bVar2) {
          if (((bVar3) || (bVar4)) || (bVar5)) {
            if (local_f8 == (undefined1 *)0x0) {
LAB_0101256c:
              uVar17 = *local_108;
              uVar21 = (int)uVar17 >> 0x1f;
              local_108 = local_108 + 1;
            }
            else {
LAB_010122c4:
              uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
              uVar21 = (int)uVar17 >> 0x1f;
            }
          }
          else if (bVar6) {
            if (local_f8 == (undefined1 *)0x0) {
              uVar17 = (uint)(short)*local_108;
              uVar21 = (int)uVar17 >> 0x1f;
              local_108 = local_108 + 1;
            }
            else {
              uVar17 = (uint)(short)*(undefined4 *)(local_f8 + (int)local_110 * 8);
              uVar21 = (int)uVar17 >> 0x1f;
            }
          }
          else {
            if (!bVar7) {
              if (local_f8 != (undefined1 *)0x0) goto LAB_010122c4;
              goto LAB_0101256c;
            }
            if (local_f8 == (undefined1 *)0x0) {
              uVar17 = (uint)(char)*local_108;
              uVar21 = (int)uVar17 >> 0x1f;
              local_108 = local_108 + 1;
            }
            else {
              uVar17 = (uint)(char)*(undefined4 *)(local_f8 + (int)local_110 * 8);
              uVar21 = (int)uVar17 >> 0x1f;
            }
          }
          goto LAB_0101160c;
        }
        if (local_f8 == (undefined1 *)0x0) goto LAB_0101235c;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
        if ((int)uVar21 < 0) goto LAB_010119c0;
LAB_01011618:
        local_100 = (undefined1 *)0x0;
        puVar20 = (undefined1 *)(uint)(local_f0 != (char *)0xffffffff);
        if (local_f0 != (char *)0xffffffff) {
LAB_01011630:
          local_118 = local_118 & 0xffffff7f;
          if (local_f0 != (char *)0x0 || (uVar17 != 0 || uVar21 != 0)) goto LAB_01011658;
          pcVar23 = local_2a + 2;
          local_f4 = 0;
          local_10c = (char *)0x0;
          pcVar18 = (char *)0x0;
          goto LAB_010116bc;
        }
LAB_0101203c:
        uVar29 = CONCAT44(uVar21,uVar17);
        if (uVar21 == 0 && uVar17 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
          pcVar18 = local_2a + 2;
        }
        else {
          pcVar18 = local_2a + 2;
          do {
            iVar26 = (int)((ulonglong)uVar29 >> 0x20);
            uVar21 = (uint)uVar29;
            FUN_01028d2c(uVar21,iVar26,AL_I2C_INTR_MASK_START_DET_SHIFT,0);
            pcVar18 = pcVar18 + -1;
            *pcVar18 = extraout_r2 + '0';
            uVar29 = FUN_01028d2c(uVar21,iVar26,AL_I2C_INTR_MASK_START_DET_SHIFT,0);
            uVar17 = (uint)uVar29;
          } while (iVar26 != 0 || 99 < uVar21);
        }
        pcVar23 = pcVar18 + -1;
        pcVar18[-1] = (char)uVar17 + '0';
      }
      pcVar18 = local_2a + (2 - (int)pcVar23);
      if (100 < (int)pcVar18) {
        return (char *)0xffffffff;
      }
      local_f4 = (int)local_f0 - (int)pcVar18;
      local_10c = pcVar18;
      if ((int)pcVar18 < (int)local_f0) {
        local_10c = local_f0;
      }
LAB_010116bc:
      local_f0 = pcVar18;
      local_110 = local_110 + 1;
      if (local_104 != 0) {
        local_10c = local_10c + 1;
      }
      if (local_100 != (undefined1 *)0x0) {
        local_10c = local_10c + 2;
      }
LAB_010116ec:
      puVar20 = DAT_0101294c;
      if ((local_118 == 0) && (local_e8 = (int)local_fc - (int)local_10c, 0 < local_e8)) {
        iVar26 = local_e8;
        if (0x10 < local_e8) {
          do {
            uVar12 = 0x20;
            for (puVar19 = DAT_01012950; FUN_01010c40(param_1,param_2,uVar12,param_5),
                puVar19 != puVar20; puVar19 = puVar19 + 1) {
              uVar12 = *puVar19;
            }
            iVar26 = iVar26 + -0x10;
          } while (0x10 < iVar26);
          local_e8 = (local_e8 + -0x10) - (local_e8 - 0x11U & 0xfffffff0);
        }
        uVar12 = 0x20;
        puVar20 = (undefined1 *)0x103263c;
        while (FUN_01010c40(param_1,param_2,uVar12,param_5),
              puVar20 != (undefined1 *)(local_e8 + 0x103263b)) {
          puVar20 = puVar20 + 1;
          uVar12 = *puVar20;
        }
      }
      if (local_104 != 0) {
        FUN_01010c40(param_1,param_2,local_104,param_5);
      }
      if (local_100 != (undefined1 *)0x0) {
        FUN_01010c40(param_1,param_2,0x30,param_5);
        FUN_01010c40(param_1,param_2,local_100,param_5);
      }
      puVar20 = DAT_01012944;
      if ((local_118 == 0x80) && (local_104 = (int)local_fc - (int)local_10c, 0 < local_104)) {
        iVar26 = local_104;
        if (0x10 < local_104) {
          do {
            uVar12 = 0x30;
            for (puVar19 = DAT_01012948; FUN_01010c40(param_1,param_2,uVar12,param_5),
                puVar20 != puVar19; puVar19 = puVar19 + 1) {
              uVar12 = *puVar19;
            }
            iVar26 = iVar26 + -0x10;
          } while (0x10 < iVar26);
          local_104 = (local_104 + -0x10) - (local_104 - 0x11U & 0xfffffff0);
        }
        cVar13 = '0';
        pcVar18 = s_0123456789abcdef0123456789ABCDEF_01032654 + 0x20;
        while (FUN_01010c40(param_1,param_2,cVar13,param_5),
              s_0123456789abcdef0123456789ABCDEF_01032654 + local_104 + 0x1f != pcVar18) {
          pcVar18 = pcVar18 + 1;
          cVar13 = *pcVar18;
        }
      }
      puVar20 = DAT_01012944;
      if (0 < local_f4) {
        iVar26 = local_f4;
        if (0x10 < local_f4) {
          do {
            uVar12 = 0x30;
            for (puVar19 = DAT_01012948; FUN_01010c40(param_1,param_2,uVar12,param_5),
                puVar20 != puVar19; puVar19 = puVar19 + 1) {
              uVar12 = *puVar19;
            }
            iVar26 = iVar26 + -0x10;
          } while (0x10 < iVar26);
          local_f4 = (local_f4 + -0x10) - (local_f4 - 0x11U & 0xfffffff0);
        }
        cVar13 = '0';
        pcVar18 = s_0123456789abcdef0123456789ABCDEF_01032654 + 0x20;
        while (FUN_01010c40(param_1,param_2,cVar13,param_5),
              s_0123456789abcdef0123456789ABCDEF_01032654 + local_f4 + 0x1f != pcVar18) {
          pcVar18 = pcVar18 + 1;
          cVar13 = *pcVar18;
        }
      }
      if (local_f0 != (char *)0x0) {
        pcVar18 = pcVar23;
        do {
          pcVar24 = pcVar18 + 1;
          FUN_01010c40(param_1,param_2,*pcVar18,param_5);
          pcVar18 = pcVar24;
        } while (pcVar23 + (int)local_f0 != pcVar24);
      }
      puVar20 = DAT_0101294c;
      if (((local_118 & 4) != 0) && (iVar26 = (int)local_fc - (int)local_10c, 0 < iVar26)) {
        iVar25 = iVar26;
        if (0x10 < iVar26) {
          do {
            uVar12 = 0x20;
            for (puVar19 = DAT_01012950; FUN_01010c40(param_1,param_2,uVar12,param_5),
                puVar20 != puVar19; puVar19 = puVar19 + 1) {
              uVar12 = *puVar19;
            }
            iVar25 = iVar25 + -0x10;
          } while (0x10 < iVar25);
          iVar26 = (iVar26 + -0x10) - (iVar26 - 0x11U & 0xfffffff0);
        }
        uVar12 = 0x20;
        puVar20 = (undefined1 *)0x103263c;
        while (FUN_01010c40(param_1,param_2,uVar12,param_5),
              (undefined1 *)(iVar26 + 0x103263b) != puVar20) {
          puVar20 = puVar20 + 1;
          uVar12 = *puVar20;
        }
      }
      if ((int)local_fc < (int)local_10c) {
        local_fc = local_10c;
      }
      if (0x7fffffff - (int)local_114 < (int)local_fc) {
        return (char *)0xffffffff;
      }
      local_114 = local_114 + (int)local_fc;
      break;
    case (undefined1 *)0x4f:
      bVar3 = true;
    case (undefined1 *)0x6f:
      puVar20 = local_f8;
      if (bVar28) {
        if (local_f8 == (undefined1 *)0x0) {
LAB_01012338:
          puVar9 = (uint *)((int)local_108 + 7U & 0xfffffff8);
          uVar17 = *puVar9;
          uVar21 = puVar9[1];
          local_100 = local_f8;
          local_108 = puVar9 + 2;
        }
        else {
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
          local_100 = (undefined1 *)0x0;
          puVar20 = (undefined1 *)0x0;
        }
      }
      else if (bVar2) {
        if (local_f8 == (undefined1 *)0x0) goto LAB_01012338;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
        puVar20 = (undefined1 *)0x0;
        local_100 = (undefined1 *)0x0;
      }
      else if (bVar3) {
LAB_01012380:
        puVar20 = (undefined1 *)0x0;
        if (local_f8 == (undefined1 *)0x0) {
LAB_01012588:
          uVar21 = 0;
          uVar17 = *local_108;
          local_100 = local_f8;
          puVar20 = local_f8;
          local_108 = local_108 + 1;
        }
        else {
          uVar21 = 0;
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          local_100 = puVar20;
        }
      }
      else if (bVar4) {
LAB_01012490:
        if (local_f8 == (undefined1 *)0x0) goto LAB_01012588;
        uVar21 = 0;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        puVar20 = (undefined1 *)0x0;
        local_100 = (undefined1 *)0x0;
      }
      else {
        if (bVar5) goto LAB_01012380;
        if (bVar6) {
          if (local_f8 == (undefined1 *)0x0) {
            uVar21 = 0;
            uVar17 = *local_108 & 0xffff;
            local_100 = local_f8;
            local_108 = local_108 + 1;
          }
          else {
            uVar21 = 0;
            uVar17 = *(uint *)(local_f8 + (int)local_110 * 8) & 0xffff;
            puVar20 = (undefined1 *)0x0;
            local_100 = (undefined1 *)0x0;
          }
        }
        else {
          if (!bVar7) goto LAB_01012490;
          if (local_f8 == (undefined1 *)0x0) {
            uVar21 = 0;
            uVar17 = *local_108 & 0xff;
            local_100 = local_f8;
            local_108 = local_108 + 1;
          }
          else {
            uVar21 = 0;
            uVar17 = (uint)(byte)local_f8[(int)local_110 * 8];
            puVar20 = (undefined1 *)0x0;
            local_100 = (undefined1 *)0x0;
          }
        }
      }
LAB_010115bc:
      local_104 = 0;
      goto LAB_010115c4;
    case (undefined1 *)0x55:
      bVar3 = true;
    case (undefined1 *)0x75:
      if (bVar28) {
        if (local_f8 == (undefined1 *)0x0) {
LAB_01012310:
          puVar9 = (uint *)((int)local_108 + 7U & 0xfffffff8);
          uVar17 = *puVar9;
          uVar21 = puVar9[1];
          local_100 = local_f8;
          puVar20 = (undefined1 *)0x1;
          local_108 = puVar9 + 2;
        }
        else {
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
          local_100 = (undefined1 *)0x0;
          puVar20 = (undefined1 *)0x1;
        }
      }
      else if (bVar2) {
        if (local_f8 == (undefined1 *)0x0) goto LAB_01012310;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
        puVar20 = (undefined1 *)0x1;
        local_100 = (undefined1 *)0x0;
      }
      else if (bVar3) {
LAB_010123a8:
        if (local_f8 == (undefined1 *)0x0) {
LAB_0101250c:
          uVar21 = 0;
          uVar17 = *local_108;
          local_100 = local_f8;
          puVar20 = (undefined1 *)0x1;
          local_108 = local_108 + 1;
        }
        else {
          uVar21 = 0;
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          puVar20 = (undefined1 *)0x1;
          local_100 = (undefined1 *)0x0;
        }
      }
      else if (bVar4) {
LAB_010124b4:
        if (local_f8 == (undefined1 *)0x0) goto LAB_0101250c;
        uVar21 = 0;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        puVar20 = (undefined1 *)0x1;
        local_100 = (undefined1 *)0x0;
      }
      else {
        if (bVar5) goto LAB_010123a8;
        if (bVar6) {
          if (local_f8 == (undefined1 *)0x0) {
            uVar21 = 0;
            uVar17 = *local_108 & 0xffff;
            local_100 = local_f8;
            puVar20 = (undefined1 *)0x1;
            local_108 = local_108 + 1;
          }
          else {
            uVar21 = 0;
            uVar17 = *(uint *)(local_f8 + (int)local_110 * 8) & 0xffff;
            puVar20 = (undefined1 *)0x1;
            local_100 = (undefined1 *)0x0;
          }
        }
        else {
          if (!bVar7) goto LAB_010124b4;
          if (local_f8 == (undefined1 *)0x0) {
            uVar21 = 0;
            uVar17 = *local_108 & 0xff;
            local_100 = local_f8;
            puVar20 = (undefined1 *)0x1;
            local_108 = local_108 + 1;
          }
          else {
            uVar21 = 0;
            uVar17 = (uint)(byte)local_f8[(int)local_110 * 8];
            puVar20 = (undefined1 *)0x1;
            local_100 = (undefined1 *)0x0;
          }
        }
      }
      goto LAB_010115bc;
    case (undefined1 *)0x58:
      local_dc = s_0123456789abcdef0123456789ABCDEF_01032654 + 0x10;
      goto LAB_01011ab8;
    case (undefined1 *)0x63:
      if (local_f8 == (undefined1 *)0x0) {
        local_8c[0] = (char)*local_108;
        local_108 = local_108 + 1;
      }
      else {
        local_8c[0] = local_f8[(int)local_110 * 8];
      }
      pcVar23 = local_8c;
      local_10c = (char *)0x1;
      local_f4 = -1;
      local_f0 = (char *)0x1;
      local_110 = local_110 + 1;
      goto LAB_01011bcc;
    case (undefined1 *)0x68:
      local_100 = (undefined1 *)(uint)*pbVar16;
      if (local_100 == (undefined1 *)0x68) {
        bVar7 = true;
        local_100 = (undefined1 *)(uint)pbVar16[1];
        pbVar16 = pbVar16 + 1;
      }
      else {
        bVar6 = true;
      }
      goto LAB_010113b0;
    case (undefined1 *)0x6a:
      bVar28 = true;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x6c:
      local_100 = (undefined1 *)(uint)*pbVar16;
      if (local_100 == (undefined1 *)0x6c) {
        bVar2 = true;
        local_100 = (undefined1 *)(uint)pbVar16[1];
        pbVar16 = pbVar16 + 1;
      }
      else {
        bVar3 = true;
      }
      goto LAB_010113b0;
    case (undefined1 *)0x6e:
      pcVar23 = local_110 + 1;
      if (bVar2) {
        if (local_f8 == (undefined1 *)0x0) {
LAB_0101245c:
          puVar8 = (undefined8 *)*local_108;
          local_108 = local_108 + 1;
        }
        else {
          puVar8 = *(undefined8 **)(local_f8 + (int)local_110 * 8);
        }
LAB_01011c94:
        uVar29 = VectorShiftRight(CONCAT44(local_114,local_114),0x20);
        *puVar8 = uVar29;
        local_110 = pcVar23;
      }
      else {
        if (bVar3) {
LAB_01012424:
          if (local_f8 == (undefined1 *)0x0) goto LAB_01012538;
LAB_01012430:
          puVar11 = *(undefined4 **)(local_f8 + (int)local_110 * 8);
        }
        else {
          if (bVar6) {
            if (local_f8 == (undefined1 *)0x0) {
              puVar10 = (undefined2 *)*local_108;
              local_108 = local_108 + 1;
            }
            else {
              puVar10 = *(undefined2 **)(local_f8 + (int)local_110 * 8);
            }
            *puVar10 = (short)local_114;
            local_110 = pcVar23;
            break;
          }
          if (bVar7) {
            if (local_f8 == (undefined1 *)0x0) {
              puVar20 = (undefined1 *)*local_108;
              local_108 = local_108 + 1;
            }
            else {
              puVar20 = *(undefined1 **)(local_f8 + (int)local_110 * 8);
            }
            *puVar20 = (char)local_114;
            local_110 = pcVar23;
            break;
          }
          if ((bVar4) || (bVar5)) goto LAB_01012424;
          if (bVar28) {
            if (local_f8 == (undefined1 *)0x0) goto LAB_0101245c;
            puVar8 = *(undefined8 **)(local_f8 + (int)local_110 * 8);
            goto LAB_01011c94;
          }
          if (local_f8 != (undefined1 *)0x0) goto LAB_01012430;
LAB_01012538:
          puVar11 = (undefined4 *)*local_108;
          local_108 = local_108 + 1;
        }
        *puVar11 = local_114;
        local_110 = pcVar23;
      }
      break;
    case (undefined1 *)0x70:
      if (local_f8 == (undefined1 *)0x0) {
        uVar17 = *local_108;
        local_108 = local_108 + 1;
      }
      else {
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
      }
      uVar21 = 0;
      local_dc = s_0123456789abcdef0123456789ABCDEF_01032654;
      local_100 = (undefined1 *)0x78;
      puVar20 = (undefined1 *)0x2;
      goto LAB_010115bc;
    case (undefined1 *)0x71:
      bVar2 = true;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x73:
      if (local_f8 == (undefined1 *)0x0) {
        pcVar23 = (char *)*local_108;
        local_108 = local_108 + 1;
      }
      else {
        pcVar23 = *(char **)(local_f8 + (int)local_110 * 8);
      }
      if (pcVar23 == (char *)0x0) {
        pcVar23 = s__null__0103264c;
      }
      if (local_f0 == (char *)0xffffffff) {
        local_f0 = (char *)FUN_0101297c(pcVar23);
        if ((int)local_f0 < 0) {
          return (char *)0xffffffff;
        }
      }
      else {
        iVar26 = FUN_01012b3c(pcVar23,0,local_f0);
        if (iVar26 != 0) {
          local_f0 = (char *)(iVar26 - (int)pcVar23);
        }
      }
      local_10c = (char *)((uint)local_f0 & ~((int)local_f0 >> 0x1f));
      local_f4 = -(int)local_f0;
      local_110 = local_110 + 1;
      goto LAB_01011bcc;
    case (undefined1 *)0x74:
      bVar4 = true;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    case (undefined1 *)0x78:
      local_dc = s_0123456789abcdef0123456789ABCDEF_01032654;
LAB_01011ab8:
      if (bVar28) {
        if (local_f8 == (undefined1 *)0x0) {
LAB_010122f8:
          puVar9 = (uint *)((int)local_108 + 7U & 0xfffffff8);
          uVar17 = *puVar9;
          uVar21 = puVar9[1];
          local_108 = puVar9 + 2;
        }
        else {
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
          uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
        }
      }
      else if (bVar2) {
        if (local_f8 == (undefined1 *)0x0) goto LAB_010122f8;
        uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        uVar21 = *(uint *)((int)(local_f8 + (int)local_110 * 8) + 4);
      }
      else if (((bVar3) || (bVar4)) || (bVar5)) {
        if (local_f8 == (undefined1 *)0x0) {
LAB_01012550:
          uVar21 = 0;
          uVar17 = *local_108;
          local_108 = local_108 + 1;
        }
        else {
LAB_010122e8:
          uVar21 = 0;
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8);
        }
      }
      else if (bVar6) {
        if (local_f8 == (undefined1 *)0x0) {
          uVar21 = 0;
          uVar17 = *local_108 & 0xffff;
          local_108 = local_108 + 1;
        }
        else {
          uVar21 = 0;
          uVar17 = *(uint *)(local_f8 + (int)local_110 * 8) & 0xffff;
        }
      }
      else {
        if (!bVar7) {
          if (local_f8 != (undefined1 *)0x0) goto LAB_010122e8;
          goto LAB_01012550;
        }
        if (local_f8 == (undefined1 *)0x0) {
          uVar21 = 0;
          uVar17 = *local_108 & 0xff;
          local_108 = local_108 + 1;
        }
        else {
          uVar21 = 0;
          uVar17 = (uint)(byte)local_f8[(int)local_110 * 8];
        }
      }
      if (bVar1) {
        if (uVar17 == 0 && uVar21 == 0) {
          local_100 = (undefined1 *)0x0;
          puVar20 = (undefined1 *)0x2;
        }
        else {
          puVar20 = (undefined1 *)0x2;
        }
      }
      else {
        puVar20 = (undefined1 *)0x2;
        local_100 = (undefined1 *)0x0;
      }
      goto LAB_010115bc;
    case (undefined1 *)0x7a:
      bVar5 = true;
      local_100 = (undefined1 *)(uint)*pbVar16;
      goto LAB_010113b0;
    }
  }
  goto LAB_01011370;
switchD_010113c0_caseD_31:
  puVar20 = local_100 + -0x30;
  iVar26 = 0;
  while( true ) {
    pcVar18 = puVar20 + iVar26;
    pbVar16 = pbVar27 + 1;
    local_100 = (undefined1 *)(uint)*pbVar27;
    iVar26 = (int)pcVar18 * 10;
    puVar20 = local_100 + -0x30;
    if ((undefined1 *)AL_I2C_INTR_MASK_STOP_DET_SHIFT < puVar20) break;
    if (0xccccccc < (int)pcVar18) {
      return (char *)0xffffffff;
    }
    pbVar27 = pbVar16;
    if (0x7fffffff - (int)puVar20 < iVar26) {
      return (char *)0xffffffff;
    }
  }
  pcVar23 = local_f0;
  if (local_100 == (undefined1 *)0x24) goto LAB_010127b0;
  goto LAB_010113b8;
LAB_010127b0:
  local_110 = pcVar18;
  if (local_f8 == (undefined1 *)0x0) {
    FUN_01010cb8(param_3);
    local_f8 = auStack_d0;
    local_100 = (undefined1 *)(uint)pbVar27[1];
  }
  else {
    local_100 = (undefined1 *)(uint)pbVar27[1];
  }
  goto LAB_010113b0;
}



/* @ 0x1012954  FUN_01012954 */

int FUN_01012954(int param_1,byte *param_2)

{
  uint uVar1;
  byte *pbVar2;
  
  pbVar2 = (byte *)(param_1 + -1);
  do {
    pbVar2 = pbVar2 + 1;
    uVar1 = (uint)*pbVar2;
    if (uVar1 != *param_2) {
      return uVar1 - *param_2;
    }
    param_2 = param_2 + 1;
  } while (uVar1 != 0);
  return 0;
}



/* @ 0x101297c  FUN_0101297c */

int FUN_0101297c(char *param_1)

{
  char *pcVar1;
  
  pcVar1 = param_1;
  if (*param_1 == '\0') {
    return 0;
  }
  do {
    pcVar1 = pcVar1 + 1;
  } while (*pcVar1 != '\0');
  return (int)pcVar1 - (int)param_1;
}



/* @ 0x10129a8  FUN_010129a8 */

byte * FUN_010129a8(byte *param_1,uint param_2)

{
  uint uVar1;
  
  uVar1 = (uint)*param_1;
  if (param_2 != uVar1) {
    while (uVar1 != 0) {
      param_1 = param_1 + 1;
      uVar1 = (uint)*param_1;
      if (uVar1 == param_2) {
        return param_1;
      }
    }
    param_1 = (byte *)0x0;
  }
  return param_1;
}



/* @ 0x10129d8  FUN_010129d8 */

void FUN_010129d8(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  uint uVar3;
  undefined1 *puVar4;
  uint uVar5;
  uint uVar6;
  
  if (param_3 != 0 && param_1 != param_2) {
    if ((((uint)param_1 | (uint)param_2) & 3) == 0) {
      uVar6 = param_3 >> 2;
      uVar5 = param_3 & 3;
    }
    else {
      if (param_3 < 4 || (((uint)param_1 ^ (uint)param_2) & 3) != 0) {
        uVar5 = 0;
        uVar6 = 0;
        uVar3 = param_3;
        param_3 = 0;
      }
      else {
        param_3 = (((uint)param_2 & 3) - 4) + param_3;
        uVar3 = 4 - ((uint)param_2 & 3);
        uVar6 = param_3 >> 2;
        uVar5 = param_3 & 3;
      }
      puVar4 = (undefined1 *)((int)param_1 + -1);
      puVar1 = (undefined4 *)((int)param_2 + uVar3);
      puVar2 = param_2;
      do {
        param_2 = (undefined4 *)((int)puVar2 + 1);
        puVar4 = puVar4 + 1;
        *puVar4 = *(undefined1 *)puVar2;
        puVar2 = param_2;
      } while (param_2 != puVar1);
      param_1 = (undefined4 *)((int)param_1 + uVar3);
    }
    if (3 < param_3) {
      puVar1 = param_2 + -1;
      puVar2 = param_1;
      uVar3 = uVar6;
      do {
        puVar1 = puVar1 + 1;
        uVar3 = uVar3 - 1;
        *puVar2 = *puVar1;
        puVar2 = puVar2 + 1;
      } while (uVar3 != 0);
      param_2 = param_2 + uVar6;
      param_1 = param_1 + uVar6;
    }
    if (uVar5 != 0) {
      puVar4 = (undefined1 *)((int)param_1 + -1);
      puVar2 = param_2;
      do {
        puVar1 = (undefined4 *)((int)puVar2 + 1);
        puVar4 = puVar4 + 1;
        *puVar4 = *(undefined1 *)puVar2;
        puVar2 = puVar1;
      } while (puVar1 != (undefined4 *)((int)param_2 + uVar5));
    }
    return;
  }
  return;
}



/* @ 0x1012ae0  FUN_01012ae0 */

void FUN_01012ae0(undefined1 *param_1,undefined1 param_2,int param_3)

{
  undefined1 *puVar1;
  undefined1 *puVar2;
  
  if (param_3 != 0) {
    puVar2 = param_1;
    do {
      puVar1 = puVar2 + 1;
      *puVar2 = param_2;
      puVar2 = puVar1;
    } while (param_1 + param_3 != puVar1);
  }
  return;
}



/* @ 0x1012b04  FUN_01012b04 */

int FUN_01012b04(byte *param_1,byte *param_2,int param_3)

{
  byte *pbVar1;
  byte *pbVar2;
  
  if (param_3 != 0) {
    pbVar2 = param_1;
    do {
      pbVar1 = pbVar2 + 1;
      if ((uint)*pbVar2 != (uint)*param_2) {
        return (uint)*pbVar2 - (uint)*param_2;
      }
      pbVar2 = pbVar1;
      param_2 = param_2 + 1;
    } while (pbVar1 != param_1 + param_3);
  }
  return 0;
}



/* @ 0x1012b3c  FUN_01012b3c */

char * FUN_01012b3c(char *param_1,char param_2,int param_3)

{
  char *pcVar1;
  char *pcVar2;
  
  if (param_3 == 0) {
    return (char *)0x0;
  }
  pcVar1 = param_1;
  do {
    pcVar2 = pcVar1 + 1;
    if (*pcVar1 == param_2) {
      return pcVar1;
    }
    pcVar1 = pcVar2;
  } while (param_1 + param_3 != pcVar2);
  return (char *)0x0;
}



/* @ 0x1012b80  FUN_01012b80 */

undefined4 FUN_01012b80(int param_1,int param_2,undefined4 param_3)

{
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,
                 s_0123456789abcdef0123456789ABCDEF_01032654 + 0x30,0x36);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,
                 s_0123456789abcdef0123456789ABCDEF_01032654 + 0x30,0x36);
    FUN_01000454(0);
  }
  if (3 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,
                 s_0123456789abcdef0123456789ABCDEF_01032654 + 0x30,0x37);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,
                 s_0123456789abcdef0123456789ABCDEF_01032654 + 0x30,0x37);
    FUN_01000454(0);
  }
  *(undefined4 *)(param_1 + param_2 * 0x40 + 0x28) = param_3;
  return 0;
}



/* @ 0x1012c90  FUN_01012c90 */

void al_iofic_unmask(int param_1,int param_2,uint param_3)

{
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_unmask_01032694,0xbd);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_unmask_01032694,0xbd);
    FUN_01000454(0);
  }
  if (3 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_unmask_01032694,0xbe);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_unmask_01032694,0xbe);
    FUN_01000454(0);
  }
  *(uint *)(param_1 + param_2 * 0x40 + 0x18) = ~param_3;
  return;
}



/* @ 0x1012da0  FUN_01012da0 */

void al_iofic_abort_mask_clear(int param_1,int param_2,uint param_3)

{
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_abort_mask_clear_010326a4,
                 0x152);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_abort_mask_clear_010326a4,
                 0x152);
    FUN_01000454(0);
  }
  if (3 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_abort_mask_clear_010326a4,
                 0x153);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___group_<_4)_010326f8,1,
                 s____HAL__drivers_iofic_al_hal_iof_010326c0,s_al_iofic_abort_mask_clear_010326a4,
                 0x153);
    FUN_01000454(0);
  }
  param_1 = param_1 + param_2 * 0x40;
  *(uint *)(param_1 + 0x30) = *(uint *)(param_1 + 0x30) & ~param_3;
  return;
}



/* @ 0x1012eb4  FUN_01012eb4 */

undefined4 al_udma_q_init(int param_1,uint param_2,uint *param_3)

{
  int iVar1;
  char cVar2;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint *puVar13;
  undefined8 uVar14;
  undefined8 uVar15;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma__010327e8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_init_01032738,0x1c2);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma__010327e8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_init_01032738,0x1c2);
    FUN_01000454(0);
  }
  if (param_3 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___q_params__010327f0,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_init_01032738,0x1c3);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___q_params__010327f0,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_init_01032738,0x1c3);
    FUN_01000454(0);
  }
  if (param_2 < *(byte *)(param_1 + 6)) {
    iVar1 = param_2 * 0x80;
    iVar7 = param_1 + iVar1;
    if (*(char *)(iVar7 + 0x90) == '\x02') {
      FUN_0100ceac(s_udma__queue___d__already_enabled_0103281c,param_2);
      FUN_01010bfc(s_udma__queue___d__already_enabled_0103281c,param_2);
      uVar4 = 0xfffffffb;
    }
    else {
      uVar5 = *param_3;
      if (uVar5 < 4) {
        FUN_0100ceac(s_udma__queue___d__size_too_small_01032840,param_2);
        FUN_01010bfc(s_udma__queue___d__size_too_small_01032840,param_2);
        uVar4 = 0xffffffea;
      }
      else if (uVar5 < 0x10001) {
        uVar6 = uVar5 - 1;
        if ((uVar6 & uVar5) == 0) {
          uVar3 = param_3[9];
          uVar11 = param_3[4];
          iVar9 = param_1 + iVar1 + 0x40;
          uVar12 = param_3[8];
          uVar15 = *(undefined8 *)(param_3 + 2);
          uVar14 = *(undefined8 *)(param_3 + 6);
          uVar10 = param_3[1];
          *(short *)(iVar7 + 0x40) = (short)uVar6;
          *(uint *)(iVar7 + 0x48) = uVar10;
          *(undefined2 *)(iVar7 + 0x4c) = 0;
          *(undefined4 *)(iVar7 + 0x50) = 1;
          *(uint *)(iVar7 + 0x54) = uVar11;
          *(uint *)(iVar7 + 0x58) = uVar12;
          *(undefined2 *)(iVar7 + 0x5c) = 0;
          *(uint *)(iVar7 + 0x60) = uVar12 * uVar6 + uVar11;
          *(undefined2 *)(iVar7 + 100) = 0;
          *(uint *)(iVar7 + 0x68) = uVar11;
          *(undefined4 *)(iVar7 + 0x6c) = 0;
          *(undefined4 *)(iVar7 + 0x70) = 1;
          *(undefined8 *)(iVar7 + 0x78) = uVar15;
          *(undefined8 *)(iVar7 + 0x80) = uVar14;
          *(undefined4 *)(iVar7 + 0x88) = 0;
          *(uint *)(iVar7 + 0x8c) = uVar5;
          *(undefined1 *)(iVar7 + 0x90) = 1;
          *(int *)(iVar7 + 0x94) = param_1;
          *(uint *)(iVar7 + 0x98) = param_2;
          *(char *)(iVar7 + 0x9c) = (char)uVar3;
          if (*(char *)(*(int *)(iVar9 + 0x54) + 4) == '\0') {
            *(uint *)(*(int *)(iVar9 + 4) + 0x74) =
                 *(uint *)(*(int *)(iVar9 + 4) + 0x74) & 0xfffffffb;
          }
          iVar8 = param_1 + iVar1;
          uVar5 = *(uint *)(iVar8 + 0x78);
          uVar4 = *(undefined4 *)(iVar8 + 0x7c);
          if ((uVar5 & 0xf) != 0) {
            FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                         s______uint32_t___al_phys_addr_t___010328b8,1,
                         s____HAL__drivers_udma_al_hal_udma_010327c0,
                         s_al_udma_q_set_pointers_01032710,0xb3);
            FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                         s______uint32_t___al_phys_addr_t___010328b8,1,
                         s____HAL__drivers_udma_al_hal_udma_010327c0,
                         s_al_udma_q_set_pointers_01032710,0xb3);
            FUN_01000454(0);
            uVar5 = *(uint *)(iVar8 + 0x78);
            uVar4 = *(undefined4 *)(iVar8 + 0x7c);
          }
          iVar8 = *(int *)(param_1 + iVar1 + 0x44);
          *(uint *)(iVar8 + 0x28) = uVar5;
          *(undefined4 *)(iVar8 + 0x2c) = uVar4;
          *(undefined4 *)(iVar8 + 0x30) = *(undefined4 *)(iVar7 + 0x8c);
          if (*(int *)(param_1 + iVar1 + 0x54) == 0) {
            uVar5 = 2;
            *(uint *)(iVar7 + 0x88) = *(uint *)(iVar7 + 0x88) | 2;
          }
          else {
            uVar5 = *(uint *)(iVar7 + 0x80);
            uVar4 = *(undefined4 *)(iVar7 + 0x84);
            if ((uVar5 & 0xf) != 0) {
              FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                           s______uint32_t___al_phys_addr_t___01032908,1,
                           s____HAL__drivers_udma_al_hal_udma_010327c0,
                           s_al_udma_q_set_pointers_01032710,0xc2);
              FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                           s______uint32_t___al_phys_addr_t___01032908,1,
                           s____HAL__drivers_udma_al_hal_udma_010327c0,
                           s_al_udma_q_set_pointers_01032710,0xc2);
              FUN_01000454(0);
              uVar5 = *(uint *)(iVar7 + 0x80);
              uVar4 = *(undefined4 *)(iVar7 + 0x84);
            }
            iVar8 = *(int *)(param_1 + iVar1 + 0x44);
            *(uint *)(iVar8 + 0x44) = uVar5;
            *(undefined4 *)(iVar8 + 0x48) = uVar4;
            uVar5 = *(uint *)(iVar7 + 0x88) & 2;
          }
          iVar8 = *(int *)(iVar7 + 0x94);
          iVar7 = *(int *)(param_1 + iVar1 + 0x44);
          cVar2 = *(char *)(iVar8 + 4);
          if (cVar2 == '\0') {
            uVar6 = *(uint *)(iVar7 + 0xa0);
            puVar13 = (uint *)(iVar7 + 0xa0);
          }
          else {
            puVar13 = (uint *)(iVar7 + 0x54);
            uVar6 = *(uint *)(iVar7 + 0x54);
          }
          if (uVar5 == 0) {
            uVar6 = uVar6 | 1;
          }
          else {
            uVar6 = uVar6 & 0xfffffffe;
          }
          *puVar13 = uVar6 | 2;
          if (cVar2 == '\x01') {
            iVar7 = *(int *)(iVar8 + 8);
            *(uint *)(iVar7 + 0x380) =
                 (*(uint *)(param_1 + iVar1 + 0x58) & 0x3f) >> 2 |
                 *(uint *)(iVar7 + 0x380) & 0xfffffff0;
          }
          uVar4 = 0;
          uVar5 = *(uint *)(*(int *)(iVar9 + 4) + 0x20);
          *(undefined1 *)(iVar9 + 0x50) = 2;
          *(uint *)(*(int *)(iVar9 + 4) + 0x20) = uVar5 | 0x30000;
        }
        else {
          FUN_0100ceac(s_udma__queue___d__size___d__must_b_01032888,uVar5,param_2);
          FUN_01010bfc(s_udma__queue___d__size___d__must_b_01032888,*param_3,param_2);
          uVar4 = 0xffffffea;
        }
      }
      else {
        FUN_0100ceac(s_udma__queue___d__size_too_large_01032864,param_2);
        FUN_01010bfc(s_udma__queue___d__size_too_large_01032864,param_2);
        uVar4 = 0xffffffea;
      }
    }
  }
  else {
    FUN_0100ceac(s_udma__invalid_queue_id___d__010327fc,param_2);
    FUN_01010bfc(s_udma__invalid_queue_id___d__010327fc,param_2);
    uVar4 = 0xffffffea;
  }
  return uVar4;
}



/* @ 0x1013328  FUN_01013328 */

bool al_udma_q_is_enabled(int param_1)

{
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_is_enabled_01032748,0x216);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_q__010315b4,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_q_is_enabled_01032748,0x216);
    FUN_01000454(0);
  }
  return (*(uint *)(*(int *)(param_1 + 4) + 0x20) & 0x30000) != 0;
}



/* @ 0x10133c8  FUN_010133c8 */

undefined4 al_udma_state_set(int param_1,int param_2)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma______void___0___010329a8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_set_01032760,0x295);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma______void___0___010329a8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_set_01032760,0x295);
    FUN_01000454(0);
  }
  if (param_2 == 2) {
    uVar1 = 1;
  }
  else if (param_2 == 3) {
    uVar1 = 4;
  }
  else {
    if (param_2 != 0) {
      FUN_0100ceac(s_udma__invalid_state___d__010329c0,param_2);
      FUN_01010bfc(s_udma__invalid_state___d__010329c0,param_2);
      return 0xffffffea;
    }
    uVar1 = 2;
  }
  *(undefined4 *)(*(int *)(param_1 + 8) + 0x204) = uVar1;
  *(char *)(param_1 + 5) = (char)param_2;
  return 0;
}



/* @ 0x10134bc  FUN_010134bc */

undefined4 al_udma_state_get(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  
  uVar2 = *(uint *)(*(int *)(param_1 + 8) + 0x200);
  uVar5 = uVar2 & 3;
  uVar4 = (uVar2 & 0x3f) >> 4;
  uVar3 = (uVar2 & 0x3fff) >> 0xc;
  uVar2 = (uVar2 & 0x3ff) >> 8;
  if (uVar5 == 3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___comp_ctrl____0x3__01032958,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___comp_ctrl____0x3__01032958,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d4);
    FUN_01000454(0);
  }
  if (uVar4 == 3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___stream_if____0x3__0103296c,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d5);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___stream_if____0x3__0103296c,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d5);
    FUN_01000454(0);
  }
  if (uVar2 == 3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data_rd____0x3__01032980,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d6);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data_rd____0x3__01032980,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d6);
    FUN_01000454(0);
  }
  if (uVar3 == 3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc_pref____0x3__01032994,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d7);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___desc_pref____0x3__01032994,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_state_get_01032774,0x2d7);
    FUN_01000454(0);
  }
  if ((uVar4 == 2 || uVar5 == 2) || (uVar3 == 2 || uVar2 == 2)) {
    uVar1 = 3;
  }
  else if (uVar4 == 1 || uVar5 == 1) {
    uVar1 = 2;
  }
  else if (uVar3 == 1 || uVar2 == 1) {
    uVar1 = 2;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}



/* @ 0x10136e0  FUN_010136e0 */

undefined4 FUN_010136e0(undefined4 *param_1,int *param_2,int param_3)

{
  byte bVar1;
  undefined1 uVar2;
  char *pcVar3;
  char *pcVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  int iVar9;
  bool bVar10;
  
  iVar9 = param_2[1];
  *(char *)(param_1 + 1) = (char)iVar9;
  bVar1 = *(byte *)((int)param_2 + 5);
  if (bVar1 == 0xff) {
    *(undefined1 *)((int)param_1 + 6) = 4;
  }
  else {
    *(byte *)((int)param_1 + 6) = bVar1;
    if (4 < bVar1) {
      FUN_0100ceac(s_udma__invalid_num_of_queues_para_01032798);
      FUN_01010bfc(s_udma__invalid_num_of_queues_para_01032798);
      return 0xffffffea;
    }
  }
  iVar5 = *param_2;
  pcVar4 = (char *)param_2[2];
  pcVar3 = s_Failed_to_get_BARs_01031578 + 0x14;
  param_1[3] = iVar5 + 0x1c000;
  iVar7 = iVar5;
  if ((char)iVar9 != '\0') {
    iVar7 = iVar5 + 0x10000;
  }
  uVar6 = *(uint *)(iVar5 + 0x1e084);
  if (pcVar4 == (char *)0x0) {
    pcVar4 = pcVar3;
  }
  if (param_3 == 0) {
    pcVar3 = (char *)0x0;
  }
  uVar2 = SUB41(pcVar3,0);
  *param_1 = pcVar4;
  param_1[2] = iVar7;
  param_1[0x90] = uVar6 & 0xfff;
  if (param_3 != 0) {
    uVar2 = al_udma_state_get(param_1);
    iVar7 = param_1[2];
  }
  iVar9 = 0x1000;
  *(undefined1 *)((int)param_1 + 5) = uVar2;
  puVar8 = param_1 + 0x10;
  while( true ) {
    puVar8[1] = iVar7 + iVar9;
    puVar8[0x15] = param_1;
    if ((param_3 == 0) || (iVar7 = al_udma_q_is_enabled(puVar8), iVar7 == 0)) {
      uVar2 = 1;
    }
    else {
      uVar2 = 2;
    }
    bVar10 = iVar9 == 0x4000;
    *(undefined1 *)(puVar8 + 0x14) = uVar2;
    iVar9 = iVar9 + 0x1000;
    if (bVar10) break;
    iVar7 = param_1[2];
    puVar8 = puVar8 + 0x20;
  }
  return 0;
}



/* @ 0x1013814  FUN_01013814 */

int al_udma_init(int param_1,undefined4 *param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma__010327e8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_init_01032728,0x1a5);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma__010327e8,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_init_01032728,0x1a5);
    FUN_01000454(0);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_params__01032788,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_init_01032728,0x1a6);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_params__01032788,1,
                 s____HAL__drivers_udma_al_hal_udma_010327c0,s_al_udma_init_01032728,0x1a6);
    FUN_01000454(0);
  }
  iVar1 = FUN_010136e0(param_1,param_2,0);
  if (iVar1 != 0) {
    return iVar1;
  }
  if (*(char *)(param_1 + 4) == '\0') {
    iVar1 = *(int *)(param_1 + 8);
    local_20 = DAT_01032708;
    uStack_1c = DAT_0103270c;
    if (1 < *(byte *)(param_1 + 0x240)) {
      *(uint *)(iVar1 + 0x310) = *(uint *)(iVar1 + 0x310) & 0xfffffc00 | 0x100;
    }
    *(undefined4 *)(iVar1 + 0x1e280) = 1000000;
    *(undefined4 *)(iVar1 + 0x408) = 0;
    al_udma_m2s_packet_size_cfg_set(param_1,&local_20);
    if (*(char *)(param_1 + 4) != '\x01') {
      uVar2 = *param_2;
      if (*(char *)(param_1 + 4) == '\0') {
        al_udma_iofic_m2s_error_ints_unmask();
        return 0;
      }
      goto LAB_01013904;
    }
  }
  else if (*(char *)(param_1 + 4) != '\x01') {
    uVar2 = *param_2;
    goto LAB_01013904;
  }
  uVar2 = *param_2;
  *(undefined4 *)(*(int *)(param_1 + 8) + 0x38c) = 0;
LAB_01013904:
  al_udma_iofic_s2m_error_ints_unmask(uVar2);
  return 0;
}



/* @ 0x10139d8  FUN_010139d8 */

undefined4 al_udma_m2s_packet_size_cfg_set(undefined4 *param_1,uint *param_2)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = *(uint *)(param_1[2] + 0x254);
  if (*(char *)(param_1 + 1) != '\0') {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_>type____UDMA_TX__01032a88,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,
                 s_al_udma_m2s_packet_size_cfg_set_010329dc,0x145);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___udma_>type____UDMA_TX__01032a88,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,
                 s_al_udma_m2s_packet_size_cfg_set_010329dc,0x145);
    FUN_01000454(0);
  }
  uVar2 = *param_2;
  if (uVar2 < 0x100000) {
    uVar3 = uVar3 & 0xfeffffff;
    uVar1 = 0;
    if (param_2[1] == 1) {
      uVar3 = uVar3 | 0x1000000;
    }
    *(uint *)(param_1[2] + 0x254) = uVar2 | uVar3 & 0xfff00000;
  }
  else {
    FUN_0100ceac(s_udma___s___requested_max_pkt_siz_01032aa4,*param_1,uVar2,0xfffff);
    FUN_01010bfc(s_udma___s___requested_max_pkt_siz_01032aa4,*param_1,*param_2,0xfffff);
    uVar1 = 0xffffffea;
  }
  return uVar1;
}



/* @ 0x1013af0  FUN_01013af0 */

undefined4 al_udma_m2s_max_descs_set(int param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  
  if (param_2 < 0x20) {
    if (param_2 == 0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_>_0__01032a48,1,
                   s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_m2s_max_descs_set_010329fc,
                   0x1d5);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_>_0__01032a48,1,
                   s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_m2s_max_descs_set_010329fc,
                   0x1d5);
      FUN_01000454(0);
      uVar1 = 0x440;
      goto LAB_01013b28;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_<__31)_01032a34,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_m2s_max_descs_set_010329fc,
                 0x1d4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_<__31)_01032a34,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_m2s_max_descs_set_010329fc,
                 0x1d4);
    FUN_01000454(0);
  }
  if (param_2 < 8) {
    uVar1 = 0x440;
  }
  else {
    uVar1 = param_2 << 8 | 0x80;
  }
LAB_01013b28:
  iVar2 = *(int *)(param_1 + 8);
  *(uint *)(iVar2 + 0x304) = (param_2 ^ *(uint *)(iVar2 + 0x304)) & 0x1f ^ *(uint *)(iVar2 + 0x304);
  *(uint *)(iVar2 + 0x308) = *(uint *)(iVar2 + 0x308) & 0xffff000f | uVar1;
  return 0;
}



/* @ 0x1013c30  FUN_01013c30 */

undefined4 al_udma_s2m_max_descs_set(int param_1,uint param_2)

{
  uint uVar1;
  
  if (param_2 < 0x20) {
    if (param_2 == 0) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_>_0__01032a48,1,
                   s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_s2m_max_descs_set_01032a18,
                   500);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_>_0__01032a48,1,
                   s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_s2m_max_descs_set_01032a18,
                   500);
      uVar1 = 0x440;
      FUN_01000454(0);
    }
    else if (param_2 < 8) {
      uVar1 = 0x440;
    }
    else {
      uVar1 = param_2 << 8 | 0x80;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_<__31)_01032a34,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_s2m_max_descs_set_01032a18,
                 499);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___max_descs_<__31)_01032a34,1,
                 s____HAL__drivers_udma_al_hal_udma_01032a5c,s_al_udma_s2m_max_descs_set_01032a18,
                 499);
    uVar1 = param_2 << 8 | 0x80;
    FUN_01000454(0);
  }
  *(uint *)(*(int *)(param_1 + 8) + 0x308) =
       *(uint *)(*(int *)(param_1 + 8) + 0x308) & 0xffff000f | uVar1;
  return 0;
}



/* @ 0x1013d68  FUN_01013d68 */

void al_udma_iofic_m2s_error_ints_unmask(int param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_udma_al_hal_udma_01032b38,
                 s_al_udma_iofic_m2s_error_ints_unm_01032af0,0x95);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_udma_al_hal_udma_01032b38,
                 s_al_udma_iofic_m2s_error_ints_unm_01032af0,0x95);
    FUN_01000454(0);
  }
  iVar1 = param_1 + 0x1e000;
  FUN_01012b80(iVar1,0,0x28);
  FUN_01012b80(param_1 + 0x1c000,3,0x28);
  al_iofic_abort_mask_clear(iVar1,0,0xfffffff);
  al_iofic_unmask(iVar1,0,0xfffffff);
  al_iofic_unmask(param_1 + 0x1c000,3,0x100);
  return;
}



/* @ 0x1013e4c  FUN_01013e4c */

void al_udma_iofic_s2m_error_ints_unmask(int param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_udma_al_hal_udma_01032b38,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b14,0xbb);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_udma_al_hal_udma_01032b38,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b14,0xbb);
    FUN_01000454(0);
  }
  iVar1 = param_1 + 0x1e000;
  FUN_01012b80(iVar1,1,0x28);
  FUN_01012b80(param_1 + 0x1c000,3,0x28);
  al_iofic_abort_mask_clear(iVar1,1,0x7fffef07);
  al_iofic_unmask(iVar1,1,0x7fffef07);
  al_iofic_unmask(param_1 + 0x1c000,3,0x200);
  return;
}



/* @ 0x1013f38  FUN_01013f38 */

int FUN_01013f38(undefined4 *param_1,undefined4 *param_2)

{
  int iVar1;
  undefined4 local_24;
  undefined1 local_20;
  undefined1 local_1f;
  char *local_1c;
  
  local_24 = *param_2;
  *param_1 = param_2[1];
  param_1[1] = local_24;
  local_1f = *(undefined1 *)(param_2 + 2);
  local_20 = 0;
  *(undefined1 *)(param_1 + 2) = local_1f;
  local_1c = s_tx_dma_01032b9c;
  iVar1 = al_udma_init(param_1 + 0x10,&local_24);
  if (iVar1 == 0) {
    al_udma_m2s_max_descs_set(param_1 + 0x10,*(undefined1 *)((int)param_2 + 9));
    local_1f = *(undefined1 *)(param_1 + 2);
    local_24 = param_1[1];
    local_1c = s_rx_dma_01032bc8;
    local_20 = 1;
    iVar1 = al_udma_init(param_1 + 0xb0,&local_24);
    if (iVar1 == 0) {
      al_udma_s2m_max_descs_set(param_1 + 0xb0,*(undefined1 *)((int)param_2 + 10));
      return 0;
    }
  }
  FUN_0100ceac(s_failed_to_initialize__s__error___01032ba4,local_1c,iVar1);
  FUN_01010bfc(s_failed_to_initialize__s__error___01032ba4,local_1c,iVar1);
  return iVar1;
}



/* @ 0x1014034  FUN_01014034 */

int FUN_01014034(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  iVar1 = al_udma_q_init(param_1 + 0x10);
  if (iVar1 == 0) {
    iVar1 = al_udma_q_init(param_1 + 0xb0,param_2,param_4);
    if (iVar1 != 0) {
      FUN_0100ceac(s___s___failed_to_initialize_rx_q___01032c00,*param_1,param_2,iVar1);
      FUN_01010bfc(s___s___failed_to_initialize_rx_q___01032c00,*param_1,param_2,iVar1);
    }
  }
  else {
    FUN_0100ceac(s___s___failed_to_initialize_tx_q___01032bd0,*param_1,param_2,iVar1);
    FUN_01010bfc(s___s___failed_to_initialize_tx_q___01032bd0,*param_1,param_2,iVar1);
  }
  return iVar1;
}



/* @ 0x10140f8  FUN_010140f8 */

int FUN_010140f8(undefined4 *param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = al_udma_state_set(param_1 + 0x10);
  if ((iVar1 != 0) || (iVar1 = al_udma_state_set(param_1 + 0xb0,param_2), iVar1 != 0)) {
    FUN_0100ceac(s_ssm___s___failed_to_change_state_01032c88 + 4,*param_1,iVar1);
    FUN_01010bfc(s_ssm___s___failed_to_change_state_01032c88 + 4,*param_1,iVar1);
  }
  return iVar1;
}



/* @ 0x1014170  FUN_01014170 */

undefined4 FUN_01014170(int param_1,int param_2,int *param_3)

{
  if (param_2 == 0) {
    param_1 = param_1 + 0x40;
  }
  else {
    param_1 = param_1 + 0x2c0;
  }
  *param_3 = param_1;
  return 0;
}



/* @ 0x1014188  FUN_01014188 */

int FUN_01014188(undefined1 *param_1,undefined1 *param_2)

{
  int iVar1;
  int iVar2;
  int local_1c;
  undefined4 local_18;
  undefined1 local_14;
  undefined1 local_13;
  undefined1 local_12;
  
  local_13 = 0x1f;
  local_12 = 0x1f;
  *param_1 = *param_2;
  local_14 = param_2[0xc];
  iVar2 = *(int *)(param_2 + 4);
  local_18 = *(undefined4 *)(param_2 + 8);
  local_1c = iVar2;
  iVar1 = FUN_01013f38(param_1 + 0x40,&local_1c);
  if (iVar1 != 0) {
    FUN_0100ceac(s_failed_to_initialize_udma__error_01032c30,iVar1);
    FUN_01010bfc(s_failed_to_initialize_udma__error_01032c30,iVar1);
  }
  iVar2 = iVar2 + 0x1e000;
  al_iofic_unmask(iVar2,1,0x40);
  al_iofic_abort_mask_clear(iVar2,1,0x40);
  return iVar1;
}



/* @ 0x1014230  FUN_01014230 */

int FUN_01014230(undefined1 *param_1,int param_2,int param_3,int param_4,undefined1 param_5)

{
  undefined1 uVar1;
  int iVar2;
  
  uVar1 = *param_1;
  *(undefined1 *)(param_3 + 0x24) = uVar1;
  *(undefined1 *)(param_4 + 0x24) = uVar1;
  iVar2 = FUN_01014034(param_1 + 0x40);
  if (iVar2 == 0) {
    param_1[param_2 + 0x580] = param_5;
  }
  else {
    FUN_0100ceac(s_ssm___s___failed_to_initialize_q_01032c58,*(undefined4 *)(param_1 + 0x40),param_2
                 ,iVar2);
    FUN_01010bfc(s_ssm___s___failed_to_initialize_q_01032c58,*(undefined4 *)(param_1 + 0x40),param_2
                 ,iVar2);
  }
  return iVar2;
}



/* @ 0x10142b8  FUN_010142b8 */

int FUN_010142b8(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_010140f8(param_1 + 0x40);
  if (iVar1 != 0) {
    FUN_0100ceac(s_ssm___s___failed_to_change_state_01032c88,*(undefined4 *)(param_1 + 0x40),iVar1);
    FUN_01010bfc(s_ssm___s___failed_to_change_state_01032c88,*(undefined4 *)(param_1 + 0x40),iVar1);
  }
  return iVar1;
}



/* @ 0x1014318  FUN_01014318 */

int FUN_01014318(int param_1,int param_2)

{
  int iVar1;
  int local_c;
  
  iVar1 = FUN_01014170(param_1 + 0x40,0,&local_c);
  if (iVar1 == 0) {
    local_c = local_c + param_2 * 0x80 + 0x40;
  }
  else {
    local_c = 0;
  }
  return local_c;
}



/* @ 0x1014364  FUN_01014364 */

int FUN_01014364(int param_1,int param_2)

{
  int iVar1;
  int local_c;
  
  iVar1 = FUN_01014170(param_1 + 0x40,1,&local_c);
  if (iVar1 == 0) {
    local_c = local_c + param_2 * 0x80 + 0x40;
  }
  else {
    local_c = 0;
  }
  return local_c;
}



/* @ 0x10146cc  FUN_010146cc */

void FUN_010146cc(int *param_1,uint param_2,int param_3,uint param_4,byte param_5)

{
  int iVar1;
  uint unaff_r4;
  uint uVar2;
  uint uVar3;
  
  if (param_2 != 7) {
    unaff_r4 = param_2;
  }
  if (param_2 == 7) {
    unaff_r4 = 3;
    param_2 = 0;
  }
  iVar1 = *param_1;
  uVar3 = param_2;
  do {
    uVar2 = uVar3 << 0xd;
    uVar3 = uVar3 + 1 & 0xff;
    uVar2 = uVar2 | 0x15;
    *(uint *)(iVar1 + 0x110) = uVar2;
    *(uint *)(iVar1 + 0x110) = uVar2;
    *(uint *)(iVar1 + 0x114) =
         ((param_4 & 0xff) + (uint)param_5 + param_3) * 8 & 0xff | *(uint *)(iVar1 + 0x114) & 7;
    uVar2 = param_2;
  } while (uVar3 <= unaff_r4);
  do {
    uVar3 = uVar2 + 1 & 0xff;
    uVar2 = uVar2 << 0xd | 0x16;
    *(uint *)(iVar1 + 0x110) = uVar2;
    *(uint *)(iVar1 + 0x110) = uVar2;
    *(uint *)(iVar1 + 0x114) = param_4 & 0xff | *(uint *)(iVar1 + 0x114) & 0xf0;
    uVar2 = uVar3;
  } while (uVar3 <= unaff_r4);
  do {
    uVar3 = param_2 << 0xd;
    param_2 = param_2 + 1 & 0xff;
    uVar3 = uVar3 | 0x17;
    *(uint *)(iVar1 + 0x110) = uVar3;
    *(uint *)(iVar1 + 0x110) = uVar3;
    *(uint *)(iVar1 + 0x114) = (uint)param_5 | *(uint *)(iVar1 + 0x114) & 0xf8;
  } while (param_2 <= unaff_r4);
  return;
}



/* @ 0x1014b8c  FUN_01014b8c */

undefined4 al_serdes_eye_diag_post_sample(int *param_1,uint param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_post_sample_01032e94,0x583);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_post_sample_01032e94,0x583);
    FUN_01000454(0);
  }
  uVar3 = param_2;
  if (param_2 < 8) {
    if (param_2 == 7) {
      param_2 = 3;
      uVar3 = 0;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_post_sample_01032e94,0x585);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_post_sample_01032e94,0x585);
    FUN_01000454(0);
  }
  iVar2 = *param_1;
  uVar1 = uVar3;
  do {
    uVar4 = uVar1 << 0xd;
    uVar1 = uVar1 + 1 & 0xff;
    uVar4 = uVar4 | 0x56;
    *(uint *)(iVar2 + 0x110) = uVar4;
    *(uint *)(iVar2 + 0x110) = uVar4;
    *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xbf | 0x40;
    uVar4 = uVar3;
  } while (uVar1 <= param_2);
  do {
    uVar1 = uVar4 + 1 & 0xff;
    uVar4 = uVar4 << 0xd | 0x56;
    *(uint *)(iVar2 + 0x110) = uVar4;
    *(uint *)(iVar2 + 0x110) = uVar4;
    *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0x7f | 0x80;
    uVar4 = uVar1;
  } while (uVar1 <= param_2);
  do {
    uVar1 = uVar3 << 0xd;
    uVar3 = uVar3 + 1 & 0xff;
    uVar1 = uVar1 | 0x57;
    *(uint *)(iVar2 + 0x110) = uVar1;
    *(uint *)(iVar2 + 0x110) = uVar1;
    *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xfe | 1;
  } while (uVar3 <= param_2);
  *(undefined4 *)(iVar2 + 0x110) = 0x8096;
  *(uint *)(iVar2 + 0x114) = (param_3 & 0xffff) >> 8;
  *(undefined4 *)(iVar2 + 0x110) = 0x8097;
  *(uint *)(iVar2 + 0x114) = param_3 & 0xff;
  return 0;
}



/* @ 0x1014d64  FUN_01014d64 */

undefined4 al_serdes_eye_diag_stop_sample(int *param_1,uint param_2,uint *param_3,uint *param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x55f);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x55f);
    FUN_01000454(0);
  }
  if (7 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x561);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x561);
    FUN_01000454(0);
  }
  if (param_4 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___value__01033088,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x562);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___value__01033088,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_stop_sample_01032e74,0x562);
    FUN_01000454(0);
  }
  uVar1 = param_2 << 0xd;
  iVar3 = *param_1;
  if (param_3 == (uint *)0x0) {
    uVar2 = uVar1;
    uVar4 = param_2;
    if (param_2 == 7) {
      param_2 = 0;
      uVar2 = 0;
      uVar4 = 3;
    }
    do {
      param_2 = param_2 + 1 & 0xff;
      *(uint *)(iVar3 + 0x110) = uVar2 | 0x1f;
      *(uint *)(iVar3 + 0x110) = uVar2 | 0x1f;
      *(uint *)(iVar3 + 0x114) = *(uint *)(iVar3 + 0x114) & 0xfb;
      uVar2 = param_2 << 0xd;
    } while (param_2 <= uVar4);
  }
  else {
    uVar2 = *param_3;
    *(uint *)(iVar3 + 0x110) = uVar1 | 0x1f;
    *(uint *)(iVar3 + 0x114) = uVar2 & 0xfb;
  }
  *(uint *)(iVar3 + 0x110) = uVar1 | 0x25;
  *(uint *)(iVar3 + 0x110) = uVar1 | 0x26;
  *param_4 = (*(uint *)(iVar3 + 0x114) & 0xff) << 8 | *(uint *)(iVar3 + 0x114) & 0xff;
  return 0;
}



/* @ 0x1014f7c  FUN_01014f7c */

uint al_serdes_eye_diag_poll_sample(int *param_1,uint param_2)

{
  int iVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_poll_sample_01032e54,0x54b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_poll_sample_01032e54,0x54b);
    FUN_01000454(0);
  }
  if (7 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_poll_sample_01032e54,0x54d);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_poll_sample_01032e54,0x54d);
    FUN_01000454(0);
  }
  iVar1 = *param_1;
  *(uint *)(iVar1 + 0x110) = param_2 << 0xd | 0x20;
  return (*(uint *)(iVar1 + 0x114) & 7) >> 2;
}



/* @ 0x101508c  FUN_0101508c */

undefined4 al_serdes_eye_diag_start_sample(int *param_1,uint param_2,byte *param_3,uint param_4,uint param_5)

{
  byte bVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_start_sample_01032e34,0x526);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_start_sample_01032e34,0x526);
    FUN_01000454(0);
  }
  if (7 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_start_sample_01032e34,0x528);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_start_sample_01032e34,0x528);
    FUN_01000454(0);
  }
  iVar2 = *param_1;
  if (0 < (int)param_4) {
    *(uint *)(iVar2 + 0x110) = param_2 << 0xd | 0x21;
    *(uint *)(iVar2 + 0x114) = param_4 & 0xff;
  }
  if (0 < (int)param_5) {
    if ((int)param_5 < 0x20) {
      uVar4 = 0x1f - (param_5 & 0xff);
    }
    else {
      uVar4 = (param_5 & 0xff) + 1;
    }
    *(uint *)(iVar2 + 0x110) = param_2 << 0xd | 0x22;
    *(uint *)(iVar2 + 0x114) = uVar4 & 0xff;
  }
  if (param_3 == (byte *)0x0) {
    uVar4 = param_2;
    if (param_2 == 7) {
      param_2 = 3;
      uVar4 = 0;
    }
    do {
      uVar3 = uVar4 + 1 & 0xff;
      uVar4 = uVar4 << 0xd | 0x1f;
      *(uint *)(iVar2 + 0x110) = uVar4;
      *(uint *)(iVar2 + 0x110) = uVar4;
      *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xfb | 4;
      uVar4 = uVar3;
    } while (uVar3 <= param_2);
  }
  else {
    bVar1 = *param_3;
    *(uint *)(iVar2 + 0x110) = param_2 << 0xd | 0x1f;
    *(uint *)(iVar2 + 0x114) = bVar1 | 4;
  }
  return 0;
}



/* @ 0x101525c  FUN_0101525c */

undefined4 al_serdes_eye_diag_pre_sample(int *param_1,uint param_2,uint *param_3,uint *param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  bool bVar7;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_pre_sample_01032e14,0x4ea);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_pre_sample_01032e14,0x4ea);
    FUN_01000454(0);
  }
  bVar7 = param_2 != 7;
  if (param_2 < 8) {
    iVar6 = *param_1;
    *(undefined4 *)(iVar6 + 0x110) = 0x8096;
    *(undefined4 *)(iVar6 + 0x110) = 0x8097;
    uVar4 = (*(uint *)(iVar6 + 0x114) & 0xff) << 8 | *(uint *)(iVar6 + 0x114) & 0xff;
    *param_3 = uVar4;
    if (bVar7) {
      uVar4 = param_2;
    }
    *(undefined4 *)(iVar6 + 0x110) = 0x8096;
    *(undefined4 *)(iVar6 + 0x114) = 0x13;
    *(undefined4 *)(iVar6 + 0x110) = 0x8097;
    uVar1 = 0x8097;
    if (bVar7) {
      uVar1 = param_2;
    }
    *(undefined4 *)(iVar6 + 0x114) = 0x88;
    uVar5 = uVar4;
    if (!bVar7) {
      uVar4 = 0;
      uVar5 = 0;
      uVar1 = 3;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_pre_sample_01032e14,0x4ec);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s_al_serdes_eye_diag_pre_sample_01032e14,0x4ec);
    FUN_01000454(0);
    iVar6 = *param_1;
    *(undefined4 *)(iVar6 + 0x110) = 0x8096;
    *(undefined4 *)(iVar6 + 0x110) = 0x8097;
    *param_3 = (*(uint *)(iVar6 + 0x114) & 0xff) << 8 | *(uint *)(iVar6 + 0x114) & 0xff;
    *(undefined4 *)(iVar6 + 0x110) = 0x8096;
    *(undefined4 *)(iVar6 + 0x114) = 0x13;
    *(undefined4 *)(iVar6 + 0x110) = 0x8097;
    *(undefined4 *)(iVar6 + 0x114) = 0x88;
    uVar4 = param_2;
    uVar5 = param_2;
    uVar1 = param_2;
  }
  do {
    uVar2 = uVar4 + 1 & 0xff;
    uVar4 = uVar4 << 0xd | 0x56;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x114) = *(uint *)(iVar6 + 0x114) & 0xbf;
    uVar3 = uVar5;
    uVar4 = uVar2;
  } while (uVar2 <= uVar1);
  do {
    uVar2 = uVar3 + 1 & 0xff;
    uVar4 = uVar3 << 0xd | 0x56;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x114) = *(uint *)(iVar6 + 0x114) & 0x7f;
    uVar4 = uVar5;
    uVar3 = uVar2;
  } while (uVar2 <= uVar1);
  do {
    uVar3 = uVar4 + 1 & 0xff;
    uVar4 = uVar4 << 0xd | 0x57;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x114) = *(uint *)(iVar6 + 0x114) & 0xfe;
    uVar4 = uVar3;
  } while (uVar3 <= uVar1);
  do {
    uVar4 = uVar5 << 0xd;
    uVar5 = uVar5 + 1 & 0xff;
    uVar4 = uVar4 | 0x27;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x110) = uVar4;
    *(uint *)(iVar6 + 0x114) = *(uint *)(iVar6 + 0x114) & 0xfb | 4;
  } while (uVar5 <= uVar1);
  if (param_4 != (uint *)0x0) {
    *(uint *)(iVar6 + 0x110) = param_2 << 0xd | 0x1f;
    *param_4 = *(uint *)(iVar6 + 0x114) & 0xff;
  }
  return 0;
}



/* @ 0x1015870  FUN_01015870 */

void al_serdes_bist_pattern_select(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int *unaff_r6;
  code *UNRECOVERED_JUMPTABLE;
  
  FUN_0100ceac(s__s__invalid_pattern___d__01032fa8,s_al_serdes_bist_pattern_select_01032d7c,param_2)
  ;
  FUN_01010bfc(s__s__invalid_pattern___d__01032fa8,s_al_serdes_bist_pattern_select_01032d7c,param_2)
  ;
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_serdes_al_hal_se_01032ff8);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_serdes_al_hal_se_01032ff8);
  FUN_01000454(0);
  iVar1 = *unaff_r6;
  *(undefined4 *)(iVar1 + 0x110) = 0x8050;
  *(undefined4 *)(iVar1 + 0x110) = 0x8050;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf8;
                    /* WARNING: Could not recover jumptable at 0x010157a4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0x1015a1c  FUN_01015a1c */

void _al_serdes_lane_rx_rate_change_sw_flow_dis(int *param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  
  if (param_2 == 7) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___lane____AL_SRDS_LANES_0123__01032f78,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s__al_serdes_lane_rx_rate_change_s_01032d9c,0x439);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___lane____AL_SRDS_LANES_0123__01032f78,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s__al_serdes_lane_rx_rate_change_s_01032d9c,0x439);
    FUN_01000454(0);
  }
  uVar2 = param_2 << 0xd;
  iVar3 = *param_1;
  *(uint *)(iVar3 + 0x110) = uVar2 | 0xc9;
  if (((((*(uint *)(iVar3 + 0x114) & 0xff) == 0xfc) &&
       (*(uint *)(iVar3 + 0x110) = uVar2 | 0xca, (*(uint *)(iVar3 + 0x114) & 0xff) == 0xff)) &&
      (*(uint *)(iVar3 + 0x110) = uVar2 | 0xcb, (*(uint *)(iVar3 + 0x114) & 0xff) == 0xff)) &&
     ((*(uint *)(iVar3 + 0x110) = uVar2 | 0xcc, (*(uint *)(iVar3 + 0x114) & 0xff) == 0xff &&
      (*(uint *)(iVar3 + 0x110) = uVar2 | 0xcd, (*(uint *)(iVar3 + 0x114) & 0xff) == 0xff)))) {
    uVar4 = param_2;
    if (param_2 == 7) {
      param_2 = 0;
      uVar2 = 0;
      uVar4 = 3;
    }
    do {
      uVar1 = uVar2 | 0xcd;
      param_2 = param_2 + 1 & 0xff;
      *(uint *)(iVar3 + 0x110) = uVar1;
      uVar2 = param_2 << 0xd;
      *(uint *)(iVar3 + 0x110) = uVar1;
      *(uint *)(iVar3 + 0x114) = *(uint *)(iVar3 + 0x114) & 0x7f;
    } while (param_2 <= uVar4);
  }
  return;
}



/* @ 0x1015b7c  FUN_01015b7c */

void _al_serdes_lane_rx_rate_change_sw_flow_en_cond(int *param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  if (param_2 == 7) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___lane____AL_SRDS_LANES_0123__01032f78,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s__al_serdes_lane_rx_rate_change_s_01032dc8,0x469);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___lane____AL_SRDS_LANES_0123__01032f78,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,
                 s__al_serdes_lane_rx_rate_change_s_01032dc8,0x469);
    FUN_01000454(0);
  }
  uVar2 = param_2 << 0xd;
  iVar1 = *param_1;
  *(uint *)(iVar1 + 0x110) = uVar2 | 0xc9;
  if (((((*(uint *)(iVar1 + 0x114) & 0xff) == 0xfc) &&
       (*(uint *)(iVar1 + 0x110) = uVar2 | 0xca, (*(uint *)(iVar1 + 0x114) & 0xff) == 0xff)) &&
      (*(uint *)(iVar1 + 0x110) = uVar2 | 0xcb, (*(uint *)(iVar1 + 0x114) & 0xff) == 0xff)) &&
     (*(uint *)(iVar1 + 0x110) = uVar2 | 0xcc, (*(uint *)(iVar1 + 0x114) & 0xff) == 0xff)) {
    *(uint *)(iVar1 + 0x110) = uVar2 | 0xcd;
    uVar4 = (uint)((*(uint *)(iVar1 + 0x114) & 0xff) == 0x7f);
  }
  else {
    uVar4 = 0;
  }
  *(undefined4 *)(iVar1 + 0x110) = 0x8002;
  if ((*(uint *)(iVar1 + 0x114) & 1) != 0) {
    *(undefined4 *)(iVar1 + 0x110) = 0x8002;
    uVar4 = uVar4 & *(uint *)(iVar1 + 0x114) >> 1;
  }
  *(uint *)(iVar1 + 0x110) = uVar2 | 2;
  uVar3 = 0;
  if ((*(uint *)(iVar1 + 0x114) & 2) != 0) {
    *(uint *)(iVar1 + 0x110) = uVar2 | 2;
    uVar3 = ~*(uint *)(iVar1 + 0x114) & 1;
  }
  *(undefined4 *)(iVar1 + 0x110) = 0x8003;
  *(uint *)(iVar1 + 0x110) = uVar2 | 3;
  if ((uVar4 != 0) &&
     (((*(uint *)(iVar1 + 0x114) & 0x1f) != 1 && (*(uint *)(iVar1 + 0x114) & 0x1f) != 1) &&
      uVar3 == 0)) {
    udelay(1);
    uVar4 = param_2;
    if (param_2 == 7) {
      param_2 = 3;
      uVar2 = 0;
      uVar4 = 0;
    }
    iVar1 = *param_1;
    do {
      uVar3 = uVar2 | 0xcd;
      uVar4 = uVar4 + 1 & 0xff;
      *(uint *)(iVar1 + 0x110) = uVar3;
      uVar2 = uVar4 << 0xd;
      *(uint *)(iVar1 + 0x110) = uVar3;
      *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x7f | 0x80;
    } while (uVar4 <= param_2);
  }
  return;
}



/* @ 0x1016588  FUN_01016588 */

undefined4 al_serdes_reg_write(int *param_1,uint param_2,uint param_3,uint param_4,byte param_5)

{
  int iVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe0);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe0);
    FUN_01000454(0);
  }
  if (7 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe2);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_0103302c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe2);
    FUN_01000454(0);
  }
  if (1 < param_3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_type__<__AL_SRDS_REG_TYP_0103324c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_type__<__AL_SRDS_REG_TYP_0103324c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_al_serdes_reg_write_01032ce0,0xe4);
    FUN_01000454(0);
  }
  iVar1 = *param_1;
  *(uint *)(iVar1 + 0x110) = param_4 | param_3 << 0xc | param_2 << 0xd;
  *(uint *)(iVar1 + 0x114) = (uint)param_5;
  return 0;
}



/* @ 0x1016964  FUN_01016964 */

undefined4 FUN_01016964(int *param_1,uint param_2,uint param_3,uint param_4,undefined1 *param_5)

{
  int iVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xb7
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xb7
                );
    FUN_01000454(0);
  }
  if (param_5 == (undefined1 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data__01033218,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xb8
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data__01033218,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xb8
                );
    FUN_01000454(0);
  }
  if (4 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_01033220,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xba
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_page__<__AL_SRDS_REG_PAG_01033220,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xba
                );
    FUN_01000454(0);
  }
  if (1 < param_3) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_type__<__AL_SRDS_REG_TYP_0103324c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xbc
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s_____int_type__<__AL_SRDS_REG_TYP_0103324c,1,
                 s____HAL__drivers_serdes_al_hal_se_01032ff8,s_0al_serdes_reg_read_01032cca + 2,0xbc
                );
    FUN_01000454(0);
  }
  iVar1 = *param_1;
  *(uint *)(iVar1 + 0x110) = param_4 | param_3 << 0xc | param_2 << 0xd;
  *param_5 = (char)*(undefined4 *)(iVar1 + 0x114);
  return 0;
}



/* @ 0x1017b88  FUN_01017b88 */

void FUN_01017b88(int param_1,int *param_2)

{
  uint uVar1;
  byte bVar2;
  byte *pbVar3;
  uint uVar4;
  int *piVar5;
  
  if (*param_2 == 0) {
    return;
  }
  uVar4 = 0;
  pbVar3 = (byte *)(param_2 + 6);
  piVar5 = param_2 + 2;
  do {
    bVar2 = *pbVar3;
    *(uint *)(param_1 + 0x110) = uVar4 << 0xd | 0x2e;
    *(uint *)(param_1 + 0x114) = (uint)bVar2;
    *(int *)(param_1 + uVar4 * 0x80 + 0x248) = *piVar5;
    uVar1 = uVar4 & 0xff;
    uVar4 = uVar4 + 1;
    *(uint *)(param_1 + 0x134) =
         (param_2[1] ^ *(uint *)(param_1 + 0x134)) & 0x80008 >> uVar1 ^ *(uint *)(param_1 + 0x134);
    pbVar3 = pbVar3 + 1;
    piVar5 = piVar5 + 1;
  } while (uVar4 != 4);
  return;
}



/* @ 0x10194b0  FUN_010194b0 */

uint FUN_010194b0(uint param_1)

{
  uint uVar1;
  
  uVar1 = param_1 & 3;
  if (-1 < (int)-param_1) {
    uVar1 = -(-param_1 & 3);
  }
  if (uVar1 == 3) {
    param_1 = param_1 + 1;
  }
  return param_1;
}



/* @ 0x10194cc  FUN_010194cc */

undefined4 al_serdes_25g_reg_write(int *param_1,int param_2,undefined4 param_3,int param_4,byte param_5)

{
  int iVar1;
  
  switch(param_2) {
  case 0:
  case 1:
    param_4 = param_4 + (param_2 + 3) * 0x800;
    break;
  default:
    FUN_0100ceac(s__s__wrong_serdes_type__d_01033e98,s_al_serdes_25g_reg_write_010332b4);
    FUN_01010bfc(s__s__wrong_serdes_type__d_01033e98,s_al_serdes_25g_reg_write_010332b4,param_3);
    return 0xffffffff;
  case 4:
    param_4 = param_4 + 0xc00;
    break;
  case 8:
    break;
  }
  iVar1 = *param_1;
  *(int *)(iVar1 + 0x110) = param_4;
  *(uint *)(iVar1 + 0x114) = param_5 | 0xff00;
  return 0;
}



/* @ 0x1019578  FUN_01019578 */

undefined4 al_serdes_25g_reg_read(int *param_1,int param_2,undefined4 param_3,int param_4,undefined1 *param_5)

{
  int iVar1;
  
  iVar1 = *param_1;
  if (param_5 == (undefined1 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data__01033218,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_reg_read_0103329c,0x67)
    ;
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___data__01033218,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_reg_read_0103329c,0x67)
    ;
    FUN_01000454(0);
  }
  switch(param_2) {
  case 0:
  case 1:
    param_4 = param_4 + (param_2 + 3) * 0x800;
    break;
  default:
    FUN_0100ceac(s__s__wrong_serdes_type__d_01033e98,s_al_serdes_25g_reg_read_0103329c,param_3);
    FUN_01010bfc(s__s__wrong_serdes_type__d_01033e98,s_al_serdes_25g_reg_read_0103329c,param_3);
    return 0xffffffff;
  case 4:
    param_4 = param_4 + 0xc00;
    break;
  case 8:
    break;
  }
  *(int *)(iVar1 + 0x110) = param_4;
  *param_5 = (char)*(undefined4 *)(iVar1 + 0x114);
  return 0;
}



/* @ 0x10196b0  FUN_010196b0 */

void FUN_010196b0(undefined4 param_1,undefined4 param_2,undefined4 param_3,uint param_4,
                 sbyte param_5,byte param_6)

{
  int iVar1;
  uint uVar2;
  byte local_21 [5];
  
  iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,param_3,local_21);
  if (iVar1 == 0) {
    uVar2 = (uint)local_21[0];
    local_21[0] = (byte)(uVar2 & ~param_4) | (byte)((uint)param_6 << param_5);
    al_serdes_25g_reg_write(param_1,param_2,0,param_3,uVar2 & ~param_4 | (uint)param_6 << param_5 & 0xff);
  }
  return;
}



/* @ 0x1019940  FUN_01019940 */

void FUN_01019940(undefined4 param_1,undefined1 param_2)

{
  FUN_010196b0(param_1,4,0x109,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,3,param_2);
  FUN_010196b0(param_1,4,0x11a,1,0,param_2);
  FUN_010196b0(param_1,4,0x114,1,0,param_2);
  FUN_010196b0(param_1,4,0x111,1,0,param_2);
  return;
}



/* @ 0x1019be0  FUN_01019be0 */

void al_serdes_25g_single_iteration_dosc_set(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int iVar2;
  char cVar3;
  undefined4 uVar4;
  char local_21 [5];
  
  FUN_01019940(param_1,1);
  iVar1 = al_serdes_25g_reg_read(param_1,4,0,0x122,local_21);
  cVar3 = local_21[0];
  if (iVar1 != 0) {
    cVar3 = '\0';
  }
  FUN_010196b0(param_1,4,0x122,0xff,0,param_2);
  if (param_3 == 0) {
    uVar4 = 3;
  }
  else {
    uVar4 = AL_I2C_TAR_10BIT_ADDR_SHIFT;
  }
  iVar1 = 500;
  FUN_010196b0(param_1,4,0x133,0xf,0,uVar4);
  FUN_010196b0(param_1,4,0x133,0x10,4,1);
  while ((iVar2 = al_serdes_25g_reg_read(param_1,4,0,0x133,local_21), iVar2 != 0 || (-1 < local_21[0]))) {
    (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),1);
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) {
      FUN_0100ceac(s__s__timeout_waiting_for_pll_fcal_01034350,
                   s_al_serdes_25g_single_iteration_d_010334fc);
      FUN_01010bfc(s__s__timeout_waiting_for_pll_fcal_01034350,
                   s_al_serdes_25g_single_iteration_d_010334fc);
LAB_01019d00:
      FUN_01019940(param_1,0);
      FUN_010196b0(param_1,4,0x122,0xff,0,cVar3);
      return;
    }
  }
  FUN_010196b0(param_1,4,0x133,0x10,4,0);
  goto LAB_01019d00;
}



/* @ 0x1019f64  FUN_01019f64 */

undefined4 al_serdes_25g_tbus_read(int *param_1,uint param_2,int param_3,uint param_4,ushort *param_5)

{
  int iVar1;
  code *pcVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  ushort uVar6;
  byte local_29 [5];
  
  uVar6 = 0;
  if (3 < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___lane_<_AL_SRDS_NUM_LANES)_0103437c,
                 1,s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_tbus_read_0103347c,
                 0x61b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___lane_<_AL_SRDS_NUM_LANES)_0103437c,
                 1,s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_tbus_read_0103347c,
                 0x61b);
    FUN_01000454(0);
  }
  if (param_3 == 1) {
    uVar5 = 0xff20;
  }
  else {
    uVar5 = (param_2 + 2) * 0x20 & 0xff | 0xff00;
  }
  iVar3 = *param_1;
  pcVar2 = (code *)param_1[0x39];
  *(undefined4 *)(iVar3 + 0x110) = 0x1a0;
  *(uint *)(iVar3 + 0x114) = param_4 | 0xff00;
  *(undefined4 *)(iVar3 + 0x110) = 0x1a1;
  iVar1 = param_1[0x3a];
  *(uint *)(iVar3 + 0x114) = uVar5;
  (*pcVar2)(iVar1,1000);
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x110) = 0x1b0;
  uVar4 = *(undefined4 *)(iVar1 + 0x114);
  iVar1 = al_serdes_25g_reg_read(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0,0x1b1,local_29);
  if (iVar1 == 0) {
    uVar6 = (ushort)local_29[0];
  }
  if (iVar1 == 0) {
    uVar6 = uVar6 & 0xf;
  }
  *param_5 = (ushort)uVar4 & 0xff | uVar6 << 8;
  return 0;
}



/* @ 0x101a0ac  FUN_0101a0ac */

undefined4 al_serdes_25g_qsample_pll_lock_check(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  char cVar3;
  byte bVar4;
  byte local_19 [5];
  
  (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),100);
  iVar1 = al_serdes_25g_reg_read(param_1,4,0,0x106,local_19);
  if (iVar1 == 0) {
    cVar3 = 'd';
    bVar4 = local_19[0] & 1;
    do {
      iVar1 = al_serdes_25g_reg_read(param_1,4,0,0x106,local_19);
      if (iVar1 != 0) {
        uVar2 = 0x946;
        goto LAB_0101a16c;
      }
      if (bVar4 != (local_19[0] & 1)) {
        return 0;
      }
      (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),100);
      cVar3 = cVar3 + -1;
    } while (cVar3 != '\0');
    uVar2 = 1;
  }
  else {
    uVar2 = 0x939;
LAB_0101a16c:
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,
                 s_al_serdes_25g_qsample_pll_lock_c_01033524,uVar2);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,
                 s_al_serdes_25g_qsample_pll_lock_c_01033524,uVar2);
    FUN_01000454(0);
    uVar2 = 0;
  }
  return uVar2;
}



/* @ 0x101a1d4  FUN_0101a1d4 */

int al_serdes_25g_gcfsm2_read(int param_1,undefined4 param_2,undefined4 param_3,ushort *param_4)

{
  int iVar1;
  int iVar2;
  byte local_22;
  byte local_21 [5];
  
  iVar2 = 0x1e8481;
  FUN_010196b0(param_1,param_2,0x580,1,0,0);
  FUN_010196b0(param_1,param_2,0x581,7,0,0);
  al_serdes_25g_reg_write(param_1,param_2,0,0x582,param_3);
  FUN_010196b0(param_1,param_2,0x580,1,0,1);
  while ((iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x583,local_21), iVar1 != 0 ||
         ((local_21[0] & 1) == 0))) {
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) {
      iVar2 = -0x3e;
      FUN_0100ceac(s__s__TO_waiting_for_GCFSM2_req_to_010338f4,s_al_serdes_25g_gcfsm2_read_01033444,
                   param_3);
      FUN_01010bfc(s__s__TO_waiting_for_GCFSM2_req_to_010338f4,s_al_serdes_25g_gcfsm2_read_01033444,
                   param_3);
LAB_0101a310:
      FUN_010196b0(param_1,param_2,0x580,1,0,0);
      return iVar2;
    }
    (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),1);
  }
  iVar2 = al_serdes_25g_reg_read(param_1,param_2,0,0x590,&local_22);
  if (iVar2 != 0) {
    FUN_0100ceac(s__s__Error_on_reading_data_low_010338b4,s_al_serdes_25g_gcfsm2_read_01033444);
    FUN_01010bfc(s__s__Error_on_reading_data_low_010338b4,s_al_serdes_25g_gcfsm2_read_01033444);
    return iVar2;
  }
  iVar2 = al_serdes_25g_reg_read(param_1,param_2,0,0x591,local_21);
  if (iVar2 != 0) {
    FUN_0100ceac(s__s__Error_on_reading_data_high_010338d4,s_al_serdes_25g_gcfsm2_read_01033444);
    FUN_01010bfc(s__s__Error_on_reading_data_high_010338d4,s_al_serdes_25g_gcfsm2_read_01033444);
    return iVar2;
  }
  *param_4 = (ushort)local_22 | (local_21[0] & 0xf) << 8;
  iVar2 = 0;
  goto LAB_0101a310;
}



/* @ 0x101a3dc  FUN_0101a3dc */

void al_serdes_25g_rx_diag_info_get(undefined4 param_1,undefined4 param_2,byte *param_3)

{
  int iVar1;
  byte bVar2;
  uint uVar3;
  uint uVar4;
  byte local_25;
  byte local_24;
  byte local_23;
  ushort local_22 [3];
  
  al_serdes_25g_reg_read(param_1,param_2,0,0x559,&local_25);
  bVar2 = local_25 & 0x1f;
  if ((local_25 & 0x20) == 0) {
    bVar2 = -bVar2;
  }
  *param_3 = bVar2;
  al_serdes_25g_reg_read(param_1,param_2,0,0x560,&local_25);
  bVar2 = local_25 & 0x1f;
  if ((local_25 & 0x20) == 0) {
    bVar2 = -bVar2;
  }
  param_3[1] = bVar2;
  iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,5,local_22);
  if (iVar1 == 0) {
    uVar4 = 0;
    local_25 = (byte)local_22[0];
    bVar2 = (byte)local_22[0] & 0x1f;
    if ((local_22[0] & 0x20) == 0) {
      bVar2 = -bVar2;
    }
    param_3[2] = bVar2;
    while (iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,uVar4 & 0xff,local_22), iVar1 == 0) {
      local_25 = (byte)local_22[0];
      bVar2 = (byte)local_22[0] & 0x1f;
      switch(uVar4) {
      case 1:
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[4] = bVar2;
        break;
      case 2:
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[5] = bVar2;
        break;
      case 3:
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[6] = bVar2;
        break;
      case 4:
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[7] = bVar2;
LAB_0101a520:
        iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,6,local_22);
        if (iVar1 != 0) {
          FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_010340d0,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_010340d0,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          return;
        }
        local_25 = (byte)local_22[0];
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[8] = bVar2;
        iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,7,local_22);
        if (iVar1 != 0) {
          FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_01034118,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_01034118,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          return;
        }
        local_25 = (byte)local_22[0];
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[9] = bVar2;
        iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,local_22);
        if (iVar1 != 0) {
          FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_01034160,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_01034160,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          return;
        }
        local_25 = (byte)local_22[0];
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[10] = bVar2;
        iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,AL_I2C_INTR_MASK_STOP_DET_SHIFT,local_22);
        if (iVar1 != 0) {
          FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_010341a8,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_010341a8,
                       s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
          return;
        }
        local_25 = (byte)local_22[0];
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[0xb] = bVar2;
        al_serdes_25g_tbus_read(param_1,param_2,2,0x11,local_22);
        uVar4 = (local_22[0] & 0x3fff) >> 6;
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[0xc] = bVar2;
        uVar3 = uVar4 & 0x3f;
        bVar2 = (byte)uVar4 & 0x1f;
        if (uVar3 >> 5 == 0) {
          bVar2 = -bVar2;
        }
        local_25 = (byte)uVar3;
        param_3[0xd] = bVar2;
        al_serdes_25g_tbus_read(param_1,param_2,2,0x12,local_22);
        uVar4 = (local_22[0] & 0x3fff) >> 6;
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[0xe] = bVar2;
        uVar3 = uVar4 & 0x3f;
        bVar2 = (byte)uVar4 & 0x1f;
        if (uVar3 >> 5 == 0) {
          bVar2 = -bVar2;
        }
        local_25 = (byte)uVar3;
        param_3[0xf] = bVar2;
        al_serdes_25g_tbus_read(param_1,param_2,2,0x13,local_22);
        uVar4 = (local_22[0] & 0x3fff) >> 6;
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[0x10] = bVar2;
        uVar3 = uVar4 & 0x3f;
        bVar2 = (byte)uVar4 & 0x1f;
        if (uVar3 >> 5 == 0) {
          bVar2 = -bVar2;
        }
        local_25 = (byte)uVar3;
        param_3[0x11] = bVar2;
        al_serdes_25g_tbus_read(param_1,param_2,2,0x23,local_22);
        uVar4 = (local_22[0] & 0x3fff) >> 6;
        bVar2 = (byte)local_22[0] & 0x1f;
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[0x12] = bVar2;
        uVar3 = uVar4 & 0x3f;
        bVar2 = (byte)uVar4 & 0x1f;
        if (uVar3 >> 5 == 0) {
          bVar2 = -bVar2;
        }
        local_25 = (byte)uVar3;
        param_3[0x13] = bVar2;
        iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x198,&local_23);
        if (iVar1 == 0) {
          param_3[0x15] = local_23 & 0x7f;
        }
        iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x199,&local_23);
        if (iVar1 == 0) {
          param_3[0x14] = local_23 & 0x7f;
        }
        iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x19a,&local_23);
        if (iVar1 == 0) {
          param_3[0x16] = local_23 & 0x7f;
        }
        iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x18c,&local_23);
        if (iVar1 == 0) {
          param_3[0x17] = local_23 & 7;
        }
        al_serdes_25g_reg_read(param_1,param_2,0,0xa7,&local_24);
        al_serdes_25g_reg_read(param_1,param_2,0,0xa6,&local_25);
        *(ushort *)(param_3 + 0x1a) = CONCAT11(local_24,local_25) & 0x1ff;
        local_24 = local_24 & 1;
        iVar1 = al_serdes_25g_gcfsm2_read(param_1,param_2,0xf,local_22);
        if (iVar1 == 0) {
          param_3[0x18] = (byte)local_22[0];
          iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x205,&local_23);
          if (iVar1 == 0) {
            param_3[0x1c] = (byte)(((uint)local_23 << 0x1d) >> 0x1e);
          }
          al_serdes_25g_reg_read(param_1,param_2,0,0x1b,&local_25);
          param_3[0x1d] = (byte)(((uint)local_25 << 0x1c) >> 0x1f);
          param_3[0x1e] = (byte)(((uint)local_25 << 0x1b) >> 0x1f);
          iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x1c,&local_23);
          if (iVar1 == 0) {
            param_3[0x1f] = local_23 & 0x1f;
          }
          iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x203,&local_23);
          if (iVar1 != 0) {
            return;
          }
          param_3[0x20] = (byte)(((uint)local_23 << 0x1b) >> 0x1e);
          return;
        }
        FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_010341f0,
                     s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
        FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_010341f0,
                     s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
        return;
      default:
        if ((local_22[0] & 0x20) == 0) {
          bVar2 = -bVar2;
        }
        param_3[3] = bVar2;
        if (uVar4 == 4) goto LAB_0101a520;
      }
      uVar4 = uVar4 + 1;
    }
    FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_01034090,
                 s_al_serdes_25g_rx_diag_info_get_010334dc,uVar4,iVar1);
    FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_01034090,
                 s_al_serdes_25g_rx_diag_info_get_010334dc,uVar4,iVar1);
  }
  else {
    FUN_0100ceac(s__s__al_serdes_25g_gcfsm2_read_fa_0103404c,
                 s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
    FUN_01010bfc(s__s__al_serdes_25g_gcfsm2_read_fa_0103404c,
                 s_al_serdes_25g_rx_diag_info_get_010334dc,iVar1);
  }
  return;
}



/* @ 0x101af88  FUN_0101af88 */

byte al_serdes_25g_rx_valid(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  byte local_9 [5];
  
  iVar1 = al_serdes_25g_reg_read(param_1,param_2,0,0x38,local_9);
  if (iVar1 == 0) {
    return local_9[0] & 1;
  }
  FUN_0100ceac(s__s__Error_on_reading_RX_valid_01034330,s_al_serdes_25g_rx_valid_0103340c);
  FUN_01010bfc(s__s__Error_on_reading_RX_valid_01034330,s_al_serdes_25g_rx_valid_0103340c);
  return 0;
}



/* @ 0x101b1fc  FUN_0101b1fc */

undefined4 al_serdes_25g_mailbox_send_cmd(int *param_1,uint param_2,byte *param_3,int param_4)

{
  byte bVar1;
  undefined4 uVar2;
  byte *pbVar3;
  int iVar5;
  int iVar6;
  byte *pbVar4;
  
  iVar6 = *param_1;
  *(undefined4 *)(iVar6 + 0x110) = 0x202;
  if ((*(uint *)(iVar6 + 0x114) & 0xff) == 0) {
LAB_0101b270:
    if (param_4 != 0) {
      pbVar4 = param_3;
      do {
        pbVar3 = pbVar4 + 1;
        bVar1 = *pbVar4;
        *(byte **)(iVar6 + 0x110) = pbVar3 + (0x202 - (int)param_3);
        *(uint *)(iVar6 + 0x114) = bVar1 | 0xff00;
        pbVar4 = pbVar3;
      } while (pbVar3 != param_3 + param_4);
    }
    uVar2 = 0;
    *(undefined4 *)(iVar6 + 0x110) = 0x200;
    *(uint *)(iVar6 + 0x114) = param_2 | 0xff00;
  }
  else {
    iVar5 = 5000000;
    do {
      (*(code *)param_1[0x39])(param_1[0x3a],1);
      iVar6 = *param_1;
      *(undefined4 *)(iVar6 + 0x110) = 0x202;
      if ((*(uint *)(iVar6 + 0x114) & 0xff) == 0) goto LAB_0101b270;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
    FUN_0100ceac(s__s__timeout_occurred_waiting_to_C_01033e30,
                 s_al_serdes_25g_mailbox_send_cmd_010332cc);
    FUN_01010bfc(s__s__timeout_occurred_waiting_to_C_01033e30,
                 s_al_serdes_25g_mailbox_send_cmd_010332cc);
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



/* @ 0x101b300  FUN_0101b300 */

undefined4 al_serdes_25g_mailbox_recv_rsp(int *param_1,undefined4 param_2,int param_3,undefined1 *param_4)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x110) = 0x212;
  if ((*(uint *)(iVar1 + 0x114) & 0xff) == 1) {
LAB_0101b37c:
    al_serdes_25g_reg_read(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0,0x210,param_2);
    iVar1 = param_3;
    do {
      iVar3 = iVar1 + 1;
      al_serdes_25g_reg_read(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0,(0x213 - param_3) + iVar1 & 0xffff,
                   iVar1);
      iVar1 = iVar3;
    } while (iVar3 != param_3 + 0x10);
    iVar1 = *param_1;
    uVar2 = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x212;
    *(undefined4 *)(iVar1 + 0x114) = 0xff01;
    *param_4 = 0x10;
  }
  else {
    iVar1 = 5000000;
    do {
      (*(code *)param_1[0x39])(param_1[0x3a],1);
      iVar3 = *param_1;
      *(undefined4 *)(iVar3 + 0x110) = 0x212;
      if ((*(uint *)(iVar3 + 0x114) & 0xff) == 1) goto LAB_0101b37c;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
    FUN_0100ceac(s__s__timeout_occurred_waiting_to_R_01033e04,
                 s_al_serdes_25g_mailbox_recv_rsp_010332ec);
    FUN_01010bfc(s__s__timeout_occurred_waiting_to_R_01033e04,
                 s_al_serdes_25g_mailbox_recv_rsp_010332ec);
    uVar2 = 0xffffffff;
    *param_4 = 0;
  }
  return uVar2;
}



/* @ 0x101b438  FUN_0101b438 */

int al_serdes_25g_eye_diag_run(undefined4 param_1,byte param_2,int param_3,int param_4,uint param_5,int param_6,
                int param_7,uint param_8,undefined4 param_9,int param_10,int param_11,int param_12)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  int iVar15;
  bool bVar16;
  int local_44;
  byte local_3a;
  byte local_39;
  byte local_38 [4];
  undefined2 local_34;
  byte local_32;
  undefined1 local_31;
  
  uVar6 = (uint)(param_7 - param_6) / param_8;
  iVar8 = (uint)(param_4 - param_3) / param_5 + 1;
  iVar15 = uVar6 * iVar8 + iVar8;
  if (param_12 != iVar15 * 8) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___buf_size_____samples_left___si_01033758,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_eye_diag_run_01033390,
                 0x30e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___buf_size_____samples_left___si_01033758,1,
                 s____HAL__drivers_serdes_al_hal_se_01033dd8,s_al_serdes_25g_eye_diag_run_01033390,
                 0x30e);
    FUN_01000454(0);
  }
  FUN_01012ae0(param_11,0,param_12);
  local_32 = param_6 < 0;
  if ((bool)local_32) {
    param_6 = -param_6;
  }
  local_38[1] = (byte)param_3;
  if (param_7 < 0) {
    local_32 = local_32 | 2;
    param_7 = -param_7;
  }
  local_38[2] = (byte)param_4;
  local_38[3] = (byte)param_5;
  local_34 = CONCAT11((char)param_7,(char)param_6);
  local_31 = (undefined1)param_8;
  local_38[0] = param_2;
  local_44 = al_serdes_25g_mailbox_send_cmd(param_1,0x83,local_38,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
  if (local_44 == 0) {
    iVar2 = al_serdes_25g_mailbox_recv_rsp(param_1,&local_3a,local_38,&local_39);
    if ((iVar2 == 0) && (local_3a == 0)) {
      if (param_10 == 0) {
        iVar2 = LZCOUNT(param_9) + 0x20;
      }
      else {
        iVar2 = LZCOUNT(param_10);
      }
      uVar9 = 0x40U - iVar2 & 0xff;
      if (0x28 < uVar9) {
        FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sample_width_<__40)_010337f0,1,
                     s____HAL__drivers_serdes_al_hal_se_01033dd8,
                     s_al_serdes_25g_eye_diag_run_01033390,0x33e);
        FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sample_width_<__40)_010337f0,1,
                     s____HAL__drivers_serdes_al_hal_se_01033dd8,
                     s_al_serdes_25g_eye_diag_run_01033390,0x33e);
        FUN_01000454(0);
      }
      local_38[1] = (undefined1)param_10;
      local_38[2] = (byte)((uint)param_9 >> 0x18);
      local_38[3] = (byte)((uint)param_9 >> 0x10);
      local_34 = (ushort)param_9 << 8 | (ushort)param_9 >> 8;
      local_38[0] = param_2;
      local_44 = al_serdes_25g_mailbox_send_cmd(param_1,0x84,local_38,6);
      if (local_44 == 0) {
        iVar2 = al_serdes_25g_mailbox_recv_rsp(param_1,&local_3a,local_38,&local_39);
        if ((iVar2 == 0) && (local_3a == 0)) {
          al_serdes_25g_mailbox_send_cmd(param_1,0x82,local_38,0);
          uVar12 = 0;
          iVar2 = 0;
          uVar11 = uVar9;
          while (local_44 = 0, local_3a != 1) {
            while( true ) {
              iVar3 = al_serdes_25g_mailbox_recv_rsp(param_1,&local_3a,local_38,&local_39);
              if ((iVar3 != 0) || (2 < local_3a)) {
                FUN_0100ceac(s__s__command__d_return_failure__r_01033808,
                             s_al_serdes_25g_eye_diag_run_01033390,0x82,local_3a);
                FUN_01010bfc(s__s__command__d_return_failure__r_01033808,
                             s_al_serdes_25g_eye_diag_run_01033390,0x82,local_3a);
                return -0x3e;
              }
              iVar3 = (uint)local_39 << 3;
              if (local_39 != 0) break;
              if (iVar15 == 0) goto LAB_0101b778;
              if (local_3a == 1) goto LAB_0101b8c4;
            }
            uVar10 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
            iVar4 = 0;
            do {
              uVar14 = uVar10;
              if (uVar11 <= uVar10) {
                uVar14 = uVar11;
              }
              pbVar1 = local_38 + iVar4;
              uVar7 = uVar10 - 1;
              uVar10 = uVar10 - uVar14 & 0xff;
              puVar5 = (uint *)(param_11 + (uVar12 * iVar8 + iVar2) * 8);
              if (uVar10 == 0) {
                iVar4 = iVar4 + 1;
              }
              uVar13 = *puVar5;
              iVar3 = iVar3 - uVar14;
              uVar7 = ((2 << (uVar7 & 0xff)) - (1 << uVar10) & (uint)*pbVar1) >> uVar10;
              if (uVar10 == 0) {
                uVar10 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
              }
              uVar11 = uVar11 - uVar14 & 0xff;
              *puVar5 = uVar13 << uVar14 | uVar7;
              puVar5[1] = puVar5[1] << uVar14 | uVar13 << (uVar14 - 0x20 & 0xff) |
                          uVar13 >> (0x20 - uVar14 & 0xff);
              if (uVar11 == 0) {
                iVar15 = iVar15 + -1;
                bVar16 = uVar6 == uVar12;
                uVar12 = uVar12 + 1;
                uVar11 = uVar9;
                if (bVar16) {
                  iVar2 = iVar2 + 1;
                  uVar12 = 0;
                }
              }
              if (iVar15 == 0) {
LAB_0101b778:
                if (local_3a == 1) {
                  return 0;
                }
                iVar8 = al_serdes_25g_mailbox_recv_rsp(param_1,&local_3a,local_38,&local_39);
                if ((iVar8 == 0) && (local_3a != 0)) {
                  return 0;
                }
                FUN_0100ceac(s__s__Parsed_enough_samples_but_f__01033834,
                             s_al_serdes_25g_eye_diag_run_01033390);
                FUN_01010bfc(s__s__Parsed_enough_samples_but_f__01033834,
                             s_al_serdes_25g_eye_diag_run_01033390);
                return -5;
              }
            } while (iVar3 != 0);
          }
LAB_0101b8c4:
          if (iVar15 != 0) {
            FUN_0100ceac(s__s__Still_need_more_samples_but_f_01033870,
                         s_al_serdes_25g_eye_diag_run_01033390);
            local_44 = -5;
            FUN_01010bfc(s__s__Still_need_more_samples_but_f_01033870,
                         s_al_serdes_25g_eye_diag_run_01033390);
          }
        }
        else {
          FUN_0100ceac(s__s__Failed_to_send_command__d_to_010337b8,
                       s_al_serdes_25g_eye_diag_run_01033390,0x84);
          local_44 = -0x3e;
          FUN_01010bfc(s__s__Failed_to_send_command__d_to_010337b8,
                       s_al_serdes_25g_eye_diag_run_01033390,0x84,local_3a);
        }
      }
      else {
        FUN_0100ceac(s__s__Failed_to_send_command__d_to_0103378c,
                     s_al_serdes_25g_eye_diag_run_01033390,0x84);
        FUN_01010bfc(s__s__Failed_to_send_command__d_to_0103378c,
                     s_al_serdes_25g_eye_diag_run_01033390,0x84);
      }
    }
    else {
      FUN_0100ceac(s__s__Failed_to_send_command__d_to_010337b8,s_al_serdes_25g_eye_diag_run_01033390
                   ,0x83);
      local_44 = -0x3e;
      FUN_01010bfc(s__s__Failed_to_send_command__d_to_010337b8,s_al_serdes_25g_eye_diag_run_01033390
                   ,0x83,local_3a);
    }
  }
  else {
    FUN_0100ceac(s__s__Failed_to_send_command__d_to_0103378c,s_al_serdes_25g_eye_diag_run_01033390,
                 0x83);
    FUN_01010bfc(s__s__Failed_to_send_command__d_to_0103378c,s_al_serdes_25g_eye_diag_run_01033390,
                 0x83);
  }
  return local_44;
}



/* @ 0x101bbd0  FUN_0101bbd0 */

void FUN_0101bbd0(undefined4 param_1,uint param_2)

{
  if ((param_2 & 3) == 0) {
    param_2 = param_2 - 1;
  }
  else {
    param_2 = FUN_010194b0(param_2);
  }
  FUN_01019940(param_1,1);
  al_serdes_25g_single_iteration_dosc_set(param_1,param_2 & 0xff,0);
  FUN_01019940(param_1,0);
  al_serdes_25g_qsample_pll_lock_check(param_1);
  return;
}



/* @ 0x101bc38  FUN_0101bc38 */

void al_serdes_25g_tx_pll_wa_find_window_edge(undefined4 param_1,uint param_2,uint param_3,uint *param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  
  iVar3 = 0;
  while( true ) {
    while( true ) {
      uVar2 = param_2 + param_3;
      iVar1 = FUN_0101bbd0(param_1,uVar2);
      if (iVar1 == 0) break;
      if (iVar3 == 1) {
        if ((param_2 & 3) == 0) {
          iVar3 = ~param_3 + uVar2;
          FUN_0100ceac(s__s__dosc__l__d__r__d__hole_01034480,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103354c,param_2,iVar3);
        }
        else {
          iVar3 = FUN_010194b0(param_2);
          FUN_0100ceac(s__s__dosc__l__d__r__d__hole_01034480,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103354c,param_2,iVar3);
        }
        FUN_01010bfc(s__s__dosc__l__d__r__d__hole_01034480,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103354c,param_2,iVar3);
      }
      if (0x83 < uVar2 - 1) goto LAB_0101bd18;
      iVar3 = 0;
      param_2 = uVar2;
    }
    iVar1 = iVar3 + 1;
    if (0x83 < uVar2 - 1) break;
    iVar3 = 1;
    param_2 = uVar2;
    if (iVar1 != 1) {
      uVar2 = uVar2 + param_3 * -2;
LAB_0101bd18:
      *param_4 = uVar2;
      return;
    }
  }
  uVar2 = uVar2 - iVar1 * param_3;
  goto LAB_0101bd18;
}



/* @ 0x101bd68  FUN_0101bd68 */

undefined4 al_serdes_25g_tx_pll_wa(int *param_1,char *param_2)

{
  char cVar1;
  ushort uVar2;
  int iVar3;
  code *pcVar4;
  int *piVar5;
  int iVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  undefined *puVar10;
  int iVar11;
  undefined4 *puVar12;
  int iVar13;
  uint uVar14;
  uint uVar15;
  undefined4 uVar16;
  uint uVar17;
  undefined4 uVar18;
  uint uVar19;
  uint uVar20;
  uint uVar21;
  bool bVar22;
  bool bVar23;
  ulonglong uVar24;
  undefined8 uVar25;
  int local_6c;
  uint *local_68;
  int local_64;
  uint local_5c;
  undefined4 local_54;
  ushort local_4a;
  uint local_48;
  uint local_44;
  int local_40 [7];
  
  cVar1 = *param_2;
  if (cVar1 != '\x12' && cVar1 != '\x15') {
    FUN_0100ceac(s__s__Unsupported_group_mode__u_0103392c,
                 s_al_serdes_25g_group_cfg_mode_adv_01033640,cVar1);
    FUN_01010bfc(s__s__Unsupported_group_mode__u_0103392c,
                 s_al_serdes_25g_group_cfg_mode_adv_01033640,*param_2);
    return 0xffffffea;
  }
  local_6c = 3;
  local_54 = 0x901100;
LAB_0101bdb0:
  iVar11 = *param_1;
  iVar8 = 10000;
  iVar3 = param_1[0x3a];
  local_6c = local_6c + -1;
  uVar18 = local_54;
  if (cVar1 == '\x15') {
    uVar18 = 0x201100;
  }
  *(undefined4 *)(iVar11 + 0x118) = uVar18;
  pcVar4 = (code *)param_1[0x39];
  *(undefined4 *)(iVar11 + 300) = 0;
  (*pcVar4)(iVar3,AL_I2C_INTR_MASK_START_DET_SHIFT);
  pcVar4 = (code *)param_1[0x39];
  iVar3 = param_1[0x3a];
  *(uint *)(iVar11 + 300) = ~*(uint *)(iVar11 + 300) & 1 ^ *(uint *)(iVar11 + 300);
  (*pcVar4)(iVar3,AL_I2C_INTR_MASK_START_DET_SHIFT);
LAB_0101be30:
  if ((*(uint *)(iVar11 + 0x130) & 0x1a) != 0x1a) goto LAB_0101be1c;
  iVar3 = 0;
  FUN_010196b0(param_1,0,0x717,1,0,1);
  FUN_010196b0(param_1,0,0x717,2,1,0);
  FUN_010196b0(param_1,1,0x717,1,0,1);
  FUN_010196b0(param_1,1,0x717,2,1,0);
  if (cVar1 == '\x15') {
    uVar18 = 7;
    uVar16 = 0xf;
  }
  else {
    uVar18 = 0;
    uVar16 = 7;
  }
  do {
    FUN_010196b0(param_1,iVar3,0x71a,1,0,0);
    FUN_010196b0(param_1,iVar3,0x299,0xf,0,uVar18);
    FUN_010196b0(param_1,iVar3,0x299,0xf0,4,uVar16);
    bVar22 = iVar3 != 1;
    iVar3 = 1;
  } while (bVar22);
  if (cVar1 == '\x15') {
    FUN_010196b0(param_1,4,0x116,0xf,0,6);
    FUN_010196b0(param_1,4,0x116,0xf0,4,6);
  }
  else {
    FUN_010196b0(param_1,4,5,0xf,0,AL_I2C_INTR_MASK_STOP_DET_SHIFT);
  }
  iVar3 = 10000;
  (*(code *)param_1[0x39])(param_1[0x3a],1);
  *(uint *)(iVar11 + 300) = ~*(uint *)(iVar11 + 300) & 2 ^ *(uint *)(iVar11 + 300);
  while ((*(uint *)(iVar11 + 0x130) & 6) != 6) {
    (*(code *)param_1[0x39])(param_1[0x3a],1);
    iVar3 = iVar3 + -1;
    if (iVar3 == 0) goto LAB_0101cf60;
  }
  if (cVar1 != '\x15') {
    iVar3 = 0;
    do {
      FUN_010196b0(param_1,iVar3,0x18c,7,0,4);
      FUN_010196b0(param_1,iVar3,0xb0,0xf,0,6);
      al_serdes_25g_reg_read(param_1,iVar3,0,0x708,&local_44);
      uVar20 = local_44 & 0xcd | 0x28;
      local_44 = CONCAT31(local_44._1_3_,(char)uVar20);
      al_serdes_25g_reg_write(param_1,iVar3,0,0x708,uVar20);
      FUN_010196b0(param_1,iVar3,0x203,0x18,3,3);
      bVar22 = iVar3 != 1;
      iVar3 = 1;
    } while (bVar22);
  }
  local_68 = &local_44;
  iVar3 = 0;
  if (cVar1 == '\x15') {
    uVar18 = 0x71;
  }
  else {
    uVar18 = 0x44;
  }
  FUN_010196b0(param_1,0,0x202,0xff,0,uVar18);
  FUN_010196b0(param_1,1,0x202,0xff,0,uVar18);
  FUN_010196b0(param_1,0,0x1a0,2,1,1);
  FUN_010196b0(param_1,1,0x1a0,2,1,1);
  if (cVar1 == '\x15') {
    uVar20 = 0x12;
  }
  else {
    uVar20 = 0x1c;
  }
  do {
    (*(code *)param_1[2])(param_1,iVar3,0,400,local_68);
    uVar21 = uVar20 | local_44 & 0xe0;
    local_44 = CONCAT31(local_44._1_3_,(char)uVar21);
    (*(code *)param_1[3])(param_1,iVar3,0,400,uVar21);
    FUN_010196b0(param_1,iVar3,0x541,0x78,3,4);
    bVar22 = iVar3 != 1;
    iVar3 = 1;
  } while (bVar22);
  iVar3 = 0;
  do {
    FUN_010196b0(param_1,iVar3,0x14,3,0,3);
    FUN_010196b0(param_1,iVar3,0x10,0xf,0,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
    FUN_010196b0(param_1,iVar3,0x709,1,0,0);
    bVar22 = iVar3 != 1;
    iVar3 = 1;
  } while (bVar22);
  local_48 = 0;
  local_44 = 0;
  puVar12 = *(undefined4 **)(param_2 + 4);
  if (puVar12 != (undefined4 *)0x0) {
    *puVar12 = 0;
    puVar12[4] = 0;
    puVar12[5] = 0;
    puVar12[10] = 0;
  }
  if (param_1[0x36] == 0) {
    FUN_0100ceac(s_can_t_apply_tx_pll_WA_without_te_0103394c);
    FUN_01010bfc(s_can_t_apply_tx_pll_WA_without_te_0103394c);
  }
  else {
    al_serdes_25g_tbus_read(param_1,0,1,0x14,&local_4a);
    local_4a = local_4a ^ local_4a >> 8;
    local_4a = local_4a ^ local_4a >> 4;
    local_4a = local_4a ^ local_4a >> 2;
    local_4a = local_4a ^ local_4a >> 1;
    uVar20 = (uint)local_4a;
    if (uVar20 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
      uVar18 = 0xf;
      local_5c = AL_I2C_INTR_MASK_START_DET_SHIFT;
      local_4a = AL_I2C_INTR_MASK_START_DET_SHIFT;
LAB_0101c278:
      FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0x319,0xf,0,uVar18);
      FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0x318,1,0,1);
      FUN_01019940(param_1,1);
      al_serdes_25g_single_iteration_dosc_set(param_1,0x28,1);
      FUN_01019940(param_1,0);
      al_serdes_25g_qsample_pll_lock_check(param_1);
    }
    else {
      if (0xd < uVar20) {
        uVar18 = AL_I2C_INTR_MASK_GEN_CALL_SHIFT;
        local_5c = 0xd;
        local_4a = 0xd;
        goto LAB_0101c278;
      }
      FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0x319,0xf,0,
                   (local_4a ^ local_4a >> 1) & 0xff);
      FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0x318,1,0,1);
      local_5c = uVar20;
    }
    pcVar4 = (code *)param_1[0x36];
    if (puVar12 == (undefined4 *)0x0) {
      local_64 = (*pcVar4)();
    }
    else {
      *puVar12 = 1;
      puVar12[2] = uVar20;
      puVar12[3] = local_5c;
      local_64 = (*pcVar4)();
      puVar12[1] = local_64;
    }
    iVar8 = 0;
    iVar3 = 0xf;
    piVar5 = DAT_0101d184;
    while (iVar3 < local_64) {
      iVar8 = iVar8 + 1;
      if (iVar8 == 7) {
        FUN_0100ceac(s__s__error__invalid_temperature_r_01033d1c,s_al_serdes_25g_tx_pll_wa_0103359c,
                     local_64);
        FUN_01010bfc(s__s__error__invalid_temperature_r_01033d1c,s_al_serdes_25g_tx_pll_wa_0103359c,
                     local_64);
        goto LAB_0101c510;
      }
      iVar3 = *piVar5;
      piVar5 = piVar5 + 3;
    }
    al_serdes_25g_tbus_read(param_1,0,1,0x10,&local_4a);
    if ((local_4a & 3) == 0) {
      uVar2 = local_4a - 1;
    }
    else {
      uVar2 = FUN_010194b0();
    }
    uVar21 = (uint)uVar2;
    iVar3 = al_serdes_25g_qsample_pll_lock_check(param_1);
    if (puVar12 != (undefined4 *)0x0) {
      puVar12[4] = iVar3;
    }
    bVar22 = uVar21 == 0x83;
    if (uVar21 < 0x84) {
      bVar22 = iVar3 == 0;
    }
    if (bVar22) {
      uVar17 = uVar21 + 1;
      iVar3 = FUN_0101bbd0(param_1,uVar17);
      if (iVar3 != 0) goto LAB_0101cec8;
      if ((uVar2 & 0xfffe) != 0) goto LAB_0101ceb0;
    }
    else {
      uVar17 = uVar21;
      if ((uVar2 & 0xfffe) != 0 && iVar3 == 0) {
LAB_0101ceb0:
        iVar3 = FUN_0101bbd0(param_1,uVar21 - 1);
        uVar17 = uVar21 - 1;
      }
      if (iVar3 != 0) {
LAB_0101cec8:
        al_serdes_25g_tx_pll_wa_find_window_edge(param_1,uVar17,1,local_68);
        al_serdes_25g_tx_pll_wa_find_window_edge(param_1,uVar17,0xffffffff,&local_48);
        uVar21 = (int)(local_44 + local_48) / 2;
        uVar19 = local_48;
        uVar15 = local_44;
        if (puVar12 != (undefined4 *)0x0) {
          puVar12[5] = 1;
          if ((uVar17 & 3) == 0) {
LAB_0101cf1c:
            uVar24 = CONCAT44(local_48,uVar17 - 1);
            uVar15 = local_44;
          }
          else {
LAB_0101c5a0:
            uVar15 = local_44;
            uVar24 = FUN_010194b0(uVar17);
          }
          iVar3 = (int)(uVar24 >> 0x20);
          puVar12[6] = (int)uVar24;
          if ((uVar24 & 0x300000000) == 0) {
            uVar25 = CONCAT44(iVar3,iVar3 + -1);
          }
          else {
            uVar25 = FUN_010194b0(iVar3);
          }
          puVar12[7] = (int)uVar25;
          if ((uVar15 & 3) == 0) {
            uVar25 = CONCAT44((int)((ulonglong)uVar25 >> 0x20),uVar15 - 1);
          }
          else {
            uVar25 = FUN_010194b0(uVar15);
          }
          puVar12[8] = (int)uVar25;
          uVar19 = (uint)((ulonglong)uVar25 >> 0x20);
        }
LAB_0101c5dc:
        iVar3 = *(int *)(iVar8 * 0xc + 0x10344a4);
        if (*(int *)(iVar8 * 0xc + 0x10344a0) == 1) {
          uVar14 = uVar15 - iVar3;
          if (0 < (int)uVar14) goto LAB_0101ce00;
          if ((uVar14 & 3) == 0) {
            iVar3 = uVar14 - 1;
          }
          else {
            iVar3 = FUN_010194b0(uVar14);
          }
          FUN_0100ceac(s__s__dosc_target__l__d__r__d_lowe_010339e8,
                       s_al_serdes_25g_tx_pll_wa_0103359c,uVar14,iVar3,1);
          FUN_01010bfc(s__s__dosc_target__l__d__r__d_lowe_010339e8,
                       s_al_serdes_25g_tx_pll_wa_0103359c,uVar14,iVar3,1);
          if ((int)local_48 < 2) {
            uVar14 = 1;
            if (0 < (int)local_44) goto LAB_0101c7c4;
          }
          else {
            uVar14 = 1;
          }
LAB_0101ce18:
          iVar3 = FUN_010194b0(uVar14);
LAB_0101c680:
          if ((uVar21 & 3) == 0) {
            iVar8 = uVar21 - 1;
          }
          else {
            iVar8 = FUN_010194b0(uVar21);
          }
          FUN_0100ceac(s__s__stepped_outside_of_window_go_01033a6c,
                       s_al_serdes_25g_tx_pll_wa_0103359c,uVar14,iVar3,uVar21,iVar8);
          FUN_01010bfc(s__s__stepped_outside_of_window_go_01033a6c,
                       s_al_serdes_25g_tx_pll_wa_0103359c,uVar14,iVar3,uVar21,iVar8);
          iVar3 = FUN_0101bbd0(param_1,uVar21);
          uVar14 = uVar21;
          if (iVar3 != 0) goto LAB_0101c850;
LAB_0101c6f4:
          FUN_0100ceac(s__s__pll_is_unlocked__returning_t_01033b10,
                       s_al_serdes_25g_tx_pll_wa_0103359c);
          FUN_01010bfc(s__s__pll_is_unlocked__returning_t_01033b10,
                       s_al_serdes_25g_tx_pll_wa_0103359c);
          iVar3 = FUN_0101bbd0(param_1,uVar17);
          uVar14 = uVar17;
          if (puVar12 != (undefined4 *)0x0) {
LAB_0101c858:
            if ((uVar14 & 3) == 0) {
              uVar25 = CONCAT44(iVar3,uVar14 - 1);
            }
            else {
              uVar25 = FUN_010194b0(uVar14);
            }
            iVar3 = (int)((ulonglong)uVar25 >> 0x20);
            *(undefined8 *)(puVar12 + 9) = uVar25;
          }
          if (iVar3 == 0) {
            FUN_0100ceac(s__s__failed_to_lock_PLL_01033b44,s_al_serdes_25g_tx_pll_wa_0103359c);
            FUN_01010bfc(s__s__failed_to_lock_PLL_01033b44,s_al_serdes_25g_tx_pll_wa_0103359c);
            goto LAB_0101c510;
          }
        }
        else {
          uVar9 = iVar3 + uVar19;
          uVar14 = uVar9;
          if ((int)uVar9 < 0x85) {
LAB_0101ce00:
            if (((int)uVar14 < (int)uVar19) || ((int)uVar15 < (int)uVar14)) {
              if ((uVar14 & 3) != 0) goto LAB_0101ce18;
LAB_0101c67c:
              iVar3 = uVar14 - 1;
              goto LAB_0101c680;
            }
          }
          else {
            if ((uVar9 & 3) == 0) {
              iVar3 = uVar9 - 1;
            }
            else {
              iVar3 = FUN_010194b0(uVar9);
            }
            uVar14 = 0x84;
            FUN_0100ceac(s__s__dosc_target__l__d__r__d_is_h_01033a28,
                         s_al_serdes_25g_tx_pll_wa_0103359c,uVar9,iVar3,0x84);
            FUN_01010bfc(s__s__dosc_target__l__d__r__d_is_h_01033a28,
                         s_al_serdes_25g_tx_pll_wa_0103359c,uVar9,iVar3,0x84);
            if (0x84 < (int)local_48) {
              iVar3 = 0x83;
              goto LAB_0101c680;
            }
            uVar14 = 0x84;
            if ((int)local_44 < 0x84) goto LAB_0101c67c;
          }
LAB_0101c7c4:
          iVar3 = FUN_0101bbd0(param_1,uVar14);
          if (iVar3 == 0 && uVar14 != uVar21) {
            do {
              FUN_0100ceac(s__s__dosc_target_inside_the_windo_01033ab4,
                           s_al_serdes_25g_tx_pll_wa_0103359c);
              FUN_01010bfc(s__s__dosc_target_inside_the_windo_01033ab4,
                           s_al_serdes_25g_tx_pll_wa_0103359c);
              if ((int)uVar14 < (int)uVar21) {
                uVar14 = uVar14 + 1;
              }
              else {
                uVar14 = uVar14 - 1;
              }
              iVar3 = FUN_0101bbd0(param_1,uVar14);
            } while (iVar3 == 0 && uVar14 != uVar21);
          }
          if (iVar3 == 0) goto LAB_0101c6f4;
LAB_0101c850:
          if (puVar12 != (undefined4 *)0x0) goto LAB_0101c858;
        }
        FUN_0100ceac(s__s__temperature__d__swing_init___01033b5c,s_al_serdes_25g_tx_pll_wa_0103359c,
                     local_64,uVar20,local_5c);
        FUN_01010bfc(s__s__temperature__d__swing_init___01033b5c,s_al_serdes_25g_tx_pll_wa_0103359c,
                     local_64,uVar20,local_5c);
        if ((uVar17 & 3) == 0) {
          iVar3 = uVar17 - 1;
        }
        else {
          iVar3 = FUN_010194b0(uVar17);
        }
        if ((uVar14 & 3) == 0) {
          iVar8 = uVar14 - 1;
        }
        else {
          iVar8 = FUN_010194b0(uVar14);
        }
        if ((local_48 & 3) == 0) {
          iVar13 = local_48 - 1;
        }
        else {
          iVar13 = FUN_010194b0();
        }
        if ((local_44 & 3) == 0) {
          iVar6 = local_44 - 1;
        }
        else {
          iVar6 = FUN_010194b0();
        }
        FUN_0100ceac(s__s__dosc_init__d__dosc_target__d_01033b94,s_al_serdes_25g_tx_pll_wa_0103359c,
                     iVar3,iVar8,iVar13,iVar6);
        if ((local_48 & 3) == 0) {
          iVar13 = local_48 - 1;
        }
        else {
          iVar13 = FUN_010194b0();
        }
        if ((local_44 & 3) == 0) {
          iVar6 = local_44 - 1;
        }
        else {
          iVar6 = FUN_010194b0();
        }
        FUN_01010bfc(s__s__dosc_init__d__dosc_target__d_01033b94,s_al_serdes_25g_tx_pll_wa_0103359c,
                     iVar3,iVar8,iVar13,iVar6);
        iVar3 = 0;
        *(uint *)(iVar11 + 300) = ~*(uint *)(iVar11 + 300) & 0xc0 ^ *(uint *)(iVar11 + 300);
        *(undefined4 *)(iVar11 + 0x210) = 0x1110;
        *(undefined4 *)(iVar11 + 0x290) = 0x1410;
        while( true ) {
          iVar8 = 10000;
          while ((*(uint *)(iVar11 + iVar3 * 0x80 + 0x214) & 1) == 0) {
            (*(code *)param_1[0x39])(param_1[0x3a],1);
            iVar8 = iVar8 + -1;
            if (iVar8 == 0) {
              FUN_0100ceac(s__s__Got_timeout_waiting_for_LN_d_01033ce4,
                           s_al_serdes_25g_group_cfg_0103360c,iVar3);
              FUN_01010bfc(s__s__Got_timeout_waiting_for_LN_d_01033ce4,
                           s_al_serdes_25g_group_cfg_0103360c,iVar3);
              uVar18 = 0xffffffc2;
              goto LAB_0101c53c;
            }
          }
          if (iVar3 == 1) break;
          iVar3 = 1;
        }
        if (cVar1 != '\x15') {
          sVar7 = 0x41c;
          iVar3 = 0;
          do {
            al_serdes_25g_tbus_read(param_1,iVar3,2,7,local_68);
            uVar20 = ((local_44 & 0x3ff) >> 5) + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
            if (0x1f < uVar20) {
              uVar20 = 0x1f;
            }
            local_44 = CONCAT22(local_44._2_2_,(short)uVar20);
            FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,sVar7,0x1f,0,uVar20);
            FUN_010196b0(param_1,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,sVar7 + -1,1,0,1);
            bVar22 = iVar3 != 1;
            sVar7 = sVar7 + 0x100;
            iVar3 = 1;
          } while (bVar22);
        }
        iVar3 = 0;
        while( true ) {
          FUN_010196b0(param_1,iVar3,0x710,0x30,4,1);
          FUN_010196b0(param_1,iVar3,0x714,4,2,1);
          FUN_010196b0(param_1,iVar3,0x715,4,2,1);
          FUN_010196b0(param_1,iVar3,0x716,0x30,4,1);
          FUN_010196b0(param_1,iVar3,0x716,0xc0,6,1);
          FUN_010196b0(param_1,iVar3,0x717,0x10,4,1);
          FUN_010196b0(param_1,iVar3,0x717,0x20,5,1);
          FUN_010196b0(param_1,iVar3,0x717,0x40,6,1);
          FUN_010196b0(param_1,iVar3,0x717,0x80,7,1);
          FUN_010196b0(param_1,iVar3,0x718,0x30,4,2);
          FUN_010196b0(param_1,iVar3,0x718,0xc0,6,2);
          FUN_010196b0(param_1,iVar3,0x724,2,1,1);
          FUN_010196b0(param_1,iVar3,0x725,2,1,1);
          FUN_010196b0(param_1,iVar3,0x726,2,1,1);
          FUN_010196b0(param_1,iVar3,0x727,2,1,1);
          FUN_010196b0(param_1,iVar3,0x728,2,1,1);
          FUN_010196b0(param_1,iVar3,0x758,1,0,1);
          if (iVar3 == 1) break;
          iVar3 = 1;
        }
        (*(code *)param_1[0x37])(param_1,local_68);
        if (local_44 != 0) {
          FUN_0100ceac(s__s__Got_FW_error__TOP_error_code_01033bf8,
                       s_al_serdes_25g_fw_init_status_gro_010335b4);
          FUN_01010bfc(s__s__Got_FW_error__TOP_error_code_01033bf8,
                       s_al_serdes_25g_fw_init_status_gro_010335b4,local_44);
        }
        if (local_40[0] != 0 || local_40[1] != 0) {
          FUN_0100ceac(s__s__Got_FW_error__CM_error_code___01033c20,
                       s_al_serdes_25g_fw_init_status_gro_010335b4);
          FUN_01010bfc(s__s__Got_FW_error__CM_error_code___01033c20,
                       s_al_serdes_25g_fw_init_status_gro_010335b4,local_40[0]);
          if (local_40[1] != 0) {
            FUN_0100ceac(s__s__FW_CM_error_is_critical___d__01033c44,
                         s_al_serdes_25g_fw_init_status_gro_010335b4);
            FUN_01010bfc(s__s__FW_CM_error_is_critical___d__01033c44,
                         s_al_serdes_25g_fw_init_status_gro_010335b4,local_40[1]);
            uVar18 = 0xfffffffb;
            goto LAB_0101c53c;
          }
        }
        iVar3 = 0;
        piVar5 = local_40 + 2;
        do {
          if ((*piVar5 != 0) || (piVar5[2] != 0)) {
            FUN_0100ceac(s__s__Got_FW_error__lane__d_error_c_01033c68,
                         s_al_serdes_25g_fw_init_status_lan_010335e4,iVar3);
            FUN_01010bfc(s__s__Got_FW_error__lane__d_error_c_01033c68,
                         s_al_serdes_25g_fw_init_status_lan_010335e4,iVar3,*piVar5);
            if (piVar5[2] != 0) {
              FUN_0100ceac(s__s__FW_lane__d_error_is_critical_01033c94,
                           s_al_serdes_25g_fw_init_status_lan_010335e4,iVar3);
              FUN_01010bfc(s__s__FW_lane__d_error_is_critical_01033c94,
                           s_al_serdes_25g_fw_init_status_lan_010335e4,iVar3,local_40[iVar3 + 4]);
              uVar18 = 0xfffffffb;
              goto LAB_0101c53c;
            }
          }
          if (iVar3 == 1) {
            puVar10 = &DAT_01033928;
            if (cVar1 == '\x15') {
              puVar10 = &DAT_01033924;
            }
            FUN_0100ceac(s_Serdes25G_power_up_OK___s_mode__01033cc0,puVar10);
            FUN_01010bfc(s_Serdes25G_power_up_OK___s_mode__01033cc0,puVar10);
            return 0;
          }
          iVar3 = 1;
          piVar5 = piVar5 + 1;
        } while( true );
      }
    }
    if ((uVar2 & 3) == 0) {
      iVar3 = uVar21 - 1;
    }
    else {
      iVar3 = FUN_010194b0(uVar21);
    }
    FUN_0100ceac(s__s__init__l__d__r__d_outside_win_01033978,
                 s_al_serdes_25g_tx_pll_wa_find_win_01033578,uVar21,iVar3);
    FUN_01010bfc(s__s__init__l__d__r__d_outside_win_01033978,
                 s_al_serdes_25g_tx_pll_wa_find_win_01033578,uVar21,iVar3);
    uVar17 = uVar21;
    do {
      if ((int)uVar21 < 0x85) {
        uVar19 = uVar21 + 4;
        uVar15 = uVar21;
        do {
          iVar3 = FUN_0101bbd0(param_1,uVar15);
          if (iVar3 != 0) {
            if (uVar15 != 0) {
              local_48 = uVar15;
              al_serdes_25g_tx_pll_wa_find_window_edge(param_1,uVar15,1,local_68);
              goto LAB_0101c570;
            }
            break;
          }
          uVar14 = uVar15 + 1;
          bVar23 = SBORROW4(uVar14,0x84);
          iVar3 = uVar15 - 0x83;
          bVar22 = uVar14 == 0x84;
          if ((int)uVar14 < 0x85) {
            bVar23 = SBORROW4(uVar14,uVar19);
            iVar3 = uVar14 - uVar19;
            bVar22 = uVar14 == uVar19;
          }
          uVar15 = uVar14;
        } while (bVar22 || iVar3 < 0 != bVar23);
      }
      if (0 < (int)uVar17) {
        uVar15 = uVar17;
        do {
          iVar3 = FUN_0101bbd0(param_1,uVar15);
          if (iVar3 != 0) {
            local_44 = uVar15;
            al_serdes_25g_tx_pll_wa_find_window_edge(param_1,uVar15,0xffffffff,&local_48);
LAB_0101c570:
            uVar17 = (int)(local_48 + local_44) / 2;
            uVar19 = local_48;
            uVar21 = uVar17;
            uVar15 = local_44;
            if (puVar12 != (undefined4 *)0x0) {
              puVar12[5] = 1;
              if ((uVar17 & 3) == 0) goto LAB_0101cf1c;
              goto LAB_0101c5a0;
            }
            goto LAB_0101c5dc;
          }
          uVar15 = uVar15 - 1;
        } while ((int)(uVar17 - 4) <= (int)uVar15 && 0 < (int)uVar15);
      }
      uVar21 = uVar21 + 5;
      uVar17 = uVar17 - 5;
    } while (0 < (int)uVar17 || (int)uVar21 < 0x85);
    if (puVar12 != (undefined4 *)0x0) {
      puVar12[5] = 0;
    }
    FUN_0100ceac(s__s__error__no_valid_dosc_window_a_010339bc,s_al_serdes_25g_tx_pll_wa_0103359c);
    FUN_01010bfc(s__s__error__no_valid_dosc_window_a_010339bc,s_al_serdes_25g_tx_pll_wa_0103359c);
  }
LAB_0101c510:
  FUN_0100ceac(s__s__failed_to_apply_tx_pll_wa_01033bd8,s_al_serdes_25g_group_cfg_0103360c);
  FUN_01010bfc(s__s__failed_to_apply_tx_pll_wa_01033bd8,s_al_serdes_25g_group_cfg_0103360c);
  uVar18 = 0xfffffffb;
  goto LAB_0101c53c;
LAB_0101be1c:
  (*(code *)param_1[0x39])(param_1[0x3a],1);
  iVar8 = iVar8 + -1;
  if (iVar8 == 0) goto LAB_0101cf24;
  goto LAB_0101be30;
LAB_0101cf24:
  FUN_0100ceac(s__s__Got_timeout_waiting_for_serd_01033d90,s_al_serdes_25g_group_cfg_0103360c);
  FUN_01010bfc(s__s__Got_timeout_waiting_for_serd_01033d90,s_al_serdes_25g_group_cfg_0103360c);
  uVar18 = 0xffffffc2;
  goto LAB_0101c53c;
LAB_0101cf60:
  FUN_0100ceac(s__s__Got_timeout_waiting_for_serd_01033d54,s_al_serdes_25g_group_cfg_0103360c);
  FUN_01010bfc(s__s__Got_timeout_waiting_for_serd_01033d54,s_al_serdes_25g_group_cfg_0103360c);
  uVar18 = 0xffffffc2;
LAB_0101c53c:
  if (local_6c == 0) {
    return uVar18;
  }
  cVar1 = *param_2;
  goto LAB_0101bdb0;
}



/* @ 0x101d1d8  FUN_0101d1d8 */

undefined4
FUN_0101d1d8(int param_1,undefined4 param_2,uint param_3,undefined4 param_4,byte *param_5)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  byte bVar4;
  undefined4 uVar5;
  byte local_29 [5];
  
  al_serdes_25g_reg_write(param_1,param_2,0,0x221,(param_3 & 0xf) << 4 | 1);
  al_serdes_25g_reg_write(param_1,param_2,0,0x223,0);
  iVar3 = 0x1e8481;
  FUN_010196b0(param_1,param_2,0x220,1,0,1);
  while ((iVar2 = al_serdes_25g_reg_read(param_1,param_2,0,0x22b,local_29), iVar2 != 0 ||
         ((local_29[0] & 2) == 0))) {
    iVar3 = iVar3 + -1;
    if (iVar3 == 0) {
      FUN_0100ceac(s__s__TO_waiting_for_RX_LEQ_FSM_re_010342e0,
                   s_al_serdes_25g_rx_leq_fsm_op_01033460,1,param_3,0);
      FUN_01010bfc(s__s__TO_waiting_for_RX_LEQ_FSM_re_010342e0,
                   s_al_serdes_25g_rx_leq_fsm_op_01033460,1,param_3,0);
      return 0xffffffc2;
    }
    (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),1);
  }
  al_serdes_25g_reg_read(param_1,param_2,0,0x224,param_5);
  bVar1 = *param_5;
  uVar5 = 0;
  if (((bVar1 | bVar1 >> 3) & 7) != 0) {
    bVar4 = bVar1 >> 3 & 7;
    FUN_0100ceac(s__s__error_in_RX_LEQ_FSM_req__err_0103429c,s_al_serdes_25g_rx_leq_fsm_op_01033460,
                 bVar1 & 7,bVar4);
    uVar5 = 0xfffffffb;
    FUN_01010bfc(s__s__error_in_RX_LEQ_FSM_req__err_0103429c,s_al_serdes_25g_rx_leq_fsm_op_01033460,
                 bVar1 & 7,bVar4);
  }
  al_serdes_25g_reg_read(param_1,param_2,0,0x227,param_4);
  FUN_010196b0(param_1,param_2,0x220,1,0,0);
  return uVar5;
}



/* @ 0x101d8bc  FUN_0101d8bc */

void FUN_0101d8bc(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8024;
  *(undefined4 *)(iVar1 + 0x114) = 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8025;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf8;
  *(undefined4 *)(iVar1 + 0x110) = 0x8020;
  *(undefined4 *)(iVar1 + 0x114) = 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x8021;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfe;
  *(undefined4 *)(iVar1 + 0x110) = 0x8021;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xc1 | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x801e;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8022;
  *(undefined4 *)(iVar1 + 0x114) = 0xf0;
  *(undefined4 *)(iVar1 + 0x110) = 0x8023;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfe;
  *(undefined4 *)(iVar1 + 0x110) = 0x8023;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xc1 | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x801f;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  *(undefined4 *)(iVar1 + 0x110) = 0x826f;
  *(undefined4 *)(iVar1 + 0x114) = 0;
  *(undefined4 *)(iVar1 + 0x110) = 0x8270;
  *(undefined4 *)(iVar1 + 0x114) = 100;
  *(undefined4 *)(iVar1 + 0x110) = 0x8273;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 3;
  *(undefined4 *)(iVar1 + 0x110) = 0x8274;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8275;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf0 | 3;
  *(undefined4 *)(iVar1 + 0x110) = 0x8276;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf0 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8096;
  *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
  *(undefined4 *)(iVar1 + 0x110) = 0x8097;
  *(undefined4 *)(iVar1 + 0x114) = 0xcc;
  return;
}



/* @ 0x101da5c  FUN_0101da5c */

undefined4 al_serdes_hssp_group_ictl_pma_val_set(int *param_1,uint *param_2,int param_3,int param_4,char param_5)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = *param_1;
  *param_2 = 0;
  if (param_5 == '\x02') {
    *param_2 = 3;
  }
  else if (param_5 == '\x04') {
    *param_2 = 4;
  }
  else if (param_5 != '\x01') {
    FUN_0100ceac(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_5);
    FUN_01010bfc(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_5);
    return 0xffffffea;
  }
  if (param_3 == 1) {
    *param_2 = *param_2 | 0x20;
  }
  else if (param_3 != 0) {
    if (param_3 != 2) {
      FUN_0100ceac(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_3);
      FUN_01010bfc(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_3);
      return 0xffffffea;
    }
    *param_2 = *param_2 | 0x30;
  }
  if (param_4 == 1) {
    uVar2 = *param_2 | 0x200;
    *param_2 = uVar2;
  }
  else if (param_4 == 0) {
    uVar2 = *param_2;
  }
  else {
    if (param_4 != 4) {
      FUN_0100ceac(s__s__invalid_L2R_clock_source___d_01034654,
                   s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_4);
      FUN_01010bfc(s__s__invalid_L2R_clock_source___d_01034654,
                   s_al_serdes_hssp_group_ictl_pma_va_010344f0,param_4);
      return 0xffffffea;
    }
    uVar2 = *param_2 | 0x300;
    *param_2 = uVar2;
  }
  *(uint *)(iVar1 + 0x128) = uVar2;
  return 0;
}



/* @ 0x101dc08  FUN_0101dc08 */

undefined4
FUN_0101dc08(int *param_1,uint *param_2,undefined4 param_3,int param_4,undefined1 param_5)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = *param_1;
  *param_2 = 0;
  switch(param_5) {
  case 1:
    break;
  case 2:
  case 3:
    *param_2 = 4;
    break;
  case 4:
    *param_2 = 3;
    break;
  default:
    FUN_0100ceac(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_hssp_group_ictl_pma_va_01034518,param_5);
    FUN_01010bfc(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_hssp_group_ictl_pma_va_01034518,param_5);
    return 0xffffffea;
  }
  if (param_4 == 1) {
    *param_2 = *param_2 | 0x20;
  }
  else if (param_4 != 0) {
    if (param_4 != 4) {
      FUN_0100ceac(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_hssp_group_ictl_pma_va_01034518,param_4);
      FUN_01010bfc(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_hssp_group_ictl_pma_va_01034518,param_4);
      return 0xffffffea;
    }
    *param_2 = *param_2 | 0x30;
  }
  switch(param_3) {
  case 0:
    uVar1 = *param_2;
    break;
  case 1:
    uVar1 = *param_2 | 0x200;
    *param_2 = uVar1;
    break;
  case 2:
  case 3:
    uVar1 = *param_2 | 0x300;
    *param_2 = uVar1;
    break;
  default:
    FUN_0100ceac(s__s__invalid_L2R_clock_source___d_01034654,
                 s_al_serdes_hssp_group_ictl_pma_va_01034518,param_3);
    FUN_01010bfc(s__s__invalid_L2R_clock_source___d_01034654,
                 s_al_serdes_hssp_group_ictl_pma_va_01034518,param_3);
    return 0xffffffea;
  }
  *(uint *)(iVar2 + 0x128) = uVar1;
  return 0;
}



/* @ 0x101dda4  FUN_0101dda4 */

void FUN_0101dda4(int *param_1,int param_2,int param_3)

{
  int *piVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  
  piVar1 = (int *)(param_2 + -4);
  piVar2 = (int *)(param_3 + -4);
  uVar3 = 0;
  do {
    piVar1 = piVar1 + 1;
    if (*piVar1 == 1) {
      iVar4 = *param_1;
      *(uint *)(iVar4 + 0x110) = uVar3 | 0x2e;
      *(uint *)(iVar4 + 0x114) = *(uint *)(iVar4 + 0x114) & 0xfd | 2;
    }
    piVar2 = piVar2 + 1;
    if (*piVar2 == 1) {
      iVar4 = *param_1;
      *(uint *)(iVar4 + 0x110) = uVar3 | 0x2e;
      *(uint *)(iVar4 + 0x114) = *(uint *)(iVar4 + 0x114) & 0xfe | 1;
    }
    uVar3 = uVar3 + 0x2000;
  } while (uVar3 != 0x8000);
  return;
}



/* @ 0x101de14  FUN_0101de14 */

void FUN_0101de14(int *param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = *param_1;
  if (param_2 == 0) {
    uVar1 = 0x20;
  }
  else {
    uVar1 = 0;
  }
  *(undefined4 *)(iVar2 + 0x130) = 1;
  *(undefined4 *)(iVar2 + 0x134) = 0x1000;
  *(undefined4 *)(iVar2 + 0x140) = uVar1;
  return;
}



/* @ 0x101de3c  FUN_0101de3c */

undefined4 FUN_0101de3c(int *param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int *piVar4;
  uint uVar5;
  int iVar6;
  
  piVar4 = (int *)(param_2 + -4);
  uVar5 = 0;
  iVar6 = *param_1;
  do {
    piVar4 = piVar4 + 1;
    uVar2 = uVar5 & 0xff;
    iVar1 = uVar5 * 0x80;
    uVar3 = uVar5 & 0xff;
    uVar5 = uVar5 + 1;
    if (*piVar4 == 0) {
      *(undefined4 *)(iVar6 + iVar1 + 0x248) = 0;
      *(uint *)(iVar6 + 0x134) =
           *(uint *)(iVar6 + 0x134) & ~(AL_I2C_INTR_MASK_ACTIVITY_SHIFT >> uVar2 | 0x80000 >> uVar3)
      ;
    }
  } while (uVar5 != 4);
  return 0;
}



/* @ 0x101de9c  FUN_0101de9c */

undefined4 FUN_0101de9c(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x124) = 0;
  *(undefined4 *)(iVar1 + 0x130) = 0;
  *(undefined4 *)(iVar1 + 0x134) = 0;
  *(undefined4 *)(iVar1 + 0x248) = 0;
  *(undefined4 *)(iVar1 + 0x2c8) = 0;
  *(undefined4 *)(iVar1 + 0x348) = 0;
  *(undefined4 *)(iVar1 + 0x3c8) = 0;
  return 0;
}



/* @ 0x101dec8  FUN_0101dec8 */

undefined4 FUN_0101dec8(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x124) = 0;
  *(undefined4 *)(iVar1 + 0x130) = 0;
  *(undefined4 *)(iVar1 + 0x134) = 0x1000;
  *(undefined4 *)(iVar1 + 0x248) = 0;
  *(undefined4 *)(iVar1 + 0x2c8) = 0;
  *(undefined4 *)(iVar1 + 0x348) = 0;
  *(undefined4 *)(iVar1 + 0x3c8) = 0;
  return 0;
}



/* @ 0x101def8  FUN_0101def8 */

undefined4 al_serdes_hssp_group_cfg_sata_mode(int *param_1,int param_2,uint param_3,int param_4,int param_5)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = *param_1;
  *(undefined4 *)(iVar3 + 0x134) = 0;
  *(undefined4 *)(iVar3 + 0x224) = 0x30990034;
  *(undefined4 *)(iVar3 + 0x2a4) = 0x30990034;
  *(undefined4 *)(iVar3 + 0x324) = 0x30990034;
  *(undefined4 *)(iVar3 + 0x3a4) = 0x30990034;
  *(undefined4 *)(iVar3 + 0x218) = 0x300;
  *(undefined4 *)(iVar3 + 0x298) = 0x300;
  *(undefined4 *)(iVar3 + 0x318) = 0x300;
  *(undefined4 *)(iVar3 + 0x398) = 0x300;
  *(undefined4 *)(iVar3 + 0x244) = 0x10;
  *(undefined4 *)(iVar3 + 0x2c4) = 0x10;
  *(undefined4 *)(iVar3 + 0x344) = 0x10;
  *(undefined4 *)(iVar3 + 0x3c4) = 0x10;
  *(undefined4 *)(iVar3 + 0x134) = 0x1000;
  udelay(1);
  *(undefined4 *)(iVar3 + 0x134) = 0;
  udelay(1);
  *(undefined4 *)(iVar3 + 0x134) = 0x1000;
  udelay(1);
  *(undefined4 *)(iVar3 + 0x130) = 1;
  *(undefined4 *)(iVar3 + 0x130) = 1;
  *(undefined4 *)(iVar3 + 0x248) = 3;
  *(undefined4 *)(iVar3 + 0x2c8) = 3;
  *(undefined4 *)(iVar3 + 0x348) = 3;
  *(undefined4 *)(iVar3 + 0x3c8) = 3;
  if (param_2 == 0) {
    if (param_4 == 1) {
      iVar2 = *param_1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe065;
      *(undefined4 *)(iVar2 + 0x114) = 0xc9;
      *(undefined4 *)(iVar2 + 0x110) = 0xe066;
      *(undefined4 *)(iVar2 + 0x114) = 0xc9;
      *(undefined4 *)(iVar2 + 0x110) = 0xe067;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0xe068;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0xe069;
      *(undefined4 *)(iVar2 + 0x114) = 0x18;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar2 + 0x114) = 0x18;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar2 + 0x114) = 0x22;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8065;
      *(undefined4 *)(iVar2 + 0x114) = 0xaa;
      *(undefined4 *)(iVar2 + 0x110) = 0x8066;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8067;
      *(undefined4 *)(iVar2 + 0x114) = 0x45;
      *(undefined4 *)(iVar2 + 0x110) = 0x8068;
      *(undefined4 *)(iVar2 + 0x114) = 0xc9;
      *(undefined4 *)(iVar2 + 0x110) = 0x8069;
      *(undefined4 *)(iVar2 + 0x114) = 0xc9;
      *(undefined4 *)(iVar2 + 0x110) = 0x806a;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0x806b;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0x806c;
      *(undefined4 *)(iVar2 + 0x114) = 0x18;
      *(undefined4 *)(iVar2 + 0x110) = 0x806d;
      *(undefined4 *)(iVar2 + 0x114) = 0x18;
      *(undefined4 *)(iVar2 + 0x110) = 0x806e;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x806f;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8070;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8071;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8072;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8073;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8074;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8075;
      *(undefined4 *)(iVar2 + 0x114) = 0xcf;
      *(undefined4 *)(iVar2 + 0x110) = 0x8076;
      *(undefined4 *)(iVar2 + 0x114) = 0xf7;
      *(undefined4 *)(iVar2 + 0x110) = 0x8077;
      *(undefined4 *)(iVar2 + 0x114) = 0xe1;
      *(undefined4 *)(iVar2 + 0x110) = 0x8078;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8079;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x807a;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x807b;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807c;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807d;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807e;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807f;
      *(undefined4 *)(iVar2 + 0x114) = 0xe3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8080;
      *(undefined4 *)(iVar2 + 0x114) = 0xe7;
      *(undefined4 *)(iVar2 + 0x110) = 0x8081;
      *(undefined4 *)(iVar2 + 0x114) = 0xdb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8082;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8083;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x8084;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x8085;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8086;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8087;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8088;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8089;
      *(undefined4 *)(iVar2 + 0x114) = 0xe3;
      *(undefined4 *)(iVar2 + 0x110) = 0x808a;
      *(undefined4 *)(iVar2 + 0x114) = 0xe7;
      *(undefined4 *)(iVar2 + 0x110) = 0x808b;
      *(undefined4 *)(iVar2 + 0x114) = 0xdb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808c;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x808d;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x808e;
      *(undefined4 *)(iVar2 + 0x114) = 0xfd;
      *(undefined4 *)(iVar2 + 0x110) = 0x808f;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8090;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8091;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8092;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8093;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8094;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8095;
      *(undefined4 *)(iVar2 + 0x114) = 0x3f;
      *(undefined4 *)(iVar2 + 0x110) = 0x8096;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8097;
      *(undefined4 *)(iVar2 + 0x114) = 0x32;
      *(undefined4 *)(iVar2 + 0x110) = 0x8098;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8099;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x809a;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0x809b;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x809c;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x809d;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x809e;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809f;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar2 + 0x114) = 0x28;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 7;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x2007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x4007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x6007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x800d;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8030;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8031;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8037;
      *(undefined4 *)(iVar2 + 0x114) = 0xb0;
      *(undefined4 *)(iVar2 + 0x110) = 0x805d;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0xe029;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8162;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8163;
      *(undefined4 *)(iVar2 + 0x114) = 0x3a;
      *(undefined4 *)(iVar2 + 0x110) = 0x8164;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      if (param_5 != 1) goto LAB_0101e548;
      *(undefined4 *)(iVar2 + 0x110) = 0x8032;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8033;
      *(undefined4 *)(iVar2 + 0x114) = 0x50;
      *(undefined4 *)(iVar2 + 0x110) = 0x8034;
      *(undefined4 *)(iVar2 + 0x114) = 0xf0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8035;
      *(undefined4 *)(iVar2 + 0x114) = 0x15;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      uVar1 = *(uint *)(iVar2 + 0x114);
    }
    else {
      if (param_4 != 2) {
        FUN_0100ceac(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_sata_mo_01034544,param_4);
        FUN_01010bfc(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_sata_mo_01034544,param_4);
        return 0xffffffea;
      }
      iVar2 = *param_1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe065;
      *(undefined4 *)(iVar2 + 0x114) = 0xb7;
      *(undefined4 *)(iVar2 + 0x110) = 0xe066;
      *(undefined4 *)(iVar2 + 0x114) = 0xb7;
      *(undefined4 *)(iVar2 + 0x110) = 0xe067;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe068;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe069;
      *(undefined4 *)(iVar2 + 0x114) = 0x12;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar2 + 0x114) = 0x12;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar2 + 0x114) = 0x22;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8065;
      *(undefined4 *)(iVar2 + 0x114) = 0xaa;
      *(undefined4 *)(iVar2 + 0x110) = 0x8066;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8067;
      *(undefined4 *)(iVar2 + 0x114) = 0x57;
      *(undefined4 *)(iVar2 + 0x110) = 0x8068;
      *(undefined4 *)(iVar2 + 0x114) = 0xb7;
      *(undefined4 *)(iVar2 + 0x110) = 0x8069;
      *(undefined4 *)(iVar2 + 0x114) = 0xb7;
      *(undefined4 *)(iVar2 + 0x110) = 0x806a;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x806b;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x806c;
      *(undefined4 *)(iVar2 + 0x114) = 0x12;
      *(undefined4 *)(iVar2 + 0x110) = 0x806d;
      *(undefined4 *)(iVar2 + 0x114) = 0x12;
      *(undefined4 *)(iVar2 + 0x110) = 0x806e;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x806f;
      *(undefined4 *)(iVar2 + 0x114) = 0x17;
      *(undefined4 *)(iVar2 + 0x110) = 0x8070;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8071;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8072;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8073;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8074;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8075;
      *(undefined4 *)(iVar2 + 0x114) = 0xc3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8076;
      *(undefined4 *)(iVar2 + 0x114) = 0xf5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8077;
      *(undefined4 *)(iVar2 + 0x114) = 0xd9;
      *(undefined4 *)(iVar2 + 0x110) = 0x8078;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8079;
      *(undefined4 *)(iVar2 + 0x114) = 0xfc;
      *(undefined4 *)(iVar2 + 0x110) = 0x807a;
      *(undefined4 *)(iVar2 + 0x114) = 0xfe;
      *(undefined4 *)(iVar2 + 0x110) = 0x807b;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807c;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807d;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807e;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807f;
      *(undefined4 *)(iVar2 + 0x114) = 0xdb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8080;
      *(undefined4 *)(iVar2 + 0x114) = 0xe9;
      *(undefined4 *)(iVar2 + 0x110) = 0x8081;
      *(undefined4 *)(iVar2 + 0x114) = 0xc2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8082;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8083;
      *(undefined4 *)(iVar2 + 0x114) = 0xfc;
      *(undefined4 *)(iVar2 + 0x110) = 0x8084;
      *(undefined4 *)(iVar2 + 0x114) = 0xfe;
      *(undefined4 *)(iVar2 + 0x110) = 0x8085;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8086;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8087;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8088;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8089;
      *(undefined4 *)(iVar2 + 0x114) = 0xdb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808a;
      *(undefined4 *)(iVar2 + 0x114) = 0xe9;
      *(undefined4 *)(iVar2 + 0x110) = 0x808b;
      *(undefined4 *)(iVar2 + 0x114) = 0xc2;
      *(undefined4 *)(iVar2 + 0x110) = 0x808c;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x808d;
      *(undefined4 *)(iVar2 + 0x114) = 0xfc;
      *(undefined4 *)(iVar2 + 0x110) = 0x808e;
      *(undefined4 *)(iVar2 + 0x114) = 0xfe;
      *(undefined4 *)(iVar2 + 0x110) = 0x808f;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8090;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8091;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8092;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8093;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8094;
      *(undefined4 *)(iVar2 + 0x114) = 0xf2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8095;
      *(undefined4 *)(iVar2 + 0x114) = 0x3f;
      *(undefined4 *)(iVar2 + 0x110) = 0x8096;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8097;
      *(undefined4 *)(iVar2 + 0x114) = 0x32;
      *(undefined4 *)(iVar2 + 0x110) = 0x8098;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8099;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x809a;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0x809b;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x809c;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x809d;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x809e;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809f;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar2 + 0x114) = 0x28;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 7;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x2007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x4007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x6007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x800d;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8030;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8031;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8037;
      *(undefined4 *)(iVar2 + 0x114) = 0xa4;
      *(undefined4 *)(iVar2 + 0x110) = 0x805d;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0xe029;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8162;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8163;
      *(undefined4 *)(iVar2 + 0x114) = 0x3a;
      *(undefined4 *)(iVar2 + 0x110) = 0x8164;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      if (param_5 != 1) goto LAB_0101e548;
      *(undefined4 *)(iVar2 + 0x110) = 0x8032;
      *(undefined4 *)(iVar2 + 0x114) = 0xfc;
      *(undefined4 *)(iVar2 + 0x110) = 0x8033;
      *(undefined4 *)(iVar2 + 0x114) = 0x40;
      *(undefined4 *)(iVar2 + 0x110) = 0x8034;
      *(undefined4 *)(iVar2 + 0x114) = 0xf0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8035;
      *(undefined4 *)(iVar2 + 0x114) = 0x20;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      uVar1 = *(uint *)(iVar2 + 0x114);
    }
    *(uint *)(iVar2 + 0x114) = uVar1 & 0xfc | 3;
    *(undefined4 *)(iVar2 + 0x110) = 0x80d5;
    *(undefined4 *)(iVar2 + 0x114) = 0x22;
    *(undefined4 *)(iVar2 + 0x110) = 0x82ca;
    *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xfe;
  }
  else {
    iVar2 = *param_1;
  }
LAB_0101e548:
  *(undefined4 *)(iVar2 + 0x110) = 0x800d;
  *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xef | 0x10;
  *(undefined4 *)(iVar2 + 0x110) = 0x800f;
  *(uint *)(iVar2 + 0x114) = *(uint *)(iVar2 + 0x114) & 0xe7 | 0x18;
  *(uint *)(iVar3 + 0x128) = param_3 | 0x1000;
  *(undefined4 *)(iVar3 + 0x124) = 0;
  *(undefined4 *)(iVar2 + 0x110) = 0xe057;
  *(undefined4 *)(iVar2 + 0x114) = 0x1f;
  *(undefined4 *)(iVar2 + 0x110) = 0x902e;
  *(undefined4 *)(iVar2 + 0x114) = 0xa3;
  *(undefined4 *)(iVar2 + 0x110) = 0x9030;
  *(undefined4 *)(iVar2 + 0x114) = 0x53;
  *(undefined4 *)(iVar3 + 0x134) = 0x11f0;
  udelay(1);
  iVar3 = *param_1;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0c9;
  *(undefined4 *)(iVar3 + 0x114) = 0xfc;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0ca;
  *(undefined4 *)(iVar3 + 0x114) = 0xff;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0cb;
  *(undefined4 *)(iVar3 + 0x114) = 0xff;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0cc;
  *(undefined4 *)(iVar3 + 0x114) = 0xff;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0cd;
  *(undefined4 *)(iVar3 + 0x114) = 0x7f;
  *(undefined4 *)(iVar3 + 0x110) = 0xe0cd;
  *(undefined4 *)(iVar3 + 0x114) = 0xff;
  udelay(1);
  return 0;
}



/* @ 0x101ec20  FUN_0101ec20 */

void FUN_0101ec20(int *param_1,int param_2,uint param_3,int param_4,int param_5,int param_6,
                 byte param_7)

{
  int iVar1;
  int iVar2;
  undefined4 local_34;
  
  iVar2 = *param_1;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf1000;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf0000;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf1000;
  udelay(1);
  iVar1 = *param_1;
  if (param_2 == 0) {
    *(undefined4 *)(iVar1 + 0x110) = 0xe065;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0xe066;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0xe067;
    *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    *(undefined4 *)(iVar1 + 0x110) = 0xe068;
    *(undefined4 *)(iVar1 + 0x114) = 5;
    *(undefined4 *)(iVar1 + 0x110) = 0xe069;
    *(undefined4 *)(iVar1 + 0x114) = 0x13;
    *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
    *(undefined4 *)(iVar1 + 0x114) = 0x22;
    *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
    *(undefined4 *)(iVar1 + 0x114) = 1;
    *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
    *(undefined4 *)(iVar1 + 0x114) = 1;
    *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
    *(undefined4 *)(iVar1 + 0x114) = 0x31;
    *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
    *(undefined4 *)(iVar1 + 0x114) = 5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8065;
    *(undefined4 *)(iVar1 + 0x114) = 0xaa;
    *(undefined4 *)(iVar1 + 0x110) = 0x8066;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8067;
    *(undefined4 *)(iVar1 + 0x114) = 0x45;
    *(undefined4 *)(iVar1 + 0x110) = 0x8068;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0x8069;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0x806a;
    *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    *(undefined4 *)(iVar1 + 0x110) = 0x806b;
    *(undefined4 *)(iVar1 + 0x114) = 5;
    *(undefined4 *)(iVar1 + 0x110) = 0x806c;
    *(undefined4 *)(iVar1 + 0x114) = 0x13;
    *(undefined4 *)(iVar1 + 0x110) = 0x806d;
    *(undefined4 *)(iVar1 + 0x114) = 0x22;
    *(undefined4 *)(iVar1 + 0x110) = 0x806e;
    *(undefined4 *)(iVar1 + 0x114) = 5;
    *(undefined4 *)(iVar1 + 0x110) = 0x806f;
    *(undefined4 *)(iVar1 + 0x114) = 5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8070;
    *(undefined4 *)(iVar1 + 0x114) = 0x10;
    *(undefined4 *)(iVar1 + 0x110) = 0x8071;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8072;
    *(undefined4 *)(iVar1 + 0x114) = 0x10;
    *(undefined4 *)(iVar1 + 0x110) = 0x8073;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8074;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8075;
    *(undefined4 *)(iVar1 + 0x114) = 0xcf;
    *(undefined4 *)(iVar1 + 0x110) = 0x8076;
    *(undefined4 *)(iVar1 + 0x114) = 0xf7;
    *(undefined4 *)(iVar1 + 0x110) = 0x8077;
    *(undefined4 *)(iVar1 + 0x114) = 0xe1;
    *(undefined4 *)(iVar1 + 0x110) = 0x8078;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8079;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x807a;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x807b;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x807c;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x807d;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x807e;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x807f;
    *(undefined4 *)(iVar1 + 0x114) = 0xe3;
    *(undefined4 *)(iVar1 + 0x110) = 0x8080;
    *(undefined4 *)(iVar1 + 0x114) = 0xe7;
    *(undefined4 *)(iVar1 + 0x110) = 0x8081;
    *(undefined4 *)(iVar1 + 0x114) = 0xdb;
    *(undefined4 *)(iVar1 + 0x110) = 0x8082;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8083;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x8084;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x8085;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8086;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8087;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8088;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8089;
    *(undefined4 *)(iVar1 + 0x114) = 0xe3;
    *(undefined4 *)(iVar1 + 0x110) = 0x808a;
    *(undefined4 *)(iVar1 + 0x114) = 0xe7;
    *(undefined4 *)(iVar1 + 0x110) = 0x808b;
    *(undefined4 *)(iVar1 + 0x114) = 0xdb;
    *(undefined4 *)(iVar1 + 0x110) = 0x808c;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x808d;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x808e;
    *(undefined4 *)(iVar1 + 0x114) = 0xfd;
    *(undefined4 *)(iVar1 + 0x110) = 0x808f;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8090;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8091;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8092;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8093;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0x8094;
    *(undefined4 *)(iVar1 + 0x114) = 0xf5;
    *(undefined4 *)(iVar1 + 0x110) = 0x8095;
    *(undefined4 *)(iVar1 + 0x114) = 0x3f;
    *(undefined4 *)(iVar1 + 0x110) = 0x8096;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8097;
    *(undefined4 *)(iVar1 + 0x114) = 0x3f;
    *(undefined4 *)(iVar1 + 0x110) = 0x8098;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8099;
    *(undefined4 *)(iVar1 + 0x114) = 3;
    *(undefined4 *)(iVar1 + 0x110) = 0x809a;
    *(undefined4 *)(iVar1 + 0x114) = 1;
    *(undefined4 *)(iVar1 + 0x110) = 0x809b;
    *(undefined4 *)(iVar1 + 0x114) = 1;
    *(undefined4 *)(iVar1 + 0x110) = 0x809c;
    *(undefined4 *)(iVar1 + 0x114) = 1;
    *(undefined4 *)(iVar1 + 0x110) = 0x809d;
    *(undefined4 *)(iVar1 + 0x114) = 4;
    *(undefined4 *)(iVar1 + 0x110) = 0x809e;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x809f;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
    *(undefined4 *)(iVar1 + 0x114) = 0x28;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
    *(undefined4 *)(iVar1 + 0x114) = 4;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
    *(undefined4 *)(iVar1 + 0x114) = 4;
    *(undefined4 *)(iVar1 + 0x110) = 7;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x2007;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x4007;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x6007;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x800d;
    *(undefined4 *)(iVar1 + 0x114) = 0x10;
    *(undefined4 *)(iVar1 + 0x110) = 0x8030;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8031;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8036;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0x8037;
    *(undefined4 *)(iVar1 + 0x114) = 0xa6;
    *(undefined4 *)(iVar1 + 0x110) = 0x805d;
    *(undefined4 *)(iVar1 + 0x114) = 2;
    *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
    *(undefined4 *)(iVar1 + 0x114) = 2;
    *(undefined4 *)(iVar1 + 0x110) = 0xe029;
    *(undefined4 *)(iVar1 + 0x114) = 6;
    *(undefined4 *)(iVar1 + 0x110) = 0x8162;
    *(undefined4 *)(iVar1 + 0x114) = 3;
    *(undefined4 *)(iVar1 + 0x110) = 0x8163;
    *(undefined4 *)(iVar1 + 0x114) = 0x3a;
    *(undefined4 *)(iVar1 + 0x110) = 0x8164;
    *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
    *(undefined4 *)(iVar1 + 0x110) = 0x8165;
    *(undefined4 *)(iVar1 + 0x114) = 3;
    *(undefined4 *)(iVar1 + 0x110) = 0x8166;
    *(undefined4 *)(iVar1 + 0x114) = 0x3e;
    *(undefined4 *)(iVar1 + 0x110) = 0x8167;
    *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
    *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
    *(undefined4 *)(iVar1 + 0x114) = 0;
  }
  if (param_4 != 0) {
    *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
    *(undefined4 *)(iVar1 + 0x114) = 5;
  }
  *(undefined4 *)(iVar1 + 0x110) = 0x801a;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xcf;
  *(undefined4 *)(iVar1 + 0x110) = 0x9016;
  *(undefined4 *)(iVar1 + 0x114) = 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x9017;
  *(undefined4 *)(iVar1 + 0x114) = 0;
  *(undefined4 *)(iVar1 + 0x110) = 0x9022;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x1007;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfc | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x3007;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfc | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x5007;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfc | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x7007;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xfc | 2;
  if (param_6 != 0) {
    *(undefined4 *)(iVar1 + 0x110) = 0x901f;
    *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 7 | (param_7 & 0x1f) << 3;
  }
  local_34 = param_3;
  if (param_5 != 0) {
    local_34 = param_3 | 0x800;
  }
  *(uint *)(iVar2 + 0x128) = local_34 | 0x1000;
  *(undefined4 *)(iVar2 + 0x124) = 1;
  *(undefined4 *)(iVar2 + 0x134) = 0xf1100;
  udelay(1);
  if (param_4 != 0) {
    iVar1 = *param_1;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0c9;
    *(undefined4 *)(iVar1 + 0x114) = 0xfc;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0ca;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0cb;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0cc;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0cd;
    *(undefined4 *)(iVar1 + 0x114) = 0x7f;
    *(undefined4 *)(iVar1 + 0x110) = 0xe0cd;
    *(undefined4 *)(iVar1 + 0x114) = 0xff;
    udelay(1);
  }
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x110) = 0x9025;
  *(undefined4 *)(iVar1 + 0x114) = 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x9024;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x1f;
  *(undefined4 *)(iVar1 + 0x110) = 0x9017;
  *(undefined4 *)(iVar1 + 0x114) = 4;
  *(undefined4 *)(iVar1 + 0x110) = 0x9016;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x7f;
  *(undefined4 *)(iVar1 + 0x110) = 0x9018;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xc1 | 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x9016;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x9023;
  *(undefined4 *)(iVar1 + 0x114) = 0;
  *(undefined4 *)(iVar1 + 0x110) = 0x9022;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x7f;
  *(undefined4 *)(iVar1 + 0x110) = 0x9024;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xe0 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x9022;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x826f;
  *(undefined4 *)(iVar1 + 0x114) = 0;
  *(undefined4 *)(iVar1 + 0x110) = 0x8270;
  *(undefined4 *)(iVar1 + 0x114) = 0x14;
  *(undefined4 *)(iVar1 + 0x110) = 0x8273;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 3;
  *(undefined4 *)(iVar1 + 0x110) = 0x8274;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8275;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf0 | 3;
  *(undefined4 *)(iVar1 + 0x110) = 0x8276;
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0xf0 | 1;
  *(undefined4 *)(iVar1 + 0x110) = 0x8096;
  *(undefined4 *)(iVar1 + 0x114) = 2;
  *(undefined4 *)(iVar1 + 0x110) = 0x8097;
  *(undefined4 *)(iVar1 + 0x114) = 0x8a;
  return;
}



/* @ 0x101f4b8  FUN_0101f4b8 */

undefined4 al_serdes_hssp_group_cfg_eth_kr_mode(int *param_1,int param_2,uint param_3,int param_4,int param_5)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = *param_1;
  *(undefined4 *)(iVar3 + 0x134) = 0;
  if (param_5 == 0) {
    uVar1 = 0x30330030;
  }
  else {
    uVar1 = 0x20330020;
  }
  *(undefined4 *)(iVar3 + 0x224) = uVar1;
  *(undefined4 *)(iVar3 + 0x2a4) = uVar1;
  *(undefined4 *)(iVar3 + 0x324) = uVar1;
  *(undefined4 *)(iVar3 + 0x3a4) = uVar1;
  *(undefined4 *)(iVar3 + 0x130) = 1;
  *(undefined4 *)(iVar3 + 0x248) = 3;
  *(undefined4 *)(iVar3 + 0x2c8) = 3;
  *(undefined4 *)(iVar3 + 0x348) = 3;
  *(undefined4 *)(iVar3 + 0x3c8) = 3;
  *(uint *)(iVar3 + 0x128) = param_3 | 0x1000;
  *(undefined4 *)(iVar3 + 0x124) = 0;
  *(undefined4 *)(iVar3 + 0x134) = 0x1000;
  udelay(1);
  *(undefined4 *)(iVar3 + 0x134) = 0;
  udelay(1);
  *(undefined4 *)(iVar3 + 0x134) = 0x1000;
  udelay(1);
  if (param_4 == 2) {
    iVar2 = *param_1;
    if (param_2 == 0) {
      *(undefined4 *)(iVar2 + 0x110) = 0xe065;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0xe066;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0xe067;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0xe068;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0xe069;
      *(undefined4 *)(iVar2 + 0x114) = 0x9f;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar2 + 0x114) = 0x9f;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar2 + 0x114) = 0x77;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8065;
      *(undefined4 *)(iVar2 + 0x114) = 0xaa;
      *(undefined4 *)(iVar2 + 0x110) = 0x8066;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8067;
      *(undefined4 *)(iVar2 + 0x114) = 0x6c;
      *(undefined4 *)(iVar2 + 0x110) = 0x8068;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0x8069;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0x806a;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x806b;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x806c;
      *(undefined4 *)(iVar2 + 0x114) = 0x9f;
      *(undefined4 *)(iVar2 + 0x110) = 0x806d;
      *(undefined4 *)(iVar2 + 0x114) = 0x9f;
      *(undefined4 *)(iVar2 + 0x110) = 0x806e;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0x806f;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8070;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8071;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8072;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8073;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8074;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8075;
      *(undefined4 *)(iVar2 + 0x114) = 0xb3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8076;
      *(undefined4 *)(iVar2 + 0x114) = 0xf6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8077;
      *(undefined4 *)(iVar2 + 0x114) = 0xd0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8078;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8079;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x807a;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807b;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807c;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807d;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807e;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807f;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8080;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8081;
      *(undefined4 *)(iVar2 + 0x114) = 0xe2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8082;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8083;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8084;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8085;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8086;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8087;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8088;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8089;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x808a;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x808b;
      *(undefined4 *)(iVar2 + 0x114) = 0xe2;
      *(undefined4 *)(iVar2 + 0x110) = 0x808c;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x808d;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808e;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808f;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8090;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8091;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8092;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8093;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8094;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8095;
      *(undefined4 *)(iVar2 + 0x114) = 0x3f;
      *(undefined4 *)(iVar2 + 0x110) = 0x8096;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8097;
      *(undefined4 *)(iVar2 + 0x114) = 0x32;
      *(undefined4 *)(iVar2 + 0x110) = 0x8098;
      *(undefined4 *)(iVar2 + 0x114) = 0x11;
      *(undefined4 *)(iVar2 + 0x110) = 0x8099;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x809a;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x809b;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809c;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809d;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x809e;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809f;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 7;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x2007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x4007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x6007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x800d;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8030;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8031;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8037;
      *(undefined4 *)(iVar2 + 0x114) = 0x95;
      *(undefined4 *)(iVar2 + 0x110) = 0x805d;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0xe029;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8162;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8163;
      *(undefined4 *)(iVar2 + 0x114) = 0x3a;
      *(undefined4 *)(iVar2 + 0x110) = 0x8164;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
    }
  }
  else {
    if (param_4 != 3) {
      FUN_0100ceac(s__s__invalid_core_clock_freq___d__010345e8,
                   s_al_serdes_hssp_group_cfg_eth_kr__01034568,param_4);
      FUN_01010bfc(s__s__invalid_core_clock_freq___d__010345e8,
                   s_al_serdes_hssp_group_cfg_eth_kr__01034568,param_4);
      return 0xffffffea;
    }
    iVar2 = *param_1;
    if (param_2 == 0) {
      *(undefined4 *)(iVar2 + 0x110) = 0xe065;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0xe066;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0xe067;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0xe068;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0xe069;
      *(undefined4 *)(iVar2 + 0x114) = 0x1b;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar2 + 0x114) = 0x1b;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar2 + 0x114) = 0x77;
      *(undefined4 *)(iVar2 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar2 + 0x114) = 5;
      *(undefined4 *)(iVar2 + 0x110) = 0x8065;
      *(undefined4 *)(iVar2 + 0x114) = 0xaa;
      *(undefined4 *)(iVar2 + 0x110) = 0x8066;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8067;
      *(undefined4 *)(iVar2 + 0x114) = 0x6c;
      *(undefined4 *)(iVar2 + 0x110) = 0x8068;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0x8069;
      *(undefined4 *)(iVar2 + 0x114) = 0xbd;
      *(undefined4 *)(iVar2 + 0x110) = 0x806a;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x806b;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x806c;
      *(undefined4 *)(iVar2 + 0x114) = 0x1b;
      *(undefined4 *)(iVar2 + 0x110) = 0x806d;
      *(undefined4 *)(iVar2 + 0x114) = 0x1b;
      *(undefined4 *)(iVar2 + 0x110) = 0x806e;
      *(undefined4 *)(iVar2 + 0x114) = 7;
      *(undefined4 *)(iVar2 + 0x110) = 0x806f;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8070;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8071;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8072;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8073;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8074;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8075;
      *(undefined4 *)(iVar2 + 0x114) = 0xb3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8076;
      *(undefined4 *)(iVar2 + 0x114) = 0xf6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8077;
      *(undefined4 *)(iVar2 + 0x114) = 0xd0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8078;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8079;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x807a;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807b;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807c;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807d;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807e;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x807f;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8080;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8081;
      *(undefined4 *)(iVar2 + 0x114) = 0xe2;
      *(undefined4 *)(iVar2 + 0x110) = 0x8082;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8083;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8084;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8085;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8086;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8087;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8088;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8089;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x808a;
      *(undefined4 *)(iVar2 + 0x114) = 0xd3;
      *(undefined4 *)(iVar2 + 0x110) = 0x808b;
      *(undefined4 *)(iVar2 + 0x114) = 0xe2;
      *(undefined4 *)(iVar2 + 0x110) = 0x808c;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x808d;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808e;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x808f;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8090;
      *(undefined4 *)(iVar2 + 0x114) = 0xef;
      *(undefined4 *)(iVar2 + 0x110) = 0x8091;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8092;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8093;
      *(undefined4 *)(iVar2 + 0x114) = 0xfb;
      *(undefined4 *)(iVar2 + 0x110) = 0x8094;
      *(undefined4 *)(iVar2 + 0x114) = 0xff;
      *(undefined4 *)(iVar2 + 0x110) = 0x8095;
      *(undefined4 *)(iVar2 + 0x114) = 0x3f;
      *(undefined4 *)(iVar2 + 0x110) = 0x8096;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8097;
      *(undefined4 *)(iVar2 + 0x114) = 0x32;
      *(undefined4 *)(iVar2 + 0x110) = 0x8098;
      *(undefined4 *)(iVar2 + 0x114) = 0x11;
      *(undefined4 *)(iVar2 + 0x110) = 0x8099;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x809a;
      *(undefined4 *)(iVar2 + 0x114) = 1;
      *(undefined4 *)(iVar2 + 0x110) = 0x809b;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809c;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809d;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x809e;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x809f;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar2 + 0x114) = 4;
      *(undefined4 *)(iVar2 + 0x110) = 7;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x2007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x4007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x6007;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x800d;
      *(undefined4 *)(iVar2 + 0x114) = 0x10;
      *(undefined4 *)(iVar2 + 0x110) = 0x8030;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8031;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8036;
      *(undefined4 *)(iVar2 + 0x114) = 0;
      *(undefined4 *)(iVar2 + 0x110) = 0x8037;
      *(undefined4 *)(iVar2 + 0x114) = 0x95;
      *(undefined4 *)(iVar2 + 0x110) = 0x805d;
      *(undefined4 *)(iVar2 + 0x114) = 2;
      *(undefined4 *)(iVar2 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0xe029;
      *(undefined4 *)(iVar2 + 0x114) = 6;
      *(undefined4 *)(iVar2 + 0x110) = 0x8162;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8163;
      *(undefined4 *)(iVar2 + 0x114) = 0x3a;
      *(undefined4 *)(iVar2 + 0x110) = 0x8164;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
    }
  }
  *(undefined4 *)(iVar2 + 0x110) = 0xe057;
  *(undefined4 *)(iVar2 + 0x114) = 0x1f;
  FUN_0101d8bc(param_1);
  *(undefined4 *)(iVar3 + 0x134) = 0x11f0;
  udelay(1);
  return 0;
}



/* @ 0x101fffc  FUN_0101fffc */

undefined4 al_serdes_hssp_group_cfg_eth_sgmii_mode(int *param_1,int param_2,int param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *param_1;
  *(undefined4 *)(iVar2 + 0x134) = 0;
  *(undefined4 *)(iVar2 + 0x224) = 0x10110010;
  *(undefined4 *)(iVar2 + 0x2a4) = 0x10110010;
  *(undefined4 *)(iVar2 + 0x324) = 0x10110010;
  *(undefined4 *)(iVar2 + 0x3a4) = 0x10110010;
  *(undefined4 *)(iVar2 + 0x130) = 1;
  *(undefined4 *)(iVar2 + 0x248) = 3;
  *(undefined4 *)(iVar2 + 0x2c8) = 3;
  *(undefined4 *)(iVar2 + 0x348) = 3;
  *(undefined4 *)(iVar2 + 0x3c8) = 3;
  *(uint *)(iVar2 + 0x128) = param_4 | 0x1000;
  *(undefined4 *)(iVar2 + 0x124) = 0;
  *(undefined4 *)(iVar2 + 0x134) = 0x1000;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0x1000;
  udelay(1);
  if (param_2 == 0) {
    if (param_3 == 2) {
      iVar1 = *param_1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe065;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe066;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe067;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe068;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe069;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar1 + 0x114) = 0x11;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar1 + 0x114) = 0xd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8065;
      *(undefined4 *)(iVar1 + 0x114) = 0xaa;
      *(undefined4 *)(iVar1 + 0x110) = 0x8066;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8067;
      *(undefined4 *)(iVar1 + 0x114) = 0x57;
      *(undefined4 *)(iVar1 + 0x110) = 0x8068;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x8069;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x806a;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806b;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806c;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0x806d;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0x806e;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x806f;
      *(undefined4 *)(iVar1 + 0x114) = 0x17;
      *(undefined4 *)(iVar1 + 0x110) = 0x8070;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8071;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8072;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8073;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8074;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8075;
      *(undefined4 *)(iVar1 + 0x114) = 0xc3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8076;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8077;
      *(undefined4 *)(iVar1 + 0x114) = 0xd9;
      *(undefined4 *)(iVar1 + 0x110) = 0x8078;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8079;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x807a;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x807b;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807c;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807d;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807e;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807f;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8080;
      *(undefined4 *)(iVar1 + 0x114) = 0xe9;
      *(undefined4 *)(iVar1 + 0x110) = 0x8081;
      *(undefined4 *)(iVar1 + 0x114) = 0xc2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8082;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8083;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x8084;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x8085;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8086;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8087;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8088;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8089;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808a;
      *(undefined4 *)(iVar1 + 0x114) = 0xe9;
      *(undefined4 *)(iVar1 + 0x110) = 0x808b;
      *(undefined4 *)(iVar1 + 0x114) = 0xc2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808c;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808d;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x808e;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x808f;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8090;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8091;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8092;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8093;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8094;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8095;
      *(undefined4 *)(iVar1 + 0x114) = 0x3f;
      *(undefined4 *)(iVar1 + 0x110) = 0x8096;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8097;
      *(undefined4 *)(iVar1 + 0x114) = 100;
      *(undefined4 *)(iVar1 + 0x110) = 0x8098;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8099;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x809a;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0x809b;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809c;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809d;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809e;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x809f;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 7;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x2007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x4007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x6007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x800d;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8030;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8031;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8036;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8037;
      *(undefined4 *)(iVar1 + 0x114) = 0x9c;
      *(undefined4 *)(iVar1 + 0x110) = 0x805d;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0xe029;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8162;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8163;
      *(undefined4 *)(iVar1 + 0x114) = 0x3a;
      *(undefined4 *)(iVar1 + 0x110) = 0x8164;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8165;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8166;
      *(undefined4 *)(iVar1 + 0x114) = 0x3e;
      *(undefined4 *)(iVar1 + 0x110) = 0x8167;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar1 + 0x114) = 0;
    }
    else if (param_3 == 3) {
      iVar1 = *param_1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe065;
      *(undefined4 *)(iVar1 + 0x114) = 0xb7;
      *(undefined4 *)(iVar1 + 0x110) = 0xe066;
      *(undefined4 *)(iVar1 + 0x114) = 0xb7;
      *(undefined4 *)(iVar1 + 0x110) = 0xe067;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe068;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe069;
      *(undefined4 *)(iVar1 + 0x114) = 0x1a;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar1 + 0x114) = 0x1a;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar1 + 0x114) = 0x11;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar1 + 0x114) = 0xd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8065;
      *(undefined4 *)(iVar1 + 0x114) = 0x99;
      *(undefined4 *)(iVar1 + 0x110) = 0x8066;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8067;
      *(undefined4 *)(iVar1 + 0x114) = 0x6c;
      *(undefined4 *)(iVar1 + 0x110) = 0x8068;
      *(undefined4 *)(iVar1 + 0x114) = 0xb7;
      *(undefined4 *)(iVar1 + 0x110) = 0x8069;
      *(undefined4 *)(iVar1 + 0x114) = 0xb7;
      *(undefined4 *)(iVar1 + 0x110) = 0x806a;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806b;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806c;
      *(undefined4 *)(iVar1 + 0x114) = 0x1a;
      *(undefined4 *)(iVar1 + 0x110) = 0x806d;
      *(undefined4 *)(iVar1 + 0x114) = 0x1a;
      *(undefined4 *)(iVar1 + 0x110) = 0x806e;
      *(undefined4 *)(iVar1 + 0x114) = 7;
      *(undefined4 *)(iVar1 + 0x110) = 0x806f;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8070;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8071;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8072;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8073;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8074;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8075;
      *(undefined4 *)(iVar1 + 0x114) = 0xb3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8076;
      *(undefined4 *)(iVar1 + 0x114) = 0xf6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8077;
      *(undefined4 *)(iVar1 + 0x114) = 0xd0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8078;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8079;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x807a;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807b;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807c;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807d;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807e;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807f;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8080;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8081;
      *(undefined4 *)(iVar1 + 0x114) = 0xe2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8082;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8083;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8084;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8085;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8086;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8087;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8088;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8089;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x808a;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x808b;
      *(undefined4 *)(iVar1 + 0x114) = 0xe2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808c;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x808d;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808e;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808f;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8090;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8091;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8092;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8093;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8094;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8095;
      *(undefined4 *)(iVar1 + 0x114) = 0x3f;
      *(undefined4 *)(iVar1 + 0x110) = 0x8096;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8097;
      *(undefined4 *)(iVar1 + 0x114) = 100;
      *(undefined4 *)(iVar1 + 0x110) = 0x8098;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8099;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809a;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x809b;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809c;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809d;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809e;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x809f;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 7;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x2007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x4007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x6007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x800d;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8030;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8031;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8036;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8037;
      *(undefined4 *)(iVar1 + 0x114) = 0xb4;
      *(undefined4 *)(iVar1 + 0x110) = 0x805d;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0xe029;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8162;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8163;
      *(undefined4 *)(iVar1 + 0x114) = 0x3a;
      *(undefined4 *)(iVar1 + 0x110) = 0x8164;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8165;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8166;
      *(undefined4 *)(iVar1 + 0x114) = 0x3e;
      *(undefined4 *)(iVar1 + 0x110) = 0x8167;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar1 + 0x114) = 0;
    }
    else {
      if (param_3 != 1) {
        FUN_0100ceac(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_eth_sgm_01034590,param_3);
        FUN_01010bfc(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_eth_sgm_01034590,param_3);
        return 0xffffffea;
      }
      iVar1 = *param_1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe065;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe066;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe067;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe068;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe069;
      *(undefined4 *)(iVar1 + 0x114) = 0x2c;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar1 + 0x114) = 0x2c;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar1 + 0x114) = 0x11;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar1 + 0x114) = 0xd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8065;
      *(undefined4 *)(iVar1 + 0x114) = 0xaa;
      *(undefined4 *)(iVar1 + 0x110) = 0x8066;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8067;
      *(undefined4 *)(iVar1 + 0x114) = 0x57;
      *(undefined4 *)(iVar1 + 0x110) = 0x8068;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x8069;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x806a;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806b;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806c;
      *(undefined4 *)(iVar1 + 0x114) = 0x2c;
      *(undefined4 *)(iVar1 + 0x110) = 0x806d;
      *(undefined4 *)(iVar1 + 0x114) = 0x2c;
      *(undefined4 *)(iVar1 + 0x110) = 0x806e;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x806f;
      *(undefined4 *)(iVar1 + 0x114) = 0x17;
      *(undefined4 *)(iVar1 + 0x110) = 0x8070;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8071;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8072;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8073;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8074;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8075;
      *(undefined4 *)(iVar1 + 0x114) = 0xc3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8076;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8077;
      *(undefined4 *)(iVar1 + 0x114) = 0xd9;
      *(undefined4 *)(iVar1 + 0x110) = 0x8078;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8079;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x807a;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x807b;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807c;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807d;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807e;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807f;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8080;
      *(undefined4 *)(iVar1 + 0x114) = 0xe9;
      *(undefined4 *)(iVar1 + 0x110) = 0x8081;
      *(undefined4 *)(iVar1 + 0x114) = 0xc2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8082;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8083;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x8084;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x8085;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8086;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8087;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8088;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8089;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808a;
      *(undefined4 *)(iVar1 + 0x114) = 0xe9;
      *(undefined4 *)(iVar1 + 0x110) = 0x808b;
      *(undefined4 *)(iVar1 + 0x114) = 0xc2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808c;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808d;
      *(undefined4 *)(iVar1 + 0x114) = 0xfc;
      *(undefined4 *)(iVar1 + 0x110) = 0x808e;
      *(undefined4 *)(iVar1 + 0x114) = 0xfe;
      *(undefined4 *)(iVar1 + 0x110) = 0x808f;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8090;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8091;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8092;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8093;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8094;
      *(undefined4 *)(iVar1 + 0x114) = 0xf2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8095;
      *(undefined4 *)(iVar1 + 0x114) = 0x3f;
      *(undefined4 *)(iVar1 + 0x110) = 0x8096;
      *(undefined4 *)(iVar1 + 0x114) = 0x27;
      *(undefined4 *)(iVar1 + 0x110) = 0x8097;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8098;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8099;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809a;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x809b;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809c;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809d;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809e;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x809f;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 7;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x2007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x4007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x6007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x800d;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8030;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8031;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8036;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8037;
      *(undefined4 *)(iVar1 + 0x114) = 0x9c;
      *(undefined4 *)(iVar1 + 0x110) = 0x805d;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0xe029;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8162;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8163;
      *(undefined4 *)(iVar1 + 0x114) = 0x3a;
      *(undefined4 *)(iVar1 + 0x110) = 0x8164;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8165;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8166;
      *(undefined4 *)(iVar1 + 0x114) = 0x3e;
      *(undefined4 *)(iVar1 + 0x110) = 0x8167;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar1 + 0x114) = 0;
    }
  }
  else {
    iVar1 = *param_1;
  }
  *(undefined4 *)(iVar1 + 0x110) = 0xe057;
  *(undefined4 *)(iVar1 + 0x114) = 0x1f;
  FUN_0101d8bc(param_1);
  *(undefined4 *)(iVar2 + 0x134) = 0x11f0;
  udelay(1);
  return 0;
}



/* @ 0x102104c  FUN_0102104c */

undefined4 al_serdes_hssp_group_cfg_eth_sgmii_2_5g_mode(int *param_1,int param_2,int param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *param_1;
  *(undefined4 *)(iVar2 + 0x134) = 0;
  *(undefined4 *)(iVar2 + 0x224) = 0x10220010;
  *(undefined4 *)(iVar2 + 0x2a4) = 0x10220010;
  *(undefined4 *)(iVar2 + 0x324) = 0x10220010;
  *(undefined4 *)(iVar2 + 0x3a4) = 0x10220010;
  *(undefined4 *)(iVar2 + 0x130) = 1;
  *(undefined4 *)(iVar2 + 0x248) = 3;
  *(undefined4 *)(iVar2 + 0x2c8) = 3;
  *(undefined4 *)(iVar2 + 0x348) = 3;
  *(undefined4 *)(iVar2 + 0x3c8) = 3;
  *(uint *)(iVar2 + 0x128) = param_4 | 0x1000;
  *(undefined4 *)(iVar2 + 0x124) = 0;
  *(undefined4 *)(iVar2 + 0x134) = 0x1000;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0;
  udelay(1);
  *(undefined4 *)(iVar2 + 0x134) = 0x1000;
  udelay(1);
  if (param_2 == 0) {
    if (param_3 == 1) {
      iVar1 = *param_1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe065;
      *(undefined4 *)(iVar1 + 0x114) = 0xb1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe066;
      *(undefined4 *)(iVar1 + 0x114) = 0xb1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe067;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0xe068;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0xe069;
      *(undefined4 *)(iVar1 + 0x114) = 0x19;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar1 + 0x114) = 0x19;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar1 + 0x114) = 0x22;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar1 + 0x114) = 0xd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8065;
      *(undefined4 *)(iVar1 + 0x114) = 0xaa;
      *(undefined4 *)(iVar1 + 0x110) = 0x8066;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8067;
      *(undefined4 *)(iVar1 + 0x114) = 0x45;
      *(undefined4 *)(iVar1 + 0x110) = 0x8068;
      *(undefined4 *)(iVar1 + 0x114) = 0xb1;
      *(undefined4 *)(iVar1 + 0x110) = 0x8069;
      *(undefined4 *)(iVar1 + 0x114) = 0xb1;
      *(undefined4 *)(iVar1 + 0x110) = 0x806a;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x806b;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x806c;
      *(undefined4 *)(iVar1 + 0x114) = 0x19;
      *(undefined4 *)(iVar1 + 0x110) = 0x806d;
      *(undefined4 *)(iVar1 + 0x114) = 0x19;
      *(undefined4 *)(iVar1 + 0x110) = 0x806e;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x806f;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8070;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8071;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8072;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8073;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8074;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8075;
      *(undefined4 *)(iVar1 + 0x114) = 0xcf;
      *(undefined4 *)(iVar1 + 0x110) = 0x8076;
      *(undefined4 *)(iVar1 + 0x114) = 0xf7;
      *(undefined4 *)(iVar1 + 0x110) = 0x8077;
      *(undefined4 *)(iVar1 + 0x114) = 0xe1;
      *(undefined4 *)(iVar1 + 0x110) = 0x8078;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8079;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x807a;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x807b;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807c;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807d;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807e;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807f;
      *(undefined4 *)(iVar1 + 0x114) = 0xe3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8080;
      *(undefined4 *)(iVar1 + 0x114) = 0xe7;
      *(undefined4 *)(iVar1 + 0x110) = 0x8081;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8082;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8083;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8084;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8085;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8086;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8087;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8088;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8089;
      *(undefined4 *)(iVar1 + 0x114) = 0xe3;
      *(undefined4 *)(iVar1 + 0x110) = 0x808a;
      *(undefined4 *)(iVar1 + 0x114) = 0xe7;
      *(undefined4 *)(iVar1 + 0x110) = 0x808b;
      *(undefined4 *)(iVar1 + 0x114) = 0xdb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808c;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x808d;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x808e;
      *(undefined4 *)(iVar1 + 0x114) = 0xfd;
      *(undefined4 *)(iVar1 + 0x110) = 0x808f;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8090;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8091;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8092;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8093;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8094;
      *(undefined4 *)(iVar1 + 0x114) = 0xf5;
      *(undefined4 *)(iVar1 + 0x110) = 0x8095;
      *(undefined4 *)(iVar1 + 0x114) = 0x3f;
      *(undefined4 *)(iVar1 + 0x110) = 0x8096;
      *(undefined4 *)(iVar1 + 0x114) = 0x27;
      *(undefined4 *)(iVar1 + 0x110) = 0x8097;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8098;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8099;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x809a;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0x809b;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809c;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809d;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809e;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x809f;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 7;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x2007;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x4007;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x6007;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x800d;
      *(undefined4 *)(iVar1 + 0x114) = 0x14;
      *(undefined4 *)(iVar1 + 0x110) = 0x8030;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8031;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8036;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8037;
      *(undefined4 *)(iVar1 + 0x114) = 0xb2;
      *(undefined4 *)(iVar1 + 0x110) = 0x805d;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0xe029;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8162;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8163;
      *(undefined4 *)(iVar1 + 0x114) = 0x3a;
      *(undefined4 *)(iVar1 + 0x110) = 0x8164;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8165;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8166;
      *(undefined4 *)(iVar1 + 0x114) = 0x3e;
      *(undefined4 *)(iVar1 + 0x110) = 0x8167;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar1 + 0x114) = 0;
    }
    else {
      if (param_3 != 3) {
        FUN_0100ceac(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_eth_sgm_010345b8,param_3);
        FUN_01010bfc(s__s__invalid_core_clock_freq___d__010345e8,
                     s_al_serdes_hssp_group_cfg_eth_sgm_010345b8,param_3);
        return 0xffffffea;
      }
      iVar1 = *param_1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe065;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe066;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0xe067;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe068;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe069;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06a;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06b;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06c;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06d;
      *(undefined4 *)(iVar1 + 0x114) = 0x22;
      *(undefined4 *)(iVar1 + 0x110) = 0xe06e;
      *(undefined4 *)(iVar1 + 0x114) = 0xd;
      *(undefined4 *)(iVar1 + 0x110) = 0x8065;
      *(undefined4 *)(iVar1 + 0x114) = 0xaa;
      *(undefined4 *)(iVar1 + 0x110) = 0x8066;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8067;
      *(undefined4 *)(iVar1 + 0x114) = 0x6c;
      *(undefined4 *)(iVar1 + 0x110) = 0x8068;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x8069;
      *(undefined4 *)(iVar1 + 0x114) = 0xbf;
      *(undefined4 *)(iVar1 + 0x110) = 0x806a;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806b;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_START_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x806c;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0x806d;
      *(undefined4 *)(iVar1 + 0x114) = 0xe;
      *(undefined4 *)(iVar1 + 0x110) = 0x806e;
      *(undefined4 *)(iVar1 + 0x114) = 7;
      *(undefined4 *)(iVar1 + 0x110) = 0x806f;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8070;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8071;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8072;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8073;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8074;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8075;
      *(undefined4 *)(iVar1 + 0x114) = 0xb3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8076;
      *(undefined4 *)(iVar1 + 0x114) = 0xf6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8077;
      *(undefined4 *)(iVar1 + 0x114) = 0xd0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8078;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8079;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x807a;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807b;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807c;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807d;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807e;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x807f;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8080;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8081;
      *(undefined4 *)(iVar1 + 0x114) = 0xe2;
      *(undefined4 *)(iVar1 + 0x110) = 0x8082;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8083;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8084;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8085;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8086;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8087;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8088;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8089;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x808a;
      *(undefined4 *)(iVar1 + 0x114) = 0xd3;
      *(undefined4 *)(iVar1 + 0x110) = 0x808b;
      *(undefined4 *)(iVar1 + 0x114) = 0xe2;
      *(undefined4 *)(iVar1 + 0x110) = 0x808c;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x808d;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808e;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x808f;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8090;
      *(undefined4 *)(iVar1 + 0x114) = 0xef;
      *(undefined4 *)(iVar1 + 0x110) = 0x8091;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8092;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8093;
      *(undefined4 *)(iVar1 + 0x114) = 0xfb;
      *(undefined4 *)(iVar1 + 0x110) = 0x8094;
      *(undefined4 *)(iVar1 + 0x114) = 0xff;
      *(undefined4 *)(iVar1 + 0x110) = 0x8095;
      *(undefined4 *)(iVar1 + 0x114) = 0x3f;
      *(undefined4 *)(iVar1 + 0x110) = 0x8096;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8097;
      *(undefined4 *)(iVar1 + 0x114) = 100;
      *(undefined4 *)(iVar1 + 0x110) = 0x8098;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8099;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x809a;
      *(undefined4 *)(iVar1 + 0x114) = 1;
      *(undefined4 *)(iVar1 + 0x110) = 0x809b;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809c;
      *(undefined4 *)(iVar1 + 0x114) = 5;
      *(undefined4 *)(iVar1 + 0x110) = 0x809d;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x809e;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x809f;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a0;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a1;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a2;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a3;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a4;
      *(undefined4 *)(iVar1 + 0x114) = 4;
      *(undefined4 *)(iVar1 + 0x110) = 7;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x2007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x4007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x6007;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x800d;
      *(undefined4 *)(iVar1 + 0x114) = 0x10;
      *(undefined4 *)(iVar1 + 0x110) = 0x8030;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8031;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8036;
      *(undefined4 *)(iVar1 + 0x114) = 0;
      *(undefined4 *)(iVar1 + 0x110) = 0x8037;
      *(undefined4 *)(iVar1 + 0x114) = 0x9c;
      *(undefined4 *)(iVar1 + 0x110) = 0x805d;
      *(undefined4 *)(iVar1 + 0x114) = 2;
      *(undefined4 *)(iVar1 + 0x110) = 0x80a5;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0xe029;
      *(undefined4 *)(iVar1 + 0x114) = 6;
      *(undefined4 *)(iVar1 + 0x110) = 0x8162;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8163;
      *(undefined4 *)(iVar1 + 0x114) = 0x3a;
      *(undefined4 *)(iVar1 + 0x110) = 0x8164;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0x8165;
      *(undefined4 *)(iVar1 + 0x114) = 3;
      *(undefined4 *)(iVar1 + 0x110) = 0x8166;
      *(undefined4 *)(iVar1 + 0x114) = 0x3e;
      *(undefined4 *)(iVar1 + 0x110) = 0x8167;
      *(undefined4 *)(iVar1 + 0x114) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
      *(undefined4 *)(iVar1 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar1 + 0x114) = 0;
    }
  }
  else {
    iVar1 = *param_1;
  }
  *(undefined4 *)(iVar1 + 0x110) = 0xe057;
  *(undefined4 *)(iVar1 + 0x114) = 0x1f;
  *(undefined4 *)(iVar2 + 0x134) = 0x11f0;
  udelay(1);
  return 0;
}



/* @ 0x1021b60  FUN_01021b60 */

undefined4 FUN_01021b60(int *param_1,byte *param_2,byte *param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x11c) = 0;
  if (param_2 == (byte *)0x0) {
    uVar3 = 0xe4;
    param_2 = (byte *)0x10346a0;
    param_3 = (byte *)0xdffc;
  }
  else {
    if (param_3 == (byte *)0x0) {
      return 0;
    }
    uVar3 = (uint)*param_2;
  }
  iVar2 = 4 - (int)param_2;
  while (*(uint *)(iVar1 + 0x120) =
              (uint)param_2[3] | uVar3 << 0x18 | (uint)param_2[1] << 0x10 | (uint)param_2[2] << 8,
        param_2 + iVar2 < param_3) {
    param_2 = param_2 + 4;
    uVar3 = (uint)*param_2;
  }
  return 0;
}



/* @ 0x1021be0  FUN_01021be0 */

undefined4 al_serdes_25g_group_ictl_pma_val_set(int *param_1,uint *param_2,int param_3,int param_4,char param_5)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = *param_1;
  *param_2 = 0;
  if (param_5 == '\x02') {
    *param_2 = 3;
  }
  else if (param_5 == '\x04') {
    *param_2 = 1;
  }
  else if (param_5 != '\x01') {
    FUN_0100ceac(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_25g_group_ictl_pma_val_01034678,param_5);
    FUN_01010bfc(s__s__invalid_core_clock_source____0103460c,
                 s_al_serdes_25g_group_ictl_pma_val_01034678,param_5);
    return 0xffffffea;
  }
  if (param_3 == 1) {
    *param_2 = *param_2 | 0x10;
  }
  else if (param_3 != 0) {
    if (param_3 != 2) {
      FUN_0100ceac(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_25g_group_ictl_pma_val_01034678,param_3);
      FUN_01010bfc(s__s__invalid_R2L_clock_source___d_01034630,
                   s_al_serdes_25g_group_ictl_pma_val_01034678,param_3);
      return 0xffffffea;
    }
    *param_2 = *param_2 | 0x20;
  }
  if (param_4 == 1) {
    uVar2 = *param_2 | 0x40;
    *param_2 = uVar2;
  }
  else if (param_4 == 0) {
    uVar2 = *param_2;
  }
  else {
    if (param_4 != 4) {
      FUN_0100ceac(s__s__invalid_L2R_clock_source___d_01034654,
                   s_al_serdes_25g_group_ictl_pma_val_01034678,param_4);
      FUN_01010bfc(s__s__invalid_L2R_clock_source___d_01034654,
                   s_al_serdes_25g_group_ictl_pma_val_01034678,param_4);
      return 0xffffffea;
    }
    uVar2 = *param_2 | 0x80;
    *param_2 = uVar2;
  }
  *(uint *)(iVar1 + 0x118) = uVar2;
  return 0;
}



/* @ 0x1021d90  FUN_01021d90 */

void FUN_01021d90(int *param_1,int param_2,int param_3)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  
  piVar1 = (int *)(param_2 + -4);
  piVar2 = (int *)(param_3 + -4);
  iVar3 = 0;
  iVar5 = *param_1;
  do {
    piVar1 = piVar1 + 1;
    if (*piVar1 == 1) {
      iVar6 = iVar5 + iVar3 * 0x80;
      uVar4 = *(uint *)(iVar6 + 0x210);
      *(uint *)(iVar6 + 0x210) = ~uVar4 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT ^ uVar4;
    }
    piVar2 = piVar2 + 1;
    if (*piVar2 == 1) {
      iVar6 = iVar5 + iVar3 * 0x80;
      uVar4 = *(uint *)(iVar6 + 0x210);
      *(uint *)(iVar6 + 0x210) = ~uVar4 & 4 ^ uVar4;
    }
    iVar3 = iVar3 + 1;
  } while (iVar3 != 4);
  return;
}



/* @ 0x1021e08  FUN_01021e08 */

void FUN_01021e08(int param_1)

{
                    /* WARNING: Could not recover jumptable at 0x01021e0c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(param_1 + 0xb4))();
  return;
}



/* @ 0x1021e10  FUN_01021e10 */

void FUN_01021e10(int param_1)

{
                    /* WARNING: Could not recover jumptable at 0x01021e14. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(param_1 + 0xb0))();
  return;
}



/* @ 0x1021e18  FUN_01021e18 */

void FUN_01021e18(int param_1,uint *param_2)

{
  uint uVar1;
  bool bVar2;
  
  uVar1 = *(uint *)(*(int *)(param_1 + 4) + 0x70);
  bVar2 = (uVar1 & 7) != 0;
  *param_2 = (uint)bVar2;
  if (bVar2) {
    param_2[1] = ((uVar1 ^ 0x10) & 0x1f) >> 4;
  }
  return;
}



/* @ 0x1021e48  FUN_01021e48 */

undefined4 FUN_01021e48(int param_1)

{
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x7c) = 4;
  return 0;
}



/* @ 0x1021e5c  FUN_01021e5c */

undefined4 FUN_01021e5c(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1;
  *(undefined4 *)(param_1[1] + 0x7c) = 1;
  if (iVar1 != 0) {
    DataMemoryBarrier(0xf);
    iVar1 = *param_1;
    if ((char)param_1[3] == '\x02') {
      *(undefined4 *)(iVar1 + 0x4810) = 0xfffffffe;
      *(undefined4 *)(iVar1 + 0x94) = 0xffffbfff;
    }
    *(undefined4 *)(iVar1 + 0x10) = 0xff77ffff;
  }
  return 0;
}



/* @ 0x1021eac  FUN_01021eac */

undefined4 FUN_01021eac(int param_1)

{
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x7c) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  return 0;
}



/* @ 0x1021ec0  FUN_01021ec0 */

undefined4 FUN_01021ec0(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1;
  *(undefined4 *)(param_1[1] + 0x7c) = 2;
  if (iVar1 != 0) {
    DataMemoryBarrier(0xf);
    iVar1 = *param_1;
    if ((char)param_1[3] == '\x02') {
      *(undefined4 *)(iVar1 + 0x4810) = 0xfffffffd;
      *(undefined4 *)(iVar1 + 0x94) = 0xffffbfff;
    }
    *(undefined4 *)(iVar1 + 0x10) = 0xff6fffff;
  }
  return 0;
}



/* @ 0x1021f10  FUN_01021f10 */

undefined4 al_ddr_rev_get(undefined4 param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

{
  int iVar1;
  
  iVar1 = *param_3;
  *param_4 = param_1;
  param_4[1] = param_2;
  param_4[2] = param_3;
  if (iVar1 == 0x120120) {
    *(undefined1 *)(param_4 + 3) = 1;
  }
  else {
    if (iVar1 != 0x250231) {
      FUN_0100ceac(s__s__Unknown_DDR_rev__08x_010426bc,s_al_ddr_rev_get_0104269c,iVar1);
      FUN_01010bfc(s__s__Unknown_DDR_rev__08x_010426bc,s_al_ddr_rev_get_0104269c,iVar1);
      FUN_0100ceac(s__s__d__al_ddr_cfg_init_failed__010426d8,s_al_ddr_cfg_init_010426ac,0xd15);
      FUN_01010bfc(s__s__d__al_ddr_cfg_init_failed__010426d8,s_al_ddr_cfg_init_010426ac,0xd15);
      return 0xfffffffb;
    }
    *(undefined1 *)(param_4 + 3) = 2;
  }
  return 0;
}



/* @ 0x1021fd4  FUN_01021fd4 */

uint FUN_01021fd4(int param_1)

{
  return *(uint *)(param_1 + 0x15c) >> 0x10;
}



/* @ 0x1021fe0  FUN_01021fe0 */

uint FUN_01021fe0(int param_1)

{
  return *(uint *)(param_1 + 0x15c) & 0xffff;
}



/* @ 0x1021fec  FUN_01021fec */

void al_pbs_axi_timeout_set(int param_1,uint param_2,uint param_3)

{
  if (*(uint *)(param_1 + 0x15c) >> 0x10 == 0) {
    *(uint *)(param_1 + 0x120) = param_2;
    return;
  }
  if ((param_2 & 0xffff) != (param_3 & 0xffff)) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___num_cyc_spi_low____num_cyc_low_01042768,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x32)
    ;
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___num_cyc_spi_low____num_cyc_low_01042768,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x32)
    ;
    FUN_01000454(0);
  }
  if ((param_2 >> 0x10 & 0xffffff00) != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_cyc_hi____0xFF0000_>>_16__0104278c,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x34)
    ;
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s____num_cyc_hi____0xFF0000_>>_16__0104278c,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x34)
    ;
    FUN_01000454(0);
  }
  if ((param_3 & 0xff0000) != 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___num_cyc_spi_med____0__010427c0,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x39)
    ;
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___num_cyc_spi_med____0__010427c0,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_axi_timeout_set_010426f8,0x39)
    ;
    FUN_01000454(0);
  }
  *(uint *)(param_1 + 0x120) = param_3 & 0xff000000 | param_2 & 0xffff | param_2 & 0xffff0000;
  return;
}



/* @ 0x10221a8  FUN_010221a8 */

void FUN_010221a8(int param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  
  uVar2 = *(uint *)(param_1 + 0xd8);
  uVar1 = uVar2;
  if (param_2 != 0) {
    uVar1 = uVar2 ^ 1;
  }
  *(uint *)(param_1 + 0xd8) = uVar1 & 1 ^ uVar2;
  return;
}



/* @ 0x10221c8  FUN_010221c8 */

void al_pbs_apb_mem_config_set(int param_1,int *param_2)

{
  uint uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs__0104272c,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_apb_mem_config_set_01042710,
                 0x5b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs__0104272c,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_apb_mem_config_set_01042710,
                 0x5b);
    FUN_01000454(0);
  }
  if (param_2 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___cfg__01042738,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_apb_mem_config_set_01042710,
                 0x5c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___cfg__01042738,1,
                 s____HAL__drivers_pbs_al_hal_pbs_u_01042740,s_al_pbs_apb_mem_config_set_01042710,
                 0x5c);
    FUN_01000454(0);
  }
  if (*param_2 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0x800;
  }
  if (param_2[1] != 0) {
    uVar1 = uVar1 | 0x1000;
  }
  if (param_2[2] != 0) {
    uVar1 = uVar1 | 0x2000;
  }
  if (param_2[3] != 0) {
    uVar1 = uVar1 | 0x4000;
  }
  if (param_2[4] != 0) {
    uVar1 = uVar1 | 0x8000;
  }
  *(uint *)(param_1 + 0x130) =
       (uVar1 ^ *(uint *)(param_1 + 0x130)) & 0xf800 ^ *(uint *)(param_1 + 0x130);
  return;
}



/* @ 0x1022318  FUN_01022318 */

void FUN_01022318(int param_1)

{
  *(undefined2 *)(param_1 + 0xf4) = 0xf;
  return;
}



/* @ 0x1022324  FUN_01022324 */

undefined4 al_bootstrap_parse(int param_1,undefined4 *param_2)

{
  undefined1 uVar1;
  uint uVar2;
  uint uVar3;
  undefined1 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  uint uVar7;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regfile_ptr__01042858,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_parse_0104281c,0x2ce);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regfile_ptr__01042858,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_parse_0104281c,0x2ce);
    FUN_01000454(0);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bootstrap__0104286c,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_parse_0104281c,0x2cf);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bootstrap__0104286c,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_parse_0104281c,0x2cf);
    FUN_01000454(0);
  }
  uVar7 = *(uint *)(param_1 + 0x110);
  uVar5 = 25000000;
  if ((uVar7 & 0x80000) != 0) {
    uVar5 = 100000000;
  }
  param_2[5] = uVar5;
  uVar6 = uVar5;
  switch(uVar7 & 0xf) {
  case 1:
    uVar6 = 1000000000;
    break;
  case 2:
    uVar6 = 1400000000;
    break;
  case 3:
    uVar6 = 1500000000;
    break;
  case 4:
    uVar6 = 1600000000;
    break;
  case 5:
    uVar6 = 1700000000;
    break;
  case 6:
    uVar6 = 1800000000;
    break;
  case 7:
    uVar6 = 1900000000;
    break;
  case 8:
    uVar6 = 2100000000;
    break;
  case 9:
    uVar6 = 2200000000;
    break;
  case 10:
    uVar6 = 2300000000;
    break;
  case 0xb:
    uVar6 = 2400000000;
    break;
  case 0xc:
    uVar6 = 2500000000;
    break;
  case 0xd:
    uVar6 = 2600000000;
    break;
  case 0xe:
    uVar6 = 2700000000;
    break;
  case 0xf:
    uVar6 = 2000000000;
  }
  *param_2 = uVar6;
  uVar6 = uVar5;
  switch((uVar7 & 0x7f) >> 4) {
  case 1:
    uVar6 = 0x3f940aaa;
    break;
  case 2:
    uVar6 = 0x27bc86aa;
    break;
  case 3:
    uVar6 = 1300000000;
    break;
  case 4:
    uVar6 = 0x37a18955;
    break;
  case 5:
    uVar6 = 1050000000;
    break;
  case 6:
    uVar6 = 1200000000;
    break;
  case 7:
    uVar6 = 800000000;
  }
  uVar2 = (uVar7 & 0x1ff) >> 7;
  param_2[1] = uVar6;
  if (uVar2 == 0) {
    param_2[2] = uVar5;
    uVar6 = uVar5;
  }
  else {
    uVar5 = 1500000000;
    uVar3 = (uVar7 & 0x7ff) >> 9;
    if (uVar2 == 1) {
      uVar5 = 3000000000;
    }
    param_2[2] = uVar5;
    if (uVar3 == 2) {
      uVar5 = 375000000;
      uVar6 = 428000000;
    }
    else if (uVar3 == 3) {
      uVar5 = 375000000;
      uVar6 = 500000000;
    }
    else if (uVar3 == 1) {
      uVar5 = 375000000;
      uVar6 = uVar5;
    }
    else {
      uVar5 = 375000000;
      uVar6 = 250000000;
    }
  }
  param_2[3] = uVar6;
  uVar2 = (uVar7 & 0x3fff) >> 0xc;
  param_2[4] = uVar5;
  uVar1 = (&DAT_010427dc)[uVar2];
  uVar4 = (&DAT_010427d8)[uVar2];
  *(byte *)(param_2 + 6) = (byte)((uVar7 << 0x14) >> 0x1f);
  *(undefined1 *)((int)param_2 + 0x19) = uVar4;
  *(undefined1 *)((int)param_2 + 0x1a) = uVar1;
  *(byte *)((int)param_2 + 0x1b) = (byte)((uVar7 << 0x11) >> 0x1f);
  param_2[0xb] = 1;
  switch((uVar7 & 0x3ffff) >> 0xf) {
  case 0:
    uVar1 = 4;
    uVar5 = 0x1c200;
    break;
  case 1:
    uVar1 = 1;
    uVar5 = 2000000;
    break;
  case 2:
    uVar1 = 0;
    uVar5 = 0x1c200;
    break;
  default:
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_boot_device_get_010427e0
                 ,0x219);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                 s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_boot_device_get_010427e0
                 ,0x219);
    FUN_01000454(0);
  case 7:
    uVar1 = 3;
    uVar5 = 0x1c200;
    break;
  case 4:
    uVar1 = 1;
    uVar5 = 0x1c200;
    break;
  case 5:
    uVar1 = 2;
    uVar5 = 0x1c200;
    break;
  case 6:
    uVar1 = 1;
    uVar5 = 1000000;
  }
  uVar2 = (uVar7 & 0x3fffff) >> 0x14;
  uVar4 = (undefined1)uVar2;
  *(undefined1 *)(param_2 + 7) = uVar1;
  param_2[8] = uVar5;
  *(byte *)(param_2 + 9) = (byte)((uVar7 << 0xd) >> 0x1f);
  if ((uVar2 != 1) && (uVar2 != 0)) {
    if (uVar2 == 3) {
      uVar4 = 2;
    }
    else {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                   s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_cpu_exist_get_01042800
                   ,0x267);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
                   s____HAL__drivers_pbs_al_hal_boots_01042830,s_al_bootstrap_cpu_exist_get_01042800
                   ,0x267);
      FUN_01000454(0);
      uVar4 = 0;
    }
  }
  *(undefined1 *)((int)param_2 + 0x25) = uVar4;
  if ((uVar7 & 0x800000) == 0) {
    uVar1 = 0x50;
  }
  else {
    uVar1 = 0x57;
  }
  *(byte *)((int)param_2 + 0x26) = (byte)((uVar7 << 9) >> 0x1f);
  *(undefined1 *)((int)param_2 + 0x27) = uVar1;
  *(undefined1 *)(param_2 + 10) = 0;
  return 0;
}



/* @ 0x1022860  FUN_01022860 */

undefined4 al_i2c_xfer_finish(int *param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = *param_1;
  uVar1 = *(ushort *)(iVar2 + 0x34) & 0x200;
  if ((*(ushort *)(iVar2 + 0x34) & 0x200) == 0) {
    do {
      uVar1 = uVar1 + 1;
      (*(code *)param_1[2])(param_1[3]);
      if ((*(ushort *)(iVar2 + 0x34) & 0x200) != 0) {
        iVar2 = *param_1;
        goto LAB_010228b8;
      }
    } while (uVar1 <= (uint)param_1[1]);
    FUN_0100ceac(s__s__Timed_out_waiting_for_stop_d_01042934,s_al_i2c_xfer_finish_01042894);
    FUN_01010bfc(s__s__Timed_out_waiting_for_stop_d_01042934,s_al_i2c_xfer_finish_01042894);
    uVar3 = 0xffffffc2;
    iVar2 = *param_1;
  }
  else {
LAB_010228b8:
    uVar3 = 0;
  }
  if ((*(uint *)(iVar2 + 0x70) & 1) != 0) {
    uVar1 = 0;
    do {
      uVar1 = uVar1 + 1;
      (*(code *)param_1[2])(param_1[3]);
      if ((*(uint *)(iVar2 + 0x70) & 1) == 0) goto LAB_01022900;
    } while (uVar1 <= (uint)param_1[1]);
    FUN_0100ceac(s__s__Timed_out_waiting_for_IDLE_s_01042960,s_al_i2c_xfer_finish_01042894);
    FUN_01010bfc(s__s__Timed_out_waiting_for_IDLE_s_01042960,s_al_i2c_xfer_finish_01042894);
    uVar3 = 0xffffffc2;
LAB_01022900:
    iVar2 = *param_1;
  }
  *(ushort *)(iVar2 + 0x6c) = *(ushort *)(iVar2 + 0x6c) & 0xfffe;
  return uVar3;
}



/* @ 0x1022a7c  FUN_01022a7c */

undefined4 al_i2c_perform_write(int *param_1,ushort param_2,byte *param_3,uint param_4)

{
  byte bVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  uint unaff_r5;
  int *unaff_r6;
  uint unaff_r8;
  byte *unaff_r9;
  bool bVar5;
  
  iVar4 = *param_1;
  *(ushort *)(iVar4 + 4) = *(ushort *)(iVar4 + 4) & 0xfc00 | param_2;
  *(ushort *)(iVar4 + 0x6c) = *(ushort *)(iVar4 + 0x6c) & 0xfffe | 1;
  if (param_4 == 0) {
    return 0;
  }
  bVar5 = param_4 != 0;
  if (bVar5) {
    unaff_r5 = 0;
    unaff_r8 = 1;
    unaff_r9 = param_3;
    unaff_r6 = param_1;
  }
  iVar4 = *param_1;
  do {
    if (!bVar5) {
      return 0;
    }
    while ((*(uint *)(iVar4 + 0x70) & 2) == 0) {
      bVar5 = (uint)unaff_r6[1] < unaff_r5;
      unaff_r5 = unaff_r5 + 1;
      if (bVar5) {
        FUN_0100ceac(s__s__Timed_out__i2c_write_failed_01042910,s_al_i2c_perform_write_0104287c);
        FUN_01010bfc(s__s__Timed_out__i2c_write_failed_01042910,s_al_i2c_perform_write_0104287c);
        return 0xffffffc2;
      }
      (*(code *)unaff_r6[2])(unaff_r6[3]);
    }
    uVar3 = unaff_r8;
    if (1 < param_4) {
      uVar3 = unaff_r8 | 1;
    }
    bVar1 = *unaff_r9;
    unaff_r5 = 0;
    unaff_r9 = unaff_r9 + 1;
    uVar2 = (ushort)bVar1;
    if (uVar3 == 0) {
      uVar2 = bVar1 | 0x200;
    }
    param_4 = param_4 - 1;
    bVar5 = param_4 != 0;
    *(ushort *)(iVar4 + 0x10) = uVar2;
  } while( true );
}



/* @ 0x1022ce4  FUN_01022ce4 */

int al_i2c_read(int *param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4,int param_5,
                undefined1 *param_6)

{
  int iVar1;
  undefined2 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  bool bVar6;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___i2c_if__010428e4,1,
                 s____HAL__drivers_pbs_al_hal_i2c_c_010428c0,s_al_i2c_read_010428b4,0x13e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___i2c_if__010428e4,1,
                 s____HAL__drivers_pbs_al_hal_i2c_c_010428c0,s_al_i2c_read_010428b4,0x13e);
    FUN_01000454(0);
  }
  iVar4 = *param_1;
  iVar1 = al_i2c_perform_write(param_1,param_2,param_4,param_3);
  if (iVar1 == 0) {
    if (param_5 != 0) {
      uVar3 = 0;
      iVar1 = param_5;
      iVar5 = param_5;
      do {
        while( true ) {
          if ((iVar1 != 0) && ((*(uint *)(iVar4 + 0x70) & 0x12) == 2)) {
            bVar6 = iVar1 == 1;
            iVar1 = iVar1 + -1;
            if (bVar6) {
              uVar2 = 0x300;
            }
            else {
              uVar2 = 0x100;
            }
            *(undefined2 *)(iVar4 + 0x10) = uVar2;
          }
          if ((*(uint *)(iVar4 + 0x70) & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) != 0) break;
          bVar6 = (uint)(param_1[1] * param_5) < uVar3;
          uVar3 = uVar3 + 1;
          if (bVar6) {
            al_i2c_xfer_finish(param_1);
            return -0x3e;
          }
          (*(code *)param_1[2])(param_1[3]);
        }
        iVar5 = iVar5 + -1;
        uVar3 = 0;
        *param_6 = (char)*(undefined2 *)(iVar4 + 0x10);
        param_6 = param_6 + 1;
      } while (iVar5 != 0);
    }
    iVar1 = al_i2c_xfer_finish(param_1);
    return iVar1;
  }
  return iVar1;
}



/* @ 0x1022e68  FUN_01022e68 */

void al_uart_handle_init(int *param_1,int param_2)

{
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_regs_base__01042a18,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_handle_init_01042988,0x37);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_regs_base__01042a18,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_handle_init_01042988,0x37);
    FUN_01000454(0);
  }
  *param_1 = param_2;
  return;
}



/* @ 0x1022efc  FUN_01022efc */

undefined4 al_uart_read(undefined4 *param_1,int param_2,undefined1 *param_3,uint param_4)

{
  undefined4 uVar1;
  uint uVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  uint uVar5;
  int iVar6;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x7f);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x7f);
    FUN_01000454(0);
  }
  if (param_3 == (undefined1 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer__01042a38,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x80);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer__01042a38,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x80);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bytelen__01042a44,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x81);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bytelen__01042a44,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_read_0104299c,0x81);
    FUN_01000454(0);
    uVar1 = 0;
  }
  else {
    param_1 = (undefined4 *)*param_1;
    iVar6 = 0;
    uVar5 = param_1[2];
    do {
      do {
        while( true ) {
          if ((uVar5 & 0xc0) == 0) {
            uVar2 = param_1[5] & 1;
          }
          else {
            uVar2 = param_1[0x21];
          }
          if (uVar2 == 0) break;
          puVar3 = param_3 + uVar2;
          puVar4 = param_3;
          do {
            param_2 = param_2 + -1;
            param_3 = puVar4 + 1;
            *puVar4 = (char)*param_1;
            puVar4 = param_3;
          } while (param_3 != puVar3 && param_2 != 0);
          if (((uint)(param_4 != 0xffffffff) & param_4 >> 0x1f) != 0) goto LAB_01022f7c;
          if (param_2 == 0) {
            return 0;
          }
          iVar6 = 0;
        }
      } while (param_4 == 0xffffffff);
      iVar6 = iVar6 + 1;
      udelay(1);
    } while (iVar6 <= (int)param_4);
LAB_01022f7c:
    FUN_0100ceac(s__s__Timed_out__01042a50,s_al_uart_read_0104299c);
    FUN_01010bfc(s__s__Timed_out__01042a50,s_al_uart_read_0104299c);
    uVar1 = 0xffffffc2;
  }
  return uVar1;
}



/* @ 0x1023154  FUN_01023154 */

undefined4 al_uart_write(undefined4 *param_1,int param_2,byte *param_3,uint param_4)

{
  undefined4 uVar1;
  int iVar2;
  byte *pbVar3;
  byte *pbVar4;
  uint *puVar5;
  uint uVar6;
  int iVar7;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xb9);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xb9);
    FUN_01000454(0);
  }
  if (param_3 == (byte *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer__01042a38,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xba);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___buffer__01042a38,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xba);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bytelen__01042a44,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xbb);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___bytelen__01042a44,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_write_010429ac,0xbb);
    FUN_01000454(0);
    uVar1 = 0;
  }
  else {
    puVar5 = (uint *)*param_1;
    iVar7 = 0;
    uVar6 = puVar5[2];
LAB_010231a4:
    do {
      do {
        if ((uVar6 & 0xc0) == 0) {
          if ((puVar5[5] & 0x20) != 0) {
            iVar2 = 1;
            goto LAB_0102322c;
          }
        }
        else {
          iVar2 = 0x40 - puVar5[0x20];
          if (iVar2 != 0) {
LAB_0102322c:
            pbVar3 = param_3 + iVar2;
            pbVar4 = param_3;
            do {
              param_3 = pbVar4 + 1;
              param_2 = param_2 + -1;
              *puVar5 = (uint)*pbVar4;
              pbVar4 = param_3;
            } while (param_3 != pbVar3 && param_2 != 0);
            if (((uint)(param_4 != 0xffffffff) & param_4 >> 0x1f) != 0) goto LAB_010231d4;
            if (param_2 == 0) {
              return 0;
            }
            iVar7 = 0;
            goto LAB_010231a4;
          }
        }
      } while (param_4 == 0xffffffff);
      iVar7 = iVar7 + 1;
      udelay(1);
    } while (iVar7 <= (int)param_4);
LAB_010231d4:
    FUN_0100ceac(s__s__Timed_out__01042a50,s_al_uart_write_010429ac);
    FUN_01010bfc(s__s__Timed_out__01042a50,s_al_uart_write_010429ac);
    uVar1 = 0xffffffc2;
  }
  return uVar1;
}



/* @ 0x10233b0  FUN_010233b0 */

int al_uart_tx_fifo_level_get(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_tx_fifo_level_get_010429bc,
                 0x18e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___uart_if__01042a2c,1,
                 s____HAL__drivers_pbs_al_hal_uart__010429f4,s_al_uart_tx_fifo_level_get_010429bc,
                 0x18e);
    FUN_01000454(0);
  }
  return 0x40 - (*(uint *)(*param_1 + 0x80) & 0xff);
}



/* @ 0x10234e4  FUN_010234e4 */

undefined4 FUN_010234e4(int param_1,undefined4 param_2,int *param_3,int *param_4)

{
  switch(param_2) {
  case 0:
    *param_3 = param_1 + 4;
    *param_4 = param_1 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    return 0;
  case 1:
    *param_3 = param_1 + AL_I2C_TAR_10BIT_ADDR_SHIFT;
    *param_4 = param_1 + 0x10;
    return 0;
  case 2:
    *param_3 = param_1 + 0x14;
    *param_4 = param_1 + 0x18;
    return 0;
  case 3:
    *param_3 = param_1 + 0x1c;
    *param_4 = param_1 + 0x20;
    return 0;
  case 4:
    *param_3 = param_1 + 0x24;
    *param_4 = param_1 + 0x28;
    return 0;
  case 5:
    *param_3 = param_1 + 0x2c;
    *param_4 = param_1 + 0x30;
    return 0;
  case 6:
    *param_3 = param_1 + 0x34;
    *param_4 = param_1 + 0x38;
    return 0;
  case 7:
    *param_3 = param_1 + 0x3c;
    *param_4 = param_1 + 0x40;
    return 0;
  case 8:
    *param_3 = param_1 + 0x44;
    *param_4 = param_1 + 0x48;
    return 0;
  case 9:
    *param_3 = param_1 + 0x4c;
    *param_4 = param_1 + 0x50;
    return 0;
  case 10:
    *param_3 = param_1 + 0x54;
    *param_4 = param_1 + 0x58;
    return 0;
  case 0xb:
    *param_3 = param_1 + 0x5c;
    *param_4 = param_1 + 0x60;
    return 0;
  case 0xc:
    *param_3 = param_1 + 100;
    *param_4 = param_1 + 0x68;
    return 0;
  case 0xd:
    *param_3 = param_1 + 0x1e8;
    *param_4 = param_1 + 0x1ec;
    return 0;
  case 0xe:
    *param_3 = param_1 + 0x6c;
    *param_4 = param_1 + 0x70;
    return 0;
  case 0xf:
    *param_3 = param_1 + 0x74;
    *param_4 = param_1 + 0x78;
    return 0;
  case 0x10:
    *param_3 = param_1 + 0x7c;
    *param_4 = param_1 + 0x80;
    return 0;
  case 0x11:
    *param_3 = param_1 + 0x200;
    *param_4 = param_1 + 0x204;
    return 0;
  case 0x12:
    *param_3 = param_1 + 0x8c;
    *param_4 = param_1 + 0x90;
    return 0;
  case 0x13:
    *param_3 = param_1 + 0xa0;
    *param_4 = param_1 + 0xa4;
    return 0;
  case 0x14:
    *param_3 = param_1 + 0x84;
    *param_4 = param_1 + 0x88;
    return 0;
  case 0x15:
    *param_3 = param_1 + 0xa8;
    *param_4 = param_1 + 0xac;
    return 0;
  case 0x16:
    *param_3 = param_1 + 0xb0;
    *param_4 = param_1 + 0xb4;
    return 0;
  case 0x17:
    *param_3 = param_1 + 0xb8;
    *param_4 = param_1 + 0xbc;
    return 0;
  case 0x18:
    *param_3 = param_1 + 0xc0;
    *param_4 = param_1 + 0xc4;
    return 0;
  case 0x19:
    *param_3 = param_1 + 200;
    *param_4 = param_1 + 0xcc;
    return 0;
  case 0x1a:
    *param_3 = param_1 + 0xd0;
    *param_4 = param_1 + 0xd4;
    return 0;
  case 0x1b:
    *param_3 = param_1 + 0x148;
    *param_4 = param_1 + 0x14c;
    return 0;
  default:
    FUN_0100ceac(s_addr_map__unknown_pasw__d_01042aec);
    FUN_01010bfc(s_addr_map__unknown_pasw__d_01042aec,param_2);
    return 0xffffffea;
  }
}



/* @ 0x1023838  FUN_01023838 */

undefined4 al_addr_map_dram_remap_set(int param_1,undefined4 param_2,uint param_3,uint param_4,uint param_5)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 extraout_r1;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  bool bVar7;
  uint *local_30;
  uint *apuStack_2c [2];
  
  iVar2 = FUN_01021fd4();
  if (iVar2 == 0) {
    uVar4 = 0x28;
  }
  else {
    uVar4 = 0x30;
  }
  if (uVar4 < param_5) {
    FUN_0100ceac(s_addr_map__max_pasw_log2size_is___01042b44,uVar4);
    FUN_01010bfc(s_addr_map__max_pasw_log2size_is___01042b44,uVar4);
    uVar3 = 0xffffffea;
  }
  else if (param_5 < 0xf) {
    FUN_0100ceac(s_addr_map__min_pasw_log2size_is_1_01042b68);
    FUN_01010bfc(s_addr_map__min_pasw_log2size_is_1_01042b68);
    uVar3 = 0xffffffea;
  }
  else {
    uVar5 = 0 << (param_5 & 0xff) | 1 << (param_5 - 0x20 & 0xff) | 1U >> (0x20 - param_5 & 0xff);
    uVar1 = 1 << (param_5 & 0xff);
    if ((param_5 == 0xf) ||
       (uVar6 = uVar1 - 1 & param_3 | uVar5 - (uVar1 == 0) & param_4, uVar6 == 0)) {
      uVar5 = param_4 + uVar5 + (uint)CARRY4(param_3,uVar1);
      uVar6 = 0 << uVar4 | 1 << (uVar4 - 0x20 & 0xff) | 1U >> (0x20 - uVar4 & 0xff);
      bVar7 = uVar6 <= uVar5;
      if (uVar5 == uVar6) {
        bVar7 = (uint)(1 << uVar4) <= param_3 + uVar1;
      }
      if (bVar7 && (uVar5 != uVar6 || param_3 + uVar1 != 1 << uVar4)) {
        FUN_0100ceac(s_addr_map__base___size_doesn_t_fi_01042bf8,uVar4);
        FUN_01010bfc(s_addr_map__base___size_doesn_t_fi_01042bf8,uVar4);
        uVar3 = 0xffffffea;
      }
      else {
        iVar2 = FUN_010234e4(param_1,param_2,&local_30,apuStack_2c);
        if (iVar2 == 0) {
          *(undefined4 *)(param_1 + 0xdc) = 1;
          *local_30 = param_4;
          *apuStack_2c[0] = param_5 - 0xf | param_3;
          *(undefined4 *)(param_1 + 0xdc) = 0;
          uVar3 = 0;
        }
        else {
          uVar3 = 0xffffffea;
        }
      }
    }
    else {
      FUN_0100ceac(s_addr_map__pasw_base_has_to_be_al_01042b9c,uVar6,param_3,param_4,param_5);
      FUN_01010bfc(s_addr_map__pasw_base_has_to_be_al_01042b9c,extraout_r1,param_3,param_4,param_5);
      uVar3 = 0xffffffea;
    }
  }
  return uVar3;
}



/* @ 0x1023a0c  FUN_01023a0c */

undefined4
FUN_01023a0c(int param_1,undefined4 param_2,uint param_3,uint param_4,uint param_5,int param_6,
            byte param_7)

{
  if (3 < param_7) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___window_size_<_4)_01042a7c,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x149);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___window_size_<_4)_01042a7c,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x149);
    FUN_01000454(0);
  }
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14a);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14a);
    FUN_01000454(0);
  }
  if (param_4 == 0 && param_3 < 0x20000000) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___dram_remap_base_>___1_<<_29))_01042aa4,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___dram_remap_base_>___1_<<_29))_01042aa4,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14c);
    FUN_01000454(0);
  }
  if (param_6 == 0 && param_5 < 0x20000000) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___dram_remap_transl_base_>___1_<_01042ac4,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___dram_remap_transl_base_>___1_<_01042ac4,1,
                 s____HAL__drivers_pbs_al_hal_addr__01042b08,s_al_addr_map_dram_remap_set_01042a60,
                 0x14e);
    FUN_01000454(0);
  }
  *(undefined4 *)(param_1 + 0xdc) = 1;
  *(uint *)(param_1 + 0x134) =
       (param_3 >> 0x1d | (param_4 & 0xff) << 3) << 5 | (uint)param_7 |
       (param_5 >> 0x1d | param_6 << 3) << 0x15;
  *(undefined4 *)(param_1 + 0xdc) = 0;
  return 0;
}



/* @ 0x1023c48  FUN_01023c48 */

uint FUN_01023c48(undefined4 *param_1)

{
  uint uVar1;
  uint uVar2;
  uint *puVar3;
  int iVar4;
  
  puVar3 = (uint *)*param_1;
  if ((int)puVar3[7] < 0) {
    uVar1 = param_1[4];
    if (-1 < (int)puVar3[6]) {
      uVar2 = *puVar3;
      iVar4 = ((uVar2 & 0xfffffff) >> 0x18) + 1;
      return ((uVar2 & 0x1fff) * uVar1 + uVar1) / (((uVar2 & 0x3fffff) >> 0x10) * iVar4 + iVar4);
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}



/* @ 0x1023c94  FUN_01023c94 */

undefined4 al_pll_init(int param_1,int param_2,int param_3,int *param_4)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x71);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___regs_base__010326e8,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x71);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___name__01043294,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x72);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___name__01043294,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x72);
    FUN_01000454(0);
  }
  if (param_4 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x73);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_init_01042c24,0x73);
    FUN_01000454(0);
  }
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    uVar1 = 0;
    param_4[2] = (int)&DAT_01042cd8;
    param_4[3] = 0x14;
    param_4[4] = 100000;
  }
  else if ((char)param_3 == '\0') {
    param_4[2] = (int)&DAT_01043080;
    param_4[3] = 0x13;
    param_4[4] = 25000;
    uVar1 = 0;
  }
  else if (param_3 == 2) {
    uVar1 = 0;
    param_4[2] = (int)&DAT_01042eb8;
    param_4[3] = 0x13;
    param_4[4] = 0x1e848;
  }
  else {
    FUN_0100ceac(s__s__invalid_ref_clk_freq_enum____0104329c,s_al_pll_init_01042c24,param_3);
    FUN_01010bfc(s__s__invalid_ref_clk_freq_enum____0104329c,s_al_pll_init_01042c24,param_3);
    uVar1 = 0xffffffea;
  }
  return uVar1;
}



/* @ 0x1023ed0  FUN_01023ed0 */

undefined4 al_pll_freq_get(int param_1,undefined1 *param_2,int *param_3)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_freq_get_01042c30,0xb8);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_freq_get_01042c30,0xb8);
    FUN_01000454(0);
  }
  puVar4 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar2 = FUN_01023c48(param_1);
  *param_3 = iVar2;
  if (0 < *(int *)(param_1 + 0xc)) {
    if (iVar2 != *(int *)(puVar4 + 4)) {
      iVar3 = 0;
      puVar5 = puVar4;
      do {
        iVar3 = iVar3 + 1;
        puVar4 = puVar5 + 0x18;
        if (iVar3 == *(int *)(param_1 + 0xc)) {
          return 0;
        }
        piVar1 = (int *)(puVar5 + 0x1c);
        puVar5 = puVar4;
      } while (iVar2 != *piVar1);
    }
    *param_2 = *puVar4;
  }
  return 0;
}



/* @ 0x1023fd0  FUN_01023fd0 */

undefined4 al_pll_channel_freq_get(int *param_1,uint param_2,uint *param_3)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x164);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x164);
    FUN_01000454(0);
  }
  if (param_3 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___freq__01042c9c,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x165);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___freq__01042c9c,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x165);
    FUN_01000454(0);
  }
  if (0xf < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___chan_idx_<_16)_01042ca4,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x166);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___chan_idx_<_16)_01042ca4,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_freq_get_01042c40,
                 0x166);
    FUN_01000454(0);
  }
  iVar5 = *param_1;
  uVar1 = 0;
  *param_3 = 0;
  if (((*(uint *)(iVar5 + 0x1c) & 0xfff) >> 8) - 6 < 2) {
    uVar2 = FUN_01023c48(param_1);
    uVar3 = *(uint *)(*param_1 + ((param_2 >> 1) + AL_I2C_INTR_MASK_ACTIVITY_SHIFT) * 4);
    if ((param_2 & 1) == 0) {
      uVar3 = uVar3 & 0xffff;
    }
    else {
      uVar3 = uVar3 >> 0x10;
    }
    uVar4 = uVar3 & 0x3ff;
    iVar5 = -((int)(short)uVar3 >> 0x1f);
    iVar6 = iVar5;
    if (uVar4 != 0) {
      iVar6 = 0;
    }
    if (iVar6 == 0) {
      if (uVar4 == 0 && iVar5 == 0) {
        *param_3 = 0;
        uVar1 = 0;
      }
      else {
        uVar3 = (uint)~(int)(short)uVar3 >> 0x1f;
        if (uVar4 == 0) {
          uVar3 = 0;
        }
        if (uVar3 == 0) {
          FUN_0100ceac(s__s__div_val____ref_clk_bypass__01042cb8,s_al_pll_channel_freq_get_01042c40)
          ;
          FUN_01010bfc(s__s__div_val____ref_clk_bypass__01042cb8,s_al_pll_channel_freq_get_01042c40)
          ;
          uVar1 = 0xfffffffb;
        }
        else {
          *param_3 = uVar2 / uVar4;
          uVar1 = 0;
        }
      }
    }
    else {
      uVar1 = 0;
      *param_3 = uVar2;
    }
  }
  return uVar1;
}



/* @ 0x1024218  FUN_01024218 */

undefined4
FUN_01024218(int *param_1,uint param_2,uint param_3,int param_4,int param_5,int param_6,int param_7)

{
  int iVar1;
  uint uVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_div_set_01042c58,0x1a2
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_div_set_01042c58,0x1a2
                );
    FUN_01000454(0);
  }
  if (0xf < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___chan_idx_<_16)_01042ca4,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_div_set_01042c58,0x1a3
                );
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___chan_idx_<_16)_01042ca4,1,
                 s____HAL__drivers_ring_al_hal_pll__01043270,s_al_pll_channel_div_set_01042c58,0x1a3
                );
    FUN_01000454(0);
  }
  if ((param_3 & 0xfffffc00) != 0) {
    FUN_0100ceac(s__s__requested_divider_too_big____01042c70,s_al_pll_channel_div_set_01042c58,
                 param_3,0x3ff);
    FUN_01010bfc(s__s__requested_divider_too_big____01042c70,s_al_pll_channel_div_set_01042c58,
                 param_3,0x3ff);
    return 0xffffffea;
  }
  param_3 = param_3 & 0xffff;
  if (param_4 != 0) {
    param_3 = param_3 | 0x800;
  }
  if (param_5 == 0) {
    param_3 = param_3 | 0x4000;
  }
  iVar1 = *param_1 + (param_2 >> 1) * 4;
  if ((param_2 & 1) == 0) {
    *(uint *)(iVar1 + 0x20) = param_3 | 0x1000 | *(uint *)(iVar1 + 0x20) & 0xffff0000;
    if (param_6 == 0) {
      return 0;
    }
    uVar2 = param_3 | 0x3000 | *(uint *)(iVar1 + 0x20) & 0xffff0000;
  }
  else {
    *(uint *)(iVar1 + 0x20) = *(uint *)(iVar1 + 0x20) & 0xffff | (param_3 | 0x1000) << 0x10;
    if (param_6 == 0) {
      return 0;
    }
    uVar2 = *(uint *)(iVar1 + 0x20) & 0xffff | (param_3 | 0x3000) << 0x10;
  }
  *(uint *)(iVar1 + 0x20) = uVar2;
  udelay(3);
  iVar1 = *param_1;
  if (param_7 != 0 && 1 < ((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6) {
    do {
      udelay(1);
      iVar1 = *param_1;
      param_7 = param_7 + -1;
    } while (1 < ((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6 && param_7 != 0);
  }
  if (((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6 < 2) {
    return 0;
  }
  FUN_0100ceac(s__s__timed_out__01043260,s_al_pll_channel_div_set_01042c58);
  FUN_01010bfc(s__s__timed_out__01043260,s_al_pll_channel_div_set_01042c58);
  return 0xffffff8c;
}



/* @ 0x10244b4  FUN_010244b4 */

undefined4 al_timer_init(int *param_1,int param_2,undefined1 param_3,undefined1 param_4)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_init_010432dc,0x62);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_init_010432dc,0x62);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___timer_regs_base______void___0__01043394,1,
                 s____HAL__drivers_sys_services_al__0104334c,s_al_timer_init_010432dc,99);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___timer_regs_base______void___0__01043394,1,
                 s____HAL__drivers_sys_services_al__0104334c,s_al_timer_init_010432dc,99);
    FUN_01000454(0);
  }
  *param_1 = param_2;
  *(undefined1 *)(param_1 + 1) = param_3;
  *(undefined1 *)((int)param_1 + 5) = param_4;
  return 0;
}



/* @ 0x10245d0  FUN_010245d0 */

void al_timer_config_set(int *param_1,int param_2,uint param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_config_set_010432ec,0x77);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_config_set_010432ec,0x77);
    FUN_01000454(0);
  }
  if (param_3 < 3) {
    uVar1 = *(uint *)(&DAT_010432d0 + param_3 * 4);
    uVar2 = ~(*(uint *)(&DAT_010432c4 + param_3 * 4) | 0xe);
  }
  else {
    uVar2 = 0xfffffff1;
    uVar1 = 0;
  }
  if (param_2 == 1) {
    uVar1 = uVar1 | 2;
  }
  if (param_4 != 0) {
    if (param_4 == 1) {
      uVar1 = uVar1 | 4;
    }
    else {
      uVar1 = uVar1 | AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
  }
  *(uint *)(*param_1 + 8) = *(uint *)(*param_1 + 8) & uVar2 | uVar1;
  return;
}



/* @ 0x10246d8  FUN_010246d8 */

void al_timer_load_set(undefined4 *param_1,undefined4 param_2)

{
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_load_set_01043300,0x8c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_load_set_01043300,0x8c);
    FUN_01000454(0);
  }
  *(undefined4 *)*param_1 = param_2;
  return;
}



/* @ 0x1024770  FUN_01024770 */

void al_timer_int_enable(int *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_int_enable_01043314,0xa9);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_int_enable_01043314,0xa9);
    FUN_01000454(0);
  }
  if (param_2 == 1) {
    uVar1 = 0x20;
  }
  else {
    uVar1 = 0;
  }
  *(uint *)(*param_1 + 8) = *(uint *)(*param_1 + 8) & 0xffffffdf | uVar1;
  return;
}



/* @ 0x1024820  FUN_01024820 */

void al_timer_enable(int *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_enable_01043328,0xbf);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_enable_01043328,0xbf);
    FUN_01000454(0);
  }
  if (param_2 == 1) {
    uVar1 = 0x80;
  }
  else {
    uVar1 = 0;
  }
  *(uint *)(*param_1 + 8) = *(uint *)(*param_1 + 8) & 0xffffff7f | uVar1;
  return;
}



/* @ 0x10248d0  FUN_010248d0 */

undefined4 al_timer_value_get(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_value_get_01043338,0xf8);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___sub_timer______void___0___01043378,
                 1,s____HAL__drivers_sys_services_al__0104334c,s_al_timer_value_get_01043338,0xf8);
    FUN_01000454(0);
  }
  return *(undefined4 *)(*param_1 + 4);
}



/* @ 0x1024964  FUN_01024964 */


undefined4 al_thermal_sensor_handle_init_ex(int *param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x3f);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x3f);
    FUN_01000454(0);
  }
  if (param_2 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___params__010434f4,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x40);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___params__010434f4,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x40);
    FUN_01000454(0);
    iVar2 = _UndefinedInstruction;
  }
  else {
    iVar2 = param_2[1];
  }
  if (iVar2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___params_>thermal_sensor_reg_bas_01043500,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x41);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                 s___params_>thermal_sensor_reg_bas_01043500,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x41);
    FUN_01000454(0);
    iVar2 = *param_2;
  }
  else {
    iVar2 = *param_2;
  }
  if (iVar2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___params_>pbs_regs_base__01043524,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x42);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___params_>pbs_regs_base__01043524,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_handle_init_ex_010433b8,0x42);
    FUN_01000454(0);
    iVar2 = *param_2;
  }
  *param_1 = param_2[1];
  iVar2 = FUN_01021fd4(iVar2);
  if (iVar2 == 0) {
    iVar2 = 0xdc3;
    iVar1 = 0x482;
  }
  else {
    iVar2 = 0xdc0;
    iVar1 = 0x442;
  }
  param_1[3] = iVar2;
  iVar2 = param_2[2];
  param_1[2] = iVar1;
  param_1[4] = iVar2;
  return 0;
}



/* @ 0x1024b8c  FUN_01024b8c */

void FUN_01024b8c(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  local_c = 0;
  local_14 = param_3;
  local_10 = param_2;
  al_thermal_sensor_handle_init_ex(param_1,&local_14);
  return;
}



/* @ 0x1024bb4  FUN_01024bb4 */

void al_thermal_sensor_trim_set(undefined4 *param_1,uint param_2)

{
  uint uVar1;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_trim_set_010433dc,
                 0x6c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_trim_set_010433dc,
                 0x6c);
    FUN_01000454(0);
  }
  uVar1 = *(uint *)*param_1;
  *(uint *)*param_1 = (param_2 ^ uVar1) & 0xf ^ uVar1;
  return;
}



/* @ 0x1024c5c  FUN_01024c5c */

void al_thermal_sensor_enable_set(int *param_1,int param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_enable_set_010433f8
                 ,0xd0);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_enable_set_010433f8
                 ,0xd0);
    FUN_01000454(0);
  }
  puVar2 = (uint *)*param_1;
  if (param_2 == 0) {
    uVar1 = 0x10;
    puVar2[2] = 0;
  }
  else {
    uVar1 = 0x20;
  }
  *puVar2 = *puVar2 & 0xffffffcf | uVar1;
  return;
}



/* @ 0x1024d10  FUN_01024d10 */

bool al_thermal_sensor_is_ready(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_is_ready_01043418,
                 0xe9);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,s_al_thermal_sensor_is_ready_01043418,
                 0xe9);
    FUN_01000454(0);
  }
  return (*(uint *)(*param_1 + 0xc) & 0x60000000) == 0x60000000;
}



/* @ 0x1024db4  FUN_01024db4 */

void al_thermal_sensor_trigger_continuous(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_trigger_contin_01043434,0x10c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_trigger_contin_01043434,0x10c);
    FUN_01000454(0);
  }
  *(undefined4 *)(*param_1 + 8) = 2;
  return;
}



/* @ 0x1024e4c  FUN_01024e4c */

int al_thermal_sensor_readout_is_valid(int *param_1)

{
  int iVar1;
  uint uVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_readout_is_val_0104345c,0x11d);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___thermal_sensor_handle__010434d8,1,
                 s____HAL__drivers_sys_services_al__010434a0,
                 s_al_thermal_sensor_readout_is_val_0104345c,0x11d);
    FUN_01000454(0);
  }
  uVar2 = *(uint *)(*param_1 + 0xc);
  if ((~uVar2 & 0x60008000) == 0) {
    iVar1 = 1;
    param_1[4] = 1;
  }
  else {
    iVar1 = param_1[4];
    if (iVar1 == 0) {
      return 0;
    }
  }
  param_1[1] = uVar2 & 0xfff;
  return iVar1;
}



/* @ 0x1024fd8  FUN_01024fd8 */

undefined4 al_otp_handle_init(int *param_1,int param_2,int param_3)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_handle__01043594,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x33);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_handle__01043594,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x33);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_regs_base__010435a4,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x34);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_regs_base__010435a4,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x34);
    FUN_01000454(0);
  }
  if (param_3 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x35);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_handle_init_01043540,0x35);
    FUN_01000454(0);
  }
  *param_1 = param_2;
  param_1[1] = param_3;
  return 0;
}



/* @ 0x1025158  FUN_01025158 */

undefined4 al_otp_read_word(int *param_1,uint param_2)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_handle__01043594,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_read_word_01043554,0x45);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___otp_handle__01043594,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_read_word_01043554,0x45);
    FUN_01000454(0);
  }
  if (0x1e < param_2) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___word_idx_<_31)_010435b8,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_read_word_01043554,0x46);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___word_idx_<_31)_010435b8,1,
                 s____HAL__drivers_sys_services_al__01043568,s_al_otp_read_word_01043554,0x46);
    FUN_01000454(0);
  }
  return *(undefined4 *)(*param_1 + (param_2 + 0x20) * 4);
}



/* @ 0x102525c  FUN_0102525c */

void al_sys_fabric_handle_init(uint *param_1,uint param_2)

{
  if (param_1 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,s_al_sys_fabric_handle_init_010435cc,
                 0x16);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,s_al_sys_fabric_handle_init_010435cc,
                 0x16);
    FUN_01000454(0);
  }
  *param_1 = (*(uint *)(param_2 + 0x4400) & 0xffff) >> 8;
  param_1[1] = param_2;
  return;
}



/* @ 0x10252fc  FUN_010252fc */

void al_sys_fabric_cluster_handle_init(int *param_1,uint *param_2,int param_3)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_handle_ini_010435e8,0x26);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_handle_ini_010435e8,0x26);
    FUN_01000454(0);
  }
  if (param_2 == (uint *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___fabric_handle__010436c8,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_handle_ini_010435e8,0x27);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___fabric_handle__010436c8,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_handle_ini_010435e8,0x27);
    FUN_01000454(0);
  }
  if (*param_2 < 4) {
    if (param_3 == 0) goto LAB_01025398;
  }
  else if (param_3 != 0) goto LAB_01025398;
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
               s_____fabric_handle_>ver_<__3)_&&_(_010436dc,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_cluster_handle_ini_010435e8,0x2c);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
               s_____fabric_handle_>ver_<__3)_&&_(_010436dc,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_cluster_handle_ini_010435e8,0x2c);
  FUN_01000454(0);
LAB_01025398:
  *param_1 = (int)param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0x1025480  FUN_01025480 */

void al_sys_fabric_cluster_pd_pu_timer_set(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_pd_pu_time_0104360c,0x18b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_cluster_pd_pu_time_0104360c,0x18b);
    FUN_01000454(0);
  }
  if (*(uint *)*param_1 < 4) {
    uVar1 = ((uint *)*param_1)[1];
    *(undefined4 *)(uVar1 + 0x100) = param_2;
    *(undefined4 *)(uVar1 + 0x104) = param_3;
    return;
  }
  FUN_0100ceac(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_cluster_pd_pu_time_0104360c);
  FUN_01010bfc(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_cluster_pd_pu_time_0104360c,*(undefined4 *)*param_1);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_cluster_pd_pu_time_0104360c,0x195);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_cluster_pd_pu_time_0104360c,0x195);
  FUN_01000454(0);
  return;
}



/* @ 0x10255dc  FUN_010255dc */

void al_sys_fabric_core_power_on_reset(int *param_1,uint param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_power_on_rese_01043634,499);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_power_on_rese_01043634,499);
    FUN_01000454(0);
  }
  if (param_2 < 4) {
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_power_on_rese_01043634,500);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_power_on_rese_01043634,500);
    FUN_01000454(0);
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  if (uVar1 < 4) {
    *(uint *)(puVar2[1] + 0x24) = 1 << (param_2 + 4 & 0xff) | 3;
    return;
  }
  FUN_0100ceac(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_power_on_rese_01043634);
  FUN_01010bfc(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_power_on_rese_01043634,*(undefined4 *)*param_1);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_power_on_rese_01043634,0x201);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_power_on_rese_01043634,0x201);
  FUN_01000454(0);
  return;
}



/* @ 0x10257a0  FUN_010257a0 */

void al_sys_fabric_core_reset_deassert(int *param_1,uint param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_reset_deasser_01043658,0x20b);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_reset_deasser_01043658,0x20b);
    FUN_01000454(0);
  }
  if (param_2 < 4) {
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_reset_deasser_01043658,0x20c);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_reset_deasser_01043658,0x20c);
    FUN_01000454(0);
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  if (uVar1 < 4) {
    uVar1 = *(uint *)(puVar2[1] + 8);
    *(uint *)(puVar2[1] + 8) = ~uVar1 & 1 << (param_2 & 0xff) ^ uVar1;
    return;
  }
  FUN_0100ceac(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_reset_deasser_01043658);
  FUN_01010bfc(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_reset_deasser_01043658,*(undefined4 *)*param_1);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_reset_deasser_01043658,0x216);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_reset_deasser_01043658,0x216);
  FUN_01000454(0);
  return;
}



/* @ 0x1025968  FUN_01025968 */

void al_sys_fabric_core_aarch64_setup(int *param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch64_setup_0104367c,0x222);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch64_setup_0104367c,0x222);
    FUN_01000454(0);
  }
  if (param_2 < 4) {
    if (param_3 == 0 && param_4 == 0) {
LAB_01025a5c:
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___entry_high____entry_low__01043744
                   ,1,s____HAL__drivers_sys_fabric_al_ha_010437ac,
                   s_al_sys_fabric_core_aarch64_setup_0104367c,0x224);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___entry_high____entry_low__01043744
                   ,1,s____HAL__drivers_sys_fabric_al_ha_010437ac,
                   s_al_sys_fabric_core_aarch64_setup_0104367c,0x224);
      FUN_01000454(param_3 | param_4);
      puVar2 = (uint *)*param_1;
      uVar1 = *puVar2;
      goto joined_r0x01025ac4;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch64_setup_0104367c,0x223);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch64_setup_0104367c,0x223);
    FUN_01000454(0);
    if (param_3 == 0 && param_4 == 0) goto LAB_01025a5c;
  }
  puVar2 = (uint *)*param_1;
  uVar1 = *puVar2;
joined_r0x01025ac4:
  if (uVar1 < 4) {
    uVar1 = puVar2[1];
    iVar3 = uVar1 + param_2 * 0x100;
    *(uint *)(iVar3 + 0x2048) = param_4;
    *(uint *)(iVar3 + 0x204c) = param_3;
    *(undefined4 *)(uVar1 + (param_2 + 0x20) * 0x100 + 4) = 1;
    return;
  }
  FUN_0100ceac(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_aarch64_setup_0104367c);
  FUN_01010bfc(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_aarch64_setup_0104367c,*(undefined4 *)*param_1);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_aarch64_setup_0104367c,0x235);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_aarch64_setup_0104367c,0x235);
  FUN_01000454(0);
  return;
}



/* @ 0x1025bc8  FUN_01025bc8 */

void al_sys_fabric_core_aarch32_setup_get(int *param_1,uint param_2,undefined4 *param_3,undefined4 *param_4)

{
  int iVar1;
  uint uVar2;
  uint *puVar3;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch32_setup_010436a0,0x25d);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___handle__010437e4,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch32_setup_010436a0,0x25d);
    FUN_01000454(0);
  }
  if (param_2 < 4) {
    if (param_3 == (undefined4 *)0x0 && param_4 == (undefined4 *)0x0) {
LAB_01025cb4:
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___entry_high____entry_low__01043744
                   ,1,s____HAL__drivers_sys_fabric_al_ha_010437ac,
                   s_al_sys_fabric_core_aarch32_setup_010436a0,0x25f);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___entry_high____entry_low__01043744
                   ,1,s____HAL__drivers_sys_fabric_al_ha_010437ac,
                   s_al_sys_fabric_core_aarch32_setup_010436a0,0x25f);
      FUN_01000454((uint)param_3 | (uint)param_4);
      puVar3 = (uint *)*param_1;
      uVar2 = *puVar3;
      goto joined_r0x01025d1c;
    }
  }
  else {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch32_setup_010436a0,0x25e);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___core_<_4)_01043760,1,
                 s____HAL__drivers_sys_fabric_al_ha_010437ac,
                 s_al_sys_fabric_core_aarch32_setup_010436a0,0x25e);
    FUN_01000454(0);
    if (param_3 == (undefined4 *)0x0 && param_4 == (undefined4 *)0x0) goto LAB_01025cb4;
  }
  puVar3 = (uint *)*param_1;
  uVar2 = *puVar3;
joined_r0x01025d1c:
  if (uVar2 < 4) {
    iVar1 = puVar3[1] + param_2 * 0x100;
    *param_4 = *(undefined4 *)(iVar1 + 0x2028);
    *param_3 = *(undefined4 *)(iVar1 + 0x202c);
    return;
  }
  FUN_0100ceac(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_aarch32_setup_010436a0);
  FUN_01010bfc(s__s__not_supported_NB_version___u_0104376c,
               s_al_sys_fabric_core_aarch32_setup_010436a0,*(undefined4 *)*param_1);
  FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_aarch32_setup_010436a0,0x269);
  FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,&DAT_010323f0,1,
               s____HAL__drivers_sys_fabric_al_ha_010437ac,
               s_al_sys_fabric_core_aarch32_setup_010436a0,0x269);
  FUN_01000454(0);
  return;
}



/* @ 0x1025e20  FUN_01025e20 */

undefined4 al_unit_adapter_handle_init(int param_1,int param_2,byte *param_3)

{
  int iVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  int iVar10;
  
  uVar6 = *(uint *)(param_3 + 0xc);
  uVar9 = *(uint *)(param_3 + 0x10);
  iVar10 = *(int *)(param_1 + 0x4c);
  if (iVar10 != 0) {
    puVar2 = *(uint **)(param_1 + 8);
    uVar8 = *(uint *)(param_3 + 4);
    uVar7 = *(uint *)(param_3 + 8);
    puVar3 = puVar2;
    do {
      *puVar3 = uVar9 | 0xc800000;
      puVar3[1] = uVar6 | 0x100000;
      puVar3[2] = uVar8;
      puVar4 = puVar3 + 4;
      puVar3[3] = uVar7;
      puVar3 = puVar4;
    } while (puVar2 + iVar10 * 4 != puVar4);
  }
  iVar10 = *(int *)(param_2 + 0x4c);
  if ((*param_3 & 1) != 0) {
    uVar9 = uVar9 | 0x10000000;
  }
  if ((*param_3 & AL_I2C_INTR_MASK_ACTIVITY_SHIFT) != 0) {
    uVar9 = uVar9 | 0x20000000;
  }
  if (iVar10 != 0) {
    iVar1 = *(int *)(param_2 + 8);
    iVar5 = 0;
    do {
      *(uint *)(iVar1 + iVar5 * 0x10) = uVar9;
      iVar5 = iVar5 + 1;
    } while (iVar5 != iVar10);
  }
  return 0;
}



/* @ 0x1025edc  FUN_01025edc */

undefined4
FUN_01025edc(undefined1 *param_1,undefined1 param_2,int param_3,undefined1 *param_4,
            undefined1 *param_5,undefined4 param_6,int param_7)

{
  if (param_1 == (undefined1 *)0x0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___unit_adapter__0104383c,1,
                 s____HAL__drivers_io_fabric_al_hal_0104380c,s_al_unit_adapter_handle_init_010437f0,
                 0x90);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___unit_adapter__0104383c,1,
                 s____HAL__drivers_io_fabric_al_hal_0104380c,s_al_unit_adapter_handle_init_010437f0,
                 0x90);
    FUN_01000454(0);
  }
  *param_1 = param_2;
  if (param_7 == 0) {
    param_7 = param_3;
  }
  *(int *)(param_1 + 4) = param_3;
  if (param_4 == (undefined1 *)0x0) {
    param_4 = &LAB_01025ebc;
  }
  if (param_5 == (undefined1 *)0x0) {
    param_5 = &LAB_01025ecc;
  }
  *(undefined1 **)(param_1 + 8) = param_4;
  *(undefined1 **)(param_1 + 0xc) = param_5;
  *(undefined4 *)(param_1 + 0x10) = param_6;
  *(int *)(param_1 + 0x14) = param_7;
  return 0;
}



/* @ 0x1025fd4  FUN_01025fd4 */

void FUN_01025fd4(int param_1,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  uint local_14;
  
  iVar2 = param_2 * 0x20 + 0x114;
  (**(code **)(param_1 + 8))(*(undefined4 *)(param_1 + 0x14),iVar2,&local_14);
  if (param_3 == 0) {
    uVar1 = 0x100;
  }
  else {
    uVar1 = 0;
  }
  (**(code **)(param_1 + 0xc))(*(undefined4 *)(param_1 + 0x14),iVar2,local_14 & 0xfffffeff | uVar1);
  return;
}



/* @ 0x1026044  FUN_01026044 */

void FUN_01026044(int param_1,int param_2,int param_3,int param_4,int param_5)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint local_24 [2];
  
  (**(code **)(param_1 + 8))(*(undefined4 *)(param_1 + 0x14),0x240,local_24);
  (**(code **)(param_1 + 0xc))(*(undefined4 *)(param_1 + 0x14),0x240,local_24[0] | 0x80008000);
  (**(code **)(param_1 + 8))(*(undefined4 *)(param_1 + 0x14),0x240,local_24);
  (**(code **)(param_1 + 0xc))(*(undefined4 *)(param_1 + 0x14),0x240,local_24[0] & 0x7fff7fff);
  (**(code **)(param_1 + 8))(*(undefined4 *)(param_1 + 0x14),0x240,local_24);
  if (param_3 == 0) {
    uVar2 = 0;
  }
  else {
    uVar2 = 2;
  }
  if (param_4 == 0) {
    uVar3 = 0;
  }
  else {
    uVar3 = 0x10000;
  }
  local_24[0] = local_24[0] & 0xfffcfffc;
  if (param_5 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0x20000;
  }
  if (param_2 != 0) {
    local_24[0] = local_24[0] | 1;
  }
  (**(code **)(param_1 + 0xc))
            (*(undefined4 *)(param_1 + 0x14),0x240,local_24[0] | uVar2 | uVar3 | uVar1);
  return;
}



/* @ 0x102614c  FUN_0102614c */

undefined4 FUN_0102614c(undefined4 param_1,int param_2,undefined4 param_3)

{
  (*(code *)(&PTR_LAB_0104384c)[param_2])(param_1,param_3);
  return 0;
}



/* @ 0x1026178  FUN_01026178 */

void FUN_01026178(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0xd8) = param_2;
  return;
}



/* @ 0x1026180  FUN_01026180 */

int al_serdes_init_cores(int param_1,int param_2,int param_3)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  bool bVar6;
  uint uVar7;
  uint uVar8;
  int *piVar9;
  undefined4 uVar10;
  int iVar11;
  undefined4 uVar12;
  int *piVar13;
  uint uVar14;
  int *piVar15;
  uint uVar16;
  byte bVar17;
  undefined1 uVar18;
  uint uVar19;
  uint uVar20;
  uint uVar21;
  uint local_1f8;
  int local_1ec [4];
  int local_1dc;
  byte local_1d8 [4];
  undefined1 auStack_1d4 [16];
  undefined1 local_1c4 [48];
  undefined4 local_194;
  undefined1 local_190;
  byte local_18c;
  undefined1 auStack_188 [16];
  undefined1 local_178;
  undefined1 local_177;
  undefined1 local_176;
  undefined4 local_170;
  undefined1 auStack_16c [16];
  undefined1 auStack_15c [20];
  undefined4 local_148;
  undefined1 local_144;
  byte local_140;
  int local_13c;
  int local_138;
  int local_134;
  int local_130;
  char local_12c;
  undefined1 local_12b;
  undefined1 local_12a;
  undefined4 local_124;
  undefined1 auStack_120 [16];
  undefined1 auStack_110 [20];
  undefined4 local_fc;
  undefined1 local_f8;
  byte local_f4;
  int local_f0;
  int local_ec;
  int local_e8;
  int local_e4;
  undefined1 local_e0;
  undefined1 local_df;
  undefined1 local_de;
  undefined1 local_db;
  undefined4 local_d8;
  undefined1 auStack_d4 [16];
  undefined1 auStack_c4 [20];
  undefined4 local_b0;
  undefined1 local_ac;
  byte local_a8;
  undefined1 local_94;
  undefined1 local_93;
  undefined1 local_92;
  undefined1 auStack_88 [16];
  undefined1 auStack_78 [16];
  byte *local_68;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined1 local_50 [4];
  undefined4 local_4c;
  undefined1 local_48;
  undefined4 local_44;
  undefined1 local_40;
  undefined4 local_3c;
  undefined1 local_38;
  undefined4 local_34 [2];
  undefined1 auStack_2c [8];
  
  if (param_1 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe3);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___obj__01033024,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe3);
    FUN_01000454(0);
  }
  if (param_2 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe4);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___pbs_regs_base__01042a90,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe4);
    FUN_01000454(0);
  }
  if (param_3 == 0) {
    FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s___cfg__01042738,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe5);
    FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s___cfg__01042738,1,
                 s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_01043860,0xe5);
    FUN_01000454(0);
  }
  FUN_010129d8(local_1d8,param_3,0x188);
  local_1ec[3] = param_1 + 0x2f4;
  local_1ec[1] = param_1 + 0xfc;
  local_1ec[2] = param_1 + 0x1f8;
  local_1dc = param_1 + 0x3f0;
  local_1ec[0] = param_1;
  iVar3 = FUN_01021fd4(param_2);
  if (iVar3 != 1) goto LAB_01026200;
  uVar4 = *(uint *)(param_2 + 0xe8);
  uVar7 = *(uint *)(param_2 + 0x1d4);
  uVar8 = *(uint *)(param_2 + 0x228);
  uVar14 = *(uint *)(param_2 + 0x22c);
  uVar16 = *(uint *)(param_2 + 0x230);
  if (local_1d8[0] != 1) {
    if (local_1d8[0] == 3) {
      uVar5 = uVar4 & 0xffffffcc | 1;
      uVar4 = 0x10;
    }
    else {
      uVar5 = uVar4 & 0xffffffcc | 2;
      uVar4 = 0x20;
    }
    uVar4 = uVar4 | uVar5;
  }
  uVar5 = (uint)local_18c;
  if (uVar5 != 1) {
    if (uVar5 == 6) {
      uVar20 = 0x200000;
      uVar4 = uVar4 & 0xffccccff | 0x22200;
    }
    else {
      uVar20 = 0x100000;
      uVar4 = uVar4 & 0xffccccff | 0x11100;
    }
    uVar4 = uVar20 | uVar4;
  }
  uVar21 = (uint)local_f4;
  uVar20 = uVar21 & 0xfd;
  bVar17 = local_140 & 0xfd;
  if (local_140 == 1) {
    if (uVar21 != 1) goto LAB_01026774;
LAB_01026810:
    uVar8 = uVar8 & 0xffccffff;
LAB_01026814:
    if ((uVar21 != 1) && (uVar8 = uVar8 & 0xefffffff, uVar21 == 0x14)) {
      uVar8 = uVar8 | 0x10000000;
    }
  }
  else {
    if (local_140 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) {
      uVar4 = uVar4 & 0xccffffff | 0x22000000;
LAB_0102699c:
      uVar19 = 0x2000;
      uVar7 = uVar7 & 0xffff888c | 0x222;
    }
    else {
      uVar4 = uVar4 & 0xccffffff | 0x11000000;
      if (local_140 == 0xf) {
        uVar19 = 0x1000;
        uVar7 = uVar7 & 0xffff888c | 0x111;
      }
      else {
        if (local_140 != 0x13) goto LAB_0102699c;
        uVar19 = 0x4000;
        uVar7 = uVar7 & 0xffff888c | 0x442;
      }
    }
    uVar7 = uVar19 | uVar7;
    if (uVar21 == 1) {
      if (bVar17 == 0x10 || local_140 == 0x13) goto LAB_010267b8;
LAB_01026d34:
      if (uVar20 == 0x10 || uVar21 == 0x14) {
LAB_01026cc4:
        uVar8 = uVar8 & 0xfffffcff | 0x200;
        if (bVar17 == 0x10) goto LAB_01026d9c;
      }
    }
    else {
LAB_01026774:
      uVar7 = uVar7 & 0x888cffff;
      if (uVar21 == 0xe) {
        uVar7 = uVar7 | 0x4420000;
        local_1f8 = 0x40000000;
      }
      else if (uVar21 == 0x14) {
        uVar7 = uVar7 | 0x2210000;
        local_1f8 = 0x20000000;
      }
      else {
        uVar7 = uVar7 | 0x1110000;
        local_1f8 = 0x10000000;
      }
      uVar7 = local_1f8 | uVar7;
      if (bVar17 != 0x10 && local_140 != 0x13) {
        if (uVar20 != 0x10) goto LAB_01026d34;
        uVar8 = uVar8 & 0xfffffffc | 2;
LAB_01026d80:
        uVar19 = uVar8 & 0xffffffcf | 0x20;
      }
      else {
LAB_010267b8:
        if (bVar17 != 0x10) {
          uVar8 = uVar8 & 0xfffffffc | 1;
          if (uVar20 != 0x10) {
            if (uVar21 == 0x14) goto LAB_01026cc4;
            goto LAB_010267f8;
          }
          goto LAB_01026d80;
        }
        uVar19 = uVar8 & 0xffffffcc | 0x11;
        if (uVar20 != 0x10 && uVar21 != 0x14) {
          uVar8 = uVar8 & 0xfffffccc | 0x111;
LAB_01026d9c:
          uVar8 = uVar8 & 0xffffcfff | 0x1000;
          goto LAB_010267f8;
        }
        if (uVar20 != 0x10) {
          uVar8 = uVar8 & 0xfffffccc | 0x211;
          goto LAB_01026d9c;
        }
      }
      uVar8 = uVar19 & 0xffffccff | 0x2200;
    }
LAB_010267f8:
    if (local_140 == 0x11) {
      if (local_13c == 1) {
        uVar8 = uVar8 & 0xfffffffc | 1;
      }
      if (local_138 == 1) {
        uVar8 = uVar8 & 0xffffffcf | 0x10;
      }
      if (local_134 == 1) {
        uVar8 = uVar8 & 0xfffffcff | 0x100;
      }
      if (local_130 == 1) {
        uVar8 = uVar8 & 0xffffcfff | 0x1000;
      }
      if (uVar21 == 0x11) goto LAB_01026c1c;
      uVar8 = uVar8 & 0xfeccffff;
      goto LAB_01026814;
    }
    if (uVar21 != 0x11) {
      if (local_140 == 1) goto LAB_01026810;
LAB_01026c6c:
      uVar8 = uVar8 & 0xfeccffff;
      if (local_140 == 0x13) {
        uVar8 = uVar8 | 0x1000000;
      }
      goto LAB_01026814;
    }
LAB_01026c1c:
    if (local_f0 == 1) {
      uVar8 = uVar8 & 0xfffffcff | 0x200;
    }
    if (local_ec == 1) {
      uVar8 = uVar8 & 0xffffffcf | 0x20;
    }
    if (local_e8 == 1) {
      uVar8 = uVar8 & 0xfffffffc | 2;
    }
    if (local_e4 == 1) {
      uVar8 = uVar8 & 0xffffcfff | 0x2000;
    }
    if (local_140 != 1) goto LAB_01026c6c;
    uVar8 = uVar8 & 0xefccffff;
  }
  if (uVar5 - 7 < 2) {
    uVar14 = uVar14 & 0xffffffcc | 0x22;
  }
  else if (local_1d8[0] == 3) {
    uVar14 = uVar14 & 0xffffffcc | 0x11;
  }
  if (uVar21 + -AL_I2C_TAR_10BIT_ADDR_SHIFT < 2) {
    uVar14 = uVar14 & 0xffffccff | 0x2200;
LAB_01026a94:
    if (uVar5 == 0xe) {
      uVar16 = uVar16 & 0xfffffffc | 1;
    }
  }
  else {
    if (local_140 == AL_I2C_INTR_MASK_STOP_DET_SHIFT) {
      uVar14 = uVar14 & 0xffffccff | 0x1100;
    }
    if (uVar21 != 0xe) goto LAB_01026a94;
    uVar16 = uVar16 & 0xfffffffc | 2;
  }
  *(uint *)(param_2 + 0xe8) = uVar4;
  *(uint *)(param_2 + 0x1d4) = uVar7;
  *(uint *)(param_2 + 0x228) = uVar8;
  *(uint *)(param_2 + 0x22c) = uVar14;
  *(uint *)(param_2 + 0x230) = uVar16;
LAB_01026200:
  piVar13 = local_1ec;
  iVar11 = 0;
  bVar6 = false;
  iVar3 = local_1ec[0];
  piVar9 = piVar13;
  if (local_1d8[0] == 1) goto LAB_01026234;
  do {
    FUN_0101de14(iVar3,bVar6);
LAB_01026234:
    do {
      iVar11 = iVar11 + 1;
      piVar15 = piVar9 + 0x13;
      if (iVar11 == 4) {
        iVar3 = 0;
        piVar9 = piVar13;
        uVar18 = local_1c4[4];
        goto LAB_0102627c;
      }
      piVar1 = piVar9 + 0x18;
      bVar6 = false;
      piVar9 = piVar15;
    } while ((char)*piVar1 == '\x01');
    iVar3 = piVar13[iVar11];
    if (iVar11 == 2) {
      bVar6 = local_12c == '\x03';
    }
  } while( true );
LAB_0102627c:
  do {
    *(undefined1 *)(piVar9 + 0xb) = uVar18;
    switch((char)piVar9[10]) {
    case '\0':
      uVar18 = 0;
    case '\x02':
    case '\x03':
      break;
    case '\x01':
      uVar18 = *(undefined1 *)((int)piVar9 + 0x2b);
      break;
    default:
      FUN_0100ceac(s__s__clk_src_r2l_invalid___d___01043940,
                   s_al_serdes_group_r2l_clk_freq_upd_01043888);
      iVar11 = -0x16;
      FUN_01010bfc(s__s__clk_src_r2l_invalid___d___01043940,
                   s_al_serdes_group_r2l_clk_freq_upd_01043888,local_1c4[iVar3 * 0x4c]);
      FUN_0100ceac(s__s__al_serdes_group_r2l_clk_freq_01043960,s_al_serdes_init_clk_route_010438f8,
                   iVar3);
      FUN_01010bfc(s__s__al_serdes_group_r2l_clk_freq_01043960,s_al_serdes_init_clk_route_010438f8,
                   iVar3);
      goto LAB_01026348;
    }
    piVar9 = piVar9 + 0x13;
    iVar3 = iVar3 + 1;
  } while (iVar3 != 5);
  iVar3 = 4;
  piVar9 = piVar13;
  do {
    while( true ) {
      cVar2 = *(char *)((int)piVar9 + 0x159);
      *(undefined1 *)((int)piVar9 + 0x15d) = local_db;
      if (cVar2 == '\x01') break;
      if (cVar2 == '\0') {
        local_db = 0;
      }
      else if (cVar2 != '\x04') {
        FUN_0100ceac(s__s__clk_src_l2r_invalid___d___01043998,
                     s_al_serdes_group_l2r_clk_freq_upd_010438ac);
        iVar11 = -0x16;
        FUN_01010bfc(s__s__clk_src_l2r_invalid___d___01043998,
                     s_al_serdes_group_l2r_clk_freq_upd_010438ac,local_1c4[iVar3 * 0x4c + 1]);
        FUN_0100ceac(s__s__al_serdes_group_l2r_clk_freq_010439b8,s_al_serdes_init_clk_route_010438f8
                     ,iVar3);
        FUN_01010bfc(s__s__al_serdes_group_l2r_clk_freq_010439b8,s_al_serdes_init_clk_route_010438f8
                     ,iVar3);
        goto LAB_01026348;
      }
      iVar3 = iVar3 + -1;
      piVar9 = piVar9 + -0x13;
      if (iVar3 == -1) goto LAB_010263bc;
    }
    iVar3 = iVar3 + -1;
    local_db = *(undefined1 *)((int)piVar9 + 0x15b);
    piVar9 = piVar9 + -0x13;
  } while (iVar3 != -1);
LAB_010263bc:
  iVar3 = 0;
  piVar9 = piVar13;
  do {
    switch(*(undefined1 *)((int)piVar9 + 0x2a)) {
    case 1:
      *(undefined1 *)(piVar13 + 0x67) = *(undefined1 *)((int)piVar9 + 0x2b);
      break;
    case 2:
    case 3:
      *(char *)(piVar13 + 0x67) = (char)piVar9[0xb];
      break;
    case 4:
      *(undefined1 *)(piVar13 + 0x67) = *(undefined1 *)((int)piVar9 + 0x2d);
      break;
    default:
      FUN_0100ceac(s__s__clk_src_core_invalid___d___010439f0,
                   s_al_serdes_group_core_clk_freq_up_010438d0);
      iVar11 = -0x16;
      FUN_01010bfc(s__s__clk_src_core_invalid___d___010439f0,
                   s_al_serdes_group_core_clk_freq_up_010438d0,local_1c4[iVar3 * 0x4c + 2]);
      FUN_0100ceac(s__s__al_serdes_group_core_clk_fre_01043a10,s_al_serdes_init_clk_route_010438f8,
                   iVar3);
      FUN_01010bfc(s__s__al_serdes_group_core_clk_fre_01043a10,s_al_serdes_init_clk_route_010438f8,
                   iVar3);
      goto LAB_01026348;
    }
    iVar3 = iVar3 + 1;
    piVar9 = piVar9 + 0x13;
    piVar13 = piVar13 + 2;
  } while (iVar3 != 5);
  if ((local_1d8[0] != 1) &&
     (iVar11 = al_serdes_hssp_group_ictl_pma_val_set(local_1ec[0],&local_4c,local_1c4[0],local_1c4[1],local_1c4[2]),
     iVar11 != 0)) {
    uVar12 = 0;
LAB_01027304:
    FUN_0100ceac(s__s__al_serdes_hssp_group_ictl_pm_01043a48,s_al_serdes_init_clk_route_010438f8,
                 uVar12);
    FUN_01010bfc(s__s__al_serdes_hssp_group_ictl_pm_01043a48,s_al_serdes_init_clk_route_010438f8,
                 uVar12);
LAB_01026348:
    FUN_0100ceac(s__s__al_serdes_init_clk_route_fai_01043cf8,s_al_serdes_init_01043860);
    FUN_01010bfc(s__s__al_serdes_init_clk_route_fai_01043cf8,s_al_serdes_init_01043860);
    return iVar11;
  }
  if ((local_18c != 1) &&
     (iVar11 = al_serdes_hssp_group_ictl_pma_val_set(local_1ec[1],&local_44,local_178,local_177,local_176), iVar11 != 0)) {
    uVar12 = 1;
    goto LAB_01027304;
  }
  if ((local_140 != 1) &&
     (iVar11 = FUN_0101dc08(local_1ec[2],&local_3c,local_12c,local_12b,local_12a), iVar11 != 0)) {
    uVar12 = 2;
LAB_0102733c:
    FUN_0100ceac(s__s__al_serdes_hssp_group_ictl_pm_01043a80,s_al_serdes_init_clk_route_010438f8,
                 uVar12);
    FUN_01010bfc(s__s__al_serdes_hssp_group_ictl_pm_01043a80,s_al_serdes_init_clk_route_010438f8,
                 uVar12);
    goto LAB_01026348;
  }
  if ((local_f4 != 1) &&
     (iVar11 = FUN_0101dc08(local_1ec[3],local_34,local_e0,local_df,local_de), iVar11 != 0)) {
    uVar12 = 3;
    goto LAB_0102733c;
  }
  iVar11 = al_serdes_25g_group_ictl_pma_val_set(local_1dc,auStack_2c,local_94,local_93,local_92);
  if (iVar11 != 0) {
    FUN_0100ceac(s__s__al_serdes_25g_group_ictl_pma_01043abc,s_al_serdes_init_clk_route_010438f8,4);
    FUN_01010bfc(s__s__al_serdes_25g_group_ictl_pma_01043abc,s_al_serdes_init_clk_route_010438f8,4);
    goto LAB_01026348;
  }
  switch(local_1d8[0]) {
  case 0:
    iVar3 = FUN_0101de9c(local_1ec[0]);
    break;
  case 1:
    goto switchD_010264f4_caseD_1;
  case 2:
    iVar3 = FUN_0101dec8(local_1ec[0]);
    break;
  case 3:
  case 4:
    iVar3 = FUN_0101ec20(local_1ec[0],local_5c,local_4c,1,0,local_194,local_190);
    break;
  case 5:
  case 6:
    iVar3 = FUN_0101ec20(local_1ec[0],local_5c,local_4c,0,0,local_194,local_190);
    break;
  default:
    FUN_0100ceac(s__s__invalid_group_A_configuratio_01043af4,s_al_serdes_init_cores_01043870);
    iVar3 = -0x16;
    FUN_01010bfc(s__s__invalid_group_A_configuratio_01043af4,s_al_serdes_init_cores_01043870,
                 local_1d8[0]);
    goto LAB_010268f0;
  case 0x12:
    iVar3 = al_serdes_hssp_group_cfg_eth_kr_mode(local_1ec[0],local_5c,local_4c,local_50[0],0);
  }
  if (2 < local_1d8[0]) {
    iVar3 = FUN_0101de3c(local_1ec[0],auStack_1d4);
    FUN_0101dda4(local_1ec[0],local_1c4 + 0xc,local_1c4 + 0x1c);
  }
  if (iVar3 != 0) {
    FUN_0100ceac(s__s__group_A_configuration_failed_01043b1c,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__group_A_configuration_failed_01043b1c,s_al_serdes_init_cores_01043870);
    goto LAB_010268f0;
  }
switchD_010264f4_caseD_1:
  switch(local_18c) {
  case 0:
    iVar3 = FUN_0101de9c(local_1ec[1]);
    break;
  case 1:
    goto switchD_01026580_caseD_1;
  case 2:
    iVar3 = FUN_0101dec8(local_1ec[1]);
    break;
  default:
    FUN_0100ceac(s__s__invalid_group_B_configuratio_01043b40,s_al_serdes_init_cores_01043870);
    iVar3 = -0x16;
    FUN_01010bfc(s__s__invalid_group_B_configuratio_01043b40,s_al_serdes_init_cores_01043870,
                 local_18c);
    goto LAB_010268f0;
  case 6:
    uVar12 = 1;
    uVar10 = 0;
    goto LAB_01026e50;
  case 7:
    uVar12 = 0;
    uVar10 = 1;
LAB_01026e50:
    iVar3 = FUN_0101ec20(local_1ec[1],local_5c,local_44,uVar10,uVar12,local_148,local_144);
    break;
  case 8:
    iVar3 = FUN_0101ec20(local_1ec[1],local_5c,local_44,0,0,local_148,local_144);
    break;
  case 0xe:
    iVar3 = al_serdes_hssp_group_cfg_sata_mode(local_1ec[1],local_5c,local_44,local_48,local_170);
    break;
  case 0x12:
    iVar3 = al_serdes_hssp_group_cfg_eth_kr_mode(local_1ec[1],local_5c,local_44,local_48,0);
  }
  if (2 < local_18c) {
    iVar3 = FUN_0101de3c(local_1ec[1],auStack_188);
    FUN_0101dda4(local_1ec[1],auStack_16c,auStack_15c);
  }
  if (iVar3 != 0) {
    FUN_0100ceac(s__s__group_B_configuration_failed_01043b68,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__group_B_configuration_failed_01043b68,s_al_serdes_init_cores_01043870);
    goto LAB_010268f0;
  }
switchD_01026580_caseD_1:
  switch(local_140) {
  case 0:
    iVar3 = FUN_0101de9c(local_1ec[2]);
    break;
  case 1:
    goto switchD_010265dc_caseD_1;
  case 2:
    iVar3 = FUN_0101dec8(local_1ec[2]);
    break;
  default:
    FUN_0100ceac(s__s__invalid_group_C_configuratio_01043b8c,s_al_serdes_init_cores_01043870);
    iVar3 = -0x16;
    FUN_01010bfc(s__s__invalid_group_C_configuratio_01043b8c,s_al_serdes_init_cores_01043870,
                 local_140);
    goto LAB_010268f0;
  case 9:
  case 10:
    iVar3 = FUN_0101ec20(local_1ec[2],local_5c,local_3c,1,0,local_fc,local_f8);
    break;
  case 0xb:
    iVar3 = FUN_0101ec20(local_1ec[2],local_5c,local_3c,0,0,local_fc,local_f8);
    break;
  case 0xf:
    iVar3 = al_serdes_hssp_group_cfg_sata_mode(local_1ec[2],local_5c,local_3c,local_40,local_124);
    break;
  case 0x10:
    iVar3 = al_serdes_hssp_group_cfg_eth_sgmii_mode(local_1ec[2],local_5c,local_40,local_3c);
    break;
  case 0x11:
    iVar3 = al_serdes_hssp_group_cfg_eth_sgmii_2_5g_mode(local_1ec[2],local_5c,local_40,local_3c);
    break;
  case 0x12:
    uVar12 = 0;
    goto LAB_01026fe4;
  case 0x13:
    uVar12 = 1;
LAB_01026fe4:
    iVar3 = al_serdes_hssp_group_cfg_eth_kr_mode(local_1ec[2],local_5c,local_3c,local_40,uVar12);
  }
  if (2 < local_140) {
    iVar3 = FUN_0101de3c(local_1ec[2],&local_13c);
    FUN_0101dda4(local_1ec[2],auStack_120,auStack_110);
  }
  if (iVar3 != 0) {
    FUN_0100ceac(s__s__group_C_configuration_failed_01043bb4,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__group_C_configuration_failed_01043bb4,s_al_serdes_init_cores_01043870);
    goto LAB_010268f0;
  }
switchD_010265dc_caseD_1:
  switch(local_f4) {
  case 0:
    iVar3 = FUN_0101de9c(local_1ec[3]);
    break;
  case 1:
    goto switchD_0102663c_caseD_1;
  case 2:
    iVar3 = FUN_0101dec8(local_1ec[3]);
    break;
  default:
    FUN_0100ceac(s__s__invalid_group_D_configuratio_01043bd8,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__invalid_group_D_configuratio_01043bd8,s_al_serdes_init_cores_01043870,
                 local_f4);
    if (2 < local_f4) goto LAB_01027294;
    iVar3 = -0x16;
    goto LAB_01027114;
  case 0xc:
    iVar3 = FUN_0101ec20(local_1ec[3],local_5c,local_34[0],1,0,local_b0,local_ac);
    break;
  case 0xd:
    iVar3 = FUN_0101ec20(local_1ec[3],local_5c,local_34[0],0,0,local_b0,local_ac);
    break;
  case 0xe:
    iVar3 = al_serdes_hssp_group_cfg_sata_mode(local_1ec[3],local_5c,local_34[0],local_38,local_d8);
    break;
  case 0x10:
    iVar3 = al_serdes_hssp_group_cfg_eth_sgmii_mode(local_1ec[3],local_5c,local_38,local_34[0]);
    break;
  case 0x11:
    iVar3 = al_serdes_hssp_group_cfg_eth_sgmii_2_5g_mode(local_1ec[3],local_5c,local_38,local_34[0]);
    break;
  case 0x12:
    uVar12 = 0;
    goto LAB_01027188;
  case 0x14:
    uVar12 = 1;
LAB_01027188:
    iVar3 = al_serdes_hssp_group_cfg_eth_kr_mode(local_1ec[3],local_5c,local_34[0],local_38,uVar12);
  }
  if (2 < local_f4) {
LAB_01027294:
    iVar3 = FUN_0101de3c(local_1ec[3],&local_f0);
    FUN_0101dda4(local_1ec[3],auStack_d4,auStack_c4);
  }
  if (iVar3 != 0) {
LAB_01027114:
    FUN_0100ceac(s__s__group_D_configuration_failed_01043c00,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__group_D_configuration_failed_01043c00,s_al_serdes_init_cores_01043870);
    goto LAB_010268f0;
  }
switchD_0102663c_caseD_1:
  if (local_a8 == 1) {
    return 0;
  }
  FUN_01021b60(local_1dc,local_58,local_54);
  if (local_68 == (byte *)0x0) {
    if (local_a8 == 0x12) {
      iVar3 = FUN_01021e10(local_1dc);
      goto LAB_01026a50;
    }
    if (local_a8 == 0x15) {
      iVar3 = FUN_01021e08(local_1dc);
      goto LAB_01026a50;
    }
    FUN_0100ceac(s__s__invalid_group_E_configuratio_01043c24,s_al_serdes_init_cores_01043870);
    FUN_01010bfc(s__s__invalid_group_E_configuratio_01043c24,s_al_serdes_init_cores_01043870,
                 local_a8);
    if (local_a8 < 3) {
      iVar3 = -0x16;
    }
    else {
      iVar3 = -0x16;
      FUN_01021d90(local_1dc,auStack_88,auStack_78);
    }
  }
  else {
    if (*local_68 != local_a8) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,
                   s___cfg_>grp_cfg_AL_SRDS_GRP_E__mo_01043c4c,1,
                   s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_cores_01043870,0x5dd
                  );
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,
                   s___cfg_>grp_cfg_AL_SRDS_GRP_E__mo_01043c4c,1,
                   s____HAL__services_serdes_al_serde_01043914,s_al_serdes_init_cores_01043870,0x5dd
                  );
      FUN_01000454(0);
    }
    iVar3 = (**(code **)(local_1dc + 0xb8))(local_1dc,local_68);
LAB_01026a50:
    if (2 < local_a8) {
      FUN_01021d90(local_1dc,auStack_88,auStack_78);
    }
    if (iVar3 == 0) {
      return 0;
    }
  }
  FUN_0100ceac(s__s__group_E_configuration_failed_01043cb0,s_al_serdes_init_cores_01043870);
  FUN_01010bfc(s__s__group_E_configuration_failed_01043cb0,s_al_serdes_init_cores_01043870);
LAB_010268f0:
  FUN_0100ceac(s__s__al_serdes_init_cores_failed__01043cd4,s_al_serdes_init_01043860);
  FUN_01010bfc(s__s__al_serdes_init_cores_failed__01043cd4,s_al_serdes_init_01043860);
  return iVar3;
}



/* @ 0x10274e4  FUN_010274e4 */

void FUN_010274e4(int param_1,int param_2)

{
  uint uVar1;
  
  if (param_2 == 0) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0x40000000;
  }
  *(uint *)(param_1 + 0xd0) = *(uint *)(param_1 + 0xd0) & 0xbfffffff | uVar1;
  return;
}



/* @ 0x1027504  FUN_01027504 */

void FUN_01027504(int param_1,int param_2)

{
  if (param_2 != 0) {
    *(undefined4 *)(param_1 + 0x4000) = 1;
    *(undefined4 *)(param_1 + 0x5000) = 1;
  }
  *(undefined4 *)(param_1 + 4) = 7;
  return;
}



/* @ 0x102752c  FUN_0102752c */

void FUN_0102752c(int param_1)

{
  *(uint *)(param_1 + 0xd0) = *(uint *)(param_1 + 0xd0) & 0xbfffffff;
  return;
}



/* @ 0x102753c  FUN_0102753c */

void FUN_0102753c(int param_1)

{
  *(undefined4 *)(param_1 + 0x4000) = 0;
  *(undefined4 *)(param_1 + 0x5000) = 0;
  *(undefined4 *)(param_1 + 4) = 0;
  return;
}



/* @ 0x1027558  FUN_01027558 */

void FUN_01027558(int param_1)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr9;
  
  *DAT_010275b4 = 1;
  do {
  } while (*DAT_010275b8 != 2);
  uVar1 = coproc_movefrom_Peripheral_System(0,in_cr0,1);
  coproc_moveto_Peripheral_System(uVar1 & 0xffffffef | 0x188,0,in_cr0,1);
  uVar1 = coprocessor_movefromRt(0xf,1,2,in_cr9,in_cr0);
  uVar2 = uVar1 & 0xfffffe38 | 0x82;
  if (param_1 != 0) {
    uVar2 = uVar1 & 0xfffffe38 | 0x200082;
  }
  coprocessor_moveto(0xf,1,2,uVar2,in_cr9,in_cr0);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x10275bc  FUN_010275bc */

char * al_flash_obj_id_to_str(uint param_1)

{
  char *pcVar1;
  uint uVar2;
  uint uVar3;
  char cVar4;
  char *pcVar5;
  
  uVar2 = param_1 & 0xfeffffff;
  if (uVar2 == 0x110) {
    pcVar1 = &DAT_01043e94;
  }
  else if (uVar2 < 0x111) {
    if (uVar2 == AL_I2C_INTR_MASK_ACTIVITY_SHIFT) {
      pcVar1 = s_UBOOT_ENV_RED_01043e40;
    }
    else if (uVar2 < AL_I2C_INTR_MASK_ACTIVITY_SHIFT + 1) {
      if (uVar2 == 3) {
        pcVar1 = s_STG2_5_01043e04;
      }
      else if (uVar2 < 4) {
        if (uVar2 == 1) {
          pcVar1 = &DAT_01043df8;
        }
        else if (uVar2 < 2) {
          pcVar1 = s_BOOT_MODE_01043dec;
        }
        else {
          pcVar1 = &DAT_01043e00;
        }
      }
      else if (uVar2 == 5) {
        pcVar1 = s_UBOOT_01043e1c;
      }
      else if (uVar2 < 5) {
        pcVar1 = &DAT_01043e0c;
      }
      else if (uVar2 == 6) {
        pcVar1 = s_UBOOT_SCRIPT_01043e24;
      }
      else {
        pcVar1 = s_UBOOT_ENV_01043e34;
        if (uVar2 != 7) {
          pcVar1 = &DAT_01043fd4;
        }
      }
    }
    else if (uVar2 == AL_I2C_TAR_10BIT_ADDR_SHIFT) {
      pcVar1 = s_PRE_BOOT_01043e60;
    }
    else if (uVar2 < AL_I2C_TAR_10BIT_ADDR_SHIFT + 1) {
      if (uVar2 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
        pcVar1 = s_ROOT_FS_01043e58;
      }
      else if (uVar2 < AL_I2C_INTR_MASK_START_DET_SHIFT + 1) {
        pcVar1 = s_KERNEL_01043e50;
      }
      else {
        pcVar1 = s_2ND_TOC_01043e78;
      }
    }
    else if (uVar2 == 0xe) {
      pcVar1 = &DAT_01043e14;
    }
    else if (uVar2 < 0xe) {
      pcVar1 = s_PRE_BOOT_V2_01043e6c;
    }
    else if (uVar2 == 0x100) {
      pcVar1 = s_APCEA_01043e80;
    }
    else {
      pcVar1 = (char *)0x1043e88;
      if (uVar2 != 0x101) {
        pcVar1 = &DAT_01043fd4;
      }
    }
  }
  else if (uVar2 == 0x240) {
    pcVar1 = s_APP_3_01043ef4;
  }
  else if (uVar2 < 0x241) {
    if (uVar2 == 0x211) {
      pcVar1 = s_APP_0_CFG_01043ec0;
    }
    else if (uVar2 < 0x212) {
      if (uVar2 == 0x201) {
        pcVar1 = s_BOOT_APP_CFG_01043ea8;
      }
      else if (uVar2 == 0x210) {
        pcVar1 = s_APP_0_01043eb8;
      }
      else {
        pcVar1 = s_BOOT_APP_01043e9c;
        if (uVar2 != 0x200) {
          pcVar1 = &DAT_01043fd4;
        }
      }
    }
    else if (uVar2 == 0x221) {
      pcVar1 = s_APP_1_CFG_01043ed4;
    }
    else if (uVar2 < 0x222) {
      pcVar1 = (char *)0x1043ecc;
      if (uVar2 != 0x220) {
        pcVar1 = &DAT_01043fd4;
      }
    }
    else if (uVar2 == 0x230) {
      pcVar1 = s_APP_2_01043ee0;
    }
    else {
      pcVar1 = s_APP_2_CFG_01043ee8;
      if (uVar2 != 0x231) {
        pcVar1 = &DAT_01043fd4;
      }
    }
  }
  else if (uVar2 == 0x500) {
    pcVar1 = s_SERDES_25G_FW_01043f18;
  }
  else if (uVar2 < 0x501) {
    if (uVar2 == 0x300) {
      pcVar1 = s_CRASH_DUMP_01043f08;
    }
    else if (uVar2 == 0x400) {
      pcVar1 = &DAT_01043f14;
    }
    else {
      pcVar1 = (char *)0x1043efc;
      if (uVar2 != 0x241) {
        pcVar1 = &DAT_01043fd4;
      }
    }
  }
  else if (uVar2 == 0x601) {
    pcVar1 = &DAT_01043f30;
  }
  else if (uVar2 < 0x602) {
    pcVar1 = (char *)0x1043f28;
    if (uVar2 != 0x600) {
      pcVar1 = &DAT_01043fd4;
    }
  }
  else if (uVar2 == 0x602) {
    pcVar1 = &DAT_01043f38;
  }
  else {
    pcVar1 = s_Unable_to_read_Board_RevID_from_E_01031668 + 0x20;
    if (uVar2 != 0x700) {
      pcVar1 = &DAT_01043fd4;
    }
  }
  if ((param_1 & 0x1000000) != 0) {
    cVar4 = *pcVar1;
    pcVar5 = (char *)((int)&DAT_01049d60 + 3);
    DAT_01049d60 = 0x5f474953;
    uVar2 = 4;
    while( true ) {
      pcVar5 = pcVar5 + 1;
      *pcVar5 = cVar4;
      pcVar1 = pcVar1 + 1;
      cVar4 = *pcVar1;
      uVar3 = uVar2 + 1;
      if (cVar4 == '\0') break;
      uVar2 = uVar3;
      if (0x27 < uVar3) {
        FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____i___j__<_40)_0104400c,1,
                     s____HAL__services_flash_contents__01043fd8,s_al_flash_obj_id_to_str_01043d20,
                     0x74);
        FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____i___j__<_40)_0104400c,1,
                     s____HAL__services_flash_contents__01043fd8,s_al_flash_obj_id_to_str_01043d20,
                     0x74);
        FUN_01000454(0);
      }
    }
    if (0x27 < uVar3) {
      FUN_0100ceac(s_TEST_ERROR____s__returned__d__s__01028ea8,s____i___j__<_40)_0104400c,1,
                   s____HAL__services_flash_contents__01043fd8,s_al_flash_obj_id_to_str_01043d20,
                   0x77);
      FUN_01010bfc(s_TEST_ERROR____s__returned__d__s__01028ea8,s____i___j__<_40)_0104400c,1,
                   s____HAL__services_flash_contents__01043fd8,s_al_flash_obj_id_to_str_01043d20,
                   0x77);
      FUN_01000454(0);
    }
    pcVar1 = (char *)&DAT_01049d60;
    *(undefined1 *)((int)&DAT_01049d60 + uVar2 + 1) = 0;
  }
  return pcVar1;
}



/* @ 0x1027a38  FUN_01027a38 */

int al_flash_toc_validate(code *param_1,int param_2,int *param_3,uint *param_4)

{
  int iVar1;
  byte *pbVar2;
  byte *pbVar3;
  int iVar4;
  uint uVar5;
  int local_58;
  int local_54 [2];
  uint local_4c;
  int local_44;
  byte abStack_40 [32];
  byte abStack_20 [4];
  
  iVar1 = (*param_1)(param_2,local_54,0x14);
  if (iVar1 == 0) {
    param_2 = param_2 + 0x14;
    if (local_54[0] == 0x70c070c) {
      iVar1 = 0;
      pbVar3 = (byte *)local_54;
      do {
        pbVar2 = pbVar3 + 1;
        iVar1 = iVar1 + (uint)*pbVar3;
        pbVar3 = pbVar2;
      } while (pbVar2 != (byte *)&local_44);
      if (local_44 == iVar1) {
        *param_4 = local_4c;
        *param_3 = local_4c * 0x20 + 0x18;
        iVar4 = 0;
        if (*param_4 != 0) {
          uVar5 = 0;
          iVar4 = 0;
          do {
            iVar1 = (*param_1)(param_2,abStack_40,0x20);
            if (iVar1 != 0) goto LAB_01027b7c;
            param_2 = param_2 + 0x20;
            iVar1 = 0;
            pbVar3 = abStack_40;
            do {
              pbVar2 = pbVar3 + 1;
              iVar1 = iVar1 + (uint)*pbVar3;
              pbVar3 = pbVar2;
            } while (pbVar2 != abStack_20);
            uVar5 = uVar5 + 1;
            iVar4 = iVar4 + iVar1;
          } while (uVar5 < *param_4);
        }
        iVar1 = (*param_1)(param_2,&local_58,4);
        if (iVar1 == 0) {
          if (local_58 == iVar4) {
            return 0;
          }
          FUN_0100ceac(s__s__flash_toc_entries_checksum_v_01044048,s_al_flash_toc_validate_01043d38)
          ;
          FUN_01010bfc(s__s__flash_toc_entries_checksum_v_01044048,s_al_flash_toc_validate_01043d38)
          ;
          return -5;
        }
        goto LAB_01027b7c;
      }
    }
    iVar1 = -5;
  }
  else {
LAB_01027b7c:
    FUN_0100ceac(s__s__device_read_failed__01043f68,s_al_flash_toc_validate_01043d38);
    FUN_01010bfc(s__s__device_read_failed__01043f68,s_al_flash_toc_validate_01043d38);
  }
  return iVar1;
}



/* @ 0x1027bc8  FUN_01027bc8 */

undefined4 FUN_01027bc8(undefined4 param_1,int param_2,int param_3,uint param_4,int *param_5)

{
  int iVar1;
  uint uVar2;
  undefined1 auStack_20 [4];
  undefined1 auStack_1c [4];
  
  if (param_4 != 0) {
    uVar2 = 0;
    do {
      iVar1 = al_flash_toc_validate(param_1,param_2,auStack_20,auStack_1c);
      if (iVar1 == 0) {
        if (param_4 <= uVar2) {
          return 0xffffffea;
        }
        *param_5 = param_2;
        return 0;
      }
      uVar2 = uVar2 + 1;
      param_2 = param_2 + param_3;
    } while (param_4 != uVar2);
  }
  return 0xffffffea;
}



/* @ 0x1027c54  FUN_01027c54 */

int al_flash_toc_find_id_with_fallback(code *param_1,int param_2,int param_3,int param_4,uint param_5,uint *param_6,
                int *param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  undefined1 auStack_7c [8];
  uint local_74;
  int local_68;
  undefined4 uStack_64;
  undefined4 local_60;
  int local_5c;
  int local_58;
  int iStack_54;
  int local_50;
  undefined4 uStack_4c;
  undefined8 local_48;
  undefined4 local_40;
  int iStack_3c;
  int local_38;
  int iStack_34;
  int local_30;
  undefined4 uStack_2c;
  
  iVar1 = (*param_1)(param_2,auStack_7c,0x14);
  iVar2 = iVar1;
  if (iVar1 == 0) {
    if (param_5 < local_74) {
      iVar3 = param_2 + param_5 * 0x20 + 0x14;
      uVar5 = 0xffffffff;
      do {
        iVar2 = (*param_1)(iVar3,&local_68);
        if (iVar2 != 0) goto LAB_01027d18;
        uVar4 = param_5;
        if (local_68 == param_3) goto LAB_01027d8c;
        uVar4 = uVar5;
        if (local_68 == param_4) {
          local_48 = CONCAT44(uStack_64,local_68);
          local_40 = local_60;
          iStack_3c = local_5c;
          local_38 = local_58;
          iStack_34 = iStack_54;
          local_30 = local_50;
          uStack_2c = uStack_4c;
          uVar4 = param_5;
        }
        param_5 = param_5 + 1;
        iVar3 = iVar3 + 0x20;
        uVar5 = uVar4;
      } while (local_74 != param_5);
      if (-1 < (int)uVar4) {
        local_68 = (int)local_48;
        uStack_64 = local_48._4_4_;
        local_60 = local_40;
        local_5c = iStack_3c;
        local_58 = local_38;
        iStack_54 = iStack_34;
        local_50 = local_30;
        uStack_4c = uStack_2c;
LAB_01027d8c:
        if (uVar4 < local_74) {
          *param_6 = uVar4;
          FUN_010129d8(param_7,&local_68,0x20);
          *param_7 = local_68;
          param_7[3] = local_5c;
          param_7[4] = local_58;
          param_7[5] = iStack_54;
          param_7[6] = local_50;
          return 0;
        }
      }
    }
    *param_6 = 0xffffffff;
  }
  else {
LAB_01027d18:
    iVar1 = iVar2;
    FUN_0100ceac(s__s__device_read_failed__01043f68,s_al_flash_toc_find_id_with_fallba_01043d50);
    FUN_01010bfc(s__s__device_read_failed__01043f68,s_al_flash_toc_find_id_with_fallba_01043d50);
  }
  return iVar1;
}



/* @ 0x1027de8  FUN_01027de8 */

void FUN_01027de8(void)

{
  al_flash_toc_find_id_with_fallback();
  return;
}



/* @ 0x1027e10  FUN_01027e10 */

int al_flash_toc_stage2_active_instance_get_with_fallback(int param_1,int param_2,code *param_3,int param_4,uint *param_5,uint *param_6)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  bool bVar6;
  undefined1 auStack_54 [8];
  uint local_4c;
  uint local_40 [4];
  uint local_30;
  
  uVar5 = *(uint *)(param_1 + param_2);
  if ((uVar5 & 1) == 0) {
    iVar1 = (*param_3)(param_4,auStack_54,0x14);
    if (iVar1 == 0) {
      if (local_4c != 0) {
        uVar4 = 0;
        iVar3 = param_4 + 0x14;
        do {
          iVar1 = (*param_3)(iVar3,local_40);
          if (iVar1 != 0) goto LAB_01027eb4;
          uVar2 = local_40[0] & 0xfffffff;
          bVar6 = uVar2 != 0;
          if (uVar2 != 1) {
            bVar6 = uVar2 != AL_I2C_TAR_10BIT_ADDR_SHIFT;
          }
          if ((!bVar6 || (uVar2 == 1 || uVar2 == 0xd)) && (local_30 == uVar5)) {
            *param_5 = local_40[0] >> 0x1c;
            *param_6 = local_40[0] >> 0x1c;
            if (uVar4 < local_4c) {
              return 0;
            }
            break;
          }
          uVar4 = uVar4 + 1;
          iVar3 = iVar3 + 0x20;
        } while (local_4c != uVar4);
      }
      iVar1 = -0x16;
      FUN_0100ceac(s__s__unable_to_find_stage_2_at_of_0104401c,
                   s_al_flash_toc_stage2_active_insta_01043d74,uVar5);
      FUN_01010bfc(s__s__unable_to_find_stage_2_at_of_0104401c,
                   s_al_flash_toc_stage2_active_insta_01043d74,uVar5);
    }
    else {
LAB_01027eb4:
      FUN_0100ceac(s__s__device_read_failed__01043f68,s_al_flash_toc_stage2_active_insta_01043d74);
      FUN_01010bfc(s__s__device_read_failed__01043f68,s_al_flash_toc_stage2_active_insta_01043d74);
    }
  }
  else {
    iVar1 = 0;
    *param_5 = (uVar5 & 0xffff) >> 8;
    *param_6 = (uVar5 & 0xffffff) >> 0x10;
  }
  return iVar1;
}



/* @ 0x1027f6c  FUN_01027f6c */

int al_flash_obj_header_read_and_validate(code *param_1,undefined4 param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  byte *pbVar3;
  int local_58;
  int iStack_54;
  int local_50;
  int iStack_4c;
  int local_48;
  int local_30;
  int local_2c;
  int local_28;
  int iStack_24;
  int local_20;
  int local_1c;
  int local_14;
  byte *pbVar4;
  
  iVar1 = (*param_1)(param_2,&local_58,0x48);
  if (iVar1 == 0) {
    iVar2 = 0;
    pbVar4 = (byte *)&local_58;
    do {
      pbVar3 = pbVar4 + 1;
      iVar2 = iVar2 + (uint)*pbVar4;
      pbVar4 = pbVar3;
    } while (pbVar3 != (byte *)&local_14);
    if (local_14 == iVar2) {
      if (local_58 == 0xb9ec7) {
        FUN_010129d8(param_3,&local_58,0x48);
        *param_3 = local_58;
        param_3[1] = iStack_54;
        param_3[2] = local_50;
        param_3[3] = iStack_4c;
        param_3[4] = local_48;
        param_3[10] = local_30;
        param_3[0xb] = local_2c;
        param_3[0xc] = local_28;
        param_3[0xd] = iStack_24;
        param_3[0xe] = local_20;
        param_3[0xf] = local_1c;
      }
      else {
        FUN_0100ceac(s__s__flash_obj_header_contains_in_01043f84,
                     s_al_flash_obj_header_read_and_val_01043dac);
        FUN_01010bfc(s__s__flash_obj_header_contains_in_01043f84,
                     s_al_flash_obj_header_read_and_val_01043dac);
        iVar1 = -5;
        FUN_0100ceac(s_expected_0x_X__read_0x_X_01043fb8,0xb9ec7,local_58);
        FUN_01010bfc(s_expected_0x_X__read_0x_X_01043fb8,0xb9ec7,local_58);
      }
    }
    else {
      iVar1 = -5;
    }
  }
  else {
    FUN_0100ceac(s__s__device_read_failed__01043f68,s_al_flash_obj_header_read_and_val_01043dac);
    FUN_01010bfc(s__s__device_read_failed__01043f68,s_al_flash_obj_header_read_and_val_01043dac);
  }
  return iVar1;
}



/* @ 0x10280d4  FUN_010280d4 */

int al_flash_obj_data_load(code *param_1,int param_2,byte *param_3)

{
  int iVar1;
  byte *pbVar2;
  int local_64;
  undefined1 auStack_60 [40];
  int local_38;
  byte *pbVar3;
  
  iVar1 = (*param_1)(param_2,auStack_60,0x48);
  if (iVar1 == 0) {
    iVar1 = (*param_1)(param_2 + 0x48,param_3,local_38);
    if ((iVar1 == 0) && (iVar1 = (*param_1)(param_2 + 0x48 + local_38,&local_64), iVar1 == 0)) {
      if (local_38 == 0) {
        iVar1 = 0;
      }
      else {
        iVar1 = 0;
        pbVar3 = param_3;
        do {
          pbVar2 = pbVar3 + 1;
          iVar1 = iVar1 + (uint)*pbVar3;
          pbVar3 = pbVar2;
        } while (param_3 + local_38 != pbVar2);
      }
      if (local_64 == iVar1) {
        return 0;
      }
      FUN_0100ceac(s__s__data_checksum_validation_fai_01043f40,s_al_flash_obj_data_load_01043dd4);
      FUN_01010bfc(s__s__data_checksum_validation_fai_01043f40,s_al_flash_obj_data_load_01043dd4);
      return -5;
    }
  }
  FUN_0100ceac(s__s__device_read_failed__01043f68,s_al_flash_obj_data_load_01043dd4);
  FUN_01010bfc(s__s__device_read_failed__01043f68,s_al_flash_obj_data_load_01043dd4);
  return iVar1;
}



/* @ 0x10281ec  FUN_010281ec */

void FUN_010281ec(undefined4 *param_1,undefined1 *param_2,int param_3,int param_4,undefined4 param_5
                 )

{
  int iVar1;
  undefined1 *puVar2;
  undefined1 *puVar3;
  
  puVar2 = param_2 + 0x40;
  puVar3 = param_2 + param_3;
  *param_1 = param_2;
  param_1[1] = param_3;
  param_1[2] = 1;
  param_1[3] = param_5;
  param_1[5] = puVar2;
  param_1[6] = puVar3;
  param_1[8] = param_2;
  if ((param_4 == 0) || (*(int *)(param_2 + 0x20) != 0x57ace123)) {
    param_2[0x10] = 0;
    param_2[0x11] = 0;
    param_2[0x12] = 0;
    param_2[0x13] = 0;
    param_2[0x14] = 0;
    param_2[0x15] = 0;
    param_2[0x16] = 0;
    param_2[0x17] = 0;
    param_2[0x1c] = 0;
    param_2[0x1d] = 0;
    param_2[0x1e] = 0;
    param_2[0x1f] = 0;
    param_2[0x18] = 0;
    param_2[0x19] = 0;
    param_2[0x1a] = 0;
    param_2[0x1b] = 0;
    *param_2 = (char)puVar2;
    param_2[1] = (char)((uint)puVar2 >> 8);
    param_2[2] = (char)((uint)puVar2 >> 0x10);
    param_2[3] = (char)((uint)puVar2 >> 0x18);
    param_2[4] = 0;
    param_2[5] = 0;
    param_2[6] = 0;
    param_2[7] = 0;
    param_2[8] = (char)puVar3;
    param_2[9] = (char)((uint)puVar3 >> 8);
    param_2[10] = (char)((uint)puVar3 >> 0x10);
    param_2[0xb] = (char)((uint)puVar3 >> 0x18);
    param_2[0xc] = 0;
    param_2[0xd] = 0;
    param_2[0xe] = 0;
    param_2[0xf] = 0;
    param_2[0x20] = 0x23;
    param_2[0x21] = 0xe1;
    param_2[0x22] = 0xac;
    param_2[0x23] = 0x57;
    iVar1 = *(int *)(param_2 + 0x10);
    param_1[7] = 1;
    param_1[4] = puVar2 + iVar1;
  }
  else {
    iVar1 = *(int *)(param_2 + 0x10);
    param_1[7] = 1;
    param_1[4] = puVar2 + iVar1;
  }
  return;
}



/* @ 0x1028410  FUN_01028410 */

void FUN_01028410(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  
  puVar3 = *(undefined1 **)(param_2 + 0x10);
  if ((*(int *)(*(int *)(param_2 + 0x20) + 0x1c) == 0) ||
     (*(undefined1 **)(param_2 + 0x18) == puVar3)) {
    puVar4 = *(undefined1 **)(param_2 + 0x14);
  }
  else {
    puVar4 = puVar3 + 1;
  }
  while (puVar4 != puVar3) {
    **(undefined1 **)(param_1 + 0x10) = *puVar4;
    iVar2 = *(int *)(param_1 + 0x10) + 1;
    *(int *)(param_1 + 0x10) = iVar2;
    if (iVar2 == *(int *)(param_1 + 0x18)) {
      iVar2 = *(int *)(param_1 + 0x20);
      *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(param_1 + 0x14);
      *(undefined1 *)(iVar2 + 0x10) = 0;
      *(undefined1 *)(iVar2 + 0x11) = 0;
      *(undefined1 *)(iVar2 + 0x12) = 0;
      *(undefined1 *)(iVar2 + 0x13) = 0;
      *(undefined1 *)(iVar2 + 0x1c) = 1;
      *(undefined1 *)(iVar2 + 0x1d) = 0;
      *(undefined1 *)(iVar2 + 0x1e) = 0;
      *(undefined1 *)(iVar2 + 0x1f) = 0;
    }
    else {
      iVar2 = *(int *)(param_1 + 0x20);
      iVar1 = *(int *)(iVar2 + 0x10) + 1;
      *(char *)(iVar2 + 0x10) = (char)iVar1;
      *(char *)(iVar2 + 0x11) = (char)((uint)iVar1 >> 8);
      *(char *)(iVar2 + 0x12) = (char)((uint)iVar1 >> 0x10);
      *(char *)(iVar2 + 0x13) = (char)((uint)iVar1 >> 0x18);
    }
    puVar4 = puVar4 + 1;
    if (*(undefined1 **)(param_2 + 0x18) == puVar4) {
      puVar4 = *(undefined1 **)(param_2 + 0x14);
    }
  }
  return;
}



/* @ 0x1028574  FUN_01028574 */

undefined8 FUN_01028574(int param_1,int param_2)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  undefined1 *puVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined1 *puVar8;
  undefined1 auStack_30 [16];
  undefined1 *puVar9;
  
  if (*(int *)(param_1 + 8) == 0) {
    return CONCAT44(param_2,param_1);
  }
  iVar5 = *(int *)(param_1 + 0x20);
  uVar2 = (uint)*(byte *)(iVar5 + 0x1a);
  uVar3 = (uint)*(byte *)(iVar5 + 0x1b);
  if (*(int *)(iVar5 + 0x18) == 0) {
    if (param_2 == 0) goto LAB_01028658;
    if (*(int *)(param_1 + 0x1c) != 0) goto LAB_01028730;
    puVar4 = *(undefined1 **)(param_1 + 0x10);
  }
  else {
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x11) = 0;
    *(undefined1 *)(iVar5 + 0x12) = 0;
    *(undefined1 *)(iVar5 + 0x13) = 0;
    *(undefined1 *)(iVar5 + 0x14) = 0;
    *(undefined1 *)(iVar5 + 0x15) = 0;
    *(undefined1 *)(iVar5 + 0x16) = 0;
    *(undefined1 *)(iVar5 + 0x17) = 0;
    *(undefined1 *)(iVar5 + 0x1c) = 0;
    *(undefined1 *)(iVar5 + 0x1d) = 0;
    *(undefined1 *)(iVar5 + 0x1e) = 0;
    *(undefined1 *)(iVar5 + 0x1f) = 0;
    *(undefined1 *)(iVar5 + 0x18) = 0;
    *(undefined1 *)(iVar5 + 0x19) = 0;
    *(undefined1 *)(iVar5 + 0x1a) = 0;
    *(undefined1 *)(iVar5 + 0x1b) = 0;
    uVar2 = (uint)*(byte *)(iVar5 + 0x12);
    iVar7 = *(int *)(iVar5 + 0x10);
    uVar3 = (uint)*(byte *)(iVar5 + 0x13);
    *(undefined4 *)(param_1 + 0x1c) = 1;
    *(int *)(param_1 + 0x10) = *(int *)(param_1 + 0x14) + iVar7;
    if (param_2 == 0) goto LAB_01028658;
LAB_01028730:
    if (*(code **)(param_1 + 0xc) == (code *)0x0) {
      puVar4 = *(undefined1 **)(param_1 + 0x10);
    }
    else {
      iVar5 = (**(code **)(param_1 + 0xc))(auStack_30,0xe);
      puVar4 = *(undefined1 **)(param_1 + 0x10);
      if (iVar5 != 0) {
        puVar9 = auStack_30;
        do {
          while( true ) {
            puVar8 = puVar9 + 1;
            *puVar4 = *puVar9;
            puVar4 = (undefined1 *)(*(int *)(param_1 + 0x10) + 1);
            *(undefined1 **)(param_1 + 0x10) = puVar4;
            puVar9 = puVar8;
            if (puVar4 != *(undefined1 **)(param_1 + 0x18)) break;
            puVar4 = *(undefined1 **)(param_1 + 0x14);
            iVar7 = *(int *)(param_1 + 0x20);
            *(undefined1 **)(param_1 + 0x10) = puVar4;
            *(undefined1 *)(iVar7 + 0x10) = 0;
            *(undefined1 *)(iVar7 + 0x11) = 0;
            *(undefined1 *)(iVar7 + 0x12) = 0;
            *(undefined1 *)(iVar7 + 0x13) = 0;
            *(undefined1 *)(iVar7 + 0x1c) = 1;
            *(undefined1 *)(iVar7 + 0x1d) = 0;
            *(undefined1 *)(iVar7 + 0x1e) = 0;
            *(undefined1 *)(iVar7 + 0x1f) = 0;
            if (puVar8 == auStack_30 + iVar5) goto LAB_01028838;
          }
          iVar7 = *(int *)(param_1 + 0x20);
          iVar6 = *(int *)(iVar7 + 0x10) + 1;
          *(char *)(iVar7 + 0x10) = (char)iVar6;
          *(char *)(iVar7 + 0x11) = (char)((uint)iVar6 >> 8);
          *(char *)(iVar7 + 0x12) = (char)((uint)iVar6 >> 0x10);
          *(char *)(iVar7 + 0x13) = (char)((uint)iVar6 >> 0x18);
        } while (puVar8 != auStack_30 + iVar5);
      }
    }
LAB_01028838:
    *(undefined4 *)(param_1 + 0x1c) = 0;
  }
  *puVar4 = (char)param_2;
  iVar5 = *(int *)(param_1 + 0x10) + 1;
  *(int *)(param_1 + 0x10) = iVar5;
  if (iVar5 == *(int *)(param_1 + 0x18)) {
    iVar5 = *(int *)(param_1 + 0x20);
    *(undefined4 *)(param_1 + 0x10) = *(undefined4 *)(param_1 + 0x14);
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x11) = 0;
    *(undefined1 *)(iVar5 + 0x12) = 0;
    *(undefined1 *)(iVar5 + 0x13) = 0;
    bVar1 = *(byte *)(iVar5 + 0x1c);
    *(undefined1 *)(iVar5 + 0x1c) = 1;
    *(undefined1 *)(iVar5 + 0x1d) = 0;
    *(undefined1 *)(iVar5 + 0x1e) = 0;
    uVar3 = (uint)*(byte *)(iVar5 + 0x1f);
    *(undefined1 *)(iVar5 + 0x1f) = 0;
  }
  else {
    iVar5 = *(int *)(param_1 + 0x20);
    uVar2 = *(int *)(iVar5 + 0x10) + 1;
    uVar3 = uVar2 >> 0x18;
    *(char *)(iVar5 + 0x10) = (char)uVar2;
    *(char *)(iVar5 + 0x11) = (char)(uVar2 >> 8);
    bVar1 = *(byte *)(iVar5 + 0x12);
    *(char *)(iVar5 + 0x12) = (char)(uVar2 >> 0x10);
    *(char *)(iVar5 + 0x13) = (char)(uVar2 >> 0x18);
  }
  uVar2 = (uint)bVar1;
  if (param_2 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
    *(undefined4 *)(param_1 + 0x1c) = 1;
  }
LAB_01028658:
  return CONCAT44(uVar3,uVar2);
}



/* @ 0x1028d2c  FUN_01028d2c */

undefined8 FUN_01028d2c(int param_1,int param_2,int param_3,int param_4)

{
  undefined8 uVar1;
  
  if ((param_4 == 0) && (param_3 == 0)) {
    if (param_2 != 0 || param_1 != 0) {
      param_2 = -1;
      param_1 = -1;
    }
    return CONCAT44(param_2,param_1);
  }
  uVar1 = FUN_01028d5c();
  return uVar1;
}



/* @ 0x1028d5c  FUN_01028d5c */

undefined8 FUN_01028d5c(uint param_1,uint param_2,uint param_3,uint param_4,uint *param_5)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  bool bVar12;
  
  bVar12 = param_4 <= param_2;
  if (param_2 == param_4) {
    bVar12 = param_3 <= param_1;
  }
  if (bVar12) {
    iVar6 = LZCOUNT(param_4);
    if (param_4 == 0) {
      iVar6 = LZCOUNT(param_3) + 0x20;
    }
    iVar3 = LZCOUNT(param_2);
    if (param_2 == 0) {
      iVar3 = LZCOUNT(param_1) + 0x20;
    }
    uVar7 = iVar6 - iVar3;
    uVar10 = uVar7 - 0x20;
    uVar11 = 0x20 - uVar7;
    uVar9 = param_4 << (uVar7 & 0xff) | param_3 << (uVar10 & 0xff) | param_3 >> (uVar11 & 0xff);
    param_3 = param_3 << (uVar7 & 0xff);
    bVar12 = uVar9 <= param_2;
    if (param_2 == uVar9) {
      bVar12 = param_3 <= param_1;
    }
    if (bVar12) {
      bVar12 = param_1 < param_3;
      param_1 = param_1 - param_3;
      param_2 = (param_2 - uVar9) - (uint)bVar12;
      uVar2 = 1 << (uVar10 & 0xff) | 1U >> (uVar11 & 0xff);
      uVar1 = 1 << (uVar7 & 0xff);
    }
    else {
      uVar1 = 0;
      uVar2 = uVar1;
    }
    if (uVar7 != 0) {
      uVar8 = param_3 >> 1 | uVar9 << 0x1f;
      uVar9 = uVar9 >> 1;
      uVar4 = uVar7;
      do {
        while( true ) {
          bVar12 = uVar9 <= param_2;
          if (param_2 == uVar9) {
            bVar12 = uVar8 <= param_1;
          }
          if (bVar12) break;
          bVar12 = CARRY4(param_1,param_1);
          param_1 = param_1 * 2;
          param_2 = param_2 * 2 + (uint)bVar12;
          uVar4 = uVar4 - 1;
          uVar5 = param_2;
          if (uVar4 == 0) goto LAB_01028e0c;
        }
        bVar12 = param_1 < uVar8;
        uVar5 = param_1 - uVar8;
        param_1 = uVar5 * 2 + 1;
        param_2 = ((param_2 - uVar9) - (uint)bVar12) * 2 + (uint)CARRY4(uVar5,uVar5) +
                  (uint)(0xfffffffe < uVar5 * 2);
        uVar4 = uVar4 - 1;
        uVar5 = param_2;
      } while (uVar4 != 0);
LAB_01028e0c:
      bVar12 = CARRY4(uVar1,param_1);
      uVar9 = uVar1 + param_1;
      param_2 = uVar5 >> (uVar7 & 0xff);
      param_1 = param_1 >> (uVar7 & 0xff) | uVar5 << (uVar11 & 0xff) | uVar5 >> (uVar10 & 0xff);
      uVar10 = param_1 << (uVar7 & 0xff);
      uVar1 = uVar9 - uVar10;
      uVar2 = ((uVar5 + uVar2 + bVar12) -
              (param_2 << (uVar7 & 0xff) | param_1 << (uVar7 - 0x20 & 0xff) |
              param_1 >> (0x20 - uVar7 & 0xff))) - (uint)(uVar9 < uVar10);
    }
  }
  else {
    uVar1 = 0;
    uVar2 = uVar1;
  }
  if (param_5 != (uint *)0x0) {
    *param_5 = param_1;
    param_5[1] = param_2;
  }
  return CONCAT44(uVar2,uVar1);
}



/* @ 0x102efac  FUN_0102efac */

uint FUN_0102efac(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar1 | 0x1000);
  InstructionSynchronizationBarrier(0xf);
  FUN_0102eff4(uVar1 | 0x1000);
  FUN_0102efe8();
  FUN_0102f66c();
  read_toc_obj_hdr();
  thunk_FUN_01005ac0();
  func_0x01005be6();
  func_0x01005fea();
  func_0x00fff97e();
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return uVar1 & 0xf;
}



/* @ 0x102efe8  FUN_0102efe8 */

uint FUN_0102efe8(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return uVar1 & 0xf;
}



/* @ 0x102eff4  FUN_0102eff4 */

void FUN_0102eff4(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x102f014  FUN_0102f014 */

undefined4 FUN_0102f014(char param_1)

{
  undefined4 uVar1;
  
  if (param_1 == '\0') {
    uVar1 = FUN_0102f128();
  }
  else if (param_1 == '\x01') {
    uVar1 = FUN_0102f1a0();
  }
  else {
    if (param_1 == '\x03') {
      return 0;
    }
    uVar1 = 0xffffffff;
  }
  return uVar1;
}



/* @ 0x102f064  FUN_0102f064 */

void FUN_0102f064(int param_1)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr9;
  
  *DAT_0102f0bc = 1;
  do {
  } while (*DAT_0102f0c0 != 2);
  uVar1 = coproc_movefrom_Peripheral_System(0,in_cr0,1);
  coproc_moveto_Peripheral_System(uVar1 | 0x188,0,in_cr0,1);
  uVar1 = coprocessor_movefromRt(0xf,1,2,in_cr9,in_cr0);
  uVar2 = uVar1 & 0xfffffe38 | 0x82;
  if (param_1 != 0) {
    uVar2 = uVar1 & 0xfffffe38 | 0x200082;
  }
  coprocessor_moveto(0xf,1,2,uVar2,in_cr9,in_cr0);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x102f0d8  FUN_0102f0d8 */

void FUN_0102f0d8(int param_1)

{
  for (; param_1 != 0; param_1 = param_1 + -1) {
  }
  return;
}



/* @ 0x102f0ec  FUN_0102f0ec */

void FUN_0102f0ec(undefined4 param_1)

{
  undefined4 in_cr0;
  undefined4 in_cr14;
  
  coprocessor_moveto(0xf,0,0,param_1,in_cr14,in_cr0);
  return;
}



/* @ 0x102f0f4  FUN_0102f0f4 */

void FUN_0102f0f4(int param_1,int param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar2 = ~(-1 << (param_3 & 0xff));
  uVar3 = param_3 * param_2 & 0x1f;
  uVar1 = param_3 * param_2 >> 5;
  *(uint *)(param_1 + uVar1 * 4) =
       *(uint *)(param_1 + uVar1 * 4) & ~(uVar2 << uVar3) | (uVar2 & param_4) << uVar3;
  return;
}



/* @ 0x102f128  FUN_0102f128 */

void FUN_0102f128(undefined4 param_1,code *param_2)

{
  (*param_2)();
  return;
}



/* @ 0x102f134  FUN_0102f134 */

void FUN_0102f134(void)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = *(uint *)(DAT_0102f194 + 0xc);
  if ((uVar2 & 0x3ff) == 0x3ff) {
    return;
  }
  iVar1 = 0;
  do {
    if (*(uint *)(DAT_0102f198 + iVar1 * 4) == (uVar2 & 0x3ff)) {
      (**(code **)(DAT_0102f19c + iVar1 * 4))(uVar2,0);
      break;
    }
    iVar1 = iVar1 + 1;
  } while (iVar1 != 4);
  *(uint *)(DAT_0102f194 + 0x10) = uVar2;
  return;
}



/* @ 0x102f1a0  FUN_0102f1a0 */

undefined4 FUN_0102f1a0(undefined4 param_1,undefined4 param_2,uint param_3,uint param_4)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar1 = FUN_0102efe8();
  if ((param_4 & 1) == 0) {
    iVar3 = 0;
    do {
      if (*(uint *)(DAT_0102f2d4 + iVar3 * 4) == param_3) {
        *(undefined4 *)(DAT_0102f2d4 + iVar3 * 4) = 0;
        *(undefined4 *)(DAT_0102f2d8 + iVar3 * 4) = 0;
        return 0;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 != 4);
  }
  else {
    iVar3 = 0;
    do {
      if (*(uint *)(DAT_0102f2d4 + iVar3 * 4) == param_3) {
        *(undefined4 *)(DAT_0102f2d8 + iVar3 * 4) = param_2;
        return 0;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 != 4);
    iVar3 = 0;
    do {
      if (*(int *)(DAT_0102f2d4 + iVar3 * 4) == 0) {
        *(undefined4 *)(DAT_0102f2d8 + iVar3 * 4) = param_2;
        *(uint *)(DAT_0102f2d4 + iVar3 * 4) = param_3;
        break;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 != 4);
  }
  uVar2 = DAT_0102f2e0;
  iVar3 = 1 << (param_3 & 0x1f);
  *(int *)(DAT_0102f2dc + (param_3 >> 5) * 4) = iVar3;
  FUN_0102f0f4(uVar2,param_3,1,0);
  FUN_0102f0f4(DAT_0102f2e4,param_3,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,0);
  FUN_0102f0f4(DAT_0102f2e8,param_3,AL_I2C_INTR_MASK_ACTIVITY_SHIFT,1 << (uVar1 & 0xff) & 0xf);
  if ((param_4 & 2) == 0) {
    uVar2 = 1;
  }
  else {
    uVar2 = 3;
  }
  FUN_0102f0f4(DAT_0102f2ec,param_3,2,uVar2);
  if ((param_4 & 1) != 0) {
    *(int *)(DAT_0102f2f0 + (param_3 >> 5) * 4) = iVar3;
  }
  return 0;
}



/* @ 0x102f2f8  FUN_0102f2f8 */

void FUN_0102f2f8(void)

{
  FUN_0102f320();
  FUN_0102f5f0(DAT_0102f31c);
  FUN_0102f610();
  FUN_0102fb7c(0xf0000000);
  FUN_0102fb96();
  return;
}



/* @ 0x102f320  FUN_0102f320 */

void FUN_0102f320(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined1 uStack_25;
  undefined1 auStack_24 [4];
  int local_20;
  undefined1 auStack_1c [20];
  
  iVar1 = DAT_0102f380;
  FUN_0102f768(DAT_0102f384,DAT_0102f380);
  iVar2 = FUN_0102f988(DAT_0102f38c,DAT_0102f390,*(int *)(iVar1 + 0x14) != DAT_0102f388,auStack_1c);
  if (((iVar2 == 0) && (iVar2 = FUN_0102f9e8(auStack_1c,&uStack_25,auStack_24), iVar2 == 0)) &&
     (iVar2 = FUN_0102fa20(auStack_1c,0,&local_20), iVar2 == 0)) {
    *(int *)(iVar1 + 4) = local_20 * 1000;
  }
  uVar3 = *(uint *)(iVar1 + 4) >> 4;
  FUN_0102f0ec(uVar3);
  *(uint *)(DAT_0102f394 + 0x1004) = uVar3;
  return;
}



/* @ 0x102f398  FUN_0102f398 */

uint FUN_0102f398(uint param_1,int param_2,uint param_3,int param_4,char *param_5,uint param_6,
                 uint param_7)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  char *pcVar4;
  char *pcVar5;
  uint uVar6;
  char cVar7;
  uint uVar8;
  
  if (param_3 < 0x11) {
    pcVar5 = param_5;
    if (((int)param_1 < 0) && (param_2 == 0)) {
      param_1 = -param_1;
      bVar1 = true;
    }
    else {
      bVar1 = false;
    }
    do {
      uVar8 = param_1 / param_3;
      pcVar4 = pcVar5 + 1;
      iVar3 = param_1 - param_3 * uVar8;
      cVar2 = (char)iVar3;
      if (iVar3 < AL_I2C_INTR_MASK_STOP_DET_SHIFT + 1) {
        cVar2 = cVar2 + '0';
      }
      else {
        if (param_4 == 0) {
          cVar7 = 'a';
        }
        else {
          cVar7 = 'A';
        }
        cVar2 = cVar2 + -10 + cVar7;
      }
      *pcVar5 = cVar2;
      param_1 = uVar8;
      pcVar5 = pcVar4;
    } while (uVar8 != 0);
    for (; (uint)((int)pcVar4 - (int)param_5) < param_6; pcVar4 = pcVar4 + 1) {
      *pcVar4 = '0';
    }
    for (; (uint)((int)pcVar4 - (int)param_5) < param_7; pcVar4 = pcVar4 + 1) {
      *pcVar4 = ' ';
    }
    pcVar5 = pcVar4;
    if (bVar1) {
      pcVar5 = pcVar4 + 1;
      *pcVar4 = '-';
    }
    *pcVar5 = '\0';
    uVar8 = (int)pcVar5 - (int)param_5;
    pcVar5 = param_5 + uVar8;
    for (uVar6 = 0; uVar6 != uVar8 >> 1; uVar6 = uVar6 + 1) {
      cVar2 = param_5[uVar6];
      param_5[uVar6] = pcVar5[-1];
      pcVar5 = pcVar5 + -1;
      *pcVar5 = cVar2;
    }
    return uVar8;
  }
  return 0;
}



/* @ 0x102f442  FUN_0102f442 */

uint FUN_0102f442(int param_1,uint param_2)

{
  uint uVar1;
  undefined1 *puVar2;
  int *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_0102fc64();
    return param_2;
  }
  uVar1 = *in_r12 + -1 + (in_r12[2] - in_r12[1]);
  if (uVar1 < param_2) {
    param_2 = uVar1;
  }
  for (uVar1 = 0; puVar2 = (undefined1 *)in_r12[1], uVar1 != param_2; uVar1 = uVar1 + 1) {
    in_r12[1] = (int)(puVar2 + 1);
    *puVar2 = *(undefined1 *)(param_1 + uVar1);
  }
  *puVar2 = 0;
  return uVar1;
}



/* @ 0x102f482  FUN_0102f482 */

undefined4 FUN_0102f482(undefined1 param_1)

{
  undefined1 *puVar1;
  undefined4 *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_0102fc48();
  }
  else {
    puVar1 = (undefined1 *)in_r12[1];
    if ((undefined1 *)*in_r12 <= puVar1 + (1 - in_r12[2])) {
      return 0;
    }
    in_r12[1] = puVar1 + 1;
    *puVar1 = param_1;
    *(undefined1 *)in_r12[1] = 0;
  }
  return 1;
}



/* @ 0x102f4b0  FUN_0102f4b0 */

undefined4 FUN_0102f4b0(undefined4 param_1,uint param_2,byte *param_3,int *param_4)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  byte bVar5;
  byte *pbVar6;
  undefined1 auStack_30 [24];
  
LAB_0102f4c0:
  do {
    while( true ) {
      uVar1 = (uint)*param_3;
      if (uVar1 == 0) {
        return 0;
      }
      if (param_2 < 2) {
        return 0;
      }
      if (uVar1 != 0x25) {
        pbVar6 = param_3 + 1;
        goto LAB_0102f5e2;
      }
      uVar1 = (uint)param_3[1];
      if (uVar1 == 0x30) {
        if (param_3[2] == 0) {
          return 0;
        }
        bVar5 = param_3[2] - 0x30;
        uVar1 = (uint)param_3[3];
        uVar4 = 0;
        pbVar6 = param_3 + 4;
        if (AL_I2C_INTR_MASK_START_DET_SHIFT - 1 < bVar5) {
          bVar5 = 0;
        }
      }
      else if (uVar1 - 0x31 < AL_I2C_INTR_MASK_ACTIVITY_SHIFT + 1) {
        pbVar6 = param_3 + 3;
        uVar4 = uVar1 - 0x30 & 0xff;
        bVar5 = 0;
        uVar1 = (uint)param_3[2];
      }
      else {
        uVar4 = 0;
        pbVar6 = param_3 + 2;
        bVar5 = 0;
      }
      param_3 = pbVar6;
      if (uVar1 != 100) break;
      uVar3 = 0;
      iVar2 = *param_4;
LAB_0102f570:
      param_4 = param_4 + 1;
      uVar3 = FUN_0102f398(iVar2,uVar3,AL_I2C_INTR_MASK_START_DET_SHIFT,0,auStack_30,bVar5,uVar4);
      FUN_0102f442(auStack_30,uVar3);
    }
    if (uVar1 < 0x65) {
      if (uVar1 == 0x58) goto LAB_0102f584;
      if (uVar1 != 99) {
        if (uVar1 == 0) {
          return 0;
        }
        goto LAB_0102f5e2;
      }
      FUN_0102f482((char)*param_4);
LAB_0102f5d8:
      param_4 = param_4 + 1;
      goto LAB_0102f4c0;
    }
    if (uVar1 == 0x73) {
      for (iVar2 = 0; *(char *)(*param_4 + iVar2) != '\0'; iVar2 = iVar2 + 1) {
      }
      FUN_0102f442();
      goto LAB_0102f5d8;
    }
    if (uVar1 < 0x74) {
      if (uVar1 != 0x70) goto LAB_0102f5e2;
      bVar5 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
LAB_0102f584:
      iVar2 = *param_4;
      param_4 = param_4 + 1;
      uVar3 = FUN_0102f398(iVar2,1,0x10,uVar1 == 0x58,auStack_30,bVar5,uVar4);
      FUN_0102f442(auStack_30,uVar3);
    }
    else {
      if (uVar1 == 0x75) {
        uVar3 = 1;
        iVar2 = *param_4;
        goto LAB_0102f570;
      }
      if (uVar1 == 0x78) goto LAB_0102f584;
LAB_0102f5e2:
      FUN_0102f482(uVar1);
      param_3 = pbVar6;
    }
  } while( true );
}



/* @ 0x102f5f0  FUN_0102f5f0 */

void FUN_0102f5f0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_0102f4b0(0,0xffffffff,param_1,&uStack_c,param_1,&uStack_c,param_3);
  return;
}



/* @ 0x102f610  FUN_0102f610 */

void FUN_0102f610(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  
  FUN_0102f72c();
  iVar2 = 0;
  do {
    iVar3 = iVar2 + 1;
    FUN_0102fac4(*DAT_0102f65c,iVar2,0,DAT_0102f660);
    puVar1 = DAT_0102f664;
    iVar2 = iVar3;
  } while (iVar3 != 4);
  DAT_0102f664[2] = 0;
  puVar1[3] = 0;
  puVar1[4] = 0;
  puVar1[5] = 0;
  puVar1[6] = 0;
  puVar1[7] = 0;
  puVar1[8] = 0;
  puVar1[9] = 0;
  puVar1[1] = 0;
  *puVar1 = DAT_0102f668;
  return;
}



/* @ 0x102f66c  FUN_0102f66c */

void FUN_0102f66c(uint param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  uint uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  code *UNRECOVERED_JUMPTABLE;
  undefined8 uVar8;
  
  uVar6 = DAT_0102f720;
  iVar7 = param_1 + 1;
  FUN_0102f72c();
  iVar1 = DAT_0102f724;
  uVar6 = uVar6 & param_1;
  uVar4 = *(uint *)(DAT_0102f724 + iVar7 * 8);
  if ((int)uVar6 < 0) {
    uVar6 = (uVar6 - 1 | 0xfffffffc) + 1;
  }
  while( true ) {
    iVar2 = DAT_0102f724;
    UNRECOVERED_JUMPTABLE = *(code **)(iVar1 + iVar7 * 8 + 4);
    uVar3 = *(undefined4 *)(DAT_0102f728 + ((int)param_1 / 4) * 4);
    if ((UNRECOVERED_JUMPTABLE != (code *)0x0) && ((uVar4 & 4) == 0)) break;
    FUN_0102fa9c(uVar3,uVar6,3);
    WaitForInterrupt();
  }
  uVar3 = FUN_0102fa9c(uVar3,uVar6,0);
  uVar6 = *(uint *)(iVar2 + 4);
  uVar8 = CONCAT44(uVar6 << 0x1d,uVar3);
  if ((int)(uVar6 << 0x1d) < 0) {
    uVar6 = uVar6 & 0xfffffffb;
    *(uint *)(iVar2 + 4) = uVar6;
    FUN_0102fb96();
    uVar8 = FUN_0102f064(1);
  }
  iVar2 = DAT_0102f724;
  uVar3 = (undefined4)((ulonglong)uVar8 >> 0x20);
  iVar5 = uVar6 << 0x1f;
  if (-1 < iVar5) {
    uVar4 = *(uint *)(iVar1 + iVar7 * 8);
    if ((uVar4 & 2) == 0) {
      uVar8 = FUN_0102fb98();
      iVar5 = *(int *)(iVar2 + iVar7 * 8);
      *(uint *)(iVar2 + iVar7 * 8) = uVar4 | 2;
    }
    FUN_0102fc20((int)uVar8,(int)((ulonglong)uVar8 >> 0x20),iVar5);
    FUN_0102fdc8();
    FUN_0102fd64();
    FUN_0102fd10();
    uVar3 = extraout_r1;
    if ((int)(uVar6 << 0x1e) < 0) {
      FUN_0102fcfc();
      uVar3 = extraout_r1_00;
    }
  }
  coproc_moveto_Invalidate_Entire_Instruction(uVar3);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
                    /* WARNING: Could not recover jumptable at 0x0102f0d4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0x102f72c  FUN_0102f72c */

void FUN_0102f72c(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
  piVar1 = DAT_0102f758;
  if (*DAT_0102f758 == 0) {
    FUN_0102fa84(DAT_0102f75c,DAT_0102f760,param_3,DAT_0102f758,param_4);
    iVar3 = 0;
    do {
      iVar2 = DAT_0102f764 + iVar3;
      iVar3 = iVar3 + AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
      FUN_0102fa94(iVar2,DAT_0102f75c,0);
    } while (iVar3 != 0x20);
    *piVar1 = 1;
  }
  return;
}



/* @ 0x102f768  FUN_0102f768 */

void FUN_0102f768(int param_1,undefined4 *param_2)

{
  bool bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined1 uVar5;
  undefined1 uVar6;
  uint uVar7;
  
  uVar4 = *(uint *)(param_1 + 0x110);
  uVar2 = DAT_0102f8d0;
  if ((uVar4 & 0x80000) == 0) {
    uVar2 = DAT_0102f8cc;
  }
  param_2[5] = uVar2;
  uVar3 = DAT_0102f90c;
  switch(uVar4 & 0xf) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_0102f8d4;
    break;
  case 2:
    uVar3 = DAT_0102f8d8;
    break;
  case 3:
    uVar3 = DAT_0102f8dc;
    break;
  case 4:
    uVar3 = DAT_0102f8e0;
    break;
  case 5:
    uVar3 = DAT_0102f8e4;
    break;
  case 6:
    uVar3 = DAT_0102f8e8;
    break;
  case 7:
    uVar3 = DAT_0102f8ec;
    break;
  case 8:
    uVar3 = DAT_0102f8f0;
    break;
  case 9:
    uVar3 = DAT_0102f8f4;
    break;
  case 10:
    uVar3 = DAT_0102f8f8;
    break;
  case 0xb:
    uVar3 = DAT_0102f8fc;
    break;
  case 0xc:
    uVar3 = DAT_0102f900;
    break;
  case 0xd:
    uVar3 = DAT_0102f904;
    break;
  case 0xe:
    uVar3 = DAT_0102f908;
  }
  *param_2 = uVar3;
  uVar3 = DAT_0102f910;
  switch((uVar4 & 0x7f) >> 4) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_0102f914;
    break;
  case 2:
    uVar3 = DAT_0102f918;
    break;
  case 3:
    uVar3 = DAT_0102f91c;
    break;
  case 4:
    uVar3 = DAT_0102f920;
    break;
  case 5:
    uVar3 = DAT_0102f924;
    break;
  case 6:
    uVar3 = DAT_0102f928;
  }
  uVar7 = (uVar4 & 0x1ff) >> 7;
  param_2[1] = uVar3;
  if (uVar7 == 0) {
    bVar1 = true;
    uVar3 = uVar2;
  }
  else {
    bVar1 = false;
    uVar3 = DAT_0102f92c;
    if (uVar7 != 1) {
      uVar3 = DAT_0102f8dc;
    }
  }
  param_2[2] = uVar3;
  if ((bVar1) ||
     (((uVar7 = (uVar4 & 0x7ff) >> 9, uVar3 = DAT_0102f934, uVar2 = DAT_0102f930, uVar7 != 2 &&
       (uVar3 = DAT_0102f938, uVar7 != 3)) && (uVar3 = DAT_0102f93c, uVar7 != 0)))) {
    uVar3 = uVar2;
  }
  param_2[4] = uVar2;
  param_2[3] = uVar3;
  uVar6 = 1;
  *(byte *)(param_2 + 6) = (byte)((uVar4 << 0x14) >> 0x1f);
  uVar7 = (uVar4 & 0x3fff) >> 0xc;
  param_2[9] = 1;
  if (uVar7 == 2) {
    uVar5 = 0;
  }
  else if ((uVar7 == 3) || (uVar7 == 1)) {
    uVar5 = 1;
    uVar6 = 0;
  }
  else {
    uVar5 = 1;
  }
  *(undefined1 *)((int)param_2 + 0x1a) = uVar5;
  *(undefined1 *)((int)param_2 + 0x19) = uVar6;
  *(byte *)((int)param_2 + 0x1b) = (byte)((uVar4 << 0x11) >> 0x1f);
  uVar7 = ((uVar4 & 0x3ffff) >> 0xf) - 2;
  if (uVar7 < 4) {
    uVar6 = *(undefined1 *)(DAT_0102f940 + uVar7);
  }
  else {
    uVar6 = 3;
  }
  *(undefined1 *)(param_2 + 7) = uVar6;
  *(byte *)((int)param_2 + 0x1d) = (byte)((uVar4 << 0xd) >> 0x1f);
  uVar7 = (uVar4 & 0x3fffff) >> 0x14;
  uVar6 = 1;
  if (uVar7 != 1) {
    if (uVar7 == 3) {
      uVar6 = 2;
    }
    else {
      uVar6 = 0;
    }
  }
  *(undefined1 *)((int)param_2 + 0x1e) = uVar6;
  *(byte *)((int)param_2 + 0x1f) = (byte)((uVar4 << 9) >> 0x1f);
  if ((uVar4 & 0x800000) == 0) {
    uVar6 = 0x50;
  }
  else {
    uVar6 = 0x57;
  }
  *(undefined1 *)((int)param_2 + 0x21) = 0;
  *(undefined1 *)(param_2 + 8) = uVar6;
  return;
}



/* @ 0x102f944  FUN_0102f944 */

uint FUN_0102f944(int *param_1)

{
  uint uVar1;
  uint *puVar2;
  uint uVar3;
  int iVar4;
  
  puVar2 = (uint *)*param_1;
  if ((int)puVar2[7] < 0) {
    uVar1 = param_1[4];
    if (-1 < (int)puVar2[6]) {
      uVar3 = *puVar2;
      iVar4 = ((uVar3 & 0x3fffff) >> 0x10) + 1;
      return ((uVar3 & 0x1fff) * uVar1 + uVar1) / (((uVar3 & 0xfffffff) >> 0x18) * iVar4 + iVar4);
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}



/* @ 0x102f976  FUN_0102f976 */

bool FUN_0102f976(int param_1)

{
  return ((*(uint *)(param_1 + 0x1c) & 0xfff) >> 8) - 6 < 2;
}



/* @ 0x102f988  FUN_0102f988 */

undefined4 FUN_0102f988(undefined4 param_1,undefined4 param_2,int param_3,undefined4 *param_4)

{
  undefined4 uVar1;
  
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    param_4[4] = DAT_0102f9d4;
    param_4[2] = DAT_0102f9d8;
    uVar1 = 0x14;
  }
  else {
    if (param_3 == 0) {
      param_4[4] = 25000;
      uVar1 = DAT_0102f9dc;
    }
    else {
      if (param_3 != 2) {
        FUN_0102f5f0(DAT_0102f9e4,DAT_0102f9e0);
        return 0xffffffea;
      }
      param_4[4] = DAT_0102f9cc;
      uVar1 = DAT_0102f9d0;
    }
    param_4[2] = uVar1;
    uVar1 = 0x13;
  }
  param_4[3] = uVar1;
  return 0;
}



/* @ 0x102f9e8  FUN_0102f9e8 */

undefined4 FUN_0102f9e8(int param_1,undefined1 *param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  
  puVar3 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar1 = FUN_0102f944();
  *param_3 = iVar1;
  iVar2 = 0;
  while( true ) {
    if (*(int *)(param_1 + 0xc) <= iVar2) {
      return 0;
    }
    if (iVar1 == *(int *)(puVar3 + 4)) break;
    iVar2 = iVar2 + 1;
    puVar3 = puVar3 + 0x18;
  }
  *param_2 = *puVar3;
  return 0;
}



/* @ 0x102fa20  FUN_0102fa20 */

uint FUN_0102fa20(int *param_1,uint param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  *param_3 = 0;
  iVar1 = FUN_0102f976(*param_1);
  if (iVar1 == 0) {
    return 0;
  }
  uVar2 = FUN_0102f944(param_1);
  uVar3 = *(uint *)(*param_1 + ((param_2 >> 1) + AL_I2C_INTR_MASK_ACTIVITY_SHIFT) * 4);
  if ((int)(param_2 << 0x1f) < 0) {
    uVar3 = uVar3 >> 0x10;
  }
  else {
    uVar3 = uVar3 & 0xffff;
  }
  uVar4 = uVar3 >> 0xf;
  if ((uVar3 & 0x3ff) == 0) {
    if (uVar4 != 0) {
      *param_3 = uVar2;
      return 0;
    }
    *param_3 = 0;
  }
  else {
    if (uVar4 != 0) {
      FUN_0102f5f0(DAT_0102fa7c,DAT_0102fa80);
      return 0xfffffffb;
    }
    *param_3 = uVar2 / (uVar3 & 0x3ff);
  }
  return uVar4;
}



/* @ 0x102fa84  FUN_0102fa84 */

void FUN_0102fa84(uint *param_1,uint param_2)

{
  uint uVar1;
  
  uVar1 = *(uint *)(param_2 + 0x4400);
  param_1[1] = param_2;
  *param_1 = (uVar1 & 0xffff) >> 8;
  return;
}



/* @ 0x102fa94  FUN_0102fa94 */

void FUN_0102fa94(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  *param_1 = param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0x102fa9c  FUN_0102fa9c */

void FUN_0102fa9c(int *param_1,int param_2,undefined4 param_3)

{
  uint uVar1;
  
  uVar1 = *(uint *)*param_1;
  if (uVar1 < 4) {
    *(undefined4 *)(((uint *)*param_1)[1] + param_2 * 0x100 + 0x2020) = param_3;
    return;
  }
  FUN_0102f5f0(DAT_0102fabc,DAT_0102fac0,uVar1);
  return;
}



/* @ 0x102fac4  FUN_0102fac4 */

void FUN_0102fac4(int *param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  if (*(uint *)*param_1 < 4) {
    iVar1 = ((uint *)*param_1)[1] + param_2 * 0x100;
    *(undefined4 *)(iVar1 + 0x2028) = param_4;
    *(undefined4 *)(iVar1 + 0x202c) = param_3;
    return;
  }
  FUN_0102f5f0(DAT_0102faf0,DAT_0102faf4);
  return;
}



/* @ 0x102faf8  FUN_0102faf8 */

void FUN_0102faf8(int param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  puVar1 = DAT_0102fb6c;
  if (param_1 != 0) {
    puVar1 = DAT_0102fb70;
  }
  iVar2 = ((*puVar1 & 0x1f) + 1) * 0x20;
  for (iVar3 = 0x20; iVar3 < iVar2; iVar3 = iVar3 + 0x20) {
    if (param_1 == 0) {
      iVar4 = 0x200000;
    }
    else {
      iVar4 = 0x9000;
    }
    *(undefined4 *)((iVar3 >> 3) + -0xfffff80 + iVar4) = 0xffffffff;
  }
  for (iVar3 = 0x20; iVar3 < iVar2; iVar3 = iVar3 + 4) {
    if (param_1 == 0) {
      iVar4 = 0x200000;
    }
    else {
      iVar4 = 0x9000;
    }
    *(undefined4 *)(iVar4 + iVar3 + -0xffffc00) = 0x80808080;
  }
  puVar1 = DAT_0102fb78;
  if (param_1 == 0) {
    puVar1 = DAT_0102fb74;
  }
  *puVar1 = *puVar1 | 1;
  return;
}



/* @ 0x102fb7c  FUN_0102fb7c */

void FUN_0102fb7c(int param_1)

{
  *(undefined4 *)(param_1 + 0x90008) = 1;
  FUN_0102faf8(0);
  FUN_0102faf8(1);
  return;
}



/* @ 0x102fb96  FUN_0102fb96 */

void FUN_0102fb96(void)

{
  return;
}



/* @ 0x102fb98  FUN_0102fb98 */

void FUN_0102fb98(void)

{
  undefined4 *puVar1;
  uint *puVar2;
  int iVar3;
  
  puVar2 = DAT_0102fc14;
  puVar1 = DAT_0102fc10;
  *(undefined4 *)(DAT_0102fc0c + 0x80) = 0xffffffff;
  *puVar1 = 0x80808080;
  puVar1[1] = 0x80808080;
  puVar1[2] = 0x80808080;
  puVar1[3] = 0x80808080;
  puVar1[4] = 0x80808080;
  puVar1[5] = 0x80808080;
  puVar1[6] = 0x80808080;
  puVar1[7] = 0x80808080;
  puVar1[-0x400ff] = 0xff;
  *puVar2 = *puVar2 & 0xfffffd82 | 0x79;
  iVar3 = DAT_0102fc18;
  *(undefined4 *)(DAT_0102fc18 + 0x80) = 0xffffffff;
  *(undefined4 *)(iVar3 + 0x400) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x404) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x408) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x40c) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x410) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x414) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x418) = 0x80808080;
  *(undefined4 *)(iVar3 + 0x41c) = 0x80808080;
  *DAT_0102fc1c = 0xff;
  *(uint *)(iVar3 + 0x1000) = *(uint *)(iVar3 + 0x1000) & 0xfffffd82 | 0x78;
  return;
}



/* @ 0x102fc20  FUN_0102fc20 */

void FUN_0102fc20(void)

{
  int iVar1;
  
  iVar1 = FUN_0102efe8();
  *(undefined4 *)(iVar1 * 0x20000 + -0xfd7ffec) = 0;
  do {
  } while (*(int *)(iVar1 * 0x20000 + -0xfd7ffec) << 0x1d < 0);
  *DAT_0102fc44 = 0xff;
  return;
}



/* @ 0x102fc48  FUN_0102fc48 */

uint FUN_0102fc48(uint param_1)

{
  FUN_0102fc78(0,param_1 & 0xff);
  if (param_1 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
    FUN_0102fc78(0,0xd);
  }
  return param_1;
}



/* @ 0x102fc64  FUN_0102fc64 */

void FUN_0102fc64(int param_1)

{
  char *pcVar1;
  
  pcVar1 = (char *)(param_1 + -1);
  while( true ) {
    pcVar1 = pcVar1 + 1;
    if (*pcVar1 == '\0') break;
    FUN_0102fc48();
  }
  return;
}



/* @ 0x102fc78  FUN_0102fc78 */

void FUN_0102fc78(int param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  
  puVar1 = *(undefined4 **)(DAT_0102fc8c + param_1 * 4);
  do {
  } while (-1 < (int)(puVar1[5] << 0x19));
  *puVar1 = param_2;
  return;
}



/* @ 0x102fcfc  FUN_0102fcfc */

void FUN_0102fcfc(void)

{
  software_hvc(0);
  return;
}



/* @ 0x102fd10  FUN_0102fd10 */

void FUN_0102fd10(void)

{
  uint uVar1;
  
  coproc_movefrom_Control();
  uVar1 = coproc_movefrom_Secure_Configuration();
  coproc_moveto_Secure_Configuration(uVar1 | 1);
  InstructionSynchronizationBarrier(0xf);
                    /* WARNING: Could not recover jumptable at 0x0102fd60. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_0102fe14)(DAT_0102fe14,uVar1 | 1);
  return;
}



/* @ 0x102fd24  FUN_0102fd24 */

void FUN_0102fd24(void)

{
  uint uVar1;
  int in_r3;
  
  if (in_r3 != 0) {
    uVar1 = coproc_movefrom_Control();
    coproc_moveto_Control(uVar1 | 0x1000);
    InstructionSynchronizationBarrier(0xf);
  }
  return;
}



/* @ 0x102fd64  FUN_0102fd64 */

void FUN_0102fd64(void)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr12;
  
  coprocessor_moveto(0xf,0,1,0x102fcb0,in_cr12,in_cr0);
  software_smc(0);
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  coprocessor_moveto(0xf,0,1,DAT_0102fe1c,in_cr12,in_cr0);
  uVar2 = coproc_movefrom_Secure_Configuration();
  coproc_moveto_Secure_Configuration(DAT_0102fe20 | uVar2 & 0xfffffd80);
  coprocessor_moveto(0xf,4,0,DAT_0102fe24,in_cr12,in_cr0);
  coproc_moveto_Secure_Configuration(DAT_0102fe20 & 0xfffffffe | uVar2 & 0xfffffd80);
                    /* WARNING: Could not recover jumptable at 0x0102fdc0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_0102fe28)(uVar1 & 0xf,DAT_0102fe28,DAT_0102fe24);
  return;
}



/* @ 0x102fdc8  FUN_0102fdc8 */

void FUN_0102fdc8(undefined4 param_1)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 in_cr15;
  
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x80000000);
  FUN_0102eff4(param_1,uVar1 | 0x80000000);
  uVar1 = coproc_movefrom_NonSecure_Access_Control();
  coproc_moveto_NonSecure_Access_Control(uVar1 | 0x60c00);
  uVar1 = coprocessor_movefromRt(0xf,1,in_cr15);
  uVar2 = coprocessor_movefromRt2(0xf,1,in_cr15);
  coprocessor_moveto2(0xf,1,uVar1 | 0x40,uVar2,in_cr15);
  return;
}



/* @ 0x102fe00  FUN_0102fe00 */

void FUN_0102fe00(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int unaff_lr;
  
  FUN_0102f134(0);
                    /* WARNING: Could not recover jumptable at 0x0102fe10. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(unaff_lr + -4))(param_1,param_2,param_3,param_4);
  return;
}



