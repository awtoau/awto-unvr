/* @ 0x1000088 */

uint FUN_01000088(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return uVar1 & 0xf;
}



/* @ 0x10000bc */

void FUN_010000bc(uint param_1,int param_2,int param_3)

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
      } while (((DAT_01000224 & uVar1) >> 0xd) + 1 != iVar5);
      iVar2 = iVar2 + 1;
    } while (iVar2 != iVar6);
    InstructionSynchronizationBarrier(0xf);
    DataSynchronizationBarrier(0xf);
    bVar7 = uVar3 != (param_1 & 0xfffffffe);
    uVar3 = param_1 & 0xfffffffe;
  } while (bVar7);
  return;
}



/* @ 0x1000164 */

void FUN_01000164(void)

{
  FUN_010000bc();
  return;
}



/* @ 0x1000170 */

void FUN_01000170(void)

{
  coproc_moveto_Invalidate_Entire_Instruction(0);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return;
}



/* @ 0x1000184 */

void FUN_01000184(undefined4 param_1,undefined4 param_2)

{
  coproc_moveto_Invalidate_unified_TLB_unlocked(param_2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return;
}



/* @ 0x10001e4 */

ulonglong FUN_010001e4(undefined4 param_1)

{
  uint uVar1;
  undefined4 in_cr2;
  undefined4 in_cr10;
  
  coprocessor_moveto2(0xf,0,param_1,0,in_cr2);
  coprocessor_moveto2(0xf,1,param_1,0,in_cr2);
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x40);
  coproc_moveto_Translation_table_control(DAT_01000228);
  coprocessor_moveto(0xf,0,0,DAT_0100022c,in_cr10,in_cr2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return CONCAT44(uVar1,DAT_0100022c) | 0x4000000000;
}



/* @ 0x1000284 */

undefined8 FUN_01000284(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 in_cr14;
  
  uVar1 = coprocessor_movefromRt(0xf,0,in_cr14);
  uVar2 = coprocessor_movefromRt2(0xf,0,in_cr14);
  return CONCAT44(uVar2,uVar1);
}



/* @ 0x1000370 */

undefined4 FUN_01000370(int param_1,int param_2,int param_3,int param_4)

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



/* @ 0x10003e4 */

ulonglong thunk_FUN_010001e4(undefined4 param_1)

{
  uint uVar1;
  undefined4 in_cr2;
  undefined4 in_cr10;
  
  coprocessor_moveto2(0xf,0,param_1,0,in_cr2);
  coprocessor_moveto2(0xf,1,param_1,0,in_cr2);
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x40);
  coproc_moveto_Translation_table_control(DAT_01000228);
  coprocessor_moveto(0xf,0,0,DAT_0100022c,in_cr10,in_cr2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  return CONCAT44(uVar1,DAT_0100022c) | 0x4000000000;
}



/* @ 0x10003e8 */

void FUN_010003e8(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_01010c00(&DAT_01028ee4,s_TEST_ERROR____s__returned__d__s__01028eac + 0x2c);
  FUN_01010be4(0x1010af4,0,param_1,&uStack_c);
  FUN_01010c00(&DAT_01028ee4,s_al_flash_toc_find_id_failed__boo_01030944 + 0x24);
  return;
}



/* @ 0x1000458 */

void FUN_01000458(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_01000088();
  FUN_010003e8(&DAT_01028ee8,uVar1);
  if (param_1 != 0) {
    FUN_0100ceb0(s_CPU__d_failed_miserably_01028eec + 0x18,param_1);
    FUN_01010c00(s_CPU__d_failed_miserably_01028eec + 0x18,param_1);
  }
  FUN_01007800(0x80ff);
  if (DAT_01049300 == 0) {
    FUN_01007274(0,1,0x100);
    DAT_01049300 = 1;
  }
  FUN_0100cef8(10000);
  FUN_0100ceb0(s_The_test_s_final_words_are___s_01028f08 + 0x1c);
  FUN_01010c00(s_The_test_s_final_words_are___s_01028f08 + 0x1c);
  FUN_0100ceb0(s_Press__r__to_reset_01028f28 + 0x10);
  FUN_01010c00(s_Press__r__to_reset_01028f28 + 0x10);
  FUN_0100ceb0(s_Press_CTRL_R_to_print_trace_agai_01028f3c + 0x20);
  FUN_01010c00(s_Press_CTRL_R_to_print_trace_agai_01028f3c + 0x20);
  do {
    while( true ) {
      do {
        iVar2 = FUN_0100d204(uVar1);
      } while (iVar2 == 0);
      iVar2 = FUN_0100d0e0(uVar1);
      if (iVar2 == 0x72) break;
      if (iVar2 == 0x12) {
        FUN_0100cef8(10000);
      }
      else if (iVar2 == 6) {
        FUN_0100cef8(0);
      }
    }
    FUN_0100cb78();
  } while( true );
}



/* @ 0x100057c */

void FUN_0100057c(undefined4 param_1,int param_2)

{
  uint uVar1;
  undefined4 unaff_r4;
  
  DAT_01049304 = param_1;
  thunk_FUN_010001e4();
  if (param_2 == -0x10000) {
    uVar1 = coproc_movefrom_Control();
    coproc_moveto_Control(uVar1 | 0x2000);
    DataSynchronizationBarrier(0xf);
    return;
  }
  if (param_2 != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_arch_arm_src_mmu_c_01028e90 + 0x10,param_2,
                 s_set_vectors_01028e84 + 8,&DAT_01028e80,0x46,unaff_r4);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_arch_arm_src_mmu_c_01028e90 + 0x10,param_2,
                 s_set_vectors_01028e84 + 8,&DAT_01028e80,0x46);
    FUN_01000458(0);
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




/* @ 0x10005ac -- WARNING: Control flow encountered bad instruction data */

void FUN_010005ac(void)

{
  FUN_01002e90();
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



/* @ 0x1000618 */

void FUN_01000618(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x1000640 */

void FUN_01000640(void)

{
  FUN_0100ceb0(s_Press_CTRL_F_to_print_full_trace_01028f60 + 0x20);
  FUN_01010c00(s_Press_CTRL_F_to_print_full_trace_01028f60 + 0x20);
  FUN_0100ceb0(&DAT_01028fc0,s__________________________________01028f84 + 0x34);
  FUN_01010c00(&DAT_01028fc0,s__________________________________01028f84 + 0x34);
  FUN_0100ceb0(&DAT_01028fe0,s_Stage_3_version___s_01028fc4 + 0x14);
  FUN_01010c00(&DAT_01028fe0,s_Stage_3_version___s_01028fc4 + 0x14);
  FUN_0100ceb0(&DAT_01028ff8,s_Commit_ID___s_01028fe4 + 0xc);
  FUN_01010c00(&DAT_01028ff8,s_Commit_ID___s_01028fe4 + 0xc);
  FUN_0100ceb0(&DAT_01029014,s_CVOS_commit_ID___s_01028ffc + 0x10);
  FUN_01010c00(&DAT_01029014,s_CVOS_commit_ID___s_01028ffc + 0x10);
  FUN_0100ceb0(&DAT_01029040,&DAT_01029034,s_HAL_commit_ID___s_01029018 + 0x10);
  FUN_01010c00(&DAT_01029040,&DAT_01029034,s_HAL_commit_ID___s_01029018 + 0x10);
  FUN_0100ceb0(s_Build_date___s__s_01029044 + 0x10);
  FUN_01010c00(s_Build_date___s__s_01029044 + 0x10);
  return;
}




void FUN_01000778(void)

{
  code *UNRECOVERED_JUMPTABLE;
  
/* @ 0x1000778 -- WARNING: Could not recover jumptable at 0x01000778. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0x1000838 */

undefined4 FUN_01000838(int param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  undefined1 auStack_6c [4];
  undefined1 auStack_68 [80];
  
  FUN_01010b88(auStack_68,s__s__iodma_memset_failed__010305f0 + 0x18,param_1);
  iVar1 = FUN_01007f88(DAT_01049d88,auStack_68);
  if (iVar1 < 0) {
    uVar4 = 0xffffffea;
  }
  else {
    iVar2 = FUN_01007ebc(DAT_01049d88,iVar1,s__soc_board_cfg_ethernet_port_d_0103060c + 0x1c,0);
    if ((iVar2 == 0) || (iVar2 = FUN_01012958(iVar2,s_status_0103062c + 4), iVar2 != 0)) {
      uVar4 = 0;
    }
    else {
      iVar2 = FUN_01007ebc(DAT_01049d88,iVar1,s_enabled_01030634 + 4,0);
      if (iVar2 != 0) {
        iVar2 = FUN_01012958(iVar2,s_ethernet_port___d__mode_not_spec_01030644 + 0x3c);
        uVar4 = DAT_01049d88;
        (&DAT_01049324)[param_1] = (uint)(iVar2 == 0);
        iVar2 = FUN_01007e68(uVar4,iVar1,&DAT_01030688,auStack_6c);
        if (iVar2 == 0) {
          (&DAT_0104a57c)[param_1] = 3;
        }
        else {
          uVar3 = *(uint *)(iVar2 + 0xc);
          (&DAT_0104a57c)[param_1] =
               uVar3 << 0x18 | (uVar3 >> 8 & 0xff) << 0x10 | (uVar3 >> 0x10 & 0xff) << 8 |
               uVar3 >> 0x18;
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar1,s_serdes_grp_0103068c + 8,auStack_6c);
        if (iVar1 == 0) {
          *(int *)(&DAT_0104a58c + param_1 * 4) = 3 - param_1;
        }
        else {
          uVar3 = *(uint *)(iVar1 + 0xc);
          *(uint *)(&DAT_0104a58c + param_1 * 4) =
               uVar3 << 0x18 | (uVar3 >> 8 & 0xff) << 0x10 | (uVar3 >> 0x10 & 0xff) << 8 |
               uVar3 >> 0x18;
        }
        FUN_01010b88(auStack_68,s_serdes_lane_01030698 + 8,param_1);
        iVar1 = FUN_01007f88(DAT_01049d88,auStack_68);
        *(uint *)(&DAT_01049314 + param_1 * 4) = (uint)(0 < iVar1);
        (&DAT_0104a56c)[param_1] = 1;
        return 0;
      }
      uVar4 = 0xffffffea;
      FUN_0100ceb0(&DAT_01030640,param_1);
      FUN_01010c00(&DAT_01030640,param_1);
    }
  }
  if (((param_1 == 2) || (param_1 == 3)) || (param_1 == 1)) {
    FUN_01003f34(param_1,0);
  }
  else {
    FUN_01003f34(0);
  }
  return uVar4;
}



/* @ 0x1000a50 */

void FUN_01000a50(int *param_1)

{
  undefined1 uStack_2d;
  undefined1 auStack_2c [4];
  int local_28;
  undefined1 auStack_24 [20];
  
  FUN_01022328(0xfd8a8000,param_1);
  FUN_01023c98(0xfd860c00,&DAT_0103056c,param_1[5] != 25000000,auStack_24);
  FUN_01023ed4(auStack_24,&uStack_2d,auStack_2c);
  FUN_01023fd4(auStack_24,0,&local_28);
  param_1[1] = local_28 * 1000;
  FUN_01023c98(0xfd860d00,&DAT_01030574,param_1[5] != 25000000,auStack_24);
  FUN_01023ed4(auStack_24,&uStack_2d,auStack_2c);
  FUN_01023fd4(auStack_24,0,&local_28);
  *param_1 = local_28 * 1000;
  return;
}



/* @ 0x1000b34 */


undefined4 FUN_01000b34(void)

{
  int iVar1;
  uint uVar2;
  undefined1 auStack_50 [8];
  char local_48 [4];
  undefined4 uStack_44;
  undefined4 local_40;
  undefined4 uStack_3c;
  undefined4 local_38;
  undefined1 auStack_34 [24];
  
  local_48 = (char  [4])s__________________________________01029058._52_4_;
  uStack_44 = DAT_01029090;
  local_40 = DAT_01029094;
  uStack_3c = DAT_01029098;
  local_38 = DAT_0102909c;
  FUN_01005340();
  DAT_01049310 = &DAT_fbff4120;
  if (((_DAT_fbff4120 & 0xffffff00) == 0xf0e1d200) && (0xc2 < (_DAT_fbff4120 & 0xff))) {
    DAT_0104a50c = 1;
  }
  else {
    DAT_01049310 = (undefined1 *)0x0;
    DAT_0104a50c = 0;
    FUN_01000618();
  }
  FUN_01000a50(DAT_01001108);
  FUN_0100cf4c(0,DAT_01049da4,DAT_01049db8);
  FUN_0100cce0(0);
  FUN_0100d2c4();
  FUN_010073ac(1,DAT_01049d98,DAT_01049d9c,DAT_01049da4);
  FUN_01000640();
  iVar1 = FUN_01002f08();
  if ((iVar1 == 0) ||
     (iVar1 = FUN_01002f2c(),
     *(uint *)(iVar1 + 0xc) < 2 || *(uint *)(iVar1 + 0xc) == 2 && *(int *)(iVar1 + 8) == 0)) {
    FUN_0102383c(0xfd8a8000,3,0,2,0x1e);
    FUN_0102383c(0xfd8a8000,8,0,2,0x1e);
    FUN_01023a10(0xfd8a8000,2,0,2,0xc0000000,0,2);
  }
  else {
    FUN_0102383c(0xfd8a8000,0,0,0,0x1f);
    FUN_0102383c(0xfd8a8000,5,0,0,0x1f);
    FUN_0102383c(0xfd8a8000,1,0,4,0x1f);
    FUN_0102383c(0xfd8a8000,6,0,4,0x1f);
    FUN_0102383c(0xfd8a8000,2,0,1,0x20);
    FUN_0102383c(0xfd8a8000,7,0,1,0x20);
    FUN_0102383c(0xfd8a8000,3,0,2,0x21);
    FUN_0102383c(0xfd8a8000,8,0,2,0x21);
    FUN_01023a10(0xfd8a8000,3,0,4,0x80000000,0,3);
  }
  FUN_01021ff0(0xfd8a8000,0x2000,0x1002000);
  FUN_010221cc(0xfd8a8000,local_48);
  FUN_0102383c(0xfd8a8000,0x14,0,0,0xf);
  FUN_0102383c(0xfd8a8000,10,0xc0000000,0,0x1b);
  FUN_0102383c(0xfd8a8000,0xb,0xc8000000,0,0x1b);
  FUN_0102383c(0xfd8a8000,0xc,0xd0000000,0,0x1b);
  FUN_0102383c(0xfd8a8000,0xd,0xd8000000,0,0x1b);
  DAT_01049d90 = FUN_01021fd8(0xfd8a8000);
  DAT_01049d94 = FUN_01021fe4(0xfd8a8000);
  iVar1 = FUN_01024fdc(auStack_50,0xfd896000,0xfd8a8000);
  if (iVar1 == 0) {
    uVar2 = FUN_0102515c(auStack_50,0x1d);
    uVar2 = (uVar2 & 0x3ff) >> 6;
    if (uVar2 != 0) {
      iVar1 = FUN_01024b90(auStack_34,0xfd860a00,0xfd8a8000);
      if (iVar1 == 0) {
        FUN_01024bb8(auStack_34,uVar2);
      }
      else {
        FUN_0100ceb0(s__s__al_otp_handle_init_failed__01030dd4 + 0x1c,s_dt_based_init_01029104 + 0xc
                    );
        FUN_01010c00(s__s__al_otp_handle_init_failed__01030dd4 + 0x1c,s_dt_based_init_01029104 + 0xc
                    );
      }
    }
  }
  else {
    FUN_0100ceb0(s_Executing_APCEA___application____01030db0 + 0x20,s_dt_based_init_01029104 + 0xc);
    FUN_01010c00(s_Executing_APCEA___application____01030db0 + 0x20,s_dt_based_init_01029104 + 0xc);
  }
  _DAT_fd8a81dc = _DAT_fd8a81dc | 0x10000;
  FUN_01025484(DAT_010492cc,400);
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
    FUN_010129dc(&SUB_fbff4200,&LAB_0102ef70,0x15fc);
    FUN_01000170();
    (*(code *)&SUB_fbff4200)();
    FUN_0102755c(DAT_01049d90 != 0 || DAT_01049d94 != 0);
    DAT_01049310 = &DAT_fbff4120;
    if (((_DAT_fbff4120 & 0xffffff00) != 0xf0e1d200) || ((_DAT_fbff4120 & 0xff) < 0xc3)) {
      DAT_01049310 = (undefined1 *)0x0;
      FUN_0100ceb0(s__s__al_thermal_sensor_handle_ini_01030df4 + 0x28,
                   s_thermal_sensor_trim_init_01029114 + 0x18);
      FUN_01010c00(s__s__al_thermal_sensor_handle_ini_01030df4 + 0x28,
                   s_thermal_sensor_trim_init_01029114 + 0x18);
    }
  }
  return 0;
}




/* @ 0x100110c -- WARNING: Control flow encountered bad instruction data */

undefined4 FUN_0100110c(void)

{
  ushort uVar1;
  undefined1 *puVar2;
  ushort uVar3;
  bool bVar4;
  int iVar5;
  undefined4 uVar6;
  byte bVar7;
  uint uVar8;
  code *pcVar9;
  int *piVar10;
  int *piVar11;
  uint uVar12;
  undefined4 *puVar13;
  int *unaff_r8;
  uint *unaff_r10;
  uint *puVar14;
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
  iVar5 = FUN_01023c98(0xfd860c00,&DAT_0103056c,DAT_01049dac != 25000000,auStack_1028);
  if (((iVar5 == 0) && (iVar5 = FUN_01023ed4(auStack_1028,local_1248), iVar5 == 0)) &&
     (iVar5 = FUN_01023fd4(auStack_1028,0,local_1128), iVar5 == 0)) {
    DAT_01049d9c = local_1128[0] * 1000;
    FUN_01003f7c();
    if (DAT_01049db1 != '\0') goto LAB_010012f0;
LAB_010011a0:
    DAT_0104a3a8 = &DAT_01049024;
    if (DAT_0104a568 == 0) {
      if (DAT_fbff4100 != '\0') goto LAB_010011cc;
      FUN_0100ceb0(s_I2C_Preload_Disabled__010306cc + 0x14);
      FUN_01010c00(s_I2C_Preload_Disabled__010306cc + 0x14);
      DAT_0104a568 = 1;
    }
  }
  else {
    FUN_0100ceb0(s__s__DT_based_initialization_fail_01030ca4 + 0x24,s_stg3_early_init_01029130 + 0xc
                );
    FUN_01010c00(s__s__DT_based_initialization_fail_01030ca4 + 0x24,s_stg3_early_init_01029130 + 0xc
                );
    if (DAT_01049db1 == '\0') goto LAB_010011a0;
LAB_010012f0:
    FUN_0100ceb0(s__soc_board_cfg_ethernet_port_d_e_010306a4 + 0x24);
    FUN_01010c00(s__soc_board_cfg_ethernet_port_d_e_010306a4 + 0x24);
    DAT_0104a3a8 = &DAT_01049024;
    DAT_0104a568 = 1;
  }
  bVar7 = DAT_fbff410c;
  FUN_01012ae4(&DAT_fbff4100,0,0x20);
  _DAT_fbff410a = 0x400;
  DAT_fbff410c = bVar7 & 3 | DAT_fbff410c;
LAB_010011cc:
  piVar10 = (int *)&DAT_fbff41ff;
  FUN_0100ceb0(s_Violation__zero_device_ID__010306e4 + 0x18,DAT_fbff4102);
  FUN_01010c00(s_Violation__zero_device_ID__010306e4 + 0x18,DAT_fbff4102);
  DAT_0104a59c = (uint)_DAT_fbff4100;
  FUN_0100ceb0(s_EEPROM_Revision_ID____02x_01030700 + 0x18,DAT_0104a59c);
  FUN_01010c00(s_EEPROM_Revision_ID____02x_01030700 + 0x18,DAT_0104a59c);
  bVar7 = DAT_fbff4103 & 3;
  if (bVar7 == 2) {
    DAT_0104a5a0 = 0x6f4312aa;
    goto switchD_01001288_caseD_4;
  }
  if (bVar7 == 3) {
    DAT_0104a5a0 = 4000000000;
  }
  else if (bVar7 == 1) {
    DAT_0104a5a0 = 1600000000;
  }
  else {
    DAT_0104a5a0 = 0x4f790d55;
  }
LAB_01001254:
  if ((DAT_fbff4103 & 4) == 0) {
    DAT_0104a5a4 = 0x20;
  }
  else {
    DAT_0104a5a4 = 0x40;
  }
  DAT_0104a5a8 = (DAT_fbff4103 & 0xf) >> 3;
  switch(DAT_fbff4108 >> 5) {
  case 0:
    goto switchD_01001288_caseD_0;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  case 4:
    goto switchD_01001288_caseD_4;
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
      FUN_0100ceb0(s_Device_ID____04x_0103071c + 0x10);
      FUN_01010c00(s_Device_ID____04x_0103071c + 0x10);
    }
    DAT_0104a5b0 = 1;
  }
  piVar10 = DAT_010023bc;
  unaff_r8 = DAT_010023bc + 3;
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
  FUN_0100ceb0(&DAT_01030734);
  FUN_01010c00(&DAT_01030734);
  do {
    puVar2 = (undefined1 *)((int)piVar10 + -0x505650b);
    piVar10 = (int *)((int)piVar10 + 1);
    *(undefined1 *)piVar10 = *puVar2;
    FUN_0100ceb0(s_Device_Info__01030738 + 0xc,*puVar2);
    FUN_01010c00(s_Device_Info__01030738 + 0xc,*(undefined1 *)piVar10);
  } while (piVar10 != unaff_r8);
  FUN_0100ceb0(s_al_flash_toc_find_id_failed__boo_01030944 + 0x24);
  FUN_01010c00(s_al_flash_toc_find_id_failed__boo_01030944 + 0x24);
  iVar5 = FUN_01003448();
  if (iVar5 != 0) {
    FUN_0100ceb0(&DAT_01030748,s_stg3_early_init_01029130 + 0xc);
    FUN_01010c00(&DAT_01030748,s_stg3_early_init_01029130 + 0xc);
  }
  if (DAT_0104a568 == 0) {
    iVar5 = FUN_01003234(auStack_1028,&local_1258,auStack_1228,local_1128);
    if (iVar5 == 0) {
      if ((DAT_0104a568 == 0) &&
         (iVar5 = FUN_01002f3c(auStack_1028,local_1258,auStack_1228,local_1128), iVar5 != 0)) {
        FUN_0100ceb0(s__s__failed_to_read_EEPROM_conten_0103076c + 0x20,
                     s_stg3_early_init_01029130 + 0xc);
        FUN_01010c00(s__s__failed_to_read_EEPROM_conten_0103076c + 0x20,
                     s_stg3_early_init_01029130 + 0xc);
        DAT_0104a568 = 1;
      }
    }
    else {
      FUN_0100ceb0(s__s__I2C_initialization_failed__0103074c + 0x1c,s_stg3_early_init_01029130 + 0xc
                  );
      FUN_01010c00(s__s__I2C_initialization_failed__0103074c + 0x1c,s_stg3_early_init_01029130 + 0xc
                  );
      DAT_0104a568 = 1;
    }
  }
  if (DAT_0104a5a0 < (uint)(DAT_01049d9c << 1)) {
    FUN_0100ceb0(s__s__failed_to_authenticate_EEPRO_01030790 + 0x2c);
    FUN_01010c00(s__s__failed_to_authenticate_EEPRO_01030790 + 0x2c);
    DAT_0104a568 = 1;
  }
  if (DAT_0104a5ac < DAT_01049d98) {
    FUN_0100ceb0(s_DRAM_frequency_violation__010307c0 + 0x18);
    FUN_01010c00(s_DRAM_frequency_violation__010307c0 + 0x18);
    DAT_0104a568 = 1;
  }
  switch(DAT_01049db4) {
  case 0:
    break;
  case 1:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 2:
  case 3:
    piVar10 = (int *)&DAT_01049d8c;
    DAT_01049d8c = FUN_01000778;
    break;
  default:
    FUN_0100ceb0(s_CPU_frequency_violation__010307dc + 0x18,s_stg3_early_init_01029130 + 0xc);
    FUN_01010c00(s_CPU_frequency_violation__010307dc + 0x18,s_stg3_early_init_01029130 + 0xc);
    piVar10 = (int *)&DAT_01049d8c;
    DAT_01049d8c = FUN_01000778;
    DAT_0104a568 = 1;
  }
  goto LAB_01001600;
switchD_01001288_caseD_4:
  goto LAB_01001254;
switchD_01001288_caseD_0:
LAB_01001600:
  if ((DAT_fbff410c & 2) == 0) {
    if ((DAT_fbff410c & 1) == 0) {
      if (DAT_0104a510 != 0) goto LAB_01001844;
    }
    else {
      DAT_0104a510 = 1;
      *piVar10 = (int)&LAB_0100080c;
LAB_01001844:
      FUN_0100ceb0(s__s__bootstrap_s_boot_device_isnt_010307f8 + 0x50);
      FUN_01010c00(s__s__bootstrap_s_boot_device_isnt_010307f8 + 0x50);
    }
    pcVar9 = (code *)*piVar10;
  }
  else {
    pcVar9 = FUN_01000778;
    DAT_0104a510 = 0;
    *piVar10 = (int)FUN_01000778;
  }
  iVar5 = FUN_01027bcc(pcVar9,0,0x20000,0x10,DAT_010023c0);
  if (iVar5 == 0) {
    iVar5 = FUN_01027e14(0xfbff4000,0,*piVar10,DAT_0104a500,&local_1250,&local_124c);
    if (iVar5 != 0) {
      FUN_0100ceb0(s_al_flash_toc_search_failed__0103085c + 0x1c);
      FUN_01010c00(s_al_flash_toc_search_failed__0103085c + 0x1c);
      local_1250 = 0;
      local_124c = 0;
    }
    puVar13 = (undefined4 *)&DAT_01049308;
    uVar8 = (*(code *)*piVar10)(0x1f000c,&DAT_0104930a,2);
    if (uVar8 != 0) {
      uVar12 = uVar8 & 0xff;
      FUN_01010c00(s_al_flash_toc_stage2_active_insta_0103087c + 0x30,uVar8);
      goto LAB_01001e44;
    }
    uVar3 = DAT_0104930a >> 8;
    uVar1 = DAT_0104930a << 8;
    DAT_0104930a = uVar3 | uVar1;
    FUN_01010c00(s_fetch_subsystem_ID_failed___d_010308b0 + 0x1c,uVar3 | uVar1);
    uVar8 = (*(code *)*piVar10)(0x1f0010,&DAT_0104930c);
    if (uVar8 != 0) {
      uVar12 = uVar8 & 0xff;
      FUN_01010c00(s_subsystem_id__0x_04x_010308d0 + 0x14,uVar8);
      goto LAB_01001e44;
    }
    uVar8 = DAT_0104930c << 0x18 | (DAT_0104930c >> 8 & 0xff) << 0x10 |
            (DAT_0104930c >> 0x10 & 0xff) << 8 | DAT_0104930c >> 0x18;
    DAT_01049309 = (undefined1)(DAT_0104930c >> 0x18);
    DAT_0104930c = uVar8;
    FUN_01010c00(s_fetch_hardware_revision_ID_faile_010308e8 + 0x24,uVar8);
    piVar11 = (int *)0x0;
    uVar12 = 0;
    switch(DAT_0104930a) {
    case 0xea16:
      break;
    case 0xea17:
    case 0xea18:
    case 0xea19:
    case 0xea1b:
    case 0xea1c:
    case 0xea1d:
    case 0xea1e:
    case 0xea1f:
    case 0xea22:
    case 0xea23:
    case 0xea24:
    case 0xea25:
    case 0xea26:
    case 0xea27:
    case 0xea28:
    case 0xea29:
    case 0xea2a:
    case 0xea2b:
    case 0xea2c:
    case 0xea2d:
    case 0xea2e:
    case 0xea2f:
      break;
    case 0xea1a:
      break;
    case 0xea20:
      break;
    case 0xea21:
      break;
    case 0xea30:
      goto switchD_01001718_caseD_ea30;
    default:
      uVar12 = 0xff;
      DAT_01049308 = 0xff;
    }
LAB_01001e44:
    unaff_r10 = local_1248;
    DAT_0104a504 = uVar12;
    local_1250 = uVar12;
    FUN_01010c00(s_hardware_revision_id__0x_08x_01030910 + 0x1c,uVar12);
    piVar11 = &local_1254;
    DAT_0104a508 = local_124c;
    iVar5 = FUN_01027dec(*piVar10,DAT_0104a500,0,0,piVar11,unaff_r10);
    if (iVar5 == 0) {
      iVar5 = *piVar10;
      if (-1 < local_1254) {
        iVar5 = FUN_01027f70(iVar5,local_1238,&DAT_010493dc);
        if (iVar5 == 0) {
          if (DAT_01049404 < 0x41) {
            iVar5 = FUN_010280d8(*piVar10,local_1238,DAT_010023c4);
            if (iVar5 != 0) {
              FUN_0100ceb0(s_Boot_mode_too_big__01030970 + 0x10);
              FUN_01010c00(s_Boot_mode_too_big__01030970 + 0x10);
              goto LAB_0100187c;
            }
            iVar5 = *piVar10;
            goto LAB_01001ee4;
          }
          FUN_0100ceb0(s_al_flash_toc_find_id_failed__boo_01030944 + 0x28);
          FUN_01010c00(s_al_flash_toc_find_id_failed__boo_01030944 + 0x28);
        }
        iVar5 = *piVar10;
      }
      DAT_0104a518 = 5;
    }
    else {
      FUN_0100ceb0(s_instance_num____d_01030930 + 0x10);
      FUN_01010c00(s_instance_num____d_01030930 + 0x10);
      iVar5 = *piVar10;
      DAT_0104a518 = 5;
    }
LAB_01001ee4:
    iVar5 = FUN_01027c58(iVar5,DAT_0104a500,local_1250 << 0x1c | 2,local_124c << 0x1c | 2,0,piVar11,
                         unaff_r10);
    if (iVar5 == 0) {
      if (local_1254 < 0) {
        FUN_0100ceb0(s_al_flash_toc_find_id_failed__DT__010309b0 + 0x2c);
        puVar13 = &DAT_0104946c;
        unaff_r8 = &DAT_01049470;
        FUN_01010c00(s_al_flash_toc_find_id_failed__DT__010309b0 + 0x2c);
      }
      else {
        iVar5 = FUN_01027f70(*piVar10,local_1238,&DAT_01049424);
        if (iVar5 == 0) {
          puVar13 = &DAT_0104946c;
          unaff_r8 = &DAT_01049470;
          DAT_0104946c = local_1238;
          DAT_01049470 = 1;
        }
        else {
          FUN_0100ceb0(s_DT_not_found_in_TOC__010309e0 + 0x14,local_1250);
          puVar13 = &DAT_0104946c;
          unaff_r8 = &DAT_01049470;
          FUN_01010c00(s_DT_not_found_in_TOC__010309e0 + 0x14,local_1250);
switchD_01001718_caseD_ea30:
        }
      }
    }
    else {
      FUN_0100ceb0(s_al_flash_obj_data_load_failed__b_01030984 + 0x28,local_1250);
      puVar13 = &DAT_0104946c;
      unaff_r8 = &DAT_01049470;
      FUN_01010c00(s_al_flash_obj_data_load_failed__b_01030984 + 0x28,local_1250);
    }
    iVar5 = FUN_01027c58(*piVar10,DAT_0104a500,local_1250 << 0x1c | 0x100,local_124c << 0x1c | 0x100
                         ,0,piVar11,unaff_r10);
    if (((iVar5 == 0) && (-1 < local_1254)) &&
       (iVar5 = FUN_01027f70(*piVar10,local_1238,&DAT_0104933c), iVar5 == 0)) {
      DAT_01049384 = local_1238;
      DAT_01049388 = 1;
    }
    FUN_01005864();
LAB_01001ff0:
    iVar5 = FUN_0100d204(0);
    if (iVar5 != 0) goto LAB_010020a4;
    while( true ) {
      iVar5 = FUN_01027c58(*piVar10,DAT_0104a500,DAT_0104a518 | local_1250 << 0x1c,
                           DAT_0104a518 | local_124c << 0x1c,0,piVar11,unaff_r10);
      uVar8 = DAT_0104a518;
      if (iVar5 != 0) {
        FUN_0100ceb0(s_User_request_for_loading_U_Boot__01030a88 + 0x20,local_1250);
        FUN_01010c00(s_User_request_for_loading_U_Boot__01030a88 + 0x20,local_1250);
        goto LAB_010018b4;
      }
      if (-1 < local_1254) break;
      uVar6 = FUN_010275c0(DAT_0104a518);
      FUN_0100ceb0(s_al_flash_toc_find_id_failed__app_01030aac + 0x30,uVar8,uVar6,local_1250);
      uVar8 = DAT_0104a518;
      uVar6 = FUN_010275c0(DAT_0104a518);
      FUN_01010c00(s_al_flash_toc_find_id_failed__app_01030aac + 0x30,uVar8,uVar6,local_1250);
      if (DAT_0104a518 == 5) goto LAB_010018b4;
      while( true ) {
        DAT_0104a518 = 5;
        iVar5 = FUN_0100d204(0);
        if (iVar5 == 0) break;
LAB_010020a4:
        uVar8 = FUN_0100d0e0(0);
        if (uVar8 == 0x70) {
          if (DAT_01049474 == 0) {
            FUN_0100ceb0(s_al_flash_obj_header_read_and_val_010309f8 + 0x40);
            FUN_01010c00(s_al_flash_obj_header_read_and_val_010309f8 + 0x40);
            DAT_01049474 = 1;
          }
        }
        else {
          if ((uVar8 == 0x65) && (DAT_01049338 == 0)) {
            FUN_0100ceb0(s_User_request_for_skipping_APCEA__01030a3c + 0x20);
            FUN_01010c00(s_User_request_for_skipping_APCEA__01030a3c + 0x20);
            DAT_01049338 = 1;
          }
          if ((uVar8 & 0xef) != 0x65) goto LAB_01001ff0;
        }
        if (DAT_0104a518 == 5) goto LAB_01001ff0;
        FUN_0100ceb0(s_User_request_for_keeping_early_A_01030a60 + 0x24);
        FUN_01010c00(s_User_request_for_keeping_early_A_01030a60 + 0x24);
      }
    }
    iVar5 = FUN_01027f70(*piVar10,local_1238,&DAT_0104938c);
    if (iVar5 == 0) {
      DAT_010493d4 = local_1238;
      DAT_010493d8 = 1;
    }
    else {
      FUN_0100ceb0(s_App_not_found_in_TOC___x__s__u___01030ae0 + 0x20,local_1250);
      FUN_01010c00(s_App_not_found_in_TOC___x__s__u___01030ae0 + 0x20,local_1250);
    }
  }
  else {
    FUN_0100ceb0(s_UART_recovery__0103084c + 0xc);
    FUN_01010c00(s_UART_recovery__0103084c + 0xc);
LAB_0100187c:
    FUN_0100ceb0(s_al_flash_obj_header_read_and_val_01030b04 + 0x40,s_stg3_early_init_01029130 + 0xc
                );
    puVar13 = &DAT_0104946c;
    unaff_r8 = &DAT_01049470;
    FUN_01010c00(s_al_flash_obj_header_read_and_val_01030b04 + 0x40,s_stg3_early_init_01029130 + 0xc
                );
  }
LAB_010018b4:
  FUN_01003e74();
  if (*unaff_r8 == 0) {
    FUN_0100ceb0(s__s__DT_reading_failed__01030ba8 + 0x14,0xc0000,0x1100000,0x10000);
    FUN_01010c00(s__s__DT_reading_failed__01030ba8 + 0x14,0xc0000,0x1100000,0x10000);
    (*(code *)*piVar10)(0xc0000,0x1100000,0x10000);
LAB_0100190c:
    DAT_01049d88 = 0x1100000;
    FUN_01010c00(s_Loading_DT_from__08x_to__08X___u_01030bc0 + 0x28,*puVar13);
  }
  else {
    FUN_0100ceb0(s__s__TOC_parsing_failed__01030b48 + 0x18,0x1100000,DAT_0104944c);
    FUN_01010c00(s__s__TOC_parsing_failed__01030b48 + 0x18,0x1100000,DAT_0104944c);
    iVar5 = FUN_010280d8(*piVar10,*puVar13,0x1100000);
    if (iVar5 == 0) goto LAB_0100190c;
    FUN_0100ceb0(s_Loading_DT_to__08X___u_bytes_____01030b64 + 0x20);
    FUN_01010c00(s_Loading_DT_to__08X___u_bytes_____01030b64 + 0x20);
    FUN_0100ceb0(s_al_flash_obj_data_load_failed__01030b88 + 0x1c,s_stg3_early_init_01029130 + 0xc);
    FUN_01010c00(s_al_flash_obj_data_load_failed__01030b88 + 0x1c,s_stg3_early_init_01029130 + 0xc);
  }
  iVar5 = FUN_01007f88(DAT_01049d88,s_obj_hdr_dt_offset__0x_04x_01030bec + 0x18);
  if ((iVar5 < 0) ||
     (iVar5 = FUN_01007ebc(DAT_01049d88,iVar5,s__soc_board_cfg_01030c08 + 0xc,0), iVar5 == 0)) {
    FUN_0100ceb0(s__hypervisor__01030c94 + 0xc,s_stg3_early_init_01029130 + 0xc);
    FUN_01010c00(s__hypervisor__01030c94 + 0xc,s_stg3_early_init_01029130 + 0xc);
  }
  else {
    FUN_0100ceb0(&DAT_01030c18,iVar5);
    FUN_01010c00(&DAT_01030c18,iVar5);
    iVar5 = FUN_01007f88(DAT_01049d88,s_Board_config_ID___s_01030c1c + 0x14);
    if (((-1 < iVar5) &&
        (iVar5 = FUN_01007e68(DAT_01049d88,iVar5,s__soc_board_cfg_pcie_01030c34 + 0x10,local_1248),
        iVar5 != 0)) && (local_1248[0] = local_1248[0] >> 2, local_1248[0] != 0)) {
      puVar14 = (uint *)(iVar5 + 8);
      iVar5 = 0;
      do {
        while( true ) {
          puVar14 = puVar14 + 1;
          uVar8 = *puVar14;
          uVar8 = uVar8 << 0x18 | (uVar8 >> 8 & 0xff) << 0x10 | (uVar8 >> 0x10 & 0xff) << 8 |
                  uVar8 >> 0x18;
          if (uVar8 < 4) break;
          iVar5 = iVar5 + 1;
          FUN_0100ceb0(s_ep_ports_01030c48 + 8,s_exec_via_agent_010290e0 + 0xc,uVar8);
          FUN_01010c00(s_ep_ports_01030c48 + 8,s_exec_via_agent_010290e0 + 0xc,uVar8);
          if ((int)local_1248[0] <= iVar5) goto LAB_01001a48;
        }
        iVar5 = iVar5 + 1;
        *(undefined4 *)(DAT_0104a3a8 + uVar8 * 0x14 + 0xc) = 1;
      } while (iVar5 < (int)local_1248[0]);
    }
LAB_01001a48:
    FUN_01000838(0);
    FUN_01000838(1);
    FUN_01000838(2);
    FUN_01000838(3);
    iVar5 = FUN_01005918();
    if (iVar5 != 0) {
      FUN_0100ceb0(s__s__invalid_port_num___d___01030c54 + 0x18,s_dt_based_init_pcie_010290f0 + 0x10
                  );
      FUN_01010c00(s__s__invalid_port_num___d___01030c54 + 0x18,s_dt_based_init_pcie_010290f0 + 0x10
                  );
    }
    iVar5 = FUN_01007f88(DAT_01049d88,s__s__serdes_initialization_failed_01030c70 + 0x20);
    if (-1 < iVar5) {
      DAT_01049334 = 1;
    }
  }
  if (DAT_0104a5b4 == 0) {
LAB_01001c08:
    if (DAT_0104a5c0 != 0) {
      if (DAT_0104a568 != 0) goto LAB_01001af0;
      FUN_01003f34(4,1);
    }
  }
  else {
    if (DAT_0104a568 == 0) {
      FUN_01003f64(0,1);
      FUN_01003f64(1);
      FUN_01003f64(2,1);
      FUN_01003f64(3,1);
      goto LAB_01001c08;
    }
    if (DAT_0104a5c0 != 0) goto LAB_01001af0;
  }
  if ((DAT_0104a5b8 != 0) && (DAT_0104a568 == 0)) {
    FUN_01003f64(5,1);
    FUN_01003f64(4,1);
  }
LAB_01001af0:
  if (DAT_01049310 == 0) {
    FUN_0100ceb0(s__s__al_thermal_sensor_handle_ini_01030df4 + 0x28,&DAT_010290c0);
    FUN_01010c00(s__s__al_thermal_sensor_handle_ini_01030df4 + 0x28,&DAT_010290c0);
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
      FUN_0102596c(DAT_010492cc,uVar8,0,0x1000);
      InstructionSynchronizationBarrier(0xf);
      DataSynchronizationBarrier(0xf);
      if (bVar4) {
        iVar5 = DAT_01049310 + uVar8 * 8;
        uVar12 = *(uint *)(iVar5 + 8);
        *(uint *)(iVar5 + 8) = ~uVar12 & 4 ^ uVar12;
      }
      uVar12 = uVar8 + 1;
      FUN_010257a4(DAT_010492cc,uVar8);
      uVar8 = uVar12;
    } while (uVar12 != 4);
  }
  FUN_0100ca10(0,1);
  FUN_01005504(DAT_01049d88);
  return 0;
}



/* @ 0x1002460 */

undefined4 FUN_01002460(void)

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
  
  iVar1 = FUN_010040d8();
  if (iVar1 != 0) {
    FUN_0100ceb0(&DAT_0103057c,s_stg3_board_init_01029140 + 0xc);
    FUN_01010c00(&DAT_0103057c,s_stg3_board_init_01029140 + 0xc);
    return 0xffffffff;
  }
  iVar1 = FUN_01002f08();
  if (iVar1 == 0) {
    local_50 = 0x20000000;
    uStack_4c = 0;
  }
  else {
    iVar1 = FUN_01002f2c();
    local_50 = *(uint *)(iVar1 + 8);
    uStack_4c = *(uint *)(iVar1 + 0xc);
    if (uStack_4c == 0 && local_50 < 0x2000001) {
LAB_01002750:
      FUN_01004434();
      FUN_01021f14(0xf0070000,0xf0080000,0xf0088000,auStack_38);
      FUN_01021e4c(auStack_38);
      FUN_01021e60(auStack_38);
      FUN_01021eb0(auStack_38);
      FUN_01021ec4(auStack_38);
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
LAB_01002728:
          FUN_0100ceb0(s__s__iodma_init_failed__01030580 + 0x14,s_stg3_board_init_01029140 + 0xc);
          FUN_01010c00(s__s__iodma_init_failed__01030580 + 0x14,s_stg3_board_init_01029140 + 0xc);
          goto LAB_01002750;
        }
        uVar7 = 0;
        uVar8 = 2;
      }
LAB_01002504:
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
      FUN_0100ceb0(s__s__DDR_size_not_supported__01030598 + 0x1c,s_stg3_board_init_01029140 + 0xc,
                   uVar5,uVar4,iVar6,iVar1);
      FUN_01010c00(s__s__DDR_size_not_supported__01030598 + 0x1c,s_stg3_board_init_01029140 + 0xc,
                   uVar5,uVar4,iVar6,iVar1);
      iVar1 = FUN_01004584(uVar4,uVar5,0);
      if (iVar1 != 0) goto LAB_01002658;
      bVar9 = uStack_4c <= uVar8;
      if (uVar8 == uStack_4c) {
        bVar9 = local_50 <= uVar7;
      }
      uVar2 = uVar7;
      uVar3 = uVar8;
      if (bVar9) goto LAB_01002750;
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
        if (uVar3 == 4 && uVar2 == 0) goto LAB_01002728;
        goto LAB_01002504;
      }
      uVar7 = 0;
      uVar8 = 2;
    }
    iVar1 = uVar4 + (uVar7 - uVar2);
    iVar6 = uVar5 + (uVar8 - (uVar3 + (uVar7 < uVar2))) + (uint)CARRY4(uVar4,uVar7 - uVar2);
    FUN_0100ceb0(s__s__DDR_size_not_supported__01030598 + 0x1c,s_stg3_board_init_01029140 + 0xc,
                 uVar5,uVar4,iVar6,iVar1);
    FUN_01010c00(s__s__DDR_size_not_supported__01030598 + 0x1c,s_stg3_board_init_01029140 + 0xc,
                 uVar5,uVar4,iVar6,iVar1);
    iVar1 = FUN_01004584(uVar4,uVar5,0);
    uVar2 = uVar7;
    uVar3 = uVar8;
  } while (iVar1 == 0);
LAB_01002658:
  FUN_0100ceb0(s__s__clearing_physical_memory__08_010305b8 + 0x34,s_stg3_board_init_01029140 + 0xc);
  FUN_01010c00(s__s__clearing_physical_memory__08_010305b8 + 0x34,s_stg3_board_init_01029140 + 0xc);
  return 0xffffffff;
}



/* @ 0x1002804 */

undefined4 FUN_01002804(void)

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
  if (DAT_0104a5d8 == 2) goto LAB_010028e8;
LAB_01002894:
  iVar4 = FUN_01003f4c(uVar5);
  puVar1 = puVar10;
  piVar2 = piVar11;
  if (iVar4 == 0) goto joined_r0x010028d4;
  iVar4 = DAT_0104a568;
  if (iVar6 != 3) goto LAB_010028b4;
  do {
    puVar1 = puVar10;
    piVar2 = piVar11;
    if (iVar4 != 0) {
LAB_010028b4:
      iVar6 = *(int *)((int)&local_50 + iVar7 * 4);
      *(uint *)(iVar6 + 0x964) = *(uint *)(iVar6 + 0x964) & 0xffff3fff | 0x4000;
      puVar1 = puVar10;
      piVar2 = piVar11;
    }
joined_r0x010028d4:
    do {
      iVar7 = iVar7 + 1;
      if (iVar7 == 4) {
        if (DAT_0104a568 != 0) {
          DAT_fbff4100 = 0;
        }
        FUN_01021f14(0xf0070000,0xf0080000,0xf0088000,auStack_60);
        FUN_01021e1c(auStack_60,local_68);
        if (local_68[0] == 1) {
          FUN_01002460();
        }
        iVar7 = FUN_01007f88(DAT_01049d88,s__s__cpu_resume_regs_invalid__01030e20 + 0x1c);
        if (-1 < iVar7) {
          iVar7 = FUN_01007e68(DAT_01049d88,iVar7,s__soc_trace_01030e40 + 8,&local_6c);
          if (iVar7 == 0) {
            FUN_0100ceb0(s_Trace__reg__property_invalid__01030e6c + 0x1c);
            FUN_01010c00(s_Trace__reg__property_invalid__01030e6c + 0x1c);
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
              FUN_0100ceb0(&DAT_01030e4c,uVar8,iVar7);
              FUN_01010c00(&DAT_01030e4c,uVar8,iVar7);
              if (uVar8 != 0) {
                FUN_0100cd3c(0,uVar8,iVar7);
                return 0;
              }
            }
            else {
              FUN_0100ceb0(s_Trace_moved_to__08x____08x_01030e50 + 0x18);
              FUN_01010c00(s_Trace_moved_to__08x____08x_01030e50 + 0x18);
            }
          }
        }
        FUN_0100cdf8(0);
        return 0;
      }
      piVar11 = piVar2 + 1;
      iVar6 = *piVar2;
      puVar10 = puVar1 + 1;
      uVar5 = *puVar1;
      if (iVar6 != 2) goto LAB_01002894;
LAB_010028e8:
      if (((&DAT_01049324)[iVar7] != 1) || (*(int *)(&DAT_01049314 + iVar7 * 4) != 1)) {
        iVar4 = FUN_01003f4c(uVar5);
        break;
      }
      iVar6 = FUN_01003f4c(uVar5);
      puVar1 = puVar10;
      piVar2 = piVar11;
      iVar4 = DAT_0104a568;
    } while (iVar6 == 0);
  } while( true );
}




/* @ 0x1002a8c -- WARNING: Restarted to delay deadcode elimination for space: ram */

void FUN_01002a8c(void)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  uint local_24;
  undefined1 auStack_20 [8];
  
  if ((DAT_01049474 != 0 || DAT_01049338 != 0) ||
     ((DAT_01049388 != 0 &&
      (iVar1 = FUN_010280d8(DAT_01049d8c,DAT_01049384,DAT_0104936c), iVar1 != 0)))) {
    DAT_01049388 = 0;
  }
  if (DAT_010493d8 == 0) {
    (*DAT_01049d8c)(0x40000,&local_24);
    FUN_0100ceb0(s_Loading_application_to__08X___u_b_01030ce4 + 0x28,0x40000,0x1100000,local_24);
    FUN_01010c00(s_Loading_application_to__08X___u_b_01030ce4 + 0x28,0x40000,0x1100000,local_24);
    if (0x100000 < local_24) {
      FUN_0100ceb0(s_Loading_application_from__08x_to_01030d10 + 0x34);
      FUN_01010c00(s_Loading_application_from__08x_to_01030d10 + 0x34);
      goto LAB_01002c90;
    }
    (*DAT_01049d8c)(0x40004);
    uVar3 = 0x1100000;
  }
  else {
    FUN_0100ceb0(s__s__PLL_init_failed_01030ccc + 0x14,DAT_010493bc,DAT_010493b4);
    FUN_01010c00(s__s__PLL_init_failed_01030ccc + 0x14,DAT_010493bc,DAT_010493b4);
    iVar1 = FUN_010280d8(DAT_01049d8c,DAT_010493d4,DAT_010493bc);
    uVar3 = DAT_010493c4;
    if (iVar1 != 0) {
      FUN_0100ceb0(s_Loading_DT_to__08X___u_bytes_____01030b64 + 0x20);
      FUN_01010c00(s_Loading_DT_to__08X___u_bytes_____01030b64 + 0x20);
      goto LAB_01002c90;
    }
  }
  FUN_01027540(0xf0090000);
  FUN_01027530(0xf0070000);
  FUN_01000164(2,1);
  FUN_01000370(0,1,0);
  FUN_01000184();
  FUN_01000164(2,0);
  FUN_01000170();
  if (DAT_01049388 == 0) {
    if (((DAT_01049338 == 0) && (DAT_01049310 != 0)) && (*(int *)(DAT_01049310 + 0x14) != 0)) {
      FUN_0100ceb0(s_Invalid_application_size__01030d48 + 0x18);
      FUN_01010c00(s_Invalid_application_size__01030d48 + 0x18);
      iVar1 = DAT_01049310;
      *(uint *)(DAT_01049310 + 0x10) =
           ~*(uint *)(DAT_01049310 + 0x10) & 0x80000000 ^ *(uint *)(DAT_01049310 + 0x10);
      do {
      } while ((*(uint *)(iVar1 + 0x10) & 0x40000000) == 0);
      FUN_0100ceb0(s_Stopping_early_APCEA_01030d64 + 0x14);
      FUN_01010c00(s_Stopping_early_APCEA_01030d64 + 0x14);
      DataMemoryBarrier(0xf);
      FUN_0102231c(0xfd882000);
    }
    else {
LAB_01002c18:
      DataMemoryBarrier(0xf);
      FUN_0102231c(0xfd882000);
    }
    iVar1 = 0;
    FUN_0100ceb0(s_Early_APCEA_stopped_01030d7c + 0x14);
    FUN_01010c00(s_Early_APCEA_stopped_01030d7c + 0x14);
  }
  else {
    FUN_01022e6c(auStack_20,0xfd883000);
    do {
      uVar2 = FUN_010233b4(auStack_20);
      iVar1 = DAT_01049374;
    } while (uVar2 < 0x40);
    if (DAT_01049374 == 0) goto LAB_01002c18;
    FUN_0100ceb0(s_Executing_application____01030d94 + 0x18);
    FUN_01010c00(s_Executing_application____01030d94 + 0x18);
  }
  FUN_0102596c(DAT_010492cc,0,0,0x1000);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  _DAT_fbff40f8 = uVar3;
  _DAT_fbff40fc = iVar1;
  FUN_010255e0(DAT_010492cc,0);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  WaitForInterrupt();
LAB_01002c90:
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}




void FUN_01002df0(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined1 auStack_10 [4];
  code *local_c [2];
  
  FUN_01000b34();
  iVar2 = DAT_01049310;
  uVar1 = DAT_010492cc;
  if (DAT_0104a50c == 0) {
    if (DAT_01049310 == 0) {
      FUN_0100ceb0(s_Trace__reg__property_not_found__01030e8c + 0x20,
                   s_power_down_secondary_cpus_010290c4 + 0x18);
      FUN_01010c00(s_Trace__reg__property_not_found__01030e8c + 0x20,
                   s_power_down_secondary_cpus_010290c4 + 0x18);
    }
    else {
      *(undefined4 *)(DAT_01049310 + 8) = 0;
      *(undefined4 *)(iVar2 + 0xc) = 0x10005a8;
      FUN_01025bcc(uVar1,0,auStack_10,local_c);
      (*local_c[0])();
    }
  }
  else {
    FUN_010005ac();
  }
  do {
/* @ 0x1002df0 -- WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x1002e90 */


bool FUN_01002e90(void)

{
  FUN_010129dc(0x1000,s_dram_clear_01029150 + 8,0x5e18);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
  FUN_0100057c(&DAT_01049000,0xffff0000);
  FUN_01000370(1,1,1);
  FUN_010274e8(0xf0070000,0);
  FUN_01027508(0xf0090000,1);
  FUN_0100110c();
  FUN_01002804();
  FUN_01002a8c();
  return _DAT_fbff4150 == 0x31415926;
}



/* @ 0x1002f08 */


bool FUN_01002f08(void)

{
  return _DAT_fbff4150 == 0x31415926;
}



/* @ 0x1002f2c */

undefined1 * FUN_01002f2c(void)

{
  return &DAT_fbff4150;
}



/* @ 0x1002f3c */

undefined4 FUN_01002f3c(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

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
  FUN_01009a58(&local_138);
  FUN_01009c44(&local_138,param_3,0x100);
  FUN_01009ae4(&local_138,local_258);
  piVar5 = (int *)&DAT_fd89608c;
  piVar3 = &iStack_25c;
  while( true ) {
    iVar4 = *piVar5;
    piVar5 = piVar5 + 1;
    piVar3 = piVar3 + 1;
    if (*piVar3 != iVar4) break;
    if (piVar5 == (int *)0xfd8960ac) goto LAB_01002fc0;
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
      FUN_0100ceb0(s_signature_decryption_does_not_ma_01030f40 + 0x3c);
      FUN_01010c00(s_signature_decryption_does_not_ma_01030f40 + 0x3c);
      return 0xfffffffb;
    }
    if (piVar5 == (int *)0xfd8960ac) break;
    iVar4 = *piVar3;
    piVar3 = piVar3 + 1;
  }
LAB_01002fc0:
  local_238 = DAT_0100321c;
  local_230 = DAT_0100322c;
  uStack_22c = DAT_01003230;
  local_228 = DAT_01003224;
  uStack_224 = DAT_01003228;
  FUN_01009a58(&local_138);
  iVar4 = param_2;
  iVar2 = param_1;
  do {
    if (iVar4 == 0) {
LAB_01003074:
      iVar4 = 2;
      FUN_01009ae4(&local_138,auStack_278);
      while( true ) {
        iVar2 = FUN_0100809c(param_4,0x100,&local_238,&local_27c,&DAT_01030ee0,3,param_3,0x100);
        if (iVar2 != 0) {
          FUN_0100ceb0(s_eeprom_data_is_not_aligned_to__d_01030f00 + 0x24,&local_238,0xe0);
          FUN_01010c00(s_eeprom_data_is_not_aligned_to__d_01030f00 + 0x24);
          return 0xfffffffb;
        }
        iVar2 = FUN_01012b08(s__s__i2c_signature_read_failed__01031058 + 0x1c);
        if ((iVar2 == 0) && (iVar2 = FUN_01012b08(auStack_278,auStack_158), iVar2 == 0)) break;
        FUN_0100ceb0(s_RSA_decryption_failed__01030f28 + 0x14);
        FUN_01010c00(s_RSA_decryption_failed__01030f28 + 0x14);
        bVar7 = iVar4 == 1;
        iVar4 = 1;
        if (bVar7) {
          return 0xfffffffb;
        }
        FUN_01009a58(&local_138);
        FUN_01009c44(&local_138,param_1,param_2);
        FUN_01009ae4(&local_138,auStack_278);
      }
      return 0;
    }
    if (iVar4 < 0xc) {
      FUN_0100ceb0(s_eeprom_preload_parser_01030ee8 + 0x14,0xc);
      FUN_01010c00(s_eeprom_preload_parser_01030ee8 + 0x14,0xc);
      FUN_01009c44(&local_138,iVar2,iVar4);
      goto LAB_01003074;
    }
    iVar6 = iVar2 + 4;
    iVar1 = FUN_01012b08(iVar6,&local_238,8);
    if (iVar1 != 0) {
      iVar1 = FUN_01012b08(iVar6,&local_230,8);
      if (iVar1 != 0) {
        iVar1 = FUN_01012b08(iVar6,&local_228,8);
        if (iVar1 != 0) {
          FUN_01009c44(&local_138,iVar2,0xc);
        }
      }
    }
    iVar4 = iVar4 + -0xc;
    iVar2 = iVar2 + 0xc;
  } while( true );
}



/* @ 0x1003234 */

undefined4 FUN_01003234(int param_1,int *param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  uVar3 = 0;
  do {
    iVar1 = FUN_01003464(DAT_01049dbf,uVar3,2,param_1 + uVar3,0xc);
    if (iVar1 != 0) {
      FUN_0100ceb0(s_eFuse_hashed_modulues_does_not_m_01030f80 + 0x3c,&DAT_01030ee4,uVar3 / 0xc,
                   uVar3 * -0x55555555);
      FUN_01010c00(s_eFuse_hashed_modulues_does_not_m_01030f80 + 0x3c,&DAT_01030ee4,uVar3 / 0xc);
      return 0xfffffffb;
    }
    iVar1 = FUN_01012b08(param_1 + uVar3,s__s__cpu_resume_regs_invalid_for_e_01030eb0 + 0x24,0xc);
    iVar2 = uVar3 + 0xc;
    if (iVar1 == 0) {
      if (uVar3 != 0) {
        if (iVar2 < 0x1000) {
          *param_2 = iVar2;
          iVar1 = FUN_01003464(DAT_01049dbf,iVar2,2,param_3,0x100);
          if (iVar1 != 0) {
            FUN_0100ceb0(s__s__failed_to_find_closing_EEPRO_01031004 + 0x30,&DAT_01030ee4);
            FUN_01010c00(s__s__failed_to_find_closing_EEPRO_01031004 + 0x30,&DAT_01030ee4);
            return 0xfffffffb;
          }
          iVar1 = FUN_01003464(DAT_01049dbf,*param_2 + 0x100,2,param_4,0x100);
          if (iVar1 == 0) {
            return 0;
          }
          FUN_0100ceb0(s__s__i2c_modulues_read_failed__01031038 + 0x1c,&DAT_01030ee4);
          FUN_01010c00(s__s__i2c_modulues_read_failed__01031038 + 0x1c,&DAT_01030ee4);
          return 0xfffffffb;
        }
LAB_010033e8:
        FUN_0100ceb0(s__s__no_opening_EEPROM_encapsulat_01030fdc + 0x24,&DAT_01030ee4);
        FUN_01010c00(s__s__no_opening_EEPROM_encapsulat_01030fdc + 0x24,&DAT_01030ee4);
        return 0xffffffea;
      }
    }
    else {
      if (uVar3 == 0) {
        FUN_0100ceb0(s__s__i2c_read___d_failed__01030fc0 + 0x18,&DAT_01030ee4);
        FUN_01010c00(s__s__i2c_read___d_failed__01030fc0 + 0x18,&DAT_01030ee4);
        return 0xffffffea;
      }
      if (0xfff < uVar3 + 0xc) goto LAB_010033e8;
    }
    uVar3 = uVar3 + 0xc;
  } while( true );
}




/* @ 0x1003448 -- WARNING: Removing unreachable block (ram,0x01022c88) */
/* WARNING: Removing unreachable block (ram,0x01022c24) */
/* WARNING: Removing unreachable block (ram,0x01022bc4) */

undefined4 FUN_01003448(void)

{
  DAT_01049478 = 0xfd880000;
  uRamfd88006c = uRamfd88006c & 0xfffe;
  if (s_i2c_read_01031157[9] == '\x01') {
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
      uRamfd880000 = 8;
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
  DAT_01049480 = 0x1022998;
  DAT_01049484 = 0;
  return 0;
}



/* @ 0x1003464 */

int FUN_01003464(undefined4 param_1,uint param_2,uint param_3,int param_4,int param_5)

{
  int iVar1;
  undefined2 local_2c;
  
  if (param_4 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s__home_winder_projects_data_custo_01031180 + 0x54,1,
                 &DAT_0103117c,&DAT_01031154,0x59);
    FUN_01010c00(s_vectors_01028ea4 + 4,s__home_winder_projects_data_custo_01031180 + 0x54,1,
                 &DAT_0103117c,&DAT_01031154,0x59);
    FUN_01000458(0);
  }
  if (2 < param_3) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___buffer______void___0___010311d8 + 0x18,1,&DAT_0103117c,
                 &DAT_01031154,0x5a);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___buffer______void___0___010311d8 + 0x18,1,&DAT_0103117c,
                 &DAT_01031154,0x5a);
    FUN_01000458(0);
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
        iVar1 = FUN_01022ce8(&DAT_01049478,param_1,param_3 & 0xff,&local_2c,param_5,iVar1);
        return iVar1;
      }
      param_5 = param_5 + -0x80;
      iVar1 = FUN_01022ce8(&DAT_01049478,param_1,param_3 & 0xff,&local_2c,0x80,iVar1);
    } while (iVar1 == 0);
  }
  return iVar1;
}




void FUN_01003814(void)

{
  code *UNRECOVERED_JUMPTABLE;
  
/* @ 0x1003814 -- WARNING: Could not recover jumptable at 0x01003814. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}





undefined4 FUN_01003b24(undefined4 param_1,uint param_2)

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
    FUN_0100c2b8(param_1,param_2,0,&local_26);
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
          if (iVar6 == 10) {
            uVar7 = (param_2 & 0xffff) >> 0xb;
            uVar5 = (param_2 & 0x7ff) >> 8;
            FUN_0100ceb0(s__s__cfg_entry_>pd_failed__0103137c + 0x18,0,uVar7,uVar5);
            FUN_01010c00(s__s__cfg_entry_>pd_failed__0103137c + 0x18,0,uVar7,uVar5);
            _FIQ = 0;
/* @ 0x1003b24 -- WARNING: Does not return */
            pcVar4 = (code *)software_udf(0,0x1003e70);
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
          FUN_0100ceb0(s__s__cfg_entry_>post_flr_failed__0103135c + 0x1c,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
          FUN_01010c00(s__s__cfg_entry_>post_flr_failed__0103135c + 0x1c,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
        }
        FUN_0100c2f0(param_1,param_2,0x1e0,&local_24);
        FUN_0100c430(param_1,param_2,0x1e0,local_24 | 1);
        FUN_0100c2f0(param_1,param_2,0x84,&local_24);
        FUN_0100c430(param_1,param_2,0x84,local_24 | 0x103);
        FUN_0100c2f0(param_1,param_2,0x1e0,&local_24);
        FUN_0100c430(param_1,param_2,0x1e0,local_24 | 0x40);
        uVar1 = 1;
      }
      else {
        if ((&DAT_01049098)[iVar3] == 0) {
          FUN_010221ac(0xfd8a8000,1);
          FUN_0100c430(param_1,param_2,0x30c,0);
          FUN_010221ac(0xfd8a8000,0);
        }
        if (((code *)(&DAT_010490a4)[iVar2 - iVar6] != (code *)0x0) &&
           (iVar3 = (*(code *)(&DAT_010490a4)[iVar2 - iVar6])(puVar8), iVar3 != 0)) {
          FUN_0100ceb0(s__s__al_unit_adapter_handle_init_f_01031304 + 0x34,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
          FUN_01010c00(s__s__al_unit_adapter_handle_init_f_01031304 + 0x34,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
        }
        FUN_0100c2f0(param_1,param_2,0x48,&local_24);
        FUN_0100c430(param_1,param_2,0x48,local_24 | 0x8000);
        FUN_01007590(1000);
        if ((&DAT_01049098)[iVar2 - iVar6] == 0) {
          FUN_0100c2f0(param_1,param_2,0x30c,&local_24);
        }
        pcVar4 = *(code **)((iVar2 - iVar6) * 4 + 0x10490a8);
        if ((pcVar4 != (code *)0x0) && (iVar2 = (*pcVar4)(puVar8), iVar2 != 0)) {
          FUN_0100ceb0(s__s__cfg_entry_>pre_flr_failed__0103133c + 0x1c,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
          FUN_01010c00(s__s__cfg_entry_>pre_flr_failed__0103133c + 0x1c,
                       s_al_pcie_int_adapter_pd_01031234 + 0x14);
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



/* @ 0x1003e74 */

void FUN_01003e74(void)

{
  int iVar1;
  undefined *puVar2;
  int iVar3;
  
  puVar2 = &DAT_01049074;
  iVar3 = 0;
  do {
    while( true ) {
      iVar1 = FUN_01025ee0(puVar2,puVar2[0x18],0,&LAB_010039e4,0x1003600,0,puVar2);
      if (iVar1 != 0) break;
      iVar3 = iVar3 + 1;
      puVar2 = puVar2 + 0x3c;
      if (iVar3 == 10) {
        return;
      }
    }
    FUN_0100ceb0(s___context_>device_specific__010312e8 + 0x18,s_pci_skip_dev_0103124c + 0xc,iVar3);
    iVar1 = iVar3 + 1;
    FUN_01010c00(s___context_>device_specific__010312e8 + 0x18,s_pci_skip_dev_0103124c + 0xc,iVar3);
    puVar2 = puVar2 + 0x3c;
    iVar3 = iVar1;
  } while (iVar1 != 10);
  return;
}



/* @ 0x1003f34 */

void FUN_01003f34(int param_1,undefined4 param_2)

{
  (&DAT_01049094)[param_1 * 0xf] = param_2;
  return;
}



/* @ 0x1003f4c */

undefined4 FUN_01003f4c(int param_1)

{
  return (&DAT_01049094)[param_1 * 0xf];
}



/* @ 0x1003f64 */

void FUN_01003f64(int param_1,undefined4 param_2)

{
  (&DAT_01049098)[param_1 * 0xf] = param_2;
  return;
}



/* @ 0x1003f7c */

void FUN_01003f7c(void)

{
  int iVar1;
  undefined1 uStack_21;
  uint local_20;
  undefined1 auStack_1c [24];
  
  iVar1 = FUN_01023c98(0xfd860b00,s_pll_fixup_crypto_boost_010313b8 + 0x14,1,auStack_1c);
  if (iVar1 != 0) {
    FUN_0100ceb0(&DAT_010313d4,s__d__d__d_implicitly_disabled_01031398 + 0x1c);
    FUN_01010c00(&DAT_010313d4,s__d__d__d_implicitly_disabled_01031398 + 0x1c);
    return;
  }
  iVar1 = FUN_01023ed4(auStack_1c,&uStack_21,&local_20);
  if (iVar1 == 0) {
    if ((local_20 == (local_20 / 600000) * 600000) || (local_20 % 300000 == 0)) {
      iVar1 = FUN_0102421c(auStack_1c);
      if (iVar1 != 0) {
        FUN_0100ceb0(s_PLL_freq_not_suitable_for_600MHz_01031414 + 0x28,
                     s__d__d__d_implicitly_disabled_01031398 + 0x1c);
        FUN_01010c00(s_PLL_freq_not_suitable_for_600MHz_01031414 + 0x28,
                     s__d__d__d_implicitly_disabled_01031398 + 0x1c);
      }
    }
    else {
      FUN_0100ceb0(s__s__al_pll_freq_get_failed__010313f4 + 0x1c,local_20 % 300000,local_20 / 600000
                   ,300000);
      FUN_01010c00(s__s__al_pll_freq_get_failed__010313f4 + 0x1c);
    }
    return;
  }
  FUN_0100ceb0(s__s__al_pll_init_failed__010313d8 + 0x18,
               s__d__d__d_implicitly_disabled_01031398 + 0x1c);
  FUN_01010c00(s__s__al_pll_init_failed__010313d8 + 0x18,
               s__d__d__d_implicitly_disabled_01031398 + 0x1c);
  return;
}



/* @ 0x10040d8 */

int FUN_010040d8(void)

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
  FUN_0100c280(0,0x2800,8,&DAT_01049a80);
  iVar2 = FUN_0100c81c(0,DAT_01049488,0,&local_84);
  if (iVar2 == 0) {
    iVar2 = FUN_0100c718(0,DAT_01049488);
    if (iVar2 == 0) {
      iVar2 = FUN_0100c734(0,DAT_01049488);
      uVar1 = DAT_01049488;
      if (iVar2 == 0) {
        FUN_0100c2f0(0,DAT_01049488,0x110,&local_48);
        local_48 = local_48 | 3;
        FUN_0100c430(0,uVar1,0x110);
        FUN_0100c2f0(0,uVar1,0x220,&local_48);
        local_48 = ~(~(local_48 >> 10) << 10);
        FUN_0100c430(0,uVar1,0x220);
        DAT_01049a98 = FUN_0100d2f8(0x1040);
        DAT_01049a9c = DAT_01049a98 + 0x3fU & 0xffffffc0;
        DAT_01049a8c = FUN_0100d2f8(0x1040);
        DAT_01049a90 = DAT_01049a8c + 0x3fU & 0xffffffc0;
        DAT_01049a84 = FUN_0100d2f8(0x1040);
        DAT_01049a88 = DAT_01049a84 + 0x3fU & 0xffffffc0;
        if ((DAT_01049a84 != 0 && DAT_01049a98 != 0) && (DAT_01049a8c != 0)) {
          FUN_01012ae4(DAT_01049a88,0,0x1000);
          local_80[0] = DAT_01049a80;
          local_7c = local_84;
          local_78 = s_Failed_to_allocate_descriptors_01031550 + 0x1c;
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
          FUN_0101418c(&DAT_010494c0,local_80);
          FUN_01014234(&DAT_010494c0,0,&local_70,&local_48,3);
          FUN_010142bc(&DAT_010494c0,2);
          DAT_01049aa0 = FUN_0101431c(&DAT_010494c0,0);
          DAT_01049a94 = FUN_01014368(&DAT_010494c0,0);
          return 0;
        }
        FUN_0100ceb0(s__s__failed_enabling_memory_regio_0103152c + 0x20);
        FUN_01010c00(s__s__failed_enabling_memory_regio_0103152c + 0x20);
        return -1;
      }
      FUN_0100ceb0(s__s__failed_enabling_bus_masterin_01031508 + 0x20,
                   s_al_udma_fast_desc_flags_set_01031464 + 0x18);
      FUN_01010c00(s__s__failed_enabling_bus_masterin_01031508 + 0x20,
                   s_al_udma_fast_desc_flags_set_01031464 + 0x18);
    }
    else {
      FUN_0100ceb0(s__s__failed_getting_UDMA_BAR__010314e8 + 0x1c,
                   s_al_udma_fast_desc_flags_set_01031464 + 0x18);
      FUN_01010c00(s__s__failed_getting_UDMA_BAR__010314e8 + 0x1c,
                   s_al_udma_fast_desc_flags_set_01031464 + 0x18);
    }
  }
  else {
    FUN_0100ceb0(s_al_udma_cdesc_get_all_010314d0 + 0x14,
                 s_al_udma_fast_desc_flags_set_01031464 + 0x18);
    FUN_01010c00(s_al_udma_cdesc_get_all_010314d0 + 0x14,
                 s_al_udma_fast_desc_flags_set_01031464 + 0x18);
  }
  FUN_0100ceb0(s_RAID_UDMA_01031570 + 8);
  FUN_01010c00(s_RAID_UDMA_01031570 + 8);
  return iVar2;
}



/* @ 0x1004434 */

void FUN_01004434(void)

{
  uint local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  FUN_0100c2f0(0,DAT_01049488,4,&local_1c);
  FUN_0100c2f0(0,DAT_01049488,0xc,&local_18);
  FUN_0100c2f0(0,DAT_01049488,0x10,&local_14);
  FUN_0100c2f0(0,DAT_01049488,0x18,&local_10);
  FUN_0100c2f0(0,DAT_01049488,0x20,&local_c);
  FUN_0100c2f0(0,DAT_01049488,0x48,&local_20);
  FUN_0100c430(0,DAT_01049488,0x48,local_20 | 0x8000);
  FUN_01007590(1000);
  FUN_0100c430(0,DAT_01049488,4,local_1c);
  FUN_0100c430(0,DAT_01049488,0xc,local_18);
  FUN_0100c430(0,DAT_01049488,0x10,local_14);
  FUN_0100c430(0,DAT_01049488,0x18,local_10);
  FUN_0100c430(0,DAT_01049488,0x20,local_c);
  FUN_0100d368(DAT_01049a98);
  FUN_0100d368(DAT_01049a8c);
  FUN_0100d368(DAT_01049a84);
  return;
}




/* @ 0x1004584 -- WARNING: Restarted to delay deadcode elimination for space: ram */

int FUN_01004584(uint param_1,uint param_2,undefined4 param_3,undefined4 param_4,uint param_5,
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
  
  FUN_01012ae4(auStack_3c,0,0x14);
  FUN_01012ae4(auStack_38,param_3,8);
  local_2c = 0xffc0;
  iVar1 = FUN_01025e24(DAT_01049aa0,DAT_01049a94,auStack_3c);
  if (iVar1 != 0) {
    return iVar1;
  }
  FUN_01028d30(param_5,param_6,0xffc0,0);
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
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
          FUN_01000458(0);
          puVar7 = DAT_01049a94;
          uVar4 = (uint)_PrefetchAbort;
          _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
          puVar8 = (uint *)(_SupervisorCall + uVar4 * 0x10);
          if (DAT_01049a94 == (ushort *)0x0) {
            FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1
                         ,s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,
                         0x216);
            FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1
                         ,s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,
                         0x216);
            FUN_01000458(0);
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
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                       &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                       &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
          FUN_01000458(0);
        }
        puVar7 = DAT_01049a94;
        bVar9 = DAT_01049a94 == (ushort *)0x0;
        *puVar8 = iVar1 << 0x18 | *puVar8 & 0xfcffffff;
        puVar8[2] = local_60;
        puVar8[3] = uStack_5c;
        if (bVar9) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,
                       0x230);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,
                       0x230);
          FUN_01000458(0);
        }
        if (*(int *)(puVar7 + 0x26) == 0) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
          FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
          FUN_01000458(0);
        }
        DataMemoryBarrier(0xf);
        *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
        puVar7 = DAT_01049aa0;
        if (DAT_01049aa0 == (ushort *)0x0) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
          FUN_01000458(0);
          puVar7 = DAT_01049aa0;
          uVar4 = (uint)_PrefetchAbort;
          _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
          puVar8 = (uint *)(_SupervisorCall + uVar4 * 0x10);
          if (DAT_01049aa0 == (ushort *)0x0) {
            FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1
                         ,s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,
                         0x216);
            FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1
                         ,s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,
                         0x216);
            FUN_01000458(0);
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
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                       &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                       &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
          FUN_01000458(0);
        }
        puVar6 = DAT_01049aa0;
        bVar9 = DAT_01049aa0 == (ushort *)0x0;
        *puVar8 = iVar1 << 0x18 | *puVar8 & 0xfcffffff;
        if (bVar9) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,
                       0x230);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,
                       0x230);
          FUN_01000458(0);
        }
        if (*(int *)(puVar6 + 0x26) == 0) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
          FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
          FUN_01000458(0);
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
        FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                     s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_action_add_010314b8 + 0x14,
                     0x2ba);
        FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                     s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_action_add_010314b8 + 0x14,
                     0x2ba);
        FUN_01000458(0);
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
      goto joined_r0x01004b50;
    }
    for (; uVar5 != 0; uVar5 = uVar5 - 1) {
      do {
        puVar6 = puVar7;
        puVar7 = puVar6;
        if (puVar6 == (ushort *)0x0) {
          FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_desc_action_add_010314b8 + 0x14,0x2ba);
          FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                       s_Failed_to_get_BARs_0103157c + 0x14,
                       s_al_udma_desc_action_add_010314b8 + 0x14,0x2ba);
          FUN_01000458(0);
          puVar7 = DAT_01049a94;
        }
        uVar2 = *(uint *)(*(int *)(puVar6 + 2) + 0x4c);
        puVar6[0x12] = (ushort)uVar2;
      } while (((uVar2 & 0xffff) - (uint)puVar6[0xe] & (uint)*puVar6) == 0);
      puVar6[0xe] = puVar6[0xe] + 1 & *puVar6;
    }
  }
  if (puVar7 == (ushort *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
    FUN_01000458(0);
    puVar7 = DAT_01049a94;
    uVar2 = (uint)_PrefetchAbort;
    _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
    puVar8 = (uint *)(_SupervisorCall + uVar2 * 0x10);
    if (DAT_01049a94 == (ushort *)0x0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                   s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,0x216);
      FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                   s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,0x216);
      FUN_01000458(0);
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
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                 &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                 &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
    FUN_01000458(0);
  }
  puVar7 = DAT_01049a94;
  bVar9 = DAT_01049a94 == (ushort *)0x0;
  puVar8[2] = local_60;
  puVar8[3] = uStack_5c;
  *puVar8 = *puVar8 & 0xfcff0000 | iVar1 << 0x18 | extraout_r2;
  if (bVar9) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,0x230);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,0x230);
    FUN_01000458(0);
  }
  if (*(int *)(puVar7 + 0x26) == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                 s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                 s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
    FUN_01000458(0);
  }
  puVar6 = DAT_01049aa0;
  DataMemoryBarrier(0xf);
  *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
  if (puVar6 == (ushort *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_iodma_pci_init_01031480 + 0xc,0x1fd);
    FUN_01000458(0);
    puVar6 = DAT_01049aa0;
    uVar2 = (uint)_PrefetchAbort;
    _PrefetchAbort = _PrefetchAbort + 1 & _Reset;
    puVar8 = (uint *)(_SupervisorCall + uVar2 * 0x10);
    if (DAT_01049aa0 == (ushort *)0x0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                   s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,0x216);
      FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                   s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_get_01031490 + 0x10,0x216);
      FUN_01000458(0);
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
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                 &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_fast_al_hal_010315c4 + 0x2c,1,
                 &DAT_010315c0,s__s__al_pll_channel_div_set_faile_01031440 + 0x20,0x92);
    FUN_01000458(0);
  }
  puVar7 = DAT_01049aa0;
  bVar9 = DAT_01049aa0 == (ushort *)0x0;
  *puVar8 = *puVar8 & 0xfcff0000 | iVar1 << 0x18 | extraout_r2;
  if (bVar9) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,0x230);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_ring_id_get_010314a4 + 0x10,0x230);
    FUN_01000458(0);
  }
  if (*(int *)(puVar7 + 0x26) == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                 s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_010315f8,1,s_Failed_to_get_BARs_0103157c + 0x14,
                 s_al_udma_ring_id_get_010314a4 + 0x10,0x231);
    FUN_01000458(0);
  }
  DataMemoryBarrier(0xf);
  uVar5 = 1;
  *(undefined4 *)(*(int *)(puVar7 + 2) + 0x38) = 1;
joined_r0x01004b50:
  do {
    do {
      puVar7 = DAT_01049a94;
      DAT_01049a94 = puVar7;
      if (puVar7 == (ushort *)0x0) {
        FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                     s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_action_add_010314b8 + 0x14,
                     0x2ba);
        FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                     s_Failed_to_get_BARs_0103157c + 0x14,s_al_udma_desc_action_add_010314b8 + 0x14,
                     0x2ba);
        FUN_01000458(0);
      }
      uVar2 = *(uint *)(*(int *)(puVar7 + 2) + 0x4c);
      puVar7[0x12] = (ushort)uVar2;
    } while (((uVar2 & 0xffff) - (uint)puVar7[0xe] & (uint)*puVar7) == 0);
    uVar5 = uVar5 - 1;
    puVar7[0xe] = puVar7[0xe] + 1 & *puVar7;
  } while (uVar5 != 0);
  return 0;
}



/* @ 0x1005340 */

void FUN_01005340(void)

{
  if (DAT_01049acc == 0) {
    FUN_01025260(&DAT_01049ac4,0xf0070000);
    FUN_01025300(&DAT_01049aa4,&DAT_01049ac4,0);
    FUN_01025300(DAT_010053e0,&DAT_01049ac4,0);
    FUN_01025300(DAT_010053e4,&DAT_01049ac4,0);
    FUN_01025300(DAT_010053e8,&DAT_01049ac4,0);
    DAT_01049acc = 1;
  }
  return;
}



/* @ 0x10053ec */

int FUN_010053ec(uint param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  int local_3c;
  undefined1 auStack_38 [16];
  undefined4 local_28;
  
  uVar1 = FUN_010275c0();
  iVar2 = FUN_01027c58(DAT_01049d8c,DAT_0104a500,param_1 | DAT_0104a504 << 0x1c,
                       param_1 | DAT_0104a508 << 0x1c,0,&local_3c,auStack_38);
  if (iVar2 == 0) {
    if (local_3c < 0) {
      iVar2 = -0x16;
      FUN_0100ceb0(s__s__al_flash_toc_find_id_failed___010316b8 + 0x30,
                   s____num_>_0______num_<__udma_q_>s_010315fc + 0x24,uVar1);
      FUN_01010c00(s__s__al_flash_toc_find_id_failed___010316b8 + 0x30,
                   s____num_>_0______num_<__udma_q_>s_010315fc + 0x24,uVar1);
    }
    else {
      iVar2 = FUN_01027f70(DAT_01049d8c,local_28,param_2);
    }
  }
  else {
    FUN_0100ceb0(s_Initialized_general_shared_data_01031694 + 0x20,
                 s____num_>_0______num_<__udma_q_>s_010315fc + 0x24,uVar1,DAT_0104a504);
    FUN_01010c00(s_Initialized_general_shared_data_01031694 + 0x20,
                 s____num_>_0______num_<__udma_q_>s_010315fc + 0x24,uVar1,DAT_0104a504);
  }
  return iVar2;
}



/* @ 0x1005504 */

undefined4 FUN_01005504(undefined4 param_1)

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
  
  iVar1 = FUN_01007f88(param_1,s_read_toc_obj_hdr_01031624 + 0x10);
  if (iVar1 < 0) {
    return 0;
  }
  uVar2 = FUN_01007ebc(param_1,iVar1,s__soc_board_cfg_ethernet_port_d_0103060c + 0x1c,&local_5c);
  if (local_5c < 0) {
    return 0;
  }
  iVar3 = FUN_01012958(uVar2,s_status_0103062c + 4);
  if (iVar3 != 0) {
    return 0;
  }
  puVar4 = (uint *)FUN_01007ebc(param_1,iVar1,s__soc_general_shared_data_01031638 + 0x18,local_58);
  if (puVar4 == (uint *)0x0) {
    puVar4 = (uint *)FUN_01007ebc(param_1,iVar1,s__address_cells_01031654 + 0xc,local_58);
    uVar6 = local_58[0];
joined_r0x010057c8:
    if (uVar6 == 2) {
      uVar6 = puVar4[1];
      DAT_01049ad0 = (undefined1 *)
                     (uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
                     uVar6 >> 0x18);
      goto LAB_010055dc;
    }
  }
  else {
    uVar6 = *puVar4;
    uVar6 = uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
            uVar6 >> 0x18;
    puVar4 = (uint *)FUN_01007ebc(param_1,iVar1,s__address_cells_01031654 + 0xc,local_58);
    if (uVar6 == local_58[0]) {
      if (uVar6 == 1) {
        uVar6 = *puVar4;
        DAT_01049ad0 = (undefined1 *)
                       (uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 |
                       uVar6 >> 0x18);
        goto LAB_010055dc;
      }
      goto joined_r0x010057c8;
    }
  }
  DAT_01049ad0 = (undefined1 *)0xfd8a4180;
LAB_010055dc:
  FUN_01012ae4(DAT_01049ad0,0,0x80);
  puVar5 = DAT_01049ad0;
  iVar1 = FUN_010053ec(0x200,local_58);
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
  iVar1 = FUN_010053ec(5,local_58);
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
  iVar1 = FUN_010053ec(0x100,local_58);
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
  iVar1 = FUN_010053ec(0x110,local_58);
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
  iVar1 = FUN_01003464(DAT_01049dbf,0x61d,2,DAT_01049ad0 + 0xc,4);
  puVar5 = DAT_01049ad0;
  if (iVar1 == 0) {
    uVar6 = *(uint *)(DAT_01049ad0 + 0xc);
    *(uint *)(DAT_01049ad0 + 0xc) =
         uVar6 << 0x18 | (uVar6 >> 8 & 0xff) << 0x10 | (uVar6 >> 0x10 & 0xff) << 8 | uVar6 >> 0x18;
  }
  else {
    FUN_0100ceb0(&DAT_01031668);
    FUN_01010c00(&DAT_01031668);
    puVar5 = DAT_01049ad0;
    *(undefined4 *)(DAT_01049ad0 + 0xc) = 0;
  }
  *puVar5 = 0xa7;
  FUN_0100ceb0(s_Unable_to_read_Board_RevID_from_E_0103166c + 0x24);
  FUN_01010c00(s_Unable_to_read_Board_RevID_from_E_0103166c + 0x24);
  return 0;
}



/* @ 0x1005864 */

void FUN_01005864(void)

{
  int iVar1;
  int local_2c;
  undefined1 auStack_28 [16];
  undefined4 local_18;
  
  iVar1 = FUN_01027c58(DAT_01049d8c,DAT_0104a500,DAT_0104a504 << 0x1c | 0x500,
                       DAT_0104a508 << 0x1c | 0x500,0,&local_2c,auStack_28);
  if (((iVar1 == 0) && (-1 < local_2c)) &&
     (iVar1 = FUN_01027f70(DAT_01049d8c,local_18,&DAT_01049ad4), iVar1 == 0)) {
    DAT_01049b1c = local_18;
    DAT_01049b20 = 1;
  }
  return;
}



/* @ 0x1005918 */


int FUN_01005918(void)

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
  bool bVar19;
  int *local_9e4;
  int *local_9e0;
  uint local_9c8;
  uint local_9a4;
  int local_9a0;
  char acStack_99c [8];
  uint local_994;
  undefined4 local_990;
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
  byte local_660 [4];
  int local_65c [3];
  int local_650;
  undefined1 local_64c [2];
  undefined2 local_64a;
  undefined1 local_648 [4];
  undefined4 uStack_644;
  byte abStack_640 [16];
  byte abStack_630 [28];
  byte local_614;
  int local_610;
  int local_60c;
  int local_608;
  int local_604;
  undefined1 local_600;
  undefined1 local_5fe;
  byte local_5c8;
  int local_5c4;
  int local_5c0;
  uint local_5bc;
  int local_5b8;
  undefined1 local_5b4;
  undefined1 local_5b2;
  undefined1 local_5b0;
  byte local_57c;
  int local_578;
  int local_574;
  int local_570;
  int local_56c;
  undefined1 local_568;
  undefined1 local_566;
  undefined1 local_563;
  byte local_530;
  undefined1 local_51b;
  undefined1 local_51a;
  undefined1 local_517;
  undefined4 local_4e0;
  undefined4 local_4dc;
  undefined4 auStack_4d8 [2];
  byte abStack_4d0 [4];
  int aiStack_4cc [298];
  
  FUN_01012ae4(local_660,0,0x188);
  local_994 = ram0x01031704;
  local_990 = DAT_01031708;
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
  FUN_01012ae4(auStack_7a0,0,0x140);
  FUN_01012ae4(aiStack_890,0,0xf0);
  local_9e4 = aiStack_930;
  do {
    iVar10 = 0;
    piVar14 = local_9e0;
    piVar17 = local_9e4;
    do {
      FUN_01010b88(auStack_4d8,s_dt_based_init_serdes_group_01031bcc + 0x18,iVar11,iVar10);
      iVar1 = FUN_01007f88(DAT_01049d88,auStack_4d8);
      *piVar14 = iVar1;
      FUN_01010b88(auStack_4d8,s__soc_board_cfg_serdes_group_d_la_01031be8 + 0x2c,iVar11,iVar10);
      iVar10 = iVar10 + 1;
      iVar1 = FUN_01007f88(DAT_01049d88,auStack_4d8);
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
  do {
    uVar12 = (uint)local_9e4 & 0xff;
    iVar1 = uVar12 * 0x4c;
    FUN_010129dc(auStack_4d8,DAT_0100690c,0x4b0);
    FUN_01010b88(auStack_8e0,s__soc_board_cfg_serdes_group_d_la_01031c18 + 0x2c,uVar12);
    iVar10 = iVar1 + 4;
    iVar11 = FUN_01007f88(DAT_01049d88,auStack_8e0);
    if (iVar11 < 0) {
      return -0x16;
    }
    uVar2 = FUN_01007ebc(DAT_01049d88,iVar11,s__soc_board_cfg_serdes_group_d_01031c48 + 0x1c,0);
    uVar3 = FUN_01007ebc(DAT_01049d88,iVar11,s_interface_01031c68 + 8,0);
    uVar4 = FUN_01007ebc(DAT_01049d88,iVar11,s_ref_clock_01031c74 + 8,0);
    iVar5 = FUN_01007e68(DAT_01049d88,iVar11,&DAT_01031c80,&local_9a4);
    if (iVar5 == 0) {
      FUN_0100ceb0(s_active_lanes_01031c84 + 0xc,&DAT_01031bc8,&DAT_01031c80);
      FUN_01010c00(s_active_lanes_01031c84 + 0xc,&DAT_01031bc8,&DAT_01031c80);
      return -0x16;
    }
    local_9a4 = local_9a4 >> 2;
    if (local_9a4 != 0) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        if (3 < uVar13) {
          FUN_0100ceb0(s__s__property___s__missing_01031c94 + 0x18,uVar13);
          FUN_01010c00(s__s__property___s__missing_01031c94 + 0x18,uVar13);
          return -0x16;
        }
        if (*(int *)(local_660 + uVar13 * 4 + iVar10) == 1) {
          FUN_0100ceb0(s_invalid_active_lane___d___01031cb0 + 0x18,uVar13);
          FUN_01010c00(s_invalid_active_lane___d___01031cb0 + 0x18,uVar13);
          return -0x16;
        }
        pbVar9 = local_660 + uVar13 * 4 + iVar10;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (local_9a4 != uVar8);
    }
    iVar5 = FUN_01012958(uVar2,s__s__s___d___not_supported_interf_01032194 + 0x24);
    if (iVar5 == 0) {
      pbVar9 = local_660 + iVar1;
      do {
        pbVar9 = pbVar9 + 4;
        if (*(int *)pbVar9 != 1) {
          FUN_0100ceb0(s_lane_defined_active_multiple_tim_01031ccc + 0x28);
          FUN_01010c00(s_lane_defined_active_multiple_tim_01031ccc + 0x28);
          return -0x16;
        }
      } while (local_64c + iVar1 + -4 != pbVar9);
    }
    iVar5 = FUN_01007e68(DAT_01049d88,iVar11,s_When_serdes_is_40gbe__all_lanes_m_01031cf8 + 0x34,
                         &local_9a4);
    if ((iVar5 != 0) && (local_9a4 = local_9a4 >> 2, local_9a4 != 0)) {
      puVar6 = (uint *)(iVar5 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        iVar5 = iVar1 + 0x20 + uVar13 * 4;
        if (3 < uVar13) goto LAB_01005cbc;
        if (*(int *)(local_660 + iVar5) == 1) {
          FUN_0100ceb0(s_invalid_lane___d___01031d50 + 0x14,uVar13);
          FUN_01010c00(s_invalid_lane___d___01031d50 + 0x14,uVar13);
          return -0x16;
        }
        pbVar9 = local_660 + iVar5;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (local_9a4 != uVar8);
    }
    iVar11 = FUN_01007e68(DAT_01049d88,iVar11,s_inv_tx_lanes_01031d30 + 0xc,&local_9a4);
    if ((iVar11 != 0) && (local_9a4 = local_9a4 >> 2, local_9a4 != 0)) {
      puVar6 = (uint *)(iVar11 + 8);
      uVar8 = 0;
      do {
        puVar6 = puVar6 + 1;
        uVar13 = *puVar6;
        uVar8 = uVar8 + 1;
        uVar13 = uVar13 << 0x18 | (uVar13 >> 8 & 0xff) << 0x10 | (uVar13 >> 0x10 & 0xff) << 8 |
                 uVar13 >> 0x18;
        iVar11 = iVar1 + 0x30 + uVar13 * 4;
        if (3 < uVar13) {
LAB_01005cbc:
          FUN_0100ceb0(s_inv_rx_lanes_01031d40 + 0xc,uVar13);
          FUN_01010c00(s_inv_rx_lanes_01031d40 + 0xc,uVar13);
          return -0x16;
        }
        if (*(int *)(local_660 + iVar11) == 1) {
          FUN_0100ceb0(s_tx_lane_defined_inverted_multipl_01031d68 + 0x2c,uVar13);
          FUN_01010c00(s_tx_lane_defined_inverted_multipl_01031d68 + 0x2c,uVar13);
          return -0x16;
        }
        pbVar9 = local_660 + iVar11;
        pbVar9[0] = 1;
        pbVar9[1] = 0;
        pbVar9[2] = 0;
        pbVar9[3] = 0;
      } while (local_9a4 != uVar8);
    }
    piVar14 = aiStack_980 + (int)local_9e4 * 4;
    puVar16 = auStack_7a0 + (int)local_9e4 * 0x10;
    do {
      iVar11 = *piVar14;
      if (((-1 < iVar11) &&
          (iVar1 = FUN_01007ebc(DAT_01049d88,iVar11,
                                s_rx_lane_defined_inverted_multipl_01031d98 + 0x2c,0), iVar1 != 0))
         && (iVar1 = FUN_01012958(iVar1,s_status_0103062c + 4), iVar1 == 0)) {
        *puVar16 = 1;
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_override_01031dc8 + 8,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)(puVar16 + 1) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,&DAT_01031dd8,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 5) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_3db_freq_01031ddc + 0xc,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 6) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_gain_01031dec + 8,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 7) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_1st_tap_ctrl_01031df8 + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)(puVar16 + 2) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_2nd_tap_ctrl_01031e0c + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 9) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_3rd_tap_ctrl_01031e20 + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 10) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_dfe_4th_tap_ctrl_01031e34 + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)puVar16 + 0xb) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar11 = FUN_01007e68(DAT_01049d88,iVar11,s_low_freq_agc_gain_01031e48 + 0x10,&local_9a0);
        if ((iVar11 != 0) && (local_9a0 != 0)) {
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
          (iVar1 = FUN_01007ebc(DAT_01049d88,iVar11,
                                s_rx_lane_defined_inverted_multipl_01031d98 + 0x2c,0), iVar1 != 0))
         && (iVar1 = FUN_01012958(iVar1,s_status_0103062c + 4), iVar1 == 0)) {
        *piVar17 = 1;
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_high_freq_agc_boost_01031e5c + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)(piVar17 + 1) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,&DAT_01031e70,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)piVar17 + 5) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_total_driver_units_01031e74 + 0x10,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)((int)piVar17 + 6) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar1 = FUN_01007e68(DAT_01049d88,iVar11,s_post_emph_01031e88 + 8,&local_9a0);
        if ((iVar1 != 0) && (local_9a0 != 0)) {
          *(char *)(piVar17 + 2) = (char)((uint)*(undefined4 *)(iVar1 + 0xc) >> 0x18);
        }
        iVar11 = FUN_01007e68(DAT_01049d88,iVar11,s_pre_emph_01031e94 + 8,&local_9a0);
        if ((iVar11 != 0) && (local_9a0 != 0)) {
          *(char *)((int)piVar17 + 9) = (char)((uint)*(undefined4 *)(iVar11 + 0xc) >> 0x18);
        }
      }
      piVar14 = piVar14 + 1;
      piVar17 = piVar17 + 3;
    } while (aiStack_920 + (int)local_9e4 * 4 != piVar14);
    iVar11 = 0;
    acStack_99c[uVar12] = '\0';
    puVar16 = auStack_4d8 + uVar12 * 0x3c;
    while ((puVar16[1] != 1 || (iVar1 = FUN_01012958(uVar2,*puVar16), iVar1 != 0))) {
      iVar11 = iVar11 + 1;
      puVar16 = puVar16 + 4;
      if (iVar11 == 0xf) {
        FUN_0100ceb0(s_al_flash_obj_data_load_failed__s_01032164 + 0x2c,&DAT_01031bc8,uVar2,uVar12);
        FUN_01010c00(s_al_flash_obj_data_load_failed__s_01032164 + 0x2c,&DAT_01031bc8,uVar2,uVar12);
        return -0x16;
      }
    }
    if ((aiStack_4cc[(uVar12 * 0xf + iVar11) * 4] == -1) ||
       (bVar7 = 1,
       *(int *)(DAT_0104a3a8 + aiStack_4cc[(uVar12 * 0xf + iVar11) * 4] * 0x14 + 0xc) == 0)) {
      bVar7 = abStack_4d0[(uVar12 * 0xf + iVar11) * 0x10];
    }
    iVar11 = uVar12 * 0x4c;
    local_660[iVar11] = bVar7;
    iVar1 = FUN_01012958(uVar3,s_slew_rate_01031ea0 + 8);
    if (iVar1 == 0) {
      iVar1 = 2;
      acStack_99c[uVar12] = '\x02';
    }
    else {
      iVar1 = FUN_01012958(uVar3,s_100Mhz_internal_01031eac + 0xc);
      if (iVar1 == 0) {
        *(undefined2 *)(local_64c + iVar11 + 2) = 0x101;
        iVar1 = 0;
      }
      else {
        iVar1 = FUN_01012958(uVar3,&DAT_01031ec0);
        if (iVar1 == 0) {
          *(undefined2 *)(local_64c + iVar11 + 2) = 0x201;
          iVar1 = 0;
        }
        else {
          iVar1 = FUN_01012958(uVar3,&DAT_01031ec8);
          if (iVar1 == 0) {
            *(undefined2 *)(local_64c + iVar11 + 2) = 0x301;
            iVar1 = 0;
          }
          else {
            iVar1 = FUN_01012958(uVar3,&DAT_01031ed4);
            if (iVar1 == 0) {
              iVar1 = 1;
              local_64c[iVar11 + 1] = 4;
              local_64c[iVar11 + 2] = 4;
              acStack_99c[uVar12] = '\x01';
            }
            else {
              iVar1 = FUN_01012958(uVar3,&DAT_01031edc);
              if (iVar1 != 0) {
                FUN_0100ceb0(s_right_01031ee0 + 4,uVar3);
                FUN_01010c00(s_right_01031ee0 + 4,uVar3);
                return -0x16;
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
    uVar8 = (uint)local_660[uVar12 * 0x4c];
    bVar19 = 0x14 < uVar8;
    if (uVar8 != 0x15) {
      bVar19 = 1 < uVar8 - 0x10;
    }
    if (!bVar19 || (uVar8 == 0x15 || uVar8 - 0x10 == 2)) {
      iVar11 = 0;
      piVar14 = DAT_01006910;
      do {
        if (((*piVar14 != 0) && (piVar14[4] == uVar12)) &&
           (*(int *)(local_660 + piVar14[8] * 4 + iVar10) == 1)) {
          FUN_01003f34(iVar11);
        }
        iVar11 = iVar11 + 1;
        piVar14 = piVar14 + 1;
      } while (iVar11 != 4);
    }
    bVar7 = local_660[uVar12 * 0x4c];
    if (bVar7 == 0x13) {
      if ((DAT_0104a56c != 0) && (DAT_0104a57c == uVar12)) {
        FUN_01003f34(0,1);
        bVar7 = local_660[uVar12 * 0x4c];
        goto LAB_01006388;
      }
    }
    else {
LAB_01006388:
      if (bVar7 == 0x14) {
        if ((DAT_0104a574 == 0) || (DAT_0104a584 != uVar12)) goto LAB_010063b4;
        FUN_01003f34(2,1);
        bVar7 = local_660[uVar12 * 0x4c];
      }
      if (bVar7 == 0xe) {
        FUN_01003f34(8,1);
        bVar7 = local_660[uVar12 * 0x4c];
      }
      if (bVar7 == 0xf) {
        FUN_01003f34(9,1);
      }
    }
LAB_010063b4:
    iVar11 = FUN_01012958(uVar4,s_status_0103062c + 4);
    if (iVar11 == 0) {
      *(undefined4 *)(local_64c + uVar12 * 0x4c + 8) = 1;
    }
    if (iVar1 == 2) {
      if (acStack_99c[local_9c8 & 0xff] != '\x02') {
        iVar11 = (local_9c8 & 0xff) * 0x4c;
        local_64c[iVar11] = 1;
        local_64c[iVar11 + 4] = local_64c[iVar11 + 3];
      }
LAB_010063f8:
      bVar19 = false;
    }
    else {
      if (iVar1 != 1) goto LAB_010063f8;
      bVar19 = true;
    }
    if (&local_984 == puVar18) {
      if ((DAT_0104a608 == 0) && ((local_660[0] & 0xfd) != 0)) {
        FUN_0100ceb0(s_Not_supported_reference_clock____01031ee8 + 0x24);
        FUN_01010c00(s_Not_supported_reference_clock____01031ee8 + 0x24);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a60c == 0) && ((local_614 & 0xfd) != 0)) {
        FUN_0100ceb0(s_group_A_off_violation__01031f10 + 0x14);
        FUN_01010c00(s_group_A_off_violation__01031f10 + 0x14);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a610 == 0) && ((local_5c8 & 0xfd) != 0)) {
        FUN_0100ceb0(s_group_B_off_violation__01031f28 + 0x14);
        FUN_01010c00(s_group_B_off_violation__01031f28 + 0x14);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a614 == 0) && ((local_57c & 0xfd) != 0)) {
        FUN_0100ceb0(s_group_C_off_violation__01031f40 + 0x14);
        FUN_01010c00(s_group_C_off_violation__01031f40 + 0x14);
        DAT_0104a568 = 1;
      }
      if ((DAT_0104a618 == 0) && (2 < local_530)) {
        FUN_0100ceb0(s_group_D_off_violation__01031f58 + 0x14);
        FUN_01010c00(s_group_D_off_violation__01031f58 + 0x14);
        DAT_0104a568 = 1;
      }
      if ((local_614 == 0xe) &&
         ((((DAT_0104a5c8 == 0 || ((local_610 != 0 && ((DAT_0104a5cc & 1) == 0)))) ||
           ((local_60c != 0 && ((DAT_0104a5cc & 2) == 0)))) ||
          (((local_608 != 0 && ((DAT_0104a5cc & 4) == 0)) ||
           ((local_604 != 0 && ((DAT_0104a5cc & 8) == 0)))))))) {
        FUN_0100ceb0(s_group_E_off_violation__01031f70 + 0x14);
        FUN_01010c00(s_group_E_off_violation__01031f70 + 0x14);
        DAT_0104a568 = 1;
      }
      uVar12 = (uint)local_5c8;
      if (uVar12 == 0xf) {
        if ((((DAT_0104a5d0 == 0) || ((local_5c4 != 0 && ((DAT_0104a5d4 & 1) == 0)))) ||
            ((local_5c0 != 0 && ((DAT_0104a5d4 & 2) == 0)))) ||
           (((local_5bc != 0 && ((DAT_0104a5d4 & 4) == 0)) ||
            ((local_5b8 != 0 && ((DAT_0104a5d4 & 8) == 0)))))) {
          FUN_0100ceb0(s_SATA_0_violation__01031f88 + 0x10);
          FUN_01010c00(s_SATA_0_violation__01031f88 + 0x10);
          uVar12 = (uint)local_5c8;
          DAT_0104a568 = 1;
          goto LAB_01006588;
        }
LAB_01006b10:
        if ((local_57c - 0x10 < 3) &&
           (((((local_578 != 0 && (DAT_0104a5e4 == 0)) || ((local_574 != 0 && (DAT_0104a5e0 == 0))))
             || ((local_570 != 0 && (DAT_0104a5dc == 0)))) ||
            ((local_56c != 0 && (DAT_0104a5d8 == 0)))))) goto LAB_010065f4;
      }
      else {
LAB_01006588:
        if ((2 < uVar12 - 0x10) ||
           (((((local_5c4 == 0 || (DAT_0104a5e4 != 0)) && ((local_5c0 == 0 || (DAT_0104a5e0 != 0))))
             && ((local_5bc == 0 || (DAT_0104a5dc != 0)))) &&
            ((local_5b8 == 0 || (DAT_0104a5d8 != 0)))))) goto LAB_01006b10;
LAB_010065f4:
        FUN_0100ceb0(s_SATA_1_violation__01031f9c + 0x10);
        FUN_01010c00(s_SATA_1_violation__01031f9c + 0x10);
        DAT_0104a568 = 1;
      }
      uVar8 = (uint)local_660[0];
      uVar12 = local_65c[1];
      if (*(int *)(DAT_0104a3a8 + 0xc) == 0) {
        if (uVar8 == 5) {
          uVar13 = 2;
LAB_01006654:
          if (local_65c[1] != 0) {
            if (local_65c[2] == 0) {
              uVar12 = 1;
            }
            else if (local_650 == 0) {
              uVar12 = 1;
            }
            else {
              uVar12 = 2;
            }
          }
        }
        else {
          uVar13 = (uint)(uVar8 - 3 < 2);
          if (uVar8 == 3) goto LAB_01006bc0;
LAB_01006648:
          if (uVar8 != 6) goto LAB_01006654;
          uVar12 = 3;
        }
LAB_01006680:
        if (DAT_0104a5e8 < uVar13) {
          FUN_0100ceb0(s_ETH_violation__01031fb0 + 0xc,uVar13);
          FUN_01010c00(s_ETH_violation__01031fb0 + 0xc,uVar13,DAT_0104a5e8);
          DAT_0104a568 = 1;
        }
        if ((uVar13 != 0) && (DAT_0104a5ec < uVar12)) {
          FUN_0100ceb0(s_PCIe_0_speed_violation___d_>__d__01031fc0 + 0x20,uVar12);
          FUN_01010c00(s_PCIe_0_speed_violation___d_>__d__01031fc0 + 0x20,uVar12,DAT_0104a5ec);
          DAT_0104a568 = 1;
        }
        uVar8 = (uint)local_660[0];
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (local_614 == 8) {
            uVar12 = 2;
          }
          else if (local_614 == 7) {
            uVar12 = 1;
          }
          else {
LAB_01006700:
            uVar12 = (uint)(uVar8 == 3);
          }
        }
        else {
          uVar12 = 3;
        }
        bVar19 = uVar8 == 3;
        uVar8 = uVar12;
        if (bVar19) goto LAB_01006e30;
        uVar12 = 0;
        if (local_60c != 0) {
          if (local_608 == 0) {
            uVar12 = 1;
          }
          else if (local_604 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_01006740:
        if (DAT_0104a5f0 < uVar8) {
          FUN_0100ceb0(s_PCIe_0_width_violation___d_>__d__01031fe4 + 0x20,uVar8);
          FUN_01010c00(s_PCIe_0_width_violation___d_>__d__01031fe4 + 0x20,uVar8,DAT_0104a5f0);
          DAT_0104a568 = 1;
        }
        if ((uVar8 != 0) && (DAT_0104a5f4 < uVar12)) {
          FUN_0100ceb0(s_PCIe_1_speed_violation___d_>__d__01032008 + 0x20,uVar12);
          FUN_01010c00(s_PCIe_1_speed_violation___d_>__d__01032008 + 0x20,uVar12,DAT_0104a5f4);
          DAT_0104a568 = 1;
        }
      }
      else {
        uVar13 = 3;
        if (uVar8 != 3) goto LAB_01006648;
LAB_01006bc0:
        if (local_65c[0] != 0) {
          if (local_65c[1] != 0) {
            uVar12 = 1;
          }
          goto LAB_01006680;
        }
        if (*(int *)(DAT_0104a3a8 + 0x20) == 0) {
          if (local_614 == 8) {
            uVar8 = 2;
            goto LAB_01006e30;
          }
          if (local_614 == 7) {
            uVar8 = 1;
            goto LAB_01006e30;
          }
          goto LAB_01006700;
        }
LAB_01006e30:
        if (local_65c[2] != 0) {
          uVar12 = (uint)(local_650 != 0);
          goto LAB_01006740;
        }
      }
      uVar8 = (uint)local_5c8;
      uVar12 = local_5bc;
      if (*(int *)(DAT_0104a3a8 + 0x34) == 0) {
        if (uVar8 == 0xb) {
          uVar13 = 2;
        }
        else {
          uVar13 = (uint)(uVar8 - 9 < 2);
          if (uVar8 == 9) goto LAB_01006b94;
        }
LAB_010067c4:
        if (local_5bc != 0) {
          if (local_5c0 == 0) {
            uVar12 = 1;
          }
          else if (local_5c4 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 2;
          }
        }
LAB_010067f0:
        if (DAT_0104a5f8 < uVar13) {
          FUN_0100ceb0(s_PCIe_1_width_violation___d_>__d__0103202c + 0x20,uVar13);
          FUN_01010c00(s_PCIe_1_width_violation___d_>__d__0103202c + 0x20,uVar13,DAT_0104a5f8);
          DAT_0104a568 = 1;
        }
        if ((uVar13 != 0) && (DAT_0104a5fc < uVar12)) {
          FUN_0100ceb0(s_PCIe_2_speed_violation___d_>__d__01032050 + 0x20,uVar12);
          FUN_01010c00(s_PCIe_2_speed_violation___d_>__d__01032050 + 0x20,uVar12,DAT_0104a5fc);
          DAT_0104a568 = 1;
        }
        uVar8 = (uint)local_5c8;
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (local_57c == 0xd) {
            uVar12 = 2;
          }
          else if (local_57c == 0xc) {
            uVar12 = 1;
          }
          else {
LAB_01006870:
            uVar12 = (uint)(uVar8 == 9);
          }
        }
        else {
          uVar12 = 3;
        }
        if (uVar8 == 9) goto LAB_01006bf8;
        uVar8 = 0;
        if (local_570 != 0) {
          if (local_574 == 0) {
            uVar8 = 1;
          }
          else if (local_578 == 0) {
            uVar8 = 1;
          }
          else {
            uVar8 = 2;
          }
        }
        if (uVar12 <= DAT_0104a600) goto LAB_010068bc;
LAB_01006c1c:
        FUN_0100ceb0(s_PCIe_2_width_violation___d_>__d__01032074 + 0x20,uVar12);
        FUN_01010c00(s_PCIe_2_width_violation___d_>__d__01032074 + 0x20,uVar12,DAT_0104a600);
        DAT_0104a568 = 1;
        if ((uVar12 != 0) && (DAT_0104a604 < uVar8)) goto LAB_01006c5c;
LAB_01006c88:
        FUN_01003f34(0);
        FUN_01003f34(2,0);
        FUN_01003f34(3,0);
        FUN_01003f34(6,0);
        FUN_01003f34(7,0);
        FUN_01003f34(8,0);
        FUN_01003f34(9,0);
        local_660[0] = 0;
        local_614 = 0;
        local_5c8 = 0;
        local_57c = 0;
      }
      else {
        uVar13 = 3;
        if (uVar8 != 9) goto LAB_010067c4;
LAB_01006b94:
        if (local_5b8 != 0) {
          if (local_5bc != 0) {
            uVar12 = 1;
          }
          goto LAB_010067f0;
        }
        if (*(int *)(DAT_0104a3a8 + 0x48) == 0) {
          if (local_57c == 0xd) {
            uVar12 = 2;
            goto LAB_01006bf8;
          }
          if (local_57c == 0xc) {
            uVar12 = 1;
            goto LAB_01006bf8;
          }
          goto LAB_01006870;
        }
        uVar12 = 3;
LAB_01006bf8:
        if (local_5c0 != 0) {
          uVar8 = (uint)(local_5c4 != 0);
          if (DAT_0104a600 < uVar12) goto LAB_01006c1c;
LAB_010068bc:
          if ((uVar12 != 0) && (DAT_0104a604 < uVar8)) {
LAB_01006c5c:
            FUN_0100ceb0(s_PCIe_3_speed_violation___d_>__d__01032098 + 0x20,uVar8);
            FUN_01010c00(s_PCIe_3_speed_violation___d_>__d__01032098 + 0x20,uVar8,DAT_0104a604);
            DAT_0104a568 = 1;
            goto LAB_01006c88;
          }
        }
        if (DAT_0104a568 != 0) goto LAB_01006c88;
      }
      uVar2 = DAT_01006914;
      iVar11 = 1000;
      FUN_01024b90(DAT_01006914,0xfd860a00,0xfd8a8000);
      FUN_01024c60(uVar2,1);
      break;
    }
    local_9e4 = (int *)puVar18[1];
    local_9c8 = *puVar18;
    puVar18 = puVar18 + 1;
  } while( true );
  while( true ) {
    FUN_01007590(1);
    iVar11 = iVar11 + -1;
    if (iVar11 == 0) break;
    iVar10 = FUN_01024d14(uVar2);
    if (iVar10 != 0) {
      FUN_01024db8(DAT_01006914);
      goto LAB_01006948;
    }
  }
  FUN_0100ceb0(s_PCIe_3_width_violation___d_>__d__010320bc + 0x20);
  FUN_01010c00(s_PCIe_3_width_violation___d_>__d__010320bc + 0x20);
LAB_01006948:
  iVar11 = DAT_0100691c;
  uVar12 = 0;
  iVar10 = DAT_0100691c;
  do {
    iVar1 = FUN_01026150(uVar12 * 0x400 + -0x2740000,uVar12 & 0xff,iVar10);
    uVar12 = uVar12 + 1;
    if (iVar1 != 0) goto LAB_01006e54;
    FUN_0102617c(iVar10,0x10057e0);
    iVar10 = iVar10 + 0xfc;
  } while (uVar12 != 4);
  iVar10 = FUN_01026150(0xfd8c2000,4,DAT_01006918);
  if (iVar10 == 0) {
    FUN_0102617c(DAT_01006918,0x10057e0);
    if ((DAT_01049b20 != 0) && (2 < local_530)) {
      FUN_0100ceb0(s_al_serdes_handle_init_failed__01032114 + 0x1c,0x1120000,DAT_01049afc);
      FUN_01010c00(s_al_serdes_handle_init_failed__01032114 + 0x1c,0x1120000,DAT_01049afc);
      iVar10 = FUN_010280d8(DAT_01049d8c,DAT_01049b1c,0x1120000);
      if (iVar10 == 0) {
        local_4e0 = 0x1120000;
        local_4dc = DAT_01049afc;
      }
      else {
        FUN_0100ceb0(s_Loading_SerDes_25G_FW_to__08X____01032134 + 0x2c);
        FUN_01010c00(s_Loading_SerDes_25G_FW_to__08X____01032134 + 0x2c);
      }
    }
    iVar10 = FUN_01026184(DAT_0100691c,0xfd8a8000,local_660);
    if (iVar10 == 0) {
      puVar16 = auStack_7a0;
      iVar1 = 0;
      piVar14 = local_65c;
      bVar7 = local_660[0];
      while( true ) {
        if (bVar7 != 0) {
          piVar15 = (int *)((int)aiStack_890 + iVar1);
          uVar12 = 0;
          piVar17 = piVar14;
          do {
            if (*piVar17 != 0) {
              if (puVar16[uVar12 * 4] == 1) {
                (**(code **)(iVar11 + 0xa4))(iVar11,uVar12 & 0xff,puVar16 + uVar12 * 4);
              }
              if (*piVar15 == 1) {
                (**(code **)(iVar11 + 0x9c))(iVar11,uVar12 & 0xff,piVar15);
              }
            }
            uVar12 = uVar12 + 1;
            piVar15 = piVar15 + 3;
            piVar17 = piVar17 + 1;
          } while (uVar12 != 4);
        }
        iVar1 = iVar1 + 0x30;
        iVar11 = iVar11 + 0xfc;
        puVar16 = puVar16 + 0x10;
        if (iVar1 == 0xf0) break;
        bVar7 = *(byte *)(piVar14 + 0x12);
        piVar14 = piVar14 + 0x13;
      }
    }
  }
  else {
LAB_01006e54:
    iVar10 = -1;
    FUN_0100ceb0(s_Thermal_sensor_failed_to_power_u_010320e0 + 0x30);
    FUN_01010c00(s_Thermal_sensor_failed_to_power_u_010320e0 + 0x30);
  }
  return iVar10;
}



/* @ 0x1007274 */

undefined4 FUN_01007274(int param_1,int param_2,int param_3)

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



/* @ 0x10072ac */

uint FUN_010072ac(void)

{
  uint uVar1;
  
  if (DAT_01049b30 == '\0') {
    uVar1 = FUN_01000284();
    return uVar1;
  }
  if (DAT_01049b30 == '\x01') {
    return *(uint *)(DAT_010492dc + 0x1008);
  }
  uVar1 = FUN_010248d4(&DAT_01049b38);
  return ~uVar1;
}



/* @ 0x10073ac */

void FUN_010073ac(int param_1,undefined4 param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  
  if (DAT_01049b2c != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_basic_src_time_c_010322bc + 0x18,1,
                 s_udelay_010322b4 + 4,s_sb_timer_init_01032298 + 0xc,0xe3);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_basic_src_time_c_010322bc + 0x18,1,
                 s_udelay_010322b4 + 4,s_sb_timer_init_01032298 + 0xc,0xe3);
    FUN_01000458(0);
  }
  DAT_01049b28 = param_3 >> 4;
  DAT_01049b30 = (undefined1)param_1;
  DAT_01049b2c = 1;
  DAT_01049b24 = param_2;
  DAT_01049b34 = param_4;
  if (param_1 == 2) {
    iVar1 = FUN_010244b8(&DAT_01049b38,0xfd890000,0);
    if (iVar1 != 0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s___ref_initialized____0__010322d8 + 0x14,iVar1,
                   s_udelay_010322b4 + 4,s_Thermal_sensor_failed_to_measure_01032260 + 0x34,0x7e);
      FUN_01010c00(s_vectors_01028ea4 + 4,s___ref_initialized____0__010322d8 + 0x14,iVar1,
                   s_udelay_010322b4 + 4,s_Thermal_sensor_failed_to_measure_01032260 + 0x34,0x7e);
      FUN_01000458(0);
    }
    FUN_01024824(&DAT_01049b38,0);
    FUN_010245d4(&DAT_01049b38,1,0);
    FUN_010246dc(&DAT_01049b38,0xffffffff);
    FUN_01024774(&DAT_01049b38,0);
    FUN_01024824(&DAT_01049b38,1);
  }
  DAT_01049b2c = 1;
  return;
}



/* @ 0x1007590 */

undefined4 FUN_01007590(uint param_1)

{
  uint uVar1;
  uint *puVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  
  if (DAT_01049b2c != 1) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_timer_init__sb_timer___void___010322f0 + 0x38,1,
                 s_udelay_010322b4 + 4,s_time_init_010322a8 + 8,0xfb);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_timer_init__sb_timer___void___010322f0 + 0x38,1,
                 s_udelay_010322b4 + 4,s_time_init_010322a8 + 8,0xfb);
    FUN_01000458(0);
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
      uVar1 = FUN_010072ac();
      uVar6 = uVar5 * (uVar3 / 1000);
      uVar4 = uVar6 / 1000;
      if (999 < uVar6) {
        do {
          uVar6 = FUN_010072ac();
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



/* @ 0x1007800 */

void FUN_01007800(void)

{
  return;
}



/* @ 0x1007804 */

undefined4 FUN_01007804(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  
  uVar2 = *param_1;
  uVar2 = uVar2 << 0x18 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 | uVar2 >> 0x18;
  if (uVar2 != 0xd00dfeed) {
    if (uVar2 != 0x2ff20112) {
      return 0xfffffff7;
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
    return 0xfffffff6;
  }
  uVar2 = param_1[6];
  if ((uVar2 << 0x18 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 | uVar2 >> 0x18) <
      0x12) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0xfffffff6;
  }
  return uVar1;
}



/* @ 0x1007878 */

int FUN_01007878(int param_1,uint param_2,uint param_3)

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



/* @ 0x10078d4 */

uint FUN_010078d4(undefined4 param_1,int param_2,uint *param_3)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  bool bVar6;
  
  *param_3 = 0xfffffff8;
  puVar1 = (uint *)FUN_01007878(param_1,param_2,4);
  if (puVar1 != (uint *)0x0) {
    uVar5 = *puVar1;
    iVar2 = param_2 + 4;
    *param_3 = 0xfffffff5;
    uVar5 = uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 |
            uVar5 >> 0x18;
    iVar3 = uVar5 - 1;
    bVar6 = iVar3 == 8;
    iVar4 = iVar2;
    switch(iVar3) {
    case 0:
      while (!bVar6) {
        iVar2 = iVar4 + 1;
        puVar1 = (uint *)FUN_01007878(param_1,iVar4);
        bVar6 = puVar1 == (uint *)0x0;
switchD_01007928_caseD_4:
        if (bVar6) goto switchD_01007928_default;
        iVar4 = iVar2;
        bVar6 = (char)*puVar1 == '\0';
      }
      break;
    case 1:
    case 3:
    case 8:
      iVar4 = iVar2 + iVar3;
      break;
    case 2:
      goto switchD_01007928_caseD_2;
    case 4:
    case 5:
    case 6:
    case 7:
      goto switchD_01007928_caseD_4;
    default:
      goto switchD_01007928_default;
    }
    iVar2 = FUN_01007878(param_1,param_2,iVar4 - param_2);
    if (iVar2 != 0) {
      *param_3 = iVar4 + 3U & 0xfffffffc;
      return uVar5;
    }
  }
switchD_01007928_default:
  uVar5 = 9;
switchD_01007928_caseD_2:
  return uVar5;
}



/* @ 0x10079e4 */

uint FUN_010079e4(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_c [2];
  
  if ((((int)param_2 < 0) || ((param_2 & 3) != 0)) ||
     (local_c[0] = param_2, iVar1 = FUN_010078d4(param_1,param_2,local_c), iVar1 != 1)) {
    local_c[0] = 0xfffffffc;
  }
  return local_c[0];
}



/* @ 0x1007a24 */

uint FUN_01007a24(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_c [2];
  
  if ((((int)param_2 < 0) || ((param_2 & 3) != 0)) ||
     (local_c[0] = param_2, iVar1 = FUN_010078d4(param_1,param_2,local_c), iVar1 != 3)) {
    local_c[0] = 0xfffffffc;
  }
  return local_c[0];
}



/* @ 0x1007a64 */

int FUN_01007a64(undefined4 param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int local_14;
  
  local_14 = 0;
  if ((param_2 < 0) || (iVar2 = FUN_010079e4(), local_14 = iVar2, -1 < iVar2)) {
    do {
      while (iVar2 = local_14, iVar1 = FUN_010078d4(param_1,local_14,&local_14), iVar1 == 2) {
        if ((param_3 != (int *)0x0) && (iVar2 = *param_3, *param_3 = iVar2 + -1, iVar2 + -1 < 0)) {
          return local_14;
        }
      }
      if (iVar1 == 9) {
        if (-1 < local_14) {
          return -1;
        }
        if (param_3 != (int *)0x0 || local_14 != -8) {
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



/* @ 0x1007bc0 */

int FUN_01007bc0(undefined4 param_1,int param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int local_24 [2];
  
  iVar1 = FUN_01007804();
  if ((iVar1 == 0) && (iVar1 = param_2, -1 < param_2)) {
    local_24[0] = 0;
    while (param_2 = FUN_01007a64(param_1,param_2,local_24), -1 < param_2 && -1 < local_24[0]) {
      if (((local_24[0] == 1) && (iVar1 = FUN_01007878(param_1,param_2 + 4,param_4 + 1), iVar1 != 0)
          ) && (iVar2 = FUN_01012b08(iVar1,param_3,param_4), iVar2 == 0)) {
        if (*(char *)(iVar1 + param_4) == '\0') {
          return param_2;
        }
        iVar2 = FUN_01012b40(param_3,0x40,param_4);
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



/* @ 0x1007cb4 */

int FUN_01007cb4(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int local_14 [3];
  
  iVar2 = FUN_010079e4();
  if (iVar2 < 0) {
    return iVar2;
  }
  while( true ) {
    iVar1 = FUN_010078d4(param_1,iVar2,local_14);
    if (iVar1 == 3) {
      return iVar2;
    }
    if (iVar1 == 9) break;
    iVar2 = local_14[0];
    if (iVar1 != 4) {
      return -1;
    }
  }
  if (local_14[0] < 0) {
    return local_14[0];
  }
  return -0xb;
}



/* @ 0x1007cec */

int FUN_01007cec(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int aiStack_14 [3];
  
  iVar2 = FUN_01007a24();
  if (iVar2 < 0) {
    return iVar2;
  }
  while( true ) {
    iVar1 = FUN_010078d4(param_1,iVar2,aiStack_14);
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



/* @ 0x1007d24 */

int FUN_01007d24(int param_1,int param_2,uint *param_3)

{
  uint uVar1;
  
  uVar1 = FUN_01007a24();
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



/* @ 0x1007d94 */

int FUN_01007d94(int param_1,undefined4 param_2,undefined4 param_3,int param_4,int *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  iVar1 = FUN_01007cb4();
  do {
    if (iVar1 < 0) {
LAB_01007e54:
      if (param_5 != (int *)0x0) {
        *param_5 = iVar1;
      }
      return 0;
    }
    iVar2 = FUN_01007d24(param_1,iVar1,param_5);
    if (iVar2 == 0) {
      iVar1 = -0xd;
      goto LAB_01007e54;
    }
    uVar5 = *(uint *)(iVar2 + 8);
    uVar4 = *(uint *)(param_1 + 0xc);
    iVar6 = param_1 + (uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 |
                      uVar5 >> 0x18) +
                      (uVar4 << 0x18 | (uVar4 >> 8 & 0xff) << 0x10 | (uVar4 >> 0x10 & 0xff) << 8 |
                      uVar4 >> 0x18);
    iVar3 = FUN_01012980(iVar6);
    if ((param_4 == iVar3) && (iVar3 = FUN_01012b08(iVar6,param_3,param_4), iVar3 == 0)) {
      return iVar2;
    }
    iVar1 = FUN_01007cec(param_1,iVar1);
  } while( true );
}



/* @ 0x1007e68 */

void FUN_01007e68(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  
  uVar1 = FUN_01012980(param_3);
  FUN_01007d94(param_1,param_2,param_3,uVar1,param_4);
  return;
}



/* @ 0x1007ebc */

int FUN_01007ebc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_01012980(param_3);
  iVar2 = FUN_01007d94(param_1,param_2,param_3,uVar1,param_4);
  if (iVar2 != 0) {
    iVar2 = iVar2 + 0xc;
  }
  return iVar2;
}



/* @ 0x1007f18 */

int FUN_01007f18(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  
  iVar1 = FUN_01007f88(param_1,s___ref_initialized____1__0103232c + 0x14);
  if ((-1 < iVar1) && (iVar1 = FUN_01007d94(param_1,iVar1,param_2,param_3,0), iVar1 != 0)) {
    return iVar1 + 0xc;
  }
  return 0;
}



/* @ 0x1007f88 */

int FUN_01007f88(undefined4 param_1,char *param_2)

{
  int iVar1;
  int iVar2;
  char cVar3;
  char *pcVar4;
  char *pcVar5;
  
  iVar1 = FUN_01012980(param_2);
  iVar2 = FUN_01007804(param_1);
  if (iVar2 == 0) {
    cVar3 = *param_2;
    pcVar4 = param_2;
    if (cVar3 != '/') {
      pcVar4 = (char *)FUN_010129ac(param_2,0x2f);
      if (pcVar4 == (char *)0x0) {
        pcVar4 = param_2 + iVar1;
      }
      iVar2 = FUN_01007f18(param_1,param_2,(int)pcVar4 - (int)param_2);
      if (iVar2 == 0) {
        return -5;
      }
      iVar2 = FUN_01007f88(param_1);
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
        pcVar4 = (char *)FUN_010129ac(pcVar5,0x2f);
        if (pcVar4 == (char *)0x0) {
          pcVar4 = param_2 + iVar1;
        }
        iVar2 = FUN_01007bc0(param_1,iVar2,pcVar5,(int)pcVar4 - (int)pcVar5);
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



/* @ 0x100809c */

int FUN_0100809c(undefined4 param_1,undefined4 param_2,int param_3,uint *param_4,undefined4 param_5,
                undefined4 param_6,undefined4 param_7,undefined4 param_8)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  undefined1 auStack_48 [16];
  undefined1 auStack_38 [16];
  undefined1 auStack_28 [16];
  
  iVar1 = FUN_0100aae8(auStack_48,auStack_38,auStack_28,0);
  if (iVar1 == 0) {
    iVar1 = FUN_0100b89c(auStack_48,param_1,param_2);
    if (((iVar1 == 0) && (iVar1 = FUN_0100b89c(auStack_38,param_5,param_6), iVar1 == 0)) &&
       (iVar1 = FUN_0100b89c(auStack_28,param_7,param_8), iVar1 == 0)) {
      iVar1 = FUN_01009fa8(auStack_28,auStack_48);
      if (iVar1 == -1) {
        iVar1 = 0x16;
      }
      else {
        iVar1 = FUN_0100a6ac(auStack_48,auStack_38,auStack_28,auStack_48);
        if (iVar1 == 0) {
          uVar2 = FUN_0100bb84(auStack_28);
          if (*param_4 < uVar2) {
            iVar1 = 6;
            *param_4 = uVar2;
          }
          else {
            iVar1 = FUN_0100bb84(auStack_48);
            iVar3 = FUN_0100bb84(auStack_28);
            if (iVar3 < iVar1) {
              iVar1 = 1;
            }
            else {
              *param_4 = uVar2;
              FUN_010081fc(param_3,uVar2);
              iVar1 = FUN_0100bb84(auStack_48);
              iVar1 = FUN_0100baec(auStack_48,param_3 + (uVar2 - iVar1));
            }
          }
        }
      }
    }
    FUN_01009f54(auStack_48,auStack_38,auStack_28,0);
  }
  return iVar1;
}



/* @ 0x10081fc */

void FUN_010081fc(undefined1 *param_1,int param_2)

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



/* @ 0x100821c */

undefined4 FUN_0100821c(int param_1,byte *param_2)

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
  piVar4 = (int *)(param_1 + 8);
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
  piVar4 = (int *)(param_1 + 8);
  while( true ) {
    puVar2 = puVar2 + 1;
    *piVar4 = uVar7 + *piVar4;
    if ((int *)(param_1 + 0x28) == piVar4 + 1) break;
    uVar7 = *puVar2;
    piVar4 = piVar4 + 1;
  }
  return 0;
}



/* @ 0x1009a58 */

undefined4 FUN_01009a58(undefined8 *param_1)

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



/* @ 0x1009ae4 */

undefined4 FUN_01009ae4(uint *param_1,undefined1 *param_2)

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
    if (uVar1 == 0x38) goto LAB_01009bac;
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
    FUN_0100821c(param_1,param_1 + 0xb);
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
LAB_01009bac:
  *(char *)(param_1 + 0x19) = (char)(uVar6 >> 0x18);
  *(char *)((int)param_1 + 0x65) = (char)(uVar6 >> 0x10);
  *(char *)((int)param_1 + 0x66) = (char)(uVar6 >> 8);
  *(char *)((int)param_1 + 0x67) = (char)uVar6;
  param_1[0x1a] =
       uVar5 << 0x18 | (uVar5 >> 8 & 0xff) << 0x10 | (uVar5 >> 0x10 & 0xff) << 8 | uVar5 >> 0x18;
  FUN_0100821c(param_1,param_1 + 0xb);
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



/* @ 0x1009c44 */

int FUN_01009c44(uint *param_1,int param_2,uint param_3)

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
        iVar1 = FUN_0100821c(param_1);
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
        FUN_010129dc((int)(param_1 + 0xb) + uVar2,param_2,uVar3);
        uVar2 = param_1[10];
        param_3 = param_3 - uVar3;
        param_1[10] = uVar3 + uVar2;
        param_2 = iVar1;
        if (uVar3 + uVar2 == 0x40) {
          iVar1 = FUN_0100821c(param_1,param_1 + 0xb);
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



/* @ 0x1009d58 */

void FUN_01009d58(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  
  FUN_0100bbb4();
  iVar2 = param_2 / 0x1c + 1;
  iVar1 = FUN_0100a9bc(param_1,iVar2,param_2 * -0x6db6db6d);
  if (iVar1 == 0) {
    *param_1 = iVar2;
    *(int *)(param_1[3] + (param_2 / 0x1c) * 4) = 1 << (param_2 % 0x1c & 0xffU);
  }
  return;
}



/* @ 0x1009dd4 */

int FUN_01009dd4(int param_1,int param_2)

{
  int iVar1;
  
  if ((param_2 == param_1) || (iVar1 = FUN_0100a0c0(), iVar1 == 0)) {
    iVar1 = 0;
    *(undefined4 *)(param_2 + 8) = 0;
  }
  return iVar1;
}



/* @ 0x1009e0c */

int FUN_01009e0c(int *param_1,int *param_2,int *param_3)

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
    iVar1 = FUN_0100a034();
    if (iVar1 == -1) {
      param_3[2] = iVar8;
      param_1 = param_2;
      param_2 = param_1;
    }
    else {
      param_3[2] = iVar7;
    }
    iVar7 = FUN_0100c048(param_1,param_2,param_3);
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
  if ((iVar8 < param_3[1]) || (iVar3 = FUN_0100a9bc(param_3,iVar1), iVar3 == 0)) {
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
    FUN_01009e94(param_3);
    iVar3 = 0;
  }
  return iVar3;
}



/* @ 0x1009e94 */

void FUN_01009e94(int *param_1)

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



/* @ 0x1009ef0 */

void FUN_01009ef0(int *param_1)

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
  FUN_0100d368(puVar4);
  *param_1 = 0;
  param_1[1] = 0;
  param_1[2] = 0;
  param_1[3] = 0;
  return;
}



/* @ 0x1009f54 */

void FUN_01009f54(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int local_c [3];
  
  piVar1 = local_c;
  local_c[2] = param_4;
  local_c[1] = param_3;
  local_c[0] = param_2;
  while (param_1 != 0) {
    FUN_01009ef0();
    param_1 = *piVar1;
    piVar1 = piVar1 + 1;
  }
  return;
}



/* @ 0x1009fa8 */

undefined4 FUN_01009fa8(int param_1,int param_2)

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
    uVar1 = FUN_0100a034(param_2,param_1);
    return uVar1;
  }
  uVar1 = FUN_0100a034();
  return uVar1;
}



/* @ 0x1009ff0 */

undefined4 FUN_01009ff0(int *param_1,uint param_2)

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



/* @ 0x100a034 */

undefined4 FUN_0100a034(int *param_1,int *param_2)

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



/* @ 0x100a0c0 */

int FUN_0100a0c0(int *param_1,int *param_2)

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
    iVar1 = FUN_0100a9bc(param_2);
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



/* @ 0x100a184 */

int FUN_0100a184(int *param_1)

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



/* @ 0x100a1c8 */

int FUN_0100a1c8(int *param_1,int *param_2)

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
    iVar4 = FUN_0100a9bc(param_2);
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
  FUN_01009e94(param_2);
  return 0;
}



/* @ 0x100a2a8 */

int FUN_0100a2a8(undefined4 param_1,int param_2,int *param_3,int param_4)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  uint uVar5;
  uint uVar6;
  undefined1 auStack_28 [16];
  
  if (param_2 < 1) {
    iVar2 = FUN_0100a0c0(param_1,param_3);
    if (param_4 != 0) {
      FUN_0100bbb4(param_4);
    }
  }
  else {
    iVar2 = FUN_0100aa44(auStack_28);
    if (iVar2 == 0) {
      if (((param_4 == 0) || (iVar2 = FUN_0100b128(param_1,param_2,auStack_28), iVar2 == 0)) &&
         (iVar2 = FUN_0100a0c0(param_1,param_3), iVar2 == 0)) {
        if (0x1b < param_2) {
          FUN_0100b964(param_3,param_2 / 0x1c);
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
        FUN_01009e94(param_3);
        if (param_4 != 0) {
          FUN_0100a66c(auStack_28,param_4);
        }
      }
      FUN_01009ef0(auStack_28);
    }
  }
  return iVar2;
}



/* @ 0x100a40c */

int FUN_0100a40c(int param_1,int *param_2,int *param_3,int *param_4)

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
    iVar1 = FUN_0100a034();
    if (iVar1 == -1) {
      if (param_4 == (int *)0x0) {
        iVar1 = 0;
      }
      else {
        iVar1 = FUN_0100a0c0(param_1,param_4);
      }
      if (param_3 != (int *)0x0) {
        FUN_0100bbb4(param_3);
      }
    }
    else {
      iVar1 = FUN_0100aae8(auStack_60,auStack_50,auStack_40,auStack_30,0);
      if (iVar1 == 0) {
        FUN_0100b9f0(auStack_40,1);
        iVar2 = FUN_0100a184(param_1);
        iVar3 = FUN_0100a184(param_2);
        iVar1 = FUN_01009dd4(param_1,auStack_60);
        if ((iVar1 == 0) && (iVar1 = FUN_01009dd4(param_2,auStack_50), iVar1 == 0)) {
          iVar2 = iVar2 - iVar3;
          iVar1 = FUN_0100b688(auStack_50,iVar2);
          if ((iVar1 == 0) && (iVar3 = FUN_0100b688(auStack_40,iVar2), iVar1 = iVar3, iVar3 == 0)) {
            while( true ) {
              bVar5 = iVar2 < 0;
              iVar2 = iVar2 + -1;
              if (bVar5) break;
              iVar1 = FUN_01009fa8(auStack_50,auStack_60);
              if (iVar1 != 1) {
                iVar1 = FUN_0100ba54(auStack_60,auStack_50);
                if ((iVar1 != 0) || (iVar1 = FUN_01009e0c(auStack_30,auStack_40), iVar1 != 0))
                goto LAB_0100a4cc;
              }
              iVar1 = FUN_0100a2a8(auStack_50,1,auStack_50,0);
              if ((iVar1 != 0) || (iVar1 = FUN_0100a2a8(auStack_40,1,auStack_40,0), iVar1 != 0))
              goto LAB_0100a4cc;
            }
            iVar2 = *(int *)(param_1 + 8);
            if (param_3 != (int *)0x0) {
              iVar4 = param_2[2];
              FUN_0100a66c(param_3,auStack_30);
              iVar1 = 0;
              if ((*param_3 != 0) && (iVar1 = 0, iVar4 != iVar2)) {
                iVar1 = 1;
              }
              param_3[2] = iVar1;
            }
            iVar1 = iVar3;
            if (param_4 != (int *)0x0) {
              FUN_0100a66c(param_4,auStack_60);
              if (*param_4 == 0) {
                iVar2 = 0;
              }
              param_4[2] = iVar2;
            }
          }
        }
LAB_0100a4cc:
        FUN_01009f54(auStack_60,auStack_50,auStack_40,auStack_30,0);
      }
      else {
        iVar1 = 1;
      }
    }
  }
  return iVar1;
}



/* @ 0x100a66c */

void FUN_0100a66c(undefined4 *param_1,undefined4 *param_2)

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



/* @ 0x100a6ac */

int FUN_0100a6ac(undefined4 param_1,int param_2,int *param_3,undefined4 param_4)

{
  int iVar1;
  undefined1 auStack_38 [16];
  undefined1 auStack_28 [16];
  
  if (param_3[2] != 1) {
    if (*(int *)(param_2 + 8) == 1) {
      iVar1 = FUN_0100aa44(auStack_38);
      if (iVar1 != 0) {
        return iVar1;
      }
      iVar1 = FUN_0100ac18(param_1,param_3,auStack_38);
      if ((iVar1 == 0) && (iVar1 = FUN_0100aa44(auStack_28), iVar1 == 0)) {
        iVar1 = FUN_01009dd4(param_2,auStack_28);
        if (iVar1 == 0) {
          iVar1 = FUN_0100a6ac(auStack_38,auStack_28,param_3,param_4);
        }
        FUN_01009f54(auStack_38,auStack_28,0);
        return iVar1;
      }
      FUN_01009ef0(auStack_38);
      return iVar1;
    }
    if ((0 < *param_3) && ((*(uint *)param_3[3] & 1) != 0)) {
      iVar1 = FUN_0100a7c0();
      return iVar1;
    }
  }
  return -3;
}



/* @ 0x100a7c0 */

int FUN_0100a7c0(undefined4 param_1,int *param_2,undefined4 param_3,undefined4 param_4,int param_5)

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
  
  FUN_0100a184(param_2);
  iVar1 = FUN_0100aa44(auStack_38);
  if (iVar1 == 0) {
    iVar1 = FUN_0100aa44(auStack_38);
    if (iVar1 == 0) {
      if (param_5 == 0) {
        iVar1 = FUN_0100b534(param_3,&local_5c);
        if ((iVar1 == 0) && (iVar1 = FUN_0100aa44(auStack_58), iVar1 == 0)) {
          iVar1 = FUN_0100b2bc(auStack_58,param_3);
          if ((iVar1 == 0) &&
             ((iVar1 = FUN_0100b814(param_1,auStack_58,param_3,auStack_38), iVar1 == 0 &&
              (iVar1 = FUN_0100a0c0(auStack_38), iVar1 == 0)))) {
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
                    iVar1 = FUN_0100ba30(auStack_58);
                    if ((iVar1 != 0) ||
                       (iVar1 = FUN_0100b3b0(auStack_58,param_3,local_5c), iVar1 != 0))
                    goto LAB_0100a9a0;
                    iVar1 = FUN_0100b7b4(auStack_58,auStack_48 + uVar6 * 0x10);
                  }
                  else {
                    iVar1 = FUN_0100ba30(auStack_58);
                  }
                  if ((iVar1 != 0) ||
                     (iVar1 = FUN_0100b3b0(auStack_58,param_3,local_5c), iVar1 != 0))
                  goto LAB_0100a9a0;
                }
                iVar5 = iVar5 + -1;
                uVar7 = uVar7 << 1;
                uVar4 = uVar3;
              } while (iVar5 != 0);
            }
            iVar1 = FUN_0100b3b0(auStack_58,param_3,local_5c);
            if (iVar1 == 0) {
              FUN_0100a66c(auStack_58,param_4);
            }
          }
LAB_0100a9a0:
          FUN_01009ef0(auStack_58);
        }
      }
      else {
        iVar1 = -3;
      }
      FUN_01009ef0(auStack_38);
    }
    FUN_01009ef0(auStack_38);
  }
  return iVar1;
}



/* @ 0x100a9bc */

undefined4 FUN_0100a9bc(int param_1,int param_2)

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
  iVar1 = FUN_0100d328(*(undefined4 *)(param_1 + 0xc),param_2 * 4);
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



/* @ 0x100aa44 */

undefined4 FUN_0100aa44(undefined4 *param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_0100d2f8(4);
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



/* @ 0x100aa9c */

void FUN_0100aa9c(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = FUN_0100aa44();
  if (iVar1 != 0) {
    return;
  }
  FUN_0100a0c0(param_2,param_1);
  return;
}



/* @ 0x100aae8 */

undefined4 FUN_0100aae8(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

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
      iVar1 = FUN_0100aa44(iVar1);
      if (iVar1 != 0) {
        iVar1 = iVar2 + -1;
        if (iVar2 != 0) {
          piVar3 = &local_c;
          do {
            iVar1 = iVar1 + -1;
            FUN_01009ef0(param_1);
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



/* @ 0x100aba4 */

undefined4 FUN_0100aba4(undefined4 *param_1,int param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *extraout_r3;
  undefined4 *puVar3;
  int iVar4;
  
  iVar4 = param_2 + 2;
  puVar1 = (undefined4 *)FUN_0100d2f8(iVar4 * 4);
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



/* @ 0x100ac18 */

int FUN_0100ac18(undefined4 param_1,int *param_2,undefined4 param_3)

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
  iVar1 = FUN_0100aae8(local_98,local_88,local_78,local_68,local_58,local_48,local_38,local_28,0);
  if (iVar1 != 0) {
    return iVar1;
  }
  iVar1 = FUN_0100b21c(param_1,param_2,local_98);
  if ((iVar1 == 0) && (iVar1 = FUN_0100a0c0(param_2,local_88), iVar1 == 0)) {
    if ((local_98[0] < 1) ||
       ((((*local_8c & 1) != 0 || (local_88[0] < 1)) || ((*local_7c & 1) != 0)))) {
      iVar1 = FUN_0100a0c0(local_98,local_78);
      if ((iVar1 == 0) && (iVar1 = FUN_0100a0c0(local_88,local_68), iVar1 == 0)) {
        FUN_0100b9f0(local_58,1);
        FUN_0100b9f0(local_28,1);
        do {
          while ((0 < local_78[0] && ((*local_6c & 1) == 0))) {
            iVar1 = FUN_0100a1c8(local_78);
            if (((iVar1 != 0) ||
                ((((0 < local_58[0] && ((*local_4c & 1) != 0)) ||
                  ((0 < local_48[0] && ((*local_3c & 1) != 0)))) &&
                 ((iVar1 = FUN_01009e0c(local_58,local_88), iVar1 != 0 ||
                  (iVar1 = FUN_0100ba54(local_48,local_98), iVar1 != 0)))))) ||
               ((iVar1 = FUN_0100a1c8(local_58), iVar1 != 0 ||
                (iVar1 = FUN_0100a1c8(local_48), iVar1 != 0)))) goto LAB_0100ad34;
          }
          while ((0 < local_68[0] && ((*local_5c & 1) == 0))) {
            iVar1 = FUN_0100a1c8(local_68);
            if ((iVar1 != 0) ||
               (((((0 < local_38[0] && ((*local_2c & 1) != 0)) ||
                  ((0 < local_28[0] && ((*local_1c & 1) != 0)))) &&
                 ((iVar1 = FUN_01009e0c(local_38,local_88), iVar1 != 0 ||
                  (iVar1 = FUN_0100ba54(local_28,local_98), iVar1 != 0)))) ||
                ((iVar1 = FUN_0100a1c8(local_38), iVar1 != 0 ||
                 (iVar1 = FUN_0100a1c8(local_28), iVar1 != 0)))))) goto LAB_0100ad34;
          }
          iVar1 = FUN_01009fa8(local_78);
          if (iVar1 == -1) {
            iVar1 = FUN_0100ba54(local_68,local_78);
            if ((iVar1 != 0) || (iVar1 = FUN_0100ba54(local_38,local_58), iVar1 != 0)) break;
            iVar1 = FUN_0100ba54(local_28,local_48);
          }
          else {
            iVar1 = FUN_0100ba54(local_78,local_68);
            if ((iVar1 != 0) || (iVar1 = FUN_0100ba54(local_58,local_38), iVar1 != 0)) break;
            iVar1 = FUN_0100ba54(local_48,local_28);
          }
          if (iVar1 != 0) break;
          if (local_78[0] == 0) {
            iVar1 = FUN_01009ff0(local_68,1);
            if (iVar1 != 0) goto LAB_0100ad30;
            goto LAB_0100afd8;
          }
        } while( true );
      }
    }
    else {
LAB_0100ad30:
      iVar1 = -3;
    }
  }
  goto LAB_0100ad34;
  while (iVar1 = FUN_01009e0c(local_38,param_2), iVar1 == 0) {
LAB_0100afd8:
    iVar1 = FUN_01009ff0(local_38,0);
    if (iVar1 != -1) goto LAB_0100b008;
  }
  goto LAB_0100ad34;
  while (iVar1 = FUN_0100ba54(local_38,param_2), iVar1 == 0) {
LAB_0100b008:
    iVar1 = FUN_0100a034(local_38,param_2);
    if (iVar1 == -1) {
      iVar1 = 0;
      FUN_0100a66c(local_38,param_3);
      break;
    }
  }
LAB_0100ad34:
  FUN_01009f54(local_98,local_88,local_78,local_68,local_58,local_48,local_38,local_28,0);
  return iVar1;
}



/* @ 0x100b084 */

int FUN_0100b084(int *param_1,int param_2)

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
      unaff_lr = 0x100b114;
      iVar1 = FUN_0100a9bc();
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



/* @ 0x100b128 */

int FUN_0100b128(int *param_1,uint param_2,int *param_3)

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
    FUN_0100bbb4(param_3);
  }
  else {
    if (*param_1 * 0x1c <= (int)param_2) {
      iVar2 = FUN_0100a0c0(param_1,param_3);
      return iVar2;
    }
    iVar2 = FUN_0100a0c0(param_1,param_3);
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
      FUN_01009e94(param_3);
    }
  }
  return iVar2;
}



/* @ 0x100b21c */

int FUN_0100b21c(undefined4 param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  undefined1 auStack_28 [8];
  int local_20;
  
  iVar1 = FUN_0100aa44(auStack_28);
  if (iVar1 == 0) {
    iVar1 = FUN_0100a40c(param_1,param_2,0,auStack_28);
    if (iVar1 == 0) {
      if (local_20 == *(int *)(param_2 + 8)) {
        FUN_0100a66c(auStack_28,param_3);
      }
      else {
        iVar1 = FUN_01009e0c(param_2,auStack_28,param_3);
      }
    }
    FUN_01009ef0(auStack_28);
  }
  return iVar1;
}



/* @ 0x100b2bc */

int FUN_0100b2bc(undefined4 param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_0100a184(param_2);
  if (*param_2 < 2) {
    iVar1 = 0;
    FUN_0100b9f0(param_1,1);
  }
  else {
    iVar2 = FUN_01009d58(param_1,iVar1 % 0x1c + (*param_2 + -1) * 0x1c + -1);
    if (iVar2 != 0) {
      return iVar2;
    }
    iVar1 = iVar1 % 0x1c + -1;
  }
  while( true ) {
    iVar2 = FUN_0100b590(param_1,param_1);
    iVar1 = iVar1 + 1;
    if (iVar2 != 0) {
      return iVar2;
    }
    iVar2 = FUN_0100a034(param_1,param_2);
    if ((iVar2 != -1) && (iVar2 = FUN_0100c048(param_1,param_2,param_1), iVar2 != 0)) break;
    if (iVar1 == 0x1c) {
      return 0;
    }
  }
  return iVar2;
}



/* @ 0x100b3b0 */

int FUN_0100b3b0(int *param_1,int *param_2,int param_3)

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
  if ((iVar4 <= param_1[1]) || (iVar11 = FUN_0100a9bc(param_1,iVar4), iVar11 == 0)) {
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
    FUN_01009e94(param_1);
    FUN_0100b964(param_1,*param_2);
    iVar4 = FUN_0100a034(param_1,param_2);
    if (iVar4 != -1) {
      iVar4 = FUN_0100c048(param_1,param_2);
      return iVar4;
    }
    iVar11 = 0;
  }
  return iVar11;
}



/* @ 0x100b534 */

undefined4 FUN_0100b534(int param_1,uint *param_2)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = **(uint **)(param_1 + 0xc);
  if ((uVar2 & 1) != 0) {
    iVar1 = ((uVar2 + 2) * 2 & 8) + uVar2;
    iVar1 = iVar1 * (2 - iVar1 * uVar2);
    iVar1 = iVar1 * (2 - iVar1 * uVar2);
    *param_2 = (iVar1 * uVar2 + -2) * iVar1 & 0xfffffff;
    return 0;
  }
  return 0xfffffffd;
}



/* @ 0x100b590 */

int FUN_0100b590(int *param_1,int *param_2)

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
    iVar2 = FUN_0100a9bc(param_2,iVar2 + 1);
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



/* @ 0x100b688 */

int FUN_0100b688(int *param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint *puVar7;
  uint *puVar8;
  
  if ((param_3 == param_1) || (iVar1 = FUN_0100a0c0(param_1,param_3), iVar1 == 0)) {
    iVar1 = param_2 / 0x1c + *param_3;
    if (((iVar1 < param_3[1]) ||
        (iVar1 = FUN_0100a9bc(param_3,iVar1 + 1,param_3[1],param_2 * -0x6db6db6d), iVar1 == 0)) &&
       ((param_2 < 0x1c || (iVar1 = FUN_0100b084(param_3,param_2 / 0x1c), iVar1 == 0)))) {
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
      FUN_01009e94(param_3);
      iVar1 = 0;
    }
  }
  return iVar1;
}



/* @ 0x100b7b4 */

void FUN_0100b7b4(int param_1,int param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = *(int *)(param_1 + 8);
  iVar3 = *(int *)(param_2 + 8);
  FUN_0100bd94();
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



/* @ 0x100b814 */

int FUN_0100b814(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined1 auStack_28 [16];
  
  iVar1 = FUN_0100aa44(auStack_28);
  if (iVar1 == 0) {
    iVar1 = FUN_0100b7b4(param_1,param_2,auStack_28);
    if (iVar1 == 0) {
      iVar1 = FUN_0100b21c(auStack_28,param_3,param_4);
    }
    FUN_01009ef0(auStack_28);
  }
  return iVar1;
}



/* @ 0x100b89c */

int FUN_0100b89c(int *param_1,byte *param_2,int param_3)

{
  int iVar1;
  byte *pbVar2;
  byte *pbVar3;
  
  if ((param_1[1] < 2) && (iVar1 = FUN_0100a9bc(param_1,2), iVar1 != 0)) {
    return iVar1;
  }
  FUN_0100bbb4(param_1);
  if (0 < param_3) {
    pbVar3 = param_2;
    do {
      iVar1 = FUN_0100b688(param_1,8,param_1);
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
  FUN_01009e94(param_1);
  return 0;
}



/* @ 0x100b964 */

void FUN_0100b964(int *param_1,int param_2)

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
    FUN_0100bbb4();
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



/* @ 0x100b9f0 */

void FUN_0100b9f0(uint *param_1,uint param_2)

{
  FUN_0100bbb4();
  *(uint *)param_1[3] = param_2 & 0xfffffff;
  *param_1 = (uint)((param_2 & 0xfffffff) != 0);
  return;
}



/* @ 0x100ba30 */

void FUN_0100ba30(undefined4 param_1,int param_2)

{
  FUN_0100bee0();
  *(undefined4 *)(param_2 + 8) = 0;
  return;
}



/* @ 0x100ba54 */

int FUN_0100ba54(int *param_1,int *param_2,int *param_3)

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
    if ((iVar8 < param_3[1]) || (iVar3 = FUN_0100a9bc(param_3,iVar7), iVar3 == 0)) {
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
      FUN_01009e94(param_3);
      iVar3 = 0;
    }
    return iVar3;
  }
  iVar8 = FUN_0100a034();
  if (iVar8 == -1) {
    param_3[2] = (uint)(iVar6 == 0);
    piVar1 = param_2;
  }
  else {
    param_3[2] = iVar6;
    piVar1 = param_1;
    param_1 = param_2;
  }
  iVar6 = FUN_0100c048(piVar1,param_1,param_3);
  return iVar6;
}



/* @ 0x100baec */

int FUN_0100baec(undefined4 param_1,int param_2)

{
  int iVar1;
  undefined1 *puVar2;
  int local_28 [3];
  undefined4 *local_1c;
  
  iVar1 = FUN_0100aa9c(local_28,param_1);
  if (iVar1 == 0) {
    puVar2 = (undefined1 *)(param_2 + -1);
    do {
      if (local_28[0] == 0) {
        FUN_0100bbe8(param_2,puVar2 + (1 - param_2));
        iVar1 = 0;
        break;
      }
      puVar2 = puVar2 + 1;
      *puVar2 = (char)*local_1c;
      iVar1 = FUN_0100a2a8(local_28,8,local_28);
    } while (iVar1 == 0);
    FUN_01009ef0(local_28);
  }
  return iVar1;
}



/* @ 0x100bb84 */

int FUN_0100bb84(void)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = FUN_0100a184();
  uVar2 = uVar1;
  if ((int)uVar1 < 0) {
    uVar2 = uVar1 + 7;
  }
  return (uint)((uVar1 & 7) != 0) + ((int)uVar2 >> 3);
}



/* @ 0x100bbb4 */

void FUN_0100bbb4(undefined4 *param_1)

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



/* @ 0x100bbe8 */

void FUN_0100bbe8(int param_1,int param_2)

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



/* @ 0x100bd94 */

int FUN_0100bd94(int *param_1,int *param_2,undefined4 param_3,int param_4)

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
  
  iVar2 = FUN_0100aba4(local_38,param_4);
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
    FUN_01009e94(local_38);
    FUN_0100a66c(local_38,param_3);
    FUN_01009ef0(local_38);
  }
  return iVar2;
}



/* @ 0x100bee0 */

int FUN_0100bee0(int *param_1,undefined4 param_2)

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
  iVar2 = FUN_0100aba4(local_38,iVar8);
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
    FUN_01009e94(local_38);
    FUN_0100a66c(local_38,param_2);
    FUN_01009ef0(local_38);
  }
  return iVar2;
}



/* @ 0x100c048 */

int FUN_0100c048(uint *param_1,uint *param_2,uint *param_3)

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
    uVar12 = FUN_0100a9bc(param_3,puVar5);
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
  FUN_01009e94(param_3);
  return 0;
}



/* @ 0x100c174 */

undefined4
FUN_0100c174(int param_1,int param_2,int param_3,int param_4,uint param_5,char param_6,
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
    FUN_0100ceb0(&DAT_010323b0,s__aliases_01032344 + 8,param_6);
    FUN_01010c00(&DAT_010323b0,s__aliases_01032344 + 8,param_6);
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s__aliases_01032344 + 8,0x4d);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s__aliases_01032344 + 8,0x4d);
    FUN_01000458(0);
  }
  return 0;
}



/* @ 0x100c280 */

void FUN_0100c280(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c174(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,1,param_4);
  return;
}



/* @ 0x100c2b8 */

void FUN_0100c2b8(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c174(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,2,param_4);
  return;
}



/* @ 0x100c2f0 */

void FUN_0100c2f0(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c174(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,4,param_4);
  return;
}



/* @ 0x100c328 */

undefined4
FUN_0100c328(int param_1,int param_2,int param_3,int param_4,uint param_5,char param_6,
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
    FUN_0100ceb0(&DAT_010323b0,s_al_pcie_read_config_01032350 + 0x10,param_6);
    FUN_01010c00(&DAT_010323b0,s_al_pcie_read_config_01032350 + 0x10,param_6);
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_read_config_01032350 + 0x10,0x85);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_read_config_01032350 + 0x10,0x85);
    FUN_01000458(0);
  }
  return 0;
}



/* @ 0x100c430 */

void FUN_0100c430(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_0100c328(*(undefined4 *)(&DAT_010323f8 + param_1 * 4),(param_2 & 0xffffff) >> 0x10,
               (param_2 & 0xffff) >> 0xb,(param_2 & 0x7ff) >> 8,param_3,4,param_4);
  return;
}



/* @ 0x100c718 */

int FUN_0100c718(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint *puVar3;
  uint local_24 [2];
  
  uVar2 = *(undefined4 *)(&DAT_010323f8 + param_1 * 4);
  puVar3 = local_24;
  iVar1 = FUN_0100c174();
  if (iVar1 != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_memory_enable_0103237c + 0x14,0xbf
                 ,puVar3);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_memory_enable_0103237c + 0x14,0xbf
                );
    FUN_01000458(0);
  }
  local_24[0] = local_24[0] | 4;
  iVar1 = FUN_0100c328(uVar2,(param_2 & 0xffffff) >> 0x10,(param_2 & 0xffff) >> 0xb,
                       (param_2 & 0x7ff) >> 8,4,2,local_24[0]);
  if (iVar1 != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_memory_enable_0103237c + 0x14,0xc2
                );
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_memory_enable_0103237c + 0x14,0xc2
                );
    FUN_01000458(0);
  }
  return iVar1;
}



/* @ 0x100c734 */

int FUN_0100c734(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint *puVar3;
  uint local_24 [2];
  
  uVar2 = *(undefined4 *)(&DAT_010323f8 + param_1 * 4);
  puVar3 = local_24;
  iVar1 = FUN_0100c174();
  if (iVar1 != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_write_config_01032364 + 0x14,0xb1,
                 puVar3);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_write_config_01032364 + 0x14,0xb1)
    ;
    FUN_01000458(0);
  }
  local_24[0] = local_24[0] | 2;
  iVar1 = FUN_0100c328(uVar2,(param_2 & 0xffffff) >> 0x10,(param_2 & 0xffff) >> 0xb,
                       (param_2 & 0x7ff) >> 8,4,2,local_24[0]);
  if (iVar1 != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_write_config_01032364 + 0x14,0xb4)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_pcie_master_enable_01032394 + 0x14,1,
                 s__s__illegal_size__u__010323b4 + 0x14,s_al_pcie_write_config_01032364 + 0x14,0xb4)
    ;
    FUN_01000458(0);
  }
  return iVar1;
}



/* @ 0x100c750 */

byte FUN_0100c750(undefined4 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  byte bVar2;
  byte local_1c;
  byte local_1b;
  ushort local_1a [3];
  
  local_1b = 0x34;
  FUN_0100c2b8(param_1,param_2,6,local_1a);
  if ((local_1a[0] & 0x10) != 0) {
    iVar1 = 0x100;
    do {
      FUN_0100c280(param_1,param_2,local_1b,&local_1b);
      bVar2 = local_1b & 0xfc;
      if (local_1b < 0x40) {
        return 0;
      }
      local_1b = bVar2;
      FUN_0100c280(param_1,param_2,bVar2,&local_1c);
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



/* @ 0x100c81c */

undefined4 FUN_0100c81c(int param_1,uint param_2,int param_3,int *param_4)

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
        uVar2 = (uint)(uVar5 < 10);
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



/* @ 0x100ca0c */

void FUN_0100ca0c(void)

{
  return;
}



/* @ 0x100ca10 */

undefined4 FUN_0100ca10(int param_1,int param_2)

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
    iVar4 = DAT_0100cb74;
    do {
      FUN_0100ca0c(0,iVar3);
      iVar1 = FUN_01003b24(0,iVar3);
      if ((iVar1 == 0) && (FUN_0100c2b8(0,iVar3,0,&local_20), (ushort)(local_20 - 1U) < 0xfffe)) {
        *(undefined4 *)(iVar4 + -0x14) = 1;
        *(int *)(iVar4 + -0x10) = iVar3 * 0x10 + -0x4400000;
        FUN_0100c2b8(0,iVar3,2,iVar4 + -4);
        FUN_0100c2b8(0,iVar3,8,iVar4 + -2);
        iVar1 = 0;
        do {
          iVar2 = FUN_0100c81c(0,iVar3,iVar1,&local_1c);
          if (iVar2 == 0) {
            *(undefined4 *)(iVar4 + iVar1 * 4) = local_1c;
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 != 6);
        *(undefined4 *)(iVar4 + -8) = 1;
        iVar1 = FUN_0100c750(0,iVar3,0x10);
        if (iVar1 != 0) {
          FUN_0100c2b8(0,iVar3,0x30e,iVar4 + -0xc);
          FUN_0100c2b8(0,iVar3,0x310,auStack_1e);
        }
      }
      iVar3 = iVar3 + 0x800;
      iVar4 = iVar4 + 0x30;
    } while (iVar3 != 0x5000);
  }
  return 0;
}





void FUN_0100cb78(void)

{
  FUN_0100ceb0(s___wd0_board_reset_01032410 + 0x10,&DAT_0103240c);
  FUN_01010c00(s___wd0_board_reset_01032410 + 0x10,&DAT_0103240c);
  FUN_01007590(10000);
  do {
/* @ 0x100cb78 -- WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x100cce0 */

void FUN_0100cce0(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(&DAT_010492e0 + param_1 * 4);
  if (iVar1 == 0) {
    return;
  }
  FUN_010281f0(&DAT_01049d20 + param_1 * 0x24,iVar1,*(int *)(&DAT_010492f0 + param_1 * 4) - iVar1,0,
               &DAT_0100cbf0);
  return;
}



/* @ 0x100cd3c */

void FUN_0100cd3c(int param_1,int param_2,int param_3)

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
  FUN_010281f0(puVar2,param_2,param_3 - param_2,1,&DAT_0100cbf0);
  if (iVar3 != 0) {
    FUN_01028414(puVar2,&local_44);
  }
  return;
}



/* @ 0x100cdf8 */

void FUN_0100cdf8(int param_1,undefined4 param_2)

{
  if (*(int *)(&DAT_010492e0 + param_1 * 4) != 0) {
    *(undefined4 *)(&DAT_01049d28 + param_1 * 0x24) = param_2;
    return;
  }
  return;
}



/* @ 0x100ce24 */

undefined4 FUN_0100ce24(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_01000088();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) == 0) {
    uVar2 = 0;
  }
  else {
    if (*(int *)(&DAT_01049d30 + iVar1 * 0x24) != 0) {
      uVar2 = FUN_01010be4(&LAB_0100cc90,0,param_1,param_2);
      return uVar2;
    }
    uVar2 = 0xffffffff;
  }
  return uVar2;
}



/* @ 0x100ceb0 */

void FUN_0100ceb0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  iVar1 = FUN_01000088();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) != 0) {
    FUN_0100ce24(param_1,&uStack_c);
  }
  return;
}



/* @ 0x100cef8 */

void FUN_0100cef8(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char *pcVar5;
  undefined4 uVar6;
  char *pcVar7;
  char local_74 [88];
  
  iVar1 = FUN_01000088();
  if (*(int *)(&DAT_010492e0 + iVar1 * 4) == 0) {
    return;
  }
  iVar1 = iVar1 * 0x24;
  uVar6 = *(undefined4 *)(&DAT_01049d28 + iVar1);
  *(undefined4 *)(&DAT_01049d28 + iVar1) = 0;
  FUN_0100ceb0(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
  FUN_01010c00(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
  FUN_0100ceb0(s__________________________________01044080 + 0x38);
  FUN_01010c00(s__________________________________01044080 + 0x38);
  FUN_0100ceb0(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
  FUN_01010c00(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
  FUN_0100ceb0(s___TRACE_DUMP__BEGIN_010440bc + 0x14,*(undefined4 *)(&DAT_01049d20 + iVar1));
  FUN_01010c00(s___TRACE_DUMP__BEGIN_010440bc + 0x14,*(undefined4 *)(&DAT_01049d20 + iVar1));
  FUN_0100ceb0(s___base____p_010440d4 + 0xc,*(undefined4 *)(&DAT_01049d24 + iVar1));
  FUN_01010c00(s___base____p_010440d4 + 0xc,*(undefined4 *)(&DAT_01049d24 + iVar1));
  FUN_0100ceb0(s___size____x_010440e4 + 0xc,uVar6);
  FUN_01010c00(s___size____x_010440e4 + 0xc,uVar6);
  FUN_0100ceb0(s___en____d_010440f4 + 8,*(undefined4 *)(&DAT_01049d30 + iVar1));
  FUN_01010c00(s___en____d_010440f4 + 8,*(undefined4 *)(&DAT_01049d30 + iVar1));
  FUN_0100ceb0(s___head____p_01044100 + 0xc,*(undefined4 *)(&DAT_01049d34 + iVar1));
  FUN_01010c00(s___head____p_01044100 + 0xc,*(undefined4 *)(&DAT_01049d34 + iVar1));
  FUN_0100ceb0(s___start____p_01044110 + 0xc,*(undefined4 *)(&DAT_01049d38 + iVar1));
  FUN_01010c00(s___start____p_01044110 + 0xc,*(undefined4 *)(&DAT_01049d38 + iVar1));
  FUN_0100ceb0(s___end____p_01044120 + 8,*(undefined4 *)(&DAT_01049d3c + iVar1));
  FUN_01010c00(s___end____p_01044120 + 8,*(undefined4 *)(&DAT_01049d3c + iVar1));
  FUN_0100ceb0(s___add_timestamp____d_0104412c + 0x14,*(undefined4 *)(&DAT_01049d40 + iVar1));
  FUN_01010c00(s___add_timestamp____d_0104412c + 0x14,*(undefined4 *)(&DAT_01049d40 + iVar1));
  FUN_0100ceb0(s___ctrl____p_01044144 + 0xc,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x10))
  ;
  FUN_01010c00(s___ctrl____p_01044144 + 0xc,*(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x10))
  ;
  FUN_0100ceb0(s___ctrl_>head____x_01044154 + 0x10,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x14));
  FUN_01010c00(s___ctrl_>head____x_01044154 + 0x10,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x14));
  FUN_0100ceb0(s___ctrl_>tail____x_01044168 + 0x10,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x18));
  FUN_01010c00(s___ctrl_>tail____x_01044168 + 0x10,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x18));
  FUN_0100ceb0(s___ctrl_>reset_request____d_0104417c + 0x18,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c));
  FUN_01010c00(s___ctrl_>reset_request____d_0104417c + 0x18,
               *(undefined4 *)(*(int *)(&DAT_01049d40 + iVar1) + 0x1c));
  FUN_0100ceb0(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
  FUN_01010c00(s__s__flash_toc_entries_checksum_v_0104404c + 0x30);
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
        if ((uint)((int)pcVar7 - (int)pcVar5) <= param_1) goto LAB_01028be8;
      }
      else if ((uint)((int)pcVar7 - iVar2) <= param_1) {
        pcVar5 = pcVar5 + *(int *)(&DAT_01049d38 + iVar1) + (-param_1 - iVar2);
        goto LAB_01028be8;
      }
      pcVar5 = pcVar7 + -param_1;
    }
  }
LAB_01028be8:
  uVar3 = 0;
  if (pcVar7 != pcVar5) {
    uVar4 = uVar3;
    do {
      if (*pcVar5 == '\n') {
        local_74[uVar4] = '\0';
        FUN_0100ceb0(s_The_test_s_final_words_are___s_01028f08 + 0x18,local_74);
        FUN_01010c00(s_The_test_s_final_words_are___s_01028f08 + 0x18,local_74);
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
  FUN_0100ceb0(&DAT_01028ee4,local_74);
  FUN_01010c00(&DAT_01028ee4,local_74);
  FUN_0100ceb0(s___ctrl_>wrap_around____d_01044198 + 0x18);
  FUN_01010c00(s___ctrl_>wrap_around____d_01044198 + 0x18);
  *(undefined4 *)(&DAT_01049d28 + iVar1) = uVar6;
  return;
}



/* @ 0x100cf4c */

undefined4 FUN_0100cf4c(int param_1,undefined4 param_2,undefined4 param_3)

{
  if (3 < param_1) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,&DAT_01032478,0x70);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,&DAT_01032478,0x70);
    FUN_01000458(0);
  }
  FUN_01022e6c(&DAT_0104a690 + param_1 * 8,
               *(undefined4 *)(s_uart_is_input_available_010324ac + param_1 * 4 + 0x14));
  *(undefined4 *)(&DAT_0104a634 + param_1 * 0x18) = param_2;
  *(undefined4 *)(&DAT_0104a638 + param_1 * 0x18) = param_3;
  return 0;
}



/* @ 0x100d0e0 */

undefined1 FUN_0100d0e0(int param_1)

{
  undefined1 local_19;
  
  if (3 < param_1) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,s_uart_write_byte_0103248c + 0xc,0x8b);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,s_uart_write_byte_0103248c + 0xc,0x8b);
    FUN_01000458(0);
  }
  if (*(int *)(&DAT_0104a690 + param_1 * 8) == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___port_<_4)_010324f4 + 8,1,&DAT_010324d0,
                 s_uart_write_byte_0103248c + 0xc,0x8c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___port_<_4)_010324f4 + 8,1,&DAT_010324d0,
                 s_uart_write_byte_0103248c + 0xc,0x8c);
    FUN_01000458(0);
  }
  FUN_01022f00(&DAT_0104a690 + param_1 * 8,1,&local_19,0xffffffff);
  return local_19;
}



/* @ 0x100d204 */

uint FUN_0100d204(int param_1)

{
  undefined4 unaff_r4;
  undefined4 unaff_r5;
  undefined4 unaff_r6;
  undefined4 unaff_lr;
  
  if (3 < param_1) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,s_uart_read_byte_0103249c + 0xc,0x96);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_uart_alpine_src_uart_c_010324d4 + 0x1c,1,
                 &DAT_010324d0,s_uart_read_byte_0103249c + 0xc,0x96);
    FUN_01000458(0);
  }
  if (*(int *)(&DAT_0104a690 + param_1 * 8) != 0) {
    if ((int *)(&DAT_0104a690 + param_1 * 8) == (int *)0x0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                   s_al_uart_is_input_available_010429dc + 0x18,
                   s_al_uart_tx_fifo_level_get_010429c0 + 0x18,0x1c9,unaff_r4,unaff_r5,unaff_r6,
                   unaff_lr);
      FUN_01010c00(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                   s_al_uart_is_input_available_010429dc + 0x18,
                   s_al_uart_tx_fifo_level_get_010429c0 + 0x18,0x1c9);
      FUN_01000458(0);
    }
    return *(uint *)(*(int *)(&DAT_0104a690 + param_1 * 8) + 0x14) & 1;
  }
  return 0;
}



/* @ 0x100d2c4 */

void FUN_0100d2c4(void)

{
  DAT_01049d44 = FUN_0100ea64(0x1812000,0x40000,0);
  return;
}



/* @ 0x100d2f8 */

void FUN_0100d2f8(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_01000088();
  FUN_0100eaec((&DAT_01049d44)[iVar1],param_1);
  return;
}



/* @ 0x100d328 */

int FUN_0100d328(int param_1,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  undefined4 unaff_r4;
  undefined4 unaff_r5;
  undefined4 unaff_r6;
  
  iVar1 = FUN_01000088();
  uVar2 = (&DAT_01049d44)[iVar1];
  if (param_1 == 0) {
    iVar1 = FUN_0100eaec(uVar2,param_2);
    return iVar1;
  }
  if (param_2 < 0xffffffc0) {
    if (param_2 < 0xb) {
      uVar3 = 0x10;
    }
    else {
      uVar3 = param_2 + 0xb & 0xfffffff8;
    }
    iVar1 = FUN_0100e5ac(uVar2,param_1 + -8,uVar3);
    if (iVar1 == 0) {
      iVar1 = FUN_0100eaec(uVar2,param_2);
      if (iVar1 != 0) {
        if ((*(uint *)(param_1 + -4) & 3) == 0) {
          iVar4 = 8;
        }
        else {
          iVar4 = 4;
        }
        uVar3 = (*(uint *)(param_1 + -4) & 0xfffffff8) - iVar4;
        if (param_2 <= uVar3) {
          uVar3 = param_2;
        }
        FUN_010129dc(iVar1,param_1,uVar3);
        FUN_010101f8(uVar2,param_1);
      }
    }
    else {
      iVar1 = iVar1 + 8;
    }
  }
  else {
    FUN_01010c00(s_mspace_realloc_0103261c + 0xc,s_sys_alloc_01032610 + 8,param_2,&DAT_01049d44,
                 unaff_r4,unaff_r5,unaff_r6);
    iVar1 = 0;
  }
  return iVar1;
}



/* @ 0x100d368 */

void FUN_0100d368(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_01000088();
  FUN_010101f8((&DAT_01049d44)[iVar1],param_1);
  return;
}




void FUN_0100d398(void)

{
  FUN_01010c00(s_munmap_01032528 + 4);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,s____uart_if_port__regs__01032500 + 0x14,0x3e);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,s____uart_if_port__regs__01032500 + 0x14,0x3e);
  FUN_01000458(0);
  do {
/* @ 0x100d398 -- WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0x100d410 */

undefined4
FUN_0100d410(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6)

{
  FUN_01010c00(s_stdlib_malloc_src_malloc_porting_01032548 + 0x20,param_1,param_2,param_3,param_4,
               param_5,param_6);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,s_abort_01032518 + 4,0x59);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,s_abort_01032518 + 4,0x59);
  FUN_01000458(0);
  return 0;
}



/* @ 0x100d4c4 */

undefined4 FUN_0100d4c4(undefined4 param_1,undefined4 param_2)

{
  FUN_01010c00(s_Received_MMAP_call__1_start____p_0103256c + 0x6c,param_1,param_2);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,&DAT_01032524,99);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s_Received_ABORT_call_01032530 + 0x14,&DAT_01032524,99);
  FUN_01000458(0);
  return 0;
}



/* @ 0x100d564 */

uint FUN_0100d564(int param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  
  uVar8 = param_1 + 8;
  iVar7 = param_1;
  if ((uVar8 & 7) != 0) {
    iVar7 = param_1 + (-uVar8 & 7);
    uVar8 = iVar7 + 8;
  }
  FUN_01012ae4(uVar8,0,0x1e0);
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
    iVar5 = iVar5 + 8;
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



/* @ 0x100d684 */

int FUN_0100d684(int param_1)

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
      if ((puVar9[3] & 9) == 1) {
        uVar13 = *puVar9;
        uVar10 = uVar13;
        if ((uVar13 + 8 & 7) != 0) {
          uVar10 = uVar13 + (-(uVar13 + 8) & 7);
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
LAB_0100da60:
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
                    FUN_0100d398();
                  }
                  else {
                    *puVar6 = 0;
                  }
                  goto LAB_0100d790;
                }
                uVar2 = *(uint *)(uVar10 + 0x10);
                if (uVar2 != 0) {
                  puVar1 = (uint *)(uVar10 + 0x10);
                  uVar4 = uVar2;
                  goto LAB_0100da60;
                }
                if (uVar3 == 0) goto LAB_0100d830;
                uVar4 = *(uint *)(uVar10 + 0x1c);
                iVar16 = param_1 + uVar4 * 4;
                if (*(uint *)(iVar16 + 0x130) == uVar10) {
                  *(undefined4 *)(iVar16 + 0x130) = 0;
                  goto LAB_0100da48;
                }
LAB_0100d7b0:
                if (uVar3 < *(uint *)(param_1 + 0x10)) {
                  FUN_0100d398();
                }
                else if (*(uint *)(uVar3 + 0x10) == uVar10) {
                  *(uint *)(uVar3 + 0x10) = uVar2;
                }
                else {
                  *(uint *)(uVar3 + 0x14) = uVar2;
                }
                if (uVar2 != 0) {
LAB_0100d7e0:
                  uVar4 = *(uint *)(param_1 + 0x10);
                  if (uVar2 < uVar4) {
LAB_0100dae4:
                    FUN_0100d398();
                  }
                  else {
                    uVar5 = *(uint *)(uVar10 + 0x10);
                    *(uint *)(uVar2 + 0x18) = uVar3;
                    if (uVar5 != 0) {
                      if (uVar5 < uVar4) {
                        FUN_0100d398();
                      }
                      else {
                        *(uint *)(uVar2 + 0x10) = uVar5;
                        *(uint *)(uVar5 + 0x18) = uVar2;
                      }
                    }
                    uVar3 = *(uint *)(uVar10 + 0x14);
                    if (uVar3 != 0) {
                      if (uVar3 < *(uint *)(param_1 + 0x10)) goto LAB_0100dae4;
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
                  FUN_0100d398();
                }
                else {
                  *(uint *)(uVar4 + 0xc) = uVar2;
                  *(uint *)(uVar2 + 8) = uVar4;
                }
LAB_0100d790:
                if (uVar3 != 0) {
                  uVar4 = *(uint *)(uVar10 + 0x1c);
                  iVar16 = param_1 + uVar4 * 4;
                  if (*(uint *)(iVar16 + 0x130) != uVar10) goto LAB_0100d7b0;
                  *(uint *)(iVar16 + 0x130) = uVar2;
                  if (uVar2 == 0) {
LAB_0100da48:
                    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & ~(1 << (uVar4 & 0xff));
                    goto LAB_0100d830;
                  }
                  goto LAB_0100d7e0;
                }
              }
            }
LAB_0100d830:
            iVar16 = FUN_0100d4c4(uVar13,uVar8);
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
                uVar8 = uVar14 - 0x100 >> 0x10 & 8;
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
                    if (bVar19 && !bVar18) goto LAB_0100db34;
                    *(uint *)(uVar8 + 0xc) = uVar10;
                    *(uint *)(uVar14 + 8) = uVar10;
                    *(uint *)(uVar10 + 8) = uVar8;
                    *(uint *)(uVar10 + 0xc) = uVar14;
                    *(undefined4 *)(uVar10 + 0x18) = 0;
                    goto joined_r0x0100d6c8;
                  }
                  uVar8 = *(uint *)(uVar14 + iVar16 * 4);
                } while (uVar8 != 0);
                if (uVar14 + iVar16 * 4 < *(uint *)(param_1 + 0x10)) {
LAB_0100db34:
                  FUN_0100d398();
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
joined_r0x0100d6c8:
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



/* @ 0x100db60 */

void FUN_0100db60(uint *param_1,uint *param_2,uint param_3)

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
    goto LAB_0100dc90;
  }
  uVar13 = *param_2;
  param_2 = (uint *)((int)param_2 - uVar13);
  if ((*puVar10 & 3) == 0) {
    iVar9 = param_3 + 0x10 + uVar13;
    iVar6 = FUN_0100d4c4(param_2,iVar9);
    if (iVar6 != 0) {
      return;
    }
    param_1[0x6c] = param_1[0x6c] - iVar9;
    return;
  }
  puVar10 = (uint *)param_1[4];
  param_3 = param_3 + uVar13;
  if (param_2 < puVar10) goto LAB_0100e010;
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
    goto LAB_0100dc90;
  }
  uVar13 = uVar13 >> 3;
  puVar3 = (uint *)param_2[3];
  if (uVar13 < 0x20) {
    puVar11 = (uint *)param_2[2];
    if (param_1 + uVar13 * 2 + 10 == puVar11) {
      if (puVar11 == puVar3) {
LAB_0100df50:
        *param_1 = *param_1 & ~(1 << (uVar13 & 0xff));
        goto LAB_0100dc90;
      }
LAB_0100e1f0:
      if ((puVar10 <= puVar3) && ((uint *)puVar3[2] == param_2)) {
LAB_0100e26c:
        puVar11[3] = (uint)puVar3;
        puVar3[2] = (uint)puVar11;
        goto LAB_0100dc90;
      }
    }
    else if ((puVar10 <= puVar11) && ((uint *)puVar11[3] == param_2)) {
      if (puVar11 != puVar3) {
        if (param_1 + uVar13 * 2 + 10 != puVar3) goto LAB_0100e1f0;
        goto LAB_0100e26c;
      }
      goto LAB_0100df50;
    }
    goto LAB_0100e204;
  }
  puVar11 = (uint *)param_2[6];
  if (puVar3 == param_2) {
    if ((uint *)param_2[5] != (uint *)0x0) {
      puVar4 = (uint *)param_2[5];
      puVar12 = param_2 + 5;
LAB_0100e134:
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
        FUN_0100d398();
        puVar10 = (uint *)param_1[4];
      }
      else {
        *puVar7 = 0;
      }
      goto LAB_0100dc00;
    }
    if ((uint *)param_2[4] != (uint *)0x0) {
      puVar4 = (uint *)param_2[4];
      puVar12 = param_2 + 4;
      goto LAB_0100e134;
    }
    if (puVar11 == (uint *)0x0) goto LAB_0100dc90;
    uVar13 = param_2[7];
    if ((uint *)param_1[uVar13 + 0x4c] == param_2) {
      param_1[uVar13 + 0x4c] = 0;
      goto LAB_0100e1a4;
    }
    puVar3 = (uint *)0x0;
LAB_0100dc24:
    if (puVar11 < puVar10) {
      FUN_0100d398();
      puVar10 = (uint *)param_1[4];
    }
    else if ((uint *)puVar11[4] == param_2) {
      puVar11[4] = (uint)puVar3;
    }
    else {
      puVar11[5] = (uint)puVar3;
    }
    if (puVar3 != (uint *)0x0) {
LAB_0100dc44:
      if (puVar3 < puVar10) {
LAB_0100e204:
        FUN_0100d398();
        puVar10 = (uint *)param_1[4];
      }
      else {
        puVar4 = (uint *)param_2[4];
        puVar3[6] = (uint)puVar11;
        if (puVar4 != (uint *)0x0) {
          if (puVar4 < puVar10) {
            FUN_0100d398();
            puVar10 = (uint *)param_1[4];
          }
          else {
            puVar3[4] = (uint)puVar4;
            puVar4[6] = (uint)puVar3;
          }
        }
        puVar11 = (uint *)param_2[5];
        if (puVar11 != (uint *)0x0) {
          if (puVar11 < puVar10) goto LAB_0100e204;
          puVar3[5] = (uint)puVar11;
          puVar11[6] = (uint)puVar3;
        }
      }
    }
  }
  else {
    puVar4 = (uint *)param_2[2];
    if (((puVar4 < puVar10) || ((uint *)puVar4[3] != param_2)) || ((uint *)puVar3[2] != param_2)) {
      FUN_0100d398();
      puVar10 = (uint *)param_1[4];
    }
    else {
      puVar4[3] = (uint)puVar3;
      puVar3[2] = (uint)puVar4;
    }
LAB_0100dc00:
    if (puVar11 != (uint *)0x0) {
      uVar13 = param_2[7];
      if ((uint *)param_1[uVar13 + 0x4c] != param_2) goto LAB_0100dc24;
      param_1[uVar13 + 0x4c] = (uint)puVar3;
      if (puVar3 == (uint *)0x0) {
LAB_0100e1a4:
        param_1[1] = param_1[1] & ~(1 << (uVar13 & 0xff));
        goto LAB_0100dc90;
      }
      goto LAB_0100dc44;
    }
  }
LAB_0100dc90:
  if (puVar8 < puVar10) goto LAB_0100e010;
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
LAB_0100df20:
          *param_1 = *param_1 & ~(1 << (uVar5 & 0xff));
        }
        else {
LAB_0100e2f0:
          if ((puVar11 < puVar10) || ((uint *)puVar11[2] != puVar8)) goto LAB_0100e304;
LAB_0100e23c:
          puVar4[3] = (uint)puVar11;
          puVar11[2] = (uint)puVar4;
        }
      }
      else {
        if ((puVar10 <= puVar4) && ((uint *)puVar4[3] == puVar8)) {
          if (puVar11 != puVar4) {
            if (param_1 + uVar5 * 2 + 10 != puVar11) goto LAB_0100e2f0;
            goto LAB_0100e23c;
          }
          goto LAB_0100df20;
        }
LAB_0100e304:
        FUN_0100d398();
        puVar3 = (uint *)param_1[5];
      }
      goto LAB_0100dda0;
    }
    puVar4 = (uint *)puVar8[6];
    if (puVar11 == puVar8) {
      puVar12 = (uint *)puVar8[5];
      if (puVar12 != (uint *)0x0) {
        puVar3 = puVar8 + 5;
LAB_0100e0d4:
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
          FUN_0100d398();
        }
        else {
          *puVar7 = 0;
        }
        goto LAB_0100dd10;
      }
      puVar12 = (uint *)puVar8[4];
      if (puVar12 != (uint *)0x0) {
        puVar3 = puVar8 + 4;
        goto LAB_0100e0d4;
      }
      if (puVar4 == (uint *)0x0) goto LAB_0100dda0;
      uVar13 = puVar8[7];
      if ((uint *)param_1[uVar13 + 0x4c] == puVar8) {
        param_1[uVar13 + 0x4c] = 0;
        goto LAB_0100e17c;
      }
      puVar11 = (uint *)0x0;
LAB_0100dd30:
      if (puVar4 < puVar10) {
        FUN_0100d398();
      }
      else if ((uint *)puVar4[4] == puVar8) {
        puVar4[4] = (uint)puVar11;
      }
      else {
        puVar4[5] = (uint)puVar11;
      }
      if (puVar11 == (uint *)0x0) goto LAB_0100df90;
LAB_0100dd50:
      puVar10 = (uint *)param_1[4];
      if (puVar11 < puVar10) {
        FUN_0100d398();
        puVar3 = (uint *)param_1[5];
      }
      else {
        puVar3 = (uint *)puVar8[4];
        puVar11[6] = (uint)puVar4;
        if (puVar3 != (uint *)0x0) {
          if (puVar3 < puVar10) {
            FUN_0100d398();
          }
          else {
            puVar11[4] = (uint)puVar3;
            puVar3[6] = (uint)puVar11;
          }
        }
        uVar13 = puVar8[5];
        if (uVar13 == 0) goto LAB_0100df90;
        if (uVar13 < param_1[4]) goto LAB_0100e304;
        puVar3 = (uint *)param_1[5];
        puVar11[5] = uVar13;
        *(uint **)(uVar13 + 0x18) = puVar11;
      }
    }
    else {
      puVar3 = (uint *)puVar8[2];
      if (((puVar3 < puVar10) || ((uint *)puVar3[3] != puVar8)) || ((uint *)puVar11[2] != puVar8)) {
        FUN_0100d398();
      }
      else {
        puVar3[3] = (uint)puVar11;
        puVar11[2] = (uint)puVar3;
      }
LAB_0100dd10:
      if (puVar4 != (uint *)0x0) {
        uVar13 = puVar8[7];
        if ((uint *)param_1[uVar13 + 0x4c] != puVar8) {
          puVar10 = (uint *)param_1[4];
          goto LAB_0100dd30;
        }
        param_1[uVar13 + 0x4c] = (uint)puVar11;
        if (puVar11 == (uint *)0x0) {
          puVar3 = (uint *)param_1[5];
LAB_0100e17c:
          param_1[1] = param_1[1] & ~(1 << (uVar13 & 0xff));
          goto LAB_0100dda0;
        }
        goto LAB_0100dd50;
      }
LAB_0100df90:
      puVar3 = (uint *)param_1[5];
    }
LAB_0100dda0:
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
        FUN_0100d398();
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
      uVar14 = uVar13 - 0x100 >> 0x10 & 8;
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
          goto LAB_0100e010;
        }
        uVar14 = *(uint *)(uVar5 + iVar6 * 4);
      } while (uVar14 != 0);
      if (uVar5 + iVar6 * 4 < param_1[4]) {
LAB_0100e010:
        FUN_0100d398();
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



/* @ 0x100e398 */

void FUN_0100e398(void)

{
  undefined1 auStack_10 [12];
  
  DAT_01049d48 = (uint)auStack_10 ^ 0x55555550 | 8;
  DAT_01049d4c = 0x1000;
  DAT_01049d50 = 0x10000;
  DAT_01049d54 = 0x40000;
  DAT_01049d58 = 0x200000;
  DAT_01049d5c = 5;
  return;
}



/* @ 0x100e5ac */

uint * FUN_0100e5ac(uint *param_1,uint *param_2,uint param_3)

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
LAB_0100e6e8:
    FUN_0100d398();
    return (uint *)0x0;
  }
  uVar2 = uVar10 & 3;
  bVar12 = uVar2 != 0;
  if (uVar2 != 1) {
    bVar12 = puVar8 <= param_2;
  }
  if ((bVar12) || (uVar7 = puVar8[1], (uVar7 & 1) == 0)) goto LAB_0100e6e8;
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
      FUN_0100db60(param_1,(int)param_2 + param_3,uVar4);
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
        goto LAB_0100e6b8;
      }
      if ((param_1 + uVar7 * 2 + 10 == puVar9) ||
         ((puVar11 <= puVar9 && (puVar8 == (uint *)puVar9[2])))) {
        puVar3[3] = (uint)puVar9;
        puVar9[2] = (uint)puVar3;
        goto LAB_0100e6b8;
      }
    }
    goto LAB_0100e9e0;
  }
  puVar3 = (uint *)puVar8[6];
  if (puVar8 == puVar9) {
    if ((uint *)puVar8[5] != (uint *)0x0) {
      puVar5 = (uint *)puVar8[5];
      puVar1 = puVar8 + 5;
LAB_0100e948:
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
      if (puVar6 < puVar11) goto LAB_0100e994;
      *puVar6 = 0;
      goto LAB_0100e834;
    }
    puVar9 = (uint *)puVar8[4];
    if (puVar9 != (uint *)0x0) {
      puVar5 = puVar9;
      puVar1 = puVar8 + 4;
      goto LAB_0100e948;
    }
    if (puVar3 == (uint *)0x0) goto LAB_0100e6b8;
    uVar7 = puVar8[7];
    if (puVar8 == (uint *)param_1[uVar7 + 0x4c]) {
      param_1[uVar7 + 0x4c] = 0;
      goto LAB_0100e9b4;
    }
LAB_0100e854:
    if (puVar3 < puVar11) {
      FUN_0100d398();
    }
    else if (puVar8 == (uint *)puVar3[4]) {
      puVar3[4] = (uint)puVar9;
    }
    else {
      puVar3[5] = (uint)puVar9;
    }
    if (puVar9 != (uint *)0x0) {
LAB_0100e874:
      puVar11 = (uint *)param_1[4];
      if (puVar11 <= puVar9) {
        puVar5 = (uint *)puVar8[4];
        puVar9[6] = (uint)puVar3;
        if (puVar5 != (uint *)0x0) {
          if (puVar5 < puVar11) {
            FUN_0100d398();
          }
          else {
            puVar9[4] = (uint)puVar5;
            puVar5[6] = (uint)puVar9;
          }
        }
        uVar7 = puVar8[5];
        if (uVar7 == 0) goto LAB_0100e930;
        if (param_1[4] <= uVar7) {
          uVar10 = param_2[1];
          puVar9[5] = uVar7;
          *(uint **)(uVar7 + 0x18) = puVar9;
          goto LAB_0100e6b8;
        }
      }
LAB_0100e9e0:
      FUN_0100d398();
      uVar10 = param_2[1];
      goto LAB_0100e6b8;
    }
  }
  else {
    puVar5 = (uint *)puVar8[2];
    if (((puVar5 < puVar11) || (puVar8 != (uint *)puVar5[3])) || (puVar8 != (uint *)puVar9[2])) {
LAB_0100e994:
      FUN_0100d398();
    }
    else {
      puVar5[3] = (uint)puVar9;
      puVar9[2] = (uint)puVar5;
    }
LAB_0100e834:
    if (puVar3 != (uint *)0x0) {
      uVar7 = puVar8[7];
      if (puVar8 != (uint *)param_1[uVar7 + 0x4c]) {
        puVar11 = (uint *)param_1[4];
        goto LAB_0100e854;
      }
      param_1[uVar7 + 0x4c] = (uint)puVar9;
      if (puVar9 == (uint *)0x0) {
        uVar10 = param_2[1];
LAB_0100e9b4:
        param_1[1] = param_1[1] & ~(1 << (uVar7 & 0xff));
        goto LAB_0100e6b8;
      }
      goto LAB_0100e874;
    }
  }
LAB_0100e930:
  uVar10 = param_2[1];
LAB_0100e6b8:
  if (0xf < uVar2) {
    param_2[1] = param_3 | uVar10 & 1 | 2;
    *(uint *)((int)param_2 + param_3 + 4) = uVar2 | 3;
    *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) | 1;
    FUN_0100db60(param_1,(int)param_2 + param_3,uVar2);
    return param_2;
  }
  param_2[1] = uVar4 | uVar10 & 1 | 2;
  *(uint *)((int)param_2 + uVar4 + 4) = *(uint *)((int)param_2 + uVar4 + 4) | 1;
  return param_2;
}



/* @ 0x100ea64 */

int FUN_0100ea64(undefined4 param_1,uint param_2)

{
  int iVar1;
  
  if (DAT_01049d48 == 0) {
    FUN_0100e398();
  }
  if (0x208 < param_2) {
    if (param_2 < -DAT_01049d4c - 0x208U) {
      iVar1 = FUN_0100d564(param_1,param_2);
      *(undefined4 *)(iVar1 + 0x1cc) = 8;
      return iVar1;
    }
  }
  return 0;
}




/* @ 0x100eaec -- WARNING: Type propagation algorithm not settling */

uint * FUN_0100eaec(uint *param_1,uint param_2)

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
    if (param_2 < 0xb) {
      uVar8 = 2;
      uVar18 = 0x10;
    }
    else {
      uVar18 = param_2 + 0xb & 0xfffffff8;
      uVar8 = param_2 + 0xb >> 3;
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
        FUN_0100d398();
      }
      else {
        puVar3[3] = (uint)(param_1 + uVar8 * 2 + 10);
        param_1[uVar8 * 2 + 0xc] = (uint)puVar3;
      }
      iVar9 = uVar26 + uVar8 * 8;
      *(uint *)(uVar26 + 4) = uVar8 * 8 | 3;
      *(uint *)(iVar9 + 4) = *(uint *)(iVar9 + 4) | 1;
      return (uint *)(uVar26 + 8);
    }
    uVar26 = param_1[2];
    if (uVar18 <= uVar26) {
LAB_0100eb48:
      uVar8 = param_1[5];
      uVar13 = uVar26 - uVar18;
      iVar9 = uVar8 + uVar26;
      if (0xf < uVar13) {
        param_1[2] = uVar13;
        param_1[5] = uVar8 + uVar18;
        *(uint *)(uVar8 + uVar18 + 4) = uVar13 | 1;
        *(uint *)(uVar8 + uVar26) = uVar13;
        *(uint *)(uVar8 + 4) = uVar18 | 3;
        goto LAB_0100eb7c;
      }
      uVar26 = uVar26 | 3;
LAB_0100efa0:
      param_1[2] = 0;
      param_1[5] = 0;
      *(uint *)(uVar8 + 4) = uVar26;
      *(uint *)(iVar9 + 4) = *(uint *)(iVar9 + 4) | 1;
LAB_0100eb7c:
      return (uint *)(uVar8 + 8);
    }
    if (uVar13 != 0) {
      iVar9 = 1 << (uVar8 & 0xff);
      uVar8 = (iVar9 * -2 | iVar9 * 2) & uVar13 << (uVar8 & 0xff);
      uVar8 = (-uVar8 & uVar8) - 1;
      uVar11 = uVar8 >> 0xc & 0x10;
      uVar8 = uVar8 >> uVar11;
      uVar13 = uVar8 >> 5 & 8;
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
        FUN_0100d398();
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
            FUN_0100d398();
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
      return (uint *)(uVar8 + 8);
    }
    uVar26 = param_1[1];
    if (uVar26 != 0) {
      uVar8 = (-uVar26 & uVar26) - 1;
      uVar11 = uVar8 >> 0xc & 0x10;
      uVar8 = uVar8 >> uVar11;
      uVar13 = uVar8 >> 5 & 8;
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
LAB_0100f9c0:
        FUN_0100d398();
      }
      else {
        puVar22 = (undefined4 *)puVar19[3];
        puVar24 = (undefined4 *)puVar19[6];
        if (puVar22 == puVar19) {
          if ((undefined4 *)puVar19[5] != (undefined4 *)0x0) {
            puVar14 = (undefined4 *)puVar19[5];
            puVar2 = puVar19 + 5;
LAB_0100fb98:
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
            if (puVar16 < puVar7) goto LAB_0100fc24;
            *puVar16 = 0;
            goto LAB_0100ee98;
          }
          puVar22 = (undefined4 *)puVar19[4];
          if (puVar22 != (undefined4 *)0x0) {
            puVar14 = puVar22;
            puVar2 = puVar19 + 4;
            goto LAB_0100fb98;
          }
          if (puVar24 != (undefined4 *)0x0) {
            uVar8 = puVar19[7];
            if (puVar19 == (undefined4 *)param_1[uVar8 + 0x4c]) {
              param_1[uVar8 + 0x4c] = 0;
              goto LAB_0100fc3c;
            }
            goto LAB_0100eeb8;
          }
        }
        else {
          puVar14 = (undefined4 *)puVar19[2];
          if (((puVar14 < puVar7) || ((undefined4 *)puVar14[3] != puVar19)) ||
             ((undefined4 *)puVar22[2] != puVar19)) {
LAB_0100fc24:
            FUN_0100d398();
          }
          else {
            puVar14[3] = puVar22;
            puVar22[2] = puVar14;
          }
LAB_0100ee98:
          if (puVar24 != (undefined4 *)0x0) {
            uVar8 = puVar19[7];
            if ((undefined4 *)param_1[uVar8 + 0x4c] == puVar19) {
              param_1[uVar8 + 0x4c] = (uint)puVar22;
              if (puVar22 != (undefined4 *)0x0) goto LAB_0100eed8;
              uVar26 = param_1[1];
LAB_0100fc3c:
              param_1[1] = uVar26 & ~(1 << (uVar8 & 0xff));
            }
            else {
              puVar7 = (undefined4 *)param_1[4];
LAB_0100eeb8:
              if (puVar24 < puVar7) {
                FUN_0100d398();
              }
              else if ((undefined4 *)puVar24[4] == puVar19) {
                puVar24[4] = puVar22;
              }
              else {
                puVar24[5] = puVar22;
              }
              if (puVar22 != (undefined4 *)0x0) {
LAB_0100eed8:
                puVar7 = (undefined4 *)param_1[4];
                if (puVar22 < puVar7) {
LAB_0100fdcc:
                  FUN_0100d398();
                }
                else {
                  puVar14 = (undefined4 *)puVar19[4];
                  puVar22[6] = puVar24;
                  if (puVar14 != (undefined4 *)0x0) {
                    if (puVar14 < puVar7) {
                      FUN_0100d398();
                    }
                    else {
                      puVar22[4] = puVar14;
                      puVar14[6] = puVar22;
                    }
                  }
                  uVar26 = puVar19[5];
                  if (uVar26 != 0) {
                    if (uVar26 < param_1[4]) goto LAB_0100fdcc;
                    puVar22[5] = uVar26;
                    *(undefined4 **)(uVar26 + 0x18) = puVar22;
                  }
                }
              }
            }
          }
        }
        if (uVar5 < 0x10) {
LAB_0100f8d8:
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
                FUN_0100d398();
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
LAB_0100f24c:
        if (puVar19 + 2 != (uint *)0x0) {
          return puVar19 + 2;
        }
      }
      uVar26 = param_1[2];
      goto LAB_0100eccc;
    }
LAB_0100ecd4:
    uVar26 = param_1[3];
    if (uVar18 < uVar26) goto LAB_0100ece0;
  }
  else {
    uVar26 = param_1[2];
    if (param_2 < 0xffffffc0) {
      uVar8 = param_1[1];
      uVar18 = param_2 + 0xb & 0xfffffff8;
      if (uVar8 != 0) {
        uVar4 = param_2 + 0xb >> 8;
        uVar5 = -uVar18;
        uVar13 = uVar18;
        if (uVar4 < 0x10000) {
          uVar11 = uVar4 - 0x100 >> 0x10 & 8;
          iVar9 = uVar4 << uVar11;
          uVar4 = iVar9 - 0x1000U >> 0x10 & 4;
          iVar9 = iVar9 << uVar4;
          uVar6 = iVar9 - 0x4000U >> 0x10 & 2;
          iVar9 = ((uint)(iVar9 << uVar6) >> 0xf) - ((uVar11 | uVar4) + uVar6);
          uVar4 = (uVar18 >> (iVar9 + 0x15U & 0xff) & 1) + (iVar9 + 0xe) * 2;
          puVar7 = (undefined4 *)param_1[uVar4 + 0x4c];
          if (puVar7 == (undefined4 *)0x0) {
LAB_0100f854:
            uVar13 = uVar8 & -2 << (uVar4 & 0xff);
            if ((uVar13 == 0) ||
               (uVar13 = (-uVar13 & uVar13) - 1, uVar12 = uVar13 >> 0xc & 0x10,
               uVar13 = uVar13 >> uVar12, uVar4 = uVar13 >> 5 & 8, uVar13 = uVar13 >> uVar4,
               uVar6 = uVar13 >> 2 & 4, uVar13 = uVar13 >> uVar6, uVar11 = uVar13 >> 1 & 2,
               uVar13 = uVar13 >> uVar11, uVar20 = -((int)(uVar13 << 0x1e) >> 0x1f),
               puVar7 = (undefined4 *)
                        param_1[(uVar12 | uVar4) + uVar6 + uVar11 + uVar20 +
                                (uVar13 >> (uVar20 & 0xff)) + 0x4c], puVar7 == (undefined4 *)0x0))
            goto LAB_0100eccc;
            uVar11 = puVar7[1] & 0xfffffff8;
            puVar19 = (undefined4 *)0x0;
LAB_0100f0d0:
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
LAB_0100f044:
            puVar25 = (undefined4 *)0x0;
            puVar19 = (undefined4 *)0x0;
            uVar6 = uVar5;
            while( true ) {
              uVar11 = puVar7[1] & 0xfffffff8;
              uVar5 = uVar11 - uVar18;
              if ((uVar5 < uVar6) && (puVar19 = puVar7, uVar6 = uVar5, uVar5 == 0))
              goto LAB_0100f0d0;
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
            if (puVar19 == (undefined4 *)0x0 && puVar25 == (undefined4 *)0x0) goto LAB_0100f854;
            if (puVar25 != (undefined4 *)0x0) {
              uVar11 = puVar25[1] & 0xfffffff8;
              puVar7 = puVar25;
              goto LAB_0100f0d0;
            }
          }
          if ((puVar19 != (undefined4 *)0x0) && (uVar5 < uVar26 - uVar18)) {
            puVar7 = (undefined4 *)param_1[4];
            if ((puVar19 < puVar7) ||
               (puVar25 = (undefined4 *)((int)puVar19 + uVar18), puVar25 <= puVar19))
            goto LAB_0100f9c0;
            puVar22 = (undefined4 *)puVar19[3];
            puVar24 = (undefined4 *)puVar19[6];
            if (puVar19 == puVar22) {
              if ((undefined4 *)puVar19[5] != (undefined4 *)0x0) {
                puVar14 = (undefined4 *)puVar19[5];
                puVar2 = puVar19 + 5;
LAB_0100faa0:
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
                if (puVar16 < puVar7) goto LAB_0100fb80;
                *puVar16 = 0;
                goto LAB_0100f160;
              }
              puVar22 = (undefined4 *)puVar19[4];
              if (puVar22 != (undefined4 *)0x0) {
                puVar14 = puVar22;
                puVar2 = puVar19 + 4;
                goto LAB_0100faa0;
              }
              if (puVar24 != (undefined4 *)0x0) {
                uVar26 = puVar19[7];
                if (puVar19 == (undefined4 *)param_1[uVar26 + 0x4c]) {
                  param_1[uVar26 + 0x4c] = 0;
                  goto LAB_0100fbe0;
                }
                goto LAB_0100f180;
              }
            }
            else {
              puVar14 = (undefined4 *)puVar19[2];
              if (((puVar14 < puVar7) || (puVar19 != (undefined4 *)puVar14[3])) ||
                 (puVar19 != (undefined4 *)puVar22[2])) {
LAB_0100fb80:
                FUN_0100d398();
              }
              else {
                puVar14[3] = puVar22;
                puVar22[2] = puVar14;
              }
LAB_0100f160:
              if (puVar24 != (undefined4 *)0x0) {
                uVar26 = puVar19[7];
                if (puVar19 == (undefined4 *)param_1[uVar26 + 0x4c]) {
                  param_1[uVar26 + 0x4c] = (uint)puVar22;
                  if (puVar22 != (undefined4 *)0x0) goto LAB_0100f1a0;
                  uVar8 = param_1[1];
LAB_0100fbe0:
                  param_1[1] = uVar8 & ~(1 << (uVar26 & 0xff));
                }
                else {
                  puVar7 = (undefined4 *)param_1[4];
LAB_0100f180:
                  if (puVar24 < puVar7) {
                    FUN_0100d398();
                  }
                  else if (puVar19 == (undefined4 *)puVar24[4]) {
                    puVar24[4] = puVar22;
                  }
                  else {
                    puVar24[5] = puVar22;
                  }
                  if (puVar22 != (undefined4 *)0x0) {
LAB_0100f1a0:
                    puVar7 = (undefined4 *)param_1[4];
                    if (puVar22 < puVar7) {
LAB_0100fd48:
                      FUN_0100d398();
                    }
                    else {
                      puVar14 = (undefined4 *)puVar19[4];
                      puVar22[6] = puVar24;
                      if (puVar14 != (undefined4 *)0x0) {
                        if (puVar14 < puVar7) {
                          FUN_0100d398();
                        }
                        else {
                          puVar22[4] = puVar14;
                          puVar14[6] = puVar22;
                        }
                      }
                      uVar26 = puVar19[5];
                      if (uVar26 != 0) {
                        if (uVar26 < param_1[4]) goto LAB_0100fd48;
                        puVar22[5] = uVar26;
                        *(undefined4 **)(uVar26 + 0x18) = puVar22;
                      }
                    }
                  }
                }
              }
            }
            if (uVar5 < 0x10) goto LAB_0100f8d8;
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
                  FUN_0100d398();
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
                uVar8 = uVar26 - 0x100 >> 0x10 & 8;
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
                    if (uVar26 < param_1[4] || uVar8 < param_1[4]) goto LAB_0100fed8;
                    *(undefined4 **)(uVar26 + 0xc) = puVar25;
                    *(undefined4 **)(uVar8 + 8) = puVar25;
                    puVar25[2] = uVar26;
                    puVar25[3] = uVar8;
                    puVar25[6] = 0;
                    goto LAB_0100f24c;
                  }
                  iVar9 = (int)uVar26 >> 0x1f;
                  uVar26 = uVar26 << 1;
                  iVar9 = 4 - iVar9;
                  uVar13 = *(uint *)(uVar8 + iVar9 * 4);
                } while (uVar13 != 0);
                if (uVar8 + iVar9 * 4 < param_1[4]) {
LAB_0100fed8:
                  FUN_0100d398();
                }
                else {
                  *(undefined4 **)(uVar8 + iVar9 * 4) = puVar25;
                  puVar25[2] = puVar25;
                  puVar25[3] = puVar25;
                  puVar25[6] = uVar8;
                }
              }
            }
            goto LAB_0100f24c;
          }
        }
        else {
          puVar7 = (undefined4 *)param_1[0x6b];
          if (puVar7 != (undefined4 *)0x0) {
            uVar4 = 0x1f;
            goto LAB_0100f044;
          }
        }
      }
LAB_0100eccc:
      if (uVar18 <= uVar26) goto LAB_0100eb48;
      goto LAB_0100ecd4;
    }
    if (uVar26 == 0xffffffff) {
      uVar8 = param_1[5];
      iVar9 = uVar8 - 1;
      uVar26 = 0xffffffff;
      goto LAB_0100efa0;
    }
    uVar18 = 0xffffffff;
  }
  if (DAT_01049d48 == 0) {
    FUN_0100e398();
  }
  if ((((param_1[0x6f] & 1) == 0) || (uVar18 < DAT_01049d54)) || (param_1[3] == 0)) {
LAB_0100ebd4:
    uVar8 = DAT_01049d50 + 0x2f + uVar18 & -DAT_01049d50;
    if (uVar8 <= uVar18) {
      return (uint *)0x0;
    }
    uVar26 = param_1[0x6e];
    if (uVar26 != 0) {
      uVar13 = param_1[0x6c];
      goto LAB_0100ec00;
    }
  }
  else {
    uVar26 = param_1[0x6e];
    uVar8 = DAT_01049d4c + 0x1e + uVar18 & -DAT_01049d4c;
    if ((uVar26 == 0) ||
       (uVar13 = param_1[0x6c], uVar8 + uVar13 <= uVar26 && uVar13 < uVar8 + uVar13)) {
      if ((uVar18 < uVar8) &&
         (puVar3 = (uint *)FUN_0100d410(0,uVar8,3,2,0xffffffff,0), puVar3 != (uint *)0xffffffff)) {
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
      goto LAB_0100ebd4;
    }
    uVar8 = DAT_01049d50 + 0x2f + uVar18 & -DAT_01049d50;
    if (uVar8 <= uVar18) {
      return (uint *)0x0;
    }
LAB_0100ec00:
    uVar5 = uVar8 + uVar13;
    uVar4 = uVar26;
    if (uVar5 <= uVar26) {
      uVar4 = uVar13;
    }
    if ((uVar5 > uVar26 || uVar5 < uVar13) || uVar4 == uVar5) {
      return (uint *)0x0;
    }
  }
  puVar3 = (uint *)FUN_0100d410(0,uVar8,3,2,0xffffffff,0);
  if (puVar3 == (uint *)0xffffffff) goto LAB_0100f438;
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
        if (((((puVar10[3] & 8) == 0) && ((puVar10[3] & 1) != 0)) && ((uint *)*puVar10 <= puVar21))
           && (puVar21 < puVar3)) {
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
          goto LAB_0100f430;
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
      if (puVar10 == (uint *)0x0) goto LAB_0100f270;
    }
    if (((puVar10[3] & 8) != 0) || ((puVar10[3] & 1) == 0)) goto LAB_0100f270;
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
    if ((uVar18 & 3) != 1) goto LAB_0100f4e0;
    puVar3 = (uint *)puVar23[3];
    uVar13 = uVar18 >> 3;
    if (uVar13 < 0x20) {
      puVar21 = (uint *)puVar23[2];
      if ((puVar21 != param_1 + uVar13 * 2 + 10) &&
         ((puVar21 < (uint *)param_1[4] || ((uint *)puVar21[3] != puVar23)))) goto LAB_01010180;
      if (puVar21 == puVar3) {
        *param_1 = *param_1 & ~(1 << (uVar13 & 0xff));
        goto LAB_0100ff20;
      }
      if ((param_1 + uVar13 * 2 + 10 != puVar3) &&
         ((puVar3 < (uint *)param_1[4] || ((uint *)puVar3[2] != puVar23)))) goto LAB_01010180;
      puVar21[3] = (uint)puVar3;
      puVar3[2] = (uint)puVar21;
      goto LAB_0100ff20;
    }
    uVar13 = puVar23[6];
    if (puVar23 == puVar3) {
      if ((uint *)puVar23[5] == (uint *)0x0) {
        puVar3 = (uint *)puVar23[4];
        if (puVar3 == (uint *)0x0) goto LAB_0101000c;
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
        FUN_0100d398();
      }
      else {
        *puVar17 = 0;
      }
    }
    else {
      uVar4 = puVar23[2];
      if (((uVar4 < param_1[4]) || (*(uint **)(uVar4 + 0xc) != puVar23)) ||
         ((uint *)puVar3[2] != puVar23)) {
        FUN_0100d398();
      }
      else {
        *(uint **)(uVar4 + 0xc) = puVar3;
        puVar3[2] = uVar4;
      }
    }
LAB_0101000c:
    if (uVar13 == 0) goto LAB_0100ff20;
    uVar4 = puVar23[7];
    if ((uint *)param_1[uVar4 + 0x4c] == puVar23) {
      param_1[uVar4 + 0x4c] = (uint)puVar3;
      if (puVar3 == (uint *)0x0) {
        param_1[1] = param_1[1] & ~(1 << (uVar4 & 0xff));
        goto LAB_0100ff20;
      }
    }
    else {
      if (uVar13 < param_1[4]) {
        FUN_0100d398();
      }
      else if (*(uint **)(uVar13 + 0x10) == puVar23) {
        *(uint **)(uVar13 + 0x10) = puVar3;
      }
      else {
        *(uint **)(uVar13 + 0x14) = puVar3;
      }
      if (puVar3 == (uint *)0x0) goto LAB_0100ff20;
    }
    puVar21 = (uint *)param_1[4];
    if (puVar21 <= puVar3) {
      puVar10 = (uint *)puVar23[4];
      puVar3[6] = uVar13;
      if (puVar10 != (uint *)0x0) {
        if (puVar10 < puVar21) {
          FUN_0100d398();
        }
        else {
          puVar3[4] = (uint)puVar10;
          puVar10[6] = (uint)puVar3;
        }
      }
      uVar13 = puVar23[5];
      if (uVar13 == 0) goto LAB_0100ff20;
      if (param_1[4] <= uVar13) {
        puVar3[5] = uVar13;
        *(uint **)(uVar13 + 0x18) = puVar3;
        goto LAB_0100ff20;
      }
    }
LAB_01010180:
    FUN_0100d398();
LAB_0100ff20:
    puVar23 = (uint *)((int)puVar23 + (uVar18 & 0xfffffff8));
    uVar8 = uVar8 + (uVar18 & 0xfffffff8);
    uVar18 = puVar23[1];
LAB_0100f4e0:
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
          FUN_0100d398();
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
      uVar4 = uVar18 - 0x100 >> 0x10 & 8;
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
        goto LAB_01010144;
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
LAB_01010144:
    FUN_0100d398();
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
  uVar4 = (int)param_1 + ((param_1[-1] & 0xfffffff8) - 8);
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
  goto LAB_0100f430;
LAB_0100f270:
  while ((puVar21 < (uint *)*puVar15 ||
         (puVar10 = (uint *)((int)*puVar15 + puVar15[1]), puVar10 <= puVar21))) {
    puVar15 = (uint *)puVar15[2];
    if (puVar15 == (uint *)0x0) {
                    /* WARNING: Does not return */
      pcVar1 = (code *)software_udf(0,0x10101f4);
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
LAB_0100f9a8:
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
          FUN_0100d398();
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
        uVar13 = uVar8 - 0x100 >> 0x10 & 8;
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
            if (uVar13 < param_1[4] || uVar8 < param_1[4]) goto LAB_0100f9a4;
            uVar26 = param_1[3];
            *(uint **)(uVar8 + 0xc) = puVar21;
            *(uint **)(uVar13 + 8) = puVar21;
            puVar21[2] = uVar8;
            puVar21[3] = uVar13;
            puVar21[6] = 0;
            goto LAB_0100f430;
          }
          iVar9 = (int)uVar8 >> 0x1f;
          uVar8 = uVar8 << 1;
          iVar9 = 4 - iVar9;
          uVar4 = *(uint *)(uVar13 + iVar9 * 4);
        } while (uVar4 != 0);
        if (uVar13 + iVar9 * 4 < param_1[4]) {
LAB_0100f9a4:
          FUN_0100d398();
          goto LAB_0100f9a8;
        }
        *(uint **)(uVar13 + iVar9 * 4) = puVar21;
        uVar26 = param_1[3];
        puVar21[2] = (uint)puVar21;
        puVar21[3] = (uint)puVar21;
        puVar21[6] = uVar13;
      }
    }
  }
LAB_0100f430:
  if (uVar18 < uVar26) {
LAB_0100ece0:
    uVar8 = param_1[6];
    param_1[3] = uVar26 - uVar18;
    param_1[6] = uVar8 + uVar18;
    *(uint *)(uVar8 + uVar18 + 4) = uVar26 - uVar18 | 1;
    *(uint *)(uVar8 + 4) = uVar18 | 3;
    return (uint *)(uVar8 + 8);
  }
LAB_0100f438:
  FUN_01010c00(s_mspace_realloc_0103261c + 0xc,s_Received_MUNMAP_call__1_start_____010325dc + 0x30);
  return (uint *)0x0;
}




void FUN_010101f8(uint *param_1,int param_2)

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
  puVar11 = (uint *)(param_2 + -8);
  if (puVar11 < puVar6) goto LAB_010103a4;
  uVar12 = *(uint *)(param_2 + -4);
  if ((uVar12 & 3) == 1) goto LAB_010103a4;
  uVar13 = uVar12 & 0xfffffff8;
  puVar14 = (uint *)((int)puVar11 + uVar13);
  if ((uVar12 & 1) != 0) goto LAB_01010248;
  uVar9 = *(uint *)(param_2 + -8);
  puVar11 = (uint *)((int)puVar11 - uVar9);
  if ((uVar12 & 3) == 0) {
    iVar4 = uVar9 + 0x10 + uVar13;
    iVar8 = FUN_0100d4c4(puVar11,iVar4);
    if (iVar8 != 0) {
      return;
    }
    param_1[0x6c] = param_1[0x6c] - iVar4;
    return;
  }
  uVar13 = uVar13 + uVar9;
  if (puVar11 < puVar6) goto LAB_010103a4;
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
    goto LAB_01010248;
  }
  uVar9 = uVar9 >> 3;
  puVar15 = (uint *)puVar11[3];
  if (uVar9 < 0x20) {
    puVar17 = (uint *)puVar11[2];
    if (param_1 + uVar9 * 2 + 10 == puVar17) {
      if (puVar15 != puVar17) {
LAB_01010930:
        if ((puVar6 <= puVar15) && ((uint *)puVar15[2] == puVar11)) {
LAB_010109ac:
          puVar17[3] = (uint)puVar15;
          puVar15[2] = (uint)puVar17;
          goto LAB_01010248;
        }
        goto LAB_01010944;
      }
    }
    else {
      if ((puVar17 < puVar6) || ((uint *)puVar17[3] != puVar11)) goto LAB_01010944;
      if (puVar15 != puVar17) {
        if (param_1 + uVar9 * 2 + 10 != puVar15) goto LAB_01010930;
        goto LAB_010109ac;
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
LAB_01010780:
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
        if (puVar10 < puVar6) goto LAB_010108cc;
        *puVar10 = 0;
        goto LAB_010105ec;
      }
      puVar15 = (uint *)puVar11[4];
      if (puVar15 != (uint *)0x0) {
        puVar7 = puVar15;
        puVar2 = puVar11 + 4;
        goto LAB_01010780;
      }
      if (puVar17 == (uint *)0x0) goto LAB_01010248;
      uVar12 = puVar11[7];
      if ((uint *)param_1[uVar12 + 0x4c] == puVar11) {
        param_1[uVar12 + 0x4c] = 0;
        goto LAB_010108e0;
      }
LAB_0101060c:
      if (puVar17 < puVar6) {
        FUN_0100d398();
      }
      else if ((uint *)puVar17[4] == puVar11) {
        puVar17[4] = (uint)puVar15;
      }
      else {
        puVar17[5] = (uint)puVar15;
      }
      if (puVar15 != (uint *)0x0) {
LAB_0101062c:
        puVar6 = (uint *)param_1[4];
        if (puVar15 < puVar6) {
LAB_01010944:
          FUN_0100d398();
        }
        else {
          puVar7 = (uint *)puVar11[4];
          puVar15[6] = (uint)puVar17;
          if (puVar7 != (uint *)0x0) {
            if (puVar7 < puVar6) {
              FUN_0100d398();
            }
            else {
              puVar15[4] = (uint)puVar7;
              puVar7[6] = (uint)puVar15;
            }
          }
          uVar12 = puVar11[5];
          if (uVar12 != 0) {
            if (uVar12 < param_1[4]) goto LAB_01010944;
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
LAB_010108cc:
        FUN_0100d398();
      }
      else {
        puVar7[3] = (uint)puVar15;
        puVar15[2] = (uint)puVar7;
      }
LAB_010105ec:
      if (puVar17 != (uint *)0x0) {
        uVar12 = puVar11[7];
        if ((uint *)param_1[uVar12 + 0x4c] != puVar11) {
          puVar6 = (uint *)param_1[4];
          goto LAB_0101060c;
        }
        param_1[uVar12 + 0x4c] = (uint)puVar15;
        if (puVar15 == (uint *)0x0) {
LAB_010108e0:
          param_1[1] = param_1[1] & ~(1 << (uVar12 & 0xff));
          goto LAB_01010248;
        }
        goto LAB_0101062c;
      }
    }
  }
LAB_01010248:
  if ((puVar14 <= puVar11) || (uVar12 = puVar14[1], (uVar12 & 1) == 0)) {
LAB_010103a4:
    FUN_0100d398();
    return;
  }
  if ((uVar12 & 2) != 0) {
    puVar14[1] = uVar12 & 0xfffffffe;
    puVar11[1] = uVar13 | 1;
    *(uint *)((int)puVar11 + uVar13) = uVar13;
    goto joined_r0x01010408;
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
      FUN_0100e398();
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
              goto LAB_0100e4fc;
            }
          }
          puVar11 = (uint *)puVar11[2];
          if (puVar11 == (uint *)0x0) {
/* @ 0x10101f8 -- WARNING: Does not return */
            pcVar1 = (code *)software_udf(0,0x100e5a8);
            (*pcVar1)();
          }
        } while( true );
      }
      goto LAB_0100e480;
    }
    goto LAB_0100e4b0;
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
          if (param_1 + uVar9 * 2 + 10 != puVar15) goto LAB_010107c0;
          goto LAB_01010924;
        }
        goto LAB_010102b8;
      }
      goto LAB_010107d4;
    }
    if (puVar15 == puVar17) {
LAB_010102b8:
      *param_1 = *param_1 & ~(1 << (uVar9 & 0xff));
    }
    else {
      puVar7 = (uint *)param_1[4];
LAB_010107c0:
      if ((puVar15 < puVar7) || ((uint *)puVar15[2] != puVar14)) goto LAB_010107d4;
LAB_01010924:
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
LAB_01010868:
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
        if (puVar7 < (uint *)param_1[4]) goto LAB_010107b8;
        *puVar7 = 0;
        goto LAB_01010520;
      }
      if ((uint *)puVar14[4] != (uint *)0x0) {
        puVar6 = (uint *)puVar14[4];
        puVar17 = puVar14 + 4;
        goto LAB_01010868;
      }
      if (uVar12 == 0) goto LAB_010102c8;
      uVar9 = puVar14[7];
      if ((uint *)param_1[uVar9 + 0x4c] == puVar14) {
        param_1[uVar9 + 0x4c] = 0;
        goto LAB_010108b4;
      }
      puVar15 = (uint *)0x0;
LAB_0101053c:
      if (uVar12 < param_1[4]) {
        FUN_0100d398();
      }
      else if (*(uint **)(uVar12 + 0x10) == puVar14) {
        *(uint **)(uVar12 + 0x10) = puVar15;
      }
      else {
        *(uint **)(uVar12 + 0x14) = puVar15;
      }
      if (puVar15 != (uint *)0x0) {
LAB_01010560:
        puVar6 = (uint *)param_1[4];
        if (puVar6 <= puVar15) {
          puVar17 = (uint *)puVar14[4];
          puVar15[6] = uVar12;
          if (puVar17 != (uint *)0x0) {
            if (puVar17 < puVar6) {
              FUN_0100d398();
            }
            else {
              puVar15[4] = (uint)puVar17;
              puVar17[6] = (uint)puVar15;
            }
          }
          uVar12 = puVar14[5];
          if (uVar12 == 0) goto LAB_010107d8;
          if (param_1[4] <= uVar12) {
            puVar6 = (uint *)param_1[5];
            puVar15[5] = uVar12;
            *(uint **)(uVar12 + 0x18) = puVar15;
            goto LAB_010102c8;
          }
        }
LAB_010107d4:
        FUN_0100d398();
      }
    }
    else {
      uVar9 = puVar14[2];
      if (((uVar9 < param_1[4]) || (*(uint **)(uVar9 + 0xc) != puVar14)) ||
         ((uint *)puVar15[2] != puVar14)) {
LAB_010107b8:
        FUN_0100d398();
      }
      else {
        *(uint **)(uVar9 + 0xc) = puVar15;
        puVar15[2] = uVar9;
      }
LAB_01010520:
      if (uVar12 != 0) {
        uVar9 = puVar14[7];
        if ((uint *)param_1[uVar9 + 0x4c] != puVar14) goto LAB_0101053c;
        param_1[uVar9 + 0x4c] = (uint)puVar15;
        if (puVar15 == (uint *)0x0) {
          puVar6 = (uint *)param_1[5];
LAB_010108b4:
          param_1[1] = param_1[1] & ~(1 << (uVar9 & 0xff));
          goto LAB_010102c8;
        }
        goto LAB_01010560;
      }
    }
LAB_010107d8:
    puVar6 = (uint *)param_1[5];
  }
LAB_010102c8:
  puVar11[1] = uVar13 | 1;
  *(uint *)((int)puVar11 + uVar13) = uVar13;
  if (puVar6 == puVar11) {
    param_1[2] = uVar13;
    return;
  }
joined_r0x01010408:
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
        FUN_0100d398();
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
      uVar9 = uVar12 - 0x100 >> 0x10 & 8;
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
          if (bVar20 && !bVar19) goto LAB_010108f4;
          *(uint **)(uVar13 + 0xc) = puVar11;
          *(uint **)(uVar9 + 8) = puVar11;
          puVar11[2] = uVar13;
          puVar11[3] = uVar9;
          puVar11[6] = 0;
          goto LAB_010104b0;
        }
        uVar18 = *(uint *)(uVar9 + iVar8 * 4);
      } while (uVar18 != 0);
      if (uVar9 + iVar8 * 4 < param_1[4]) {
LAB_010108f4:
        FUN_0100d398();
      }
      else {
        *(uint **)(uVar9 + iVar8 * 4) = puVar11;
        puVar11[2] = (uint)puVar11;
        puVar11[3] = (uint)puVar11;
        puVar11[6] = uVar9;
      }
    }
LAB_010104b0:
    uVar12 = param_1[8];
    param_1[8] = uVar12 - 1;
    if (uVar12 - 1 == 0) {
      FUN_0100d684(param_1);
      return;
    }
  }
  return;
  while (puVar14 = (uint *)puVar14[2], puVar14 != (uint *)0x0) {
LAB_0100e4fc:
    if ((puVar15 <= puVar14) && (puVar14 < (uint *)((int)puVar15 + uVar12))) goto LAB_0100e480;
  }
  iVar8 = FUN_0100d4c4((int)puVar15 + (uVar12 - uVar13),uVar13,DAT_01049d50,puVar15,unaff_r4,
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
LAB_0100e480:
    uVar13 = 0;
  }
  iVar8 = FUN_0100d684(param_1);
  if (uVar13 + iVar8 != 0) {
    uVar3 = 1;
    goto LAB_0100e4b4;
  }
  if (param_1[7] < param_1[3]) {
    param_1[7] = 0xffffffff;
    uVar3 = 0;
    goto LAB_0100e4b4;
  }
LAB_0100e4b0:
  uVar3 = 0;
LAB_0100e4b4:
                    /* WARNING: Could not recover jumptable at 0x0100e4c4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(uVar3);
  return;
}



/* @ 0x1010b88 */

int FUN_01010b88(undefined1 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

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
  iVar1 = FUN_0101130c(0,local_24,param_2,local_1c,0);
  *local_24[0] = 0;
  return iVar1 + 1;
}



/* @ 0x1010be4 */

void FUN_01010be4(void)

{
  FUN_0101130c();
  return;
}



/* @ 0x1010c00 */

void FUN_01010c00(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_0101130c(0x1010af4,0,param_1,&uStack_c,0);
  return;
}




void FUN_01010c44(code *UNRECOVERED_JUMPTABLE,int *param_2,undefined4 param_3,int *param_4)

{
  int iVar1;
  
  if (param_2 == (int *)0x0) {
/* @ 0x1010c44 -- WARNING: Could not recover jumptable at 0x01010ca0. Too many branches */
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




/* @ 0x101130c -- WARNING: Control flow encountered bad instruction data */
/* WARNING: Removing unreachable block (ram,0x0101162c) */
/* WARNING: Removing unreachable block (ram,0x010125f8) */
/* WARNING: Removing unreachable block (ram,0x010128ac) */
/* WARNING: Removing unreachable block (ram,0x01012600) */
/* WARNING: Removing unreachable block (ram,0x010120d0) */
/* WARNING: Removing unreachable block (ram,0x010120dc) */
/* WARNING: Removing unreachable block (ram,0x0101223c) */
/* WARNING: Removing unreachable block (ram,0x01012780) */
/* WARNING: Removing unreachable block (ram,0x01012248) */
/* WARNING: Removing unreachable block (ram,0x01012620) */
/* WARNING: Removing unreachable block (ram,0x0101273c) */
/* WARNING: Removing unreachable block (ram,0x01012628) */
/* WARNING: Removing unreachable block (ram,0x010115f4) */
/* WARNING: Removing unreachable block (ram,0x01011600) */
/* WARNING: Removing unreachable block (ram,0x01012198) */
/* WARNING: Removing unreachable block (ram,0x01012650) */
/* WARNING: Removing unreachable block (ram,0x010128ec) */
/* WARNING: Removing unreachable block (ram,0x0101265c) */
/* WARNING: Removing unreachable block (ram,0x0101213c) */
/* WARNING: Removing unreachable block (ram,0x01012684) */
/* WARNING: Removing unreachable block (ram,0x010126d0) */
/* WARNING: Removing unreachable block (ram,0x01012690) */
/* WARNING: Removing unreachable block (ram,0x01011558) */
/* WARNING: Removing unreachable block (ram,0x01011564) */
/* WARNING: Removing unreachable block (ram,0x01011a80) */
/* WARNING: Removing unreachable block (ram,0x010124c4) */
/* WARNING: Removing unreachable block (ram,0x010115a0) */
/* WARNING: Removing unreachable block (ram,0x010115ac) */
/* WARNING: Removing unreachable block (ram,0x010124a0) */
/* WARNING: Removing unreachable block (ram,0x01011a50) */
/* WARNING: Removing unreachable block (ram,0x010121f0) */
/* WARNING: Removing unreachable block (ram,0x010119a4) */
/* WARNING: Removing unreachable block (ram,0x01011634) */
/* WARNING: Removing unreachable block (ram,0x01011644) */
/* WARNING: Removing unreachable block (ram,0x01011648) */
/* WARNING: Removing unreachable block (ram,0x0101164c) */
/* WARNING: Removing unreachable block (ram,0x01011650) */
/* WARNING: Removing unreachable block (ram,0x01011654) */
/* WARNING: Removing unreachable block (ram,0x01011f08) */
/* WARNING: Removing unreachable block (ram,0x01011ad0) */
/* WARNING: Removing unreachable block (ram,0x010122fc) */
/* WARNING: Removing unreachable block (ram,0x01011adc) */
/* WARNING: Removing unreachable block (ram,0x010122e0) */
/* WARNING: Removing unreachable block (ram,0x010122ec) */
/* WARNING: Removing unreachable block (ram,0x010122c8) */
/* WARNING: Removing unreachable block (ram,0x010123b8) */
/* WARNING: Removing unreachable block (ram,0x01012390) */
/* WARNING: Removing unreachable block (ram,0x01011af0) */
/* WARNING: Removing unreachable block (ram,0x01011af8) */
/* WARNING: Removing unreachable block (ram,0x010120ec) */

uint * FUN_0101130c(undefined4 param_1,undefined4 param_2,byte *param_3,uint *param_4,
                   undefined4 param_5)

{
  undefined1 *puVar1;
  uint *puVar2;
  undefined1 uVar3;
  char cVar4;
  byte bVar5;
  char extraout_r2;
  byte *pbVar6;
  undefined4 *puVar7;
  uint uVar8;
  uint uVar9;
  char *pcVar10;
  undefined1 *puVar11;
  uint uVar12;
  byte *unaff_r6;
  byte *pbVar13;
  int iVar14;
  byte *pbVar15;
  bool bVar16;
  undefined8 uVar17;
  uint local_118;
  uint *local_114;
  byte *local_10c;
  uint *local_108;
  int local_104;
  int local_100;
  byte *local_fc;
  uint local_f8;
  int local_f4;
  byte *local_f0;
  int local_e8;
  int local_dc;
  undefined1 auStack_d8 [4];
  uint *local_d4;
  byte local_8c [98];
  byte local_2a [6];
  
  local_114 = (uint *)0x0;
  local_f8 = 0;
  local_d4 = param_4;
  puVar2 = (uint *)FUN_01012ae4(auStack_d8,0,4);
  local_108 = param_4;
  pbVar15 = param_3;
LAB_01011374:
  pbVar13 = param_3;
  uVar12 = (uint)*pbVar13;
  param_3 = pbVar13 + 1;
  if (uVar12 != 0) goto code_r0x01011388;
  if (pbVar15 == pbVar13) {
    return local_114;
  }
  goto LAB_010117d0;
code_r0x01011388:
  if (uVar12 == 0x25) {
    if (pbVar15 == pbVar13) goto LAB_0101139c;
    uVar12 = 1;
LAB_010117d0:
    unaff_r6 = pbVar13 + -(int)pbVar15;
    if (((int)unaff_r6 < 0) || (0x7fffffff - (int)local_114 < (int)unaff_r6)) {
LAB_0101186c:
      local_114 = (uint *)0xffffffff;
      puVar2 = local_114;
switchD_010113c4_caseD_a:
      return puVar2;
    }
    do {
      pbVar6 = pbVar15 + 1;
      puVar2 = (uint *)FUN_01010c44(param_1,param_2,*pbVar15,param_5);
      pbVar15 = pbVar6;
    } while (pbVar13 != pbVar6);
    local_114 = (uint *)((int)local_114 + (int)unaff_r6);
    if (uVar12 == 0) {
      return local_114;
    }
    param_3 = pbVar13 + 1;
switchD_010113c4_caseD_11:
LAB_0101139c:
    uVar8 = (uint)pbVar13[1];
    local_118 = 0;
    local_104 = 0;
    local_fc = (byte *)0x0;
LAB_010113b4:
    param_3 = param_3 + 1;
LAB_010113bc:
    pbVar6 = (byte *)(uVar8 - 0x20);
    uVar9 = uVar8;
    pbVar15 = param_3;
    switch(pbVar6) {
    case (byte *)0x0:
      goto switchD_010113c4_caseD_0;
    case (byte *)0x1:
    case (byte *)0x2:
    case (byte *)0x4:
    case (byte *)0x5:
    case (byte *)0x6:
    case (byte *)0x8:
    case (byte *)0x9:
    case (byte *)0xc:
    case (byte *)0xf:
    case (byte *)0x1a:
    case (byte *)0x1b:
    case (byte *)0x1c:
    case (byte *)0x1d:
    case (byte *)0x1e:
    case (byte *)0x1f:
    case (byte *)0x20:
    case (byte *)0x21:
    case (byte *)0x22:
    case (byte *)0x23:
    case (byte *)0x25:
    case (byte *)0x26:
    case (byte *)0x27:
    case (byte *)0x28:
    case (byte *)0x29:
    case (byte *)0x2a:
    case (byte *)0x2b:
    case (byte *)0x2c:
    case (byte *)0x2d:
    case (byte *)0x2e:
    case (byte *)0x30:
    case (byte *)0x31:
    case (byte *)0x32:
    case (byte *)0x33:
    case (byte *)0x34:
    case (byte *)0x36:
    case (byte *)0x37:
    case (byte *)0x39:
    case (byte *)0x3a:
    case (byte *)0x3b:
    case (byte *)0x3c:
    case (byte *)0x3d:
    case (byte *)0x3e:
    case (byte *)0x3f:
    case (byte *)0x40:
    case (byte *)0x41:
    case (byte *)0x42:
    case (byte *)0x45:
    case (byte *)0x46:
    case (byte *)0x47:
    case (byte *)0x4b:
    case (byte *)0x4d:
    case (byte *)0x52:
    case (byte *)0x56:
    case (byte *)0x57:
    case (byte *)0x59:
      break;
    case (byte *)0x3:
      goto LAB_010113b4;
    case (byte *)0x7:
      goto LAB_010113b4;
    case (byte *)0xa:
      goto switchD_010113c4_caseD_a;
    case (byte *)0xb:
      goto switchD_010113c4_caseD_b;
    case (byte *)0xd:
      local_118 = 4;
      uVar8 = (uint)*param_3;
      local_fc = pbVar6;
      goto switchD_010113c4_caseD_b;
    case (byte *)0xe:
      goto LAB_010113b4;
    case (byte *)0x10:
      pbVar13 = (byte *)*local_108;
      uVar12 = 0;
      uVar9 = 2;
      local_100 = 0;
      local_108 = local_108 + 1;
      break;
    case (byte *)0x11:
    case (byte *)0x12:
    case (byte *)0x13:
    case (byte *)0x14:
    case (byte *)0x15:
    case (byte *)0x16:
    case (byte *)0x17:
    case (byte *)0x18:
    case (byte *)0x19:
      goto switchD_010113c4_caseD_11;
    case (byte *)0x24:
      goto switchD_010113c4_caseD_24;
    case (byte *)0x2f:
      break;
    case (byte *)0x35:
                    /* WARNING: Bad instruction - Truncating control flow here */
      halt_baddata();
    case (byte *)0x38:
      goto switchD_010113c4_caseD_38;
    case (byte *)0x43:
      goto LAB_01011bd0;
    case (byte *)0x44:
    case (byte *)0x49:
      if ((uVar8 & 0x20) == 0) {
        if (((uVar8 & 0x10) == 0) && ((uVar8 & 0x40) != 0)) {
          pbVar13 = (byte *)(int)(short)*local_108;
          uVar12 = (int)pbVar13 >> 0x1f;
          local_108 = local_108 + 1;
        }
        else {
          pbVar13 = (byte *)*local_108;
          uVar12 = (int)pbVar13 >> 0x1f;
          local_108 = local_108 + 1;
        }
      }
      else {
        puVar7 = (undefined4 *)((uint)((int)local_108 + 7) & 0xfffffff8);
        puVar2 = puVar7 + 2;
        pbVar13 = (byte *)*puVar7;
        uVar12 = puVar7[1];
        local_108 = puVar2;
      }
      if ((int)uVar12 < 0) {
        bVar16 = pbVar13 != (byte *)0x0;
        pbVar13 = (byte *)-(int)pbVar13;
        uVar12 = -(uVar12 + bVar16);
        local_100 = 0;
        local_104 = 0x2d;
        uVar9 = 1;
        local_118 = uVar8;
        goto switchD_010113c4_caseD_24;
      }
      local_100 = 0;
      local_118 = uVar8;
      goto LAB_01012040;
    case (byte *)0x48:
      goto LAB_010113b4;
    case (byte *)0x4a:
      goto LAB_010113b4;
    case (byte *)0x4c:
      goto LAB_01011374;
    case (byte *)0x4e:
      goto switchD_010113c4_caseD_4e;
    case (byte *)0x4f:
      uVar9 = local_f8;
      if ((uVar8 & 0x20) == 0) {
        if (((uVar8 & 0x10) == 0) && ((uVar8 & 0x40) != 0)) {
          uVar12 = 0;
          pbVar13 = (byte *)(*local_108 & 0xffff);
          local_100 = 0;
          local_118 = uVar8;
          local_108 = local_108 + 1;
        }
        else {
          uVar12 = 0;
          uVar9 = 0;
          pbVar13 = (byte *)*local_108;
          local_100 = 0;
          local_118 = uVar8;
          local_108 = local_108 + 1;
        }
      }
      else {
        puVar7 = (undefined4 *)((uint)((int)local_108 + 7) & 0xfffffff8);
        pbVar13 = (byte *)*puVar7;
        uVar12 = puVar7[1];
        local_100 = 0;
        local_118 = uVar8;
        local_108 = puVar7 + 2;
      }
      break;
    case (byte *)0x50:
      goto LAB_010113b4;
    case (byte *)0x51:
      goto LAB_010113b4;
    case (byte *)0x53:
      goto LAB_010113b4;
    case (byte *)0x54:
      break;
    case (byte *)0x55:
      if ((uVar8 & 0x20) == 0) {
        if (((uVar8 & 0x10) == 0) && ((uVar8 & 0x40) != 0)) {
          uVar12 = 0;
          pbVar13 = (byte *)(*local_108 & 0xffff);
          local_100 = 0;
          uVar9 = 1;
          local_118 = uVar8;
          local_108 = local_108 + 1;
        }
        else {
          uVar12 = 0;
          pbVar13 = (byte *)*local_108;
          local_100 = 0;
          uVar9 = 1;
          local_118 = uVar8;
          local_108 = local_108 + 1;
        }
      }
      else {
        puVar7 = (undefined4 *)((uint)((int)local_108 + 7) & 0xfffffff8);
        local_100 = 0;
        pbVar13 = (byte *)*puVar7;
        uVar12 = puVar7[1];
        uVar9 = 1;
        local_118 = uVar8;
        local_108 = puVar7 + 2;
      }
      break;
    case (byte *)0x58:
      goto LAB_010113b4;
    case (byte *)0x5a:
      break;
    default:
      if (uVar8 == 0) {
        return local_114;
      }
      unaff_r6 = local_8c;
      local_10c = (byte *)0x1;
      local_f4 = -1;
      local_f0 = (byte *)0x1;
      local_8c[0] = (byte)uVar8;
LAB_01011bd0:
      local_100 = 0;
switchD_010113c4_caseD_38:
      local_104 = 0;
      goto LAB_010116f0;
    }
    local_104 = 0;
switchD_010113c4_caseD_24:
    if (uVar9 == 1) {
LAB_01012040:
      uVar17 = CONCAT44(uVar12,pbVar13);
      if (uVar12 == 0 && pbVar13 < (byte *)0xa) {
        pbVar6 = local_2a + 2;
      }
      else {
        pbVar6 = local_2a + 2;
        do {
          iVar14 = (int)((ulonglong)uVar17 >> 0x20);
          uVar12 = (uint)uVar17;
          FUN_01028d30(uVar12,iVar14,10,0);
          pbVar6 = pbVar6 + -1;
          *pbVar6 = extraout_r2 + 0x30;
          uVar17 = FUN_01028d30(uVar12,iVar14,10,0);
          pbVar13 = (byte *)uVar17;
        } while (iVar14 != 0 || 99 < uVar12);
      }
      puVar2 = (uint *)(pbVar13 + 0x30);
      unaff_r6 = pbVar6 + -1;
      pbVar6[-1] = (byte)puVar2;
    }
    else if (uVar9 == 2) {
      unaff_r6 = local_2a + 2;
      do {
        uVar8 = (uint)pbVar13 & 0xf;
        pbVar13 = (byte *)((uint)pbVar13 >> 4 | uVar12 << 0x1c);
        uVar12 = uVar12 >> 4;
        unaff_r6 = unaff_r6 + -1;
        *unaff_r6 = *(byte *)(local_dc + uVar8);
      } while (pbVar13 != (byte *)0x0 || uVar12 != 0);
    }
    else {
      unaff_r6 = local_2a + 2;
      do {
        pbVar6 = unaff_r6;
        iVar14 = ((uint)pbVar13 & 7) + 0x30;
        pbVar13 = (byte *)((uint)pbVar13 >> 3 | uVar12 << 0x1d);
        uVar12 = uVar12 >> 3;
        pbVar6[-1] = (byte)iVar14;
        unaff_r6 = pbVar6 + -1;
      } while (pbVar13 != (byte *)0x0 || uVar12 != 0);
      if (iVar14 == 0x30) {
        uVar12 = 0;
      }
      else {
        uVar12 = local_118 & 1;
      }
      if (uVar12 != 0) {
        pbVar6[-2] = 0x30;
        unaff_r6 = pbVar6 + -2;
      }
    }
    local_f0 = local_2a + (2 - (int)unaff_r6);
    if (100 < (int)local_f0) goto LAB_0101186c;
    local_f4 = -1 - (int)local_f0;
    local_10c = local_f0;
    if ((int)local_f0 < -1) {
      local_10c = (byte *)0xffffffff;
    }
    if (local_104 != 0) {
      local_10c = local_10c + 1;
    }
    if (local_100 != 0) {
      local_10c = local_10c + 2;
    }
LAB_010116f0:
    puVar1 = DAT_01012950;
    if (((local_118 & 0x84) == 0) && (local_e8 = (int)local_fc - (int)local_10c, 0 < local_e8)) {
      iVar14 = local_e8;
      if (0x10 < local_e8) {
        do {
          uVar3 = 0x20;
          for (puVar11 = DAT_01012954; FUN_01010c44(param_1,param_2,uVar3,param_5),
              puVar11 != puVar1; puVar11 = puVar11 + 1) {
            uVar3 = *puVar11;
          }
          iVar14 = iVar14 + -0x10;
        } while (0x10 < iVar14);
        local_e8 = (local_e8 + -0x10) - (local_e8 - 0x11U & 0xfffffff0);
      }
      cVar4 = ' ';
      pcVar10 = s__s__Malloc_failed_0103262c + 0x10;
      while (puVar2 = (uint *)FUN_01010c44(param_1,param_2,cVar4,param_5),
            pcVar10 != s__s__Malloc_failed_0103262c + local_e8 + 0xf) {
        pcVar10 = pcVar10 + 1;
        cVar4 = *pcVar10;
      }
    }
    if (local_104 != 0) {
      puVar2 = (uint *)FUN_01010c44(param_1,param_2,local_104,param_5);
    }
    if (local_100 != 0) {
      FUN_01010c44(param_1,param_2,0x30,param_5);
      puVar2 = (uint *)FUN_01010c44(param_1,param_2,local_100,param_5);
    }
    puVar1 = DAT_01012948;
    if (((local_118 & 0x84) == 0x80) && (local_104 = (int)local_fc - (int)local_10c, 0 < local_104))
    {
      iVar14 = local_104;
      if (0x10 < local_104) {
        do {
          uVar3 = 0x30;
          for (puVar11 = DAT_0101294c; FUN_01010c44(param_1,param_2,uVar3,param_5),
              puVar1 != puVar11; puVar11 = puVar11 + 1) {
            uVar3 = *puVar11;
          }
          iVar14 = iVar14 + -0x10;
        } while (0x10 < iVar14);
        local_104 = (local_104 + -0x10) - (local_104 - 0x11U & 0xfffffff0);
      }
      cVar4 = '0';
      pcVar10 = s_0123456789abcdef0123456789ABCDEF_01032658 + 0x1c;
      while (puVar2 = (uint *)FUN_01010c44(param_1,param_2,cVar4,param_5),
            s_0123456789abcdef0123456789ABCDEF_01032658 + local_104 + 0x1b != pcVar10) {
        pcVar10 = pcVar10 + 1;
        cVar4 = *pcVar10;
      }
    }
    puVar1 = DAT_01012948;
    if (0 < local_f4) {
      iVar14 = local_f4;
      if (0x10 < local_f4) {
        do {
          uVar3 = 0x30;
          for (puVar11 = DAT_0101294c; FUN_01010c44(param_1,param_2,uVar3,param_5),
              puVar1 != puVar11; puVar11 = puVar11 + 1) {
            uVar3 = *puVar11;
          }
          iVar14 = iVar14 + -0x10;
        } while (0x10 < iVar14);
        local_f4 = (local_f4 + -0x10) - (local_f4 - 0x11U & 0xfffffff0);
      }
      cVar4 = '0';
      pcVar10 = s_0123456789abcdef0123456789ABCDEF_01032658 + 0x1c;
      while (puVar2 = (uint *)FUN_01010c44(param_1,param_2,cVar4,param_5),
            s_0123456789abcdef0123456789ABCDEF_01032658 + local_f4 + 0x1b != pcVar10) {
        pcVar10 = pcVar10 + 1;
        cVar4 = *pcVar10;
      }
    }
    if (local_f0 != (byte *)0x0) {
      pbVar6 = unaff_r6 + (int)local_f0;
      pbVar13 = unaff_r6;
      do {
        unaff_r6 = pbVar13 + 1;
        puVar2 = (uint *)FUN_01010c44(param_1,param_2,*pbVar13,param_5);
        pbVar13 = unaff_r6;
      } while (pbVar6 != unaff_r6);
    }
    puVar1 = DAT_01012950;
    if (((local_118 & 4) != 0) && (pbVar13 = local_fc + -(int)local_10c, 0 < (int)pbVar13)) {
      pbVar6 = pbVar13;
      if (0x10 < (int)pbVar13) {
        do {
          uVar3 = 0x20;
          for (puVar11 = DAT_01012954; FUN_01010c44(param_1,param_2,uVar3,param_5),
              puVar1 != puVar11; puVar11 = puVar11 + 1) {
            uVar3 = *puVar11;
          }
          unaff_r6 = pbVar6 + -0x10;
          pbVar6 = unaff_r6;
        } while (0x10 < (int)unaff_r6);
        pbVar13 = pbVar13 + (-0x10 - ((uint)(pbVar13 + -0x11) & 0xfffffff0));
      }
      bVar5 = 0x20;
      pbVar6 = (byte *)(s__s__Malloc_failed_0103262c + 0x10);
      while (puVar2 = (uint *)FUN_01010c44(param_1,param_2,bVar5,param_5),
            pbVar13 + 0x103263b != pbVar6) {
        pbVar6 = pbVar6 + 1;
        bVar5 = *pbVar6;
      }
    }
    if ((int)local_fc < (int)local_10c) {
      local_fc = local_10c;
    }
    if (0x7fffffff - (int)local_114 < (int)local_fc) goto LAB_0101186c;
    local_114 = (uint *)((int)local_114 + (int)local_fc);
  }
  goto LAB_01011374;
switchD_010113c4_caseD_4e:
  goto LAB_010113bc;
switchD_010113c4_caseD_b:
  goto LAB_010113b4;
switchD_010113c4_caseD_0:
  goto LAB_010113b4;
}



/* @ 0x1012958 */

int FUN_01012958(int param_1,byte *param_2)

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



/* @ 0x1012980 */

int FUN_01012980(char *param_1)

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



/* @ 0x10129ac */

byte * FUN_010129ac(byte *param_1,uint param_2)

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



/* @ 0x10129dc */

void FUN_010129dc(undefined4 *param_1,undefined4 *param_2,uint param_3)

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



/* @ 0x1012ae4 */

void FUN_01012ae4(undefined1 *param_1,undefined1 param_2,int param_3)

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



/* @ 0x1012b08 */

int FUN_01012b08(byte *param_1,byte *param_2,int param_3)

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



/* @ 0x1012b40 */

char * FUN_01012b40(char *param_1,char param_2,int param_3)

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



/* @ 0x1012b84 */

undefined4 FUN_01012b84(int param_1,int param_2,undefined4 param_3)

{
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x2c,0x36);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x2c,0x36);
    FUN_01000458(0);
  }
  if (3 < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x2c,0x37);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x2c,0x37);
    FUN_01000458(0);
  }
  *(undefined4 *)(param_1 + param_2 * 0x40 + 0x28) = param_3;
  return 0;
}



/* @ 0x1012c94 */

void FUN_01012c94(int param_1,int param_2,uint param_3)

{
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x3c,0xbd);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x3c,0xbd);
    FUN_01000458(0);
  }
  if (3 < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x3c,0xbe);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,
                 s_0123456789abcdef0123456789ABCDEF_01032658 + 0x3c,0xbe);
    FUN_01000458(0);
  }
  *(uint *)(param_1 + param_2 * 0x40 + 0x18) = ~param_3;
  return;
}



/* @ 0x1012da4 */

void FUN_01012da4(int param_1,int param_2,uint param_3)

{
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,s_al_iofic_unmask_01032698 + 0xc,0x152)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,s_al_iofic_unmask_01032698 + 0xc,0x152)
    ;
    FUN_01000458(0);
  }
  if (3 < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,s_al_iofic_unmask_01032698 + 0xc,0x153)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s___regs_base__010326ec + 0xc,1,
                 s_al_iofic_abort_mask_clear_010326a8 + 0x18,s_al_iofic_unmask_01032698 + 0xc,0x153)
    ;
    FUN_01000458(0);
  }
  param_1 = param_1 + param_2 * 0x40;
  *(uint *)(param_1 + 0x30) = *(uint *)(param_1 + 0x30) & ~param_3;
  return;
}



/* @ 0x1012eb8 */

undefined4 FUN_01012eb8(int param_1,uint param_2,uint *param_3)

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
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_010327c4 + 0x24,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_init_0103272c + 0xc,
                 0x1c2);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_010327c4 + 0x24,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_init_0103272c + 0xc,
                 0x1c2);
    FUN_01000458(0);
  }
  if (param_3 == (uint *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_010327f0,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_init_0103272c + 0xc,
                 0x1c3);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_010327f0,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_init_0103272c + 0xc,
                 0x1c3);
    FUN_01000458(0);
  }
  if (param_2 < *(byte *)(param_1 + 6)) {
    iVar1 = param_2 * 0x80;
    iVar7 = param_1 + iVar1;
    if (*(char *)(iVar7 + 0x90) == '\x02') {
      FUN_0100ceb0(s_udma__invalid_queue_id___d__01032800 + 0x1c,param_2);
      FUN_01010c00(s_udma__invalid_queue_id___d__01032800 + 0x1c,param_2);
      uVar4 = 0xfffffffb;
    }
    else {
      uVar5 = *param_3;
      if (uVar5 < 4) {
        FUN_0100ceb0(s_udma__queue___d__already_enabled_01032820 + 0x20,param_2);
        FUN_01010c00(s_udma__queue___d__already_enabled_01032820 + 0x20,param_2);
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
            FUN_0100ceb0(s_vectors_01028ea4 + 4,s_udma__queue___d__size___d__must_b_0103288c + 0x2c,
                         1,s_udma__invalid_num_of_queues_para_0103279c + 0x24,&DAT_01032710,0xb3);
            FUN_01010c00(s_vectors_01028ea4 + 4,s_udma__queue___d__size___d__must_b_0103288c + 0x2c,
                         1,s_udma__invalid_num_of_queues_para_0103279c + 0x24,&DAT_01032710,0xb3);
            FUN_01000458(0);
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
              FUN_0100ceb0(s_vectors_01028ea4 + 4,s______uint32_t___al_phys_addr_t___010328bc + 0x4c
                           ,1,s_udma__invalid_num_of_queues_para_0103279c + 0x24,&DAT_01032710,0xc2)
              ;
              FUN_01010c00(s_vectors_01028ea4 + 4,s______uint32_t___al_phys_addr_t___010328bc + 0x4c
                           ,1,s_udma__invalid_num_of_queues_para_0103279c + 0x24,&DAT_01032710,0xc2)
              ;
              FUN_01000458(0);
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
          FUN_0100ceb0(s_udma__queue___d__size_too_large_01032868 + 0x20,uVar5,param_2);
          FUN_01010c00(s_udma__queue___d__size_too_large_01032868 + 0x20,*param_3,param_2);
          uVar4 = 0xffffffea;
        }
      }
      else {
        FUN_0100ceb0(s_udma__queue___d__size_too_small_01032844 + 0x20,param_2);
        FUN_01010c00(s_udma__queue___d__size_too_small_01032844 + 0x20,param_2);
        uVar4 = 0xffffffea;
      }
    }
  }
  else {
    FUN_0100ceb0(s___q_params__010327f4 + 8,param_2);
    FUN_01010c00(s___q_params__010327f4 + 8,param_2);
    uVar4 = 0xffffffea;
  }
  return uVar4;
}



/* @ 0x101332c */

bool FUN_0101332c(int param_1)

{
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_q_init_0103273c + 0xc,
                 0x216);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__include_udma_al_hal_udma_01031594 + 0x20,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,s_al_udma_q_init_0103273c + 0xc,
                 0x216);
    FUN_01000458(0);
  }
  return (*(uint *)(*(int *)(param_1 + 4) + 0x20) & 0x30000) != 0;
}



/* @ 0x10133cc */

undefined4 FUN_010133cc(int param_1,int param_2)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___desc_pref____0x3__01032998 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_is_enabled_0103274c + 0x14,0x295);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___desc_pref____0x3__01032998 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_is_enabled_0103274c + 0x14,0x295);
    FUN_01000458(0);
  }
  if (param_2 == 2) {
    uVar1 = 1;
  }
  else if (param_2 == 3) {
    uVar1 = 4;
  }
  else {
    if (param_2 != 0) {
      FUN_0100ceb0(s___udma______void___0___010329ac + 0x14,param_2);
      FUN_01010c00(s___udma______void___0___010329ac + 0x14,param_2);
      return 0xffffffea;
    }
    uVar1 = 2;
  }
  *(undefined4 *)(*(int *)(param_1 + 8) + 0x204) = uVar1;
  *(char *)(param_1 + 5) = (char)param_2;
  return 0;
}



/* @ 0x10134c0 */

undefined4 FUN_010134c0(int param_1)

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
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s______uint32_t___al_phys_addr_t___0103290c + 0x4c,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d4);
    FUN_01010c00(s_vectors_01028ea4 + 4,s______uint32_t___al_phys_addr_t___0103290c + 0x4c,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d4);
    FUN_01000458(0);
  }
  if (uVar4 == 3) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___comp_ctrl____0x3__0103295c + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d5);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___comp_ctrl____0x3__0103295c + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d5);
    FUN_01000458(0);
  }
  if (uVar2 == 3) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___stream_if____0x3__01032970 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d6);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___stream_if____0x3__01032970 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d6);
    FUN_01000458(0);
  }
  if (uVar3 == 3) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___data_rd____0x3__01032984 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d7);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___data_rd____0x3__01032984 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_state_set_01032764 + 0x10,0x2d7);
    FUN_01000458(0);
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



/* @ 0x10136e4 */

undefined4 FUN_010136e4(undefined4 *param_1,int *param_2,int param_3)

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
      FUN_0100ceb0(s___udma_params__0103278c + 0xc);
      FUN_01010c00(s___udma_params__0103278c + 0xc);
      return 0xffffffea;
    }
  }
  iVar5 = *param_2;
  pcVar4 = (char *)param_2[2];
  pcVar3 = s_Failed_to_get_BARs_0103157c + 0x10;
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
    uVar2 = FUN_010134c0(param_1);
    iVar7 = param_1[2];
  }
  iVar9 = 0x1000;
  *(undefined1 *)((int)param_1 + 5) = uVar2;
  puVar8 = param_1 + 0x10;
  while( true ) {
    puVar8[1] = iVar7 + iVar9;
    puVar8[0x15] = param_1;
    if ((param_3 == 0) || (iVar7 = FUN_0101332c(puVar8), iVar7 == 0)) {
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



/* @ 0x1013818 */


int FUN_01013818(int param_1,undefined4 *param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_010327c4 + 0x24,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_set_pointers_01032714 + 0x14,0x1a5);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_010327c4 + 0x24,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_set_pointers_01032714 + 0x14,0x1a5);
    FUN_01000458(0);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_udma_state_get_01032778 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_set_pointers_01032714 + 0x14,0x1a6);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_udma_state_get_01032778 + 0x10,1,
                 s_udma__invalid_num_of_queues_para_0103279c + 0x24,
                 s_al_udma_q_set_pointers_01032714 + 0x14,0x1a6);
    FUN_01000458(0);
  }
  iVar1 = FUN_010136e4(param_1,param_2,0);
  if (iVar1 != 0) {
    return iVar1;
  }
  if (*(char *)(param_1 + 4) == '\0') {
    iVar1 = *(int *)(param_1 + 8);
    local_20 = ram0x01032708;
    uStack_1c = DAT_0103270c;
    if (1 < *(byte *)(param_1 + 0x240)) {
      *(uint *)(iVar1 + 0x310) = *(uint *)(iVar1 + 0x310) & 0xfffffc00 | 0x100;
    }
    *(undefined4 *)(iVar1 + 0x1e280) = 1000000;
    *(undefined4 *)(iVar1 + 0x408) = 0;
    FUN_010139dc(param_1,&local_20);
    if (*(char *)(param_1 + 4) != '\x01') {
      uVar2 = *param_2;
      if (*(char *)(param_1 + 4) == '\0') {
        FUN_01013d6c();
        return 0;
      }
      goto LAB_01013908;
    }
  }
  else if (*(char *)(param_1 + 4) != '\x01') {
    uVar2 = *param_2;
    goto LAB_01013908;
  }
  uVar2 = *param_2;
  *(undefined4 *)(*(int *)(param_1 + 8) + 0x38c) = 0;
LAB_01013908:
  FUN_01013e50(uVar2);
  return 0;
}



/* @ 0x10139dc */

undefined4 FUN_010139dc(undefined4 *param_1,uint *param_2)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = *(uint *)(param_1[2] + 0x254);
  if (*(char *)(param_1 + 1) != '\0') {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_01032a60 + 0x28,1,
                 s___max_descs_>_0__01032a4c + 0x10,s_udma__invalid_state___d__010329c4 + 0x18,0x145
                );
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_udma_al_hal_udma_01032a60 + 0x28,1,
                 s___max_descs_>_0__01032a4c + 0x10,s_udma__invalid_state___d__010329c4 + 0x18,0x145
                );
    FUN_01000458(0);
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
    FUN_0100ceb0(s___udma_>type____UDMA_TX__01032a8c + 0x18,*param_1,uVar2,0xfffff);
    FUN_01010c00(s___udma_>type____UDMA_TX__01032a8c + 0x18,*param_1,*param_2,0xfffff);
    uVar1 = 0xffffffea;
  }
  return uVar1;
}



/* @ 0x1013af4 */

undefined4 FUN_01013af4(int param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  
  if (param_2 < 0x20) {
    if (param_2 == 0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s___max_descs_<__31)_01032a38 + 0x10,1,
                   s___max_descs_>_0__01032a4c + 0x10,
                   s_al_udma_m2s_packet_size_cfg_set_010329e0 + 0x1c,0x1d5);
      FUN_01010c00(s_vectors_01028ea4 + 4,s___max_descs_<__31)_01032a38 + 0x10,1,
                   s___max_descs_>_0__01032a4c + 0x10,
                   s_al_udma_m2s_packet_size_cfg_set_010329e0 + 0x1c,0x1d5);
      FUN_01000458(0);
      uVar1 = 0x440;
      goto LAB_01013b2c;
    }
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_udma_s2m_max_descs_set_01032a1c + 0x18,1,
                 s___max_descs_>_0__01032a4c + 0x10,
                 s_al_udma_m2s_packet_size_cfg_set_010329e0 + 0x1c,0x1d4);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_udma_s2m_max_descs_set_01032a1c + 0x18,1,
                 s___max_descs_>_0__01032a4c + 0x10,
                 s_al_udma_m2s_packet_size_cfg_set_010329e0 + 0x1c,0x1d4);
    FUN_01000458(0);
  }
  if (param_2 < 8) {
    uVar1 = 0x440;
  }
  else {
    uVar1 = param_2 << 8 | 0x80;
  }
LAB_01013b2c:
  iVar2 = *(int *)(param_1 + 8);
  *(uint *)(iVar2 + 0x304) = (param_2 ^ *(uint *)(iVar2 + 0x304)) & 0x1f ^ *(uint *)(iVar2 + 0x304);
  *(uint *)(iVar2 + 0x308) = *(uint *)(iVar2 + 0x308) & 0xffff000f | uVar1;
  return 0;
}



/* @ 0x1013c34 */

undefined4 FUN_01013c34(int param_1,uint param_2)

{
  uint uVar1;
  
  if (param_2 < 0x20) {
    if (param_2 == 0) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s___max_descs_<__31)_01032a38 + 0x10,1,
                   s___max_descs_>_0__01032a4c + 0x10,s_al_udma_m2s_max_descs_set_01032a00 + 0x18,
                   500);
      FUN_01010c00(s_vectors_01028ea4 + 4,s___max_descs_<__31)_01032a38 + 0x10,1,
                   s___max_descs_>_0__01032a4c + 0x10,s_al_udma_m2s_max_descs_set_01032a00 + 0x18,
                   500);
      uVar1 = 0x440;
      FUN_01000458(0);
    }
    else if (param_2 < 8) {
      uVar1 = 0x440;
    }
    else {
      uVar1 = param_2 << 8 | 0x80;
    }
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_udma_s2m_max_descs_set_01032a1c + 0x18,1,
                 s___max_descs_>_0__01032a4c + 0x10,s_al_udma_m2s_max_descs_set_01032a00 + 0x18,499)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_udma_s2m_max_descs_set_01032a1c + 0x18,1,
                 s___max_descs_>_0__01032a4c + 0x10,s_al_udma_m2s_max_descs_set_01032a00 + 0x18,499)
    ;
    uVar1 = param_2 << 8 | 0x80;
    FUN_01000458(0);
  }
  *(uint *)(*(int *)(param_1 + 8) + 0x308) =
       *(uint *)(*(int *)(param_1 + 8) + 0x308) & 0xffff000f | uVar1;
  return 0;
}



/* @ 0x1013d6c */

void FUN_01013d6c(int param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b18 + 0x20,
                 s_udma___s___requested_max_pkt_siz_01032aa8 + 0x48,0x95);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b18 + 0x20,
                 s_udma___s___requested_max_pkt_siz_01032aa8 + 0x48,0x95);
    FUN_01000458(0);
  }
  iVar1 = param_1 + 0x1e000;
  FUN_01012b84(iVar1,0,0x28);
  FUN_01012b84(param_1 + 0x1c000,3,0x28);
  FUN_01012da4(iVar1,0,0xfffffff);
  FUN_01012c94(iVar1,0,0xfffffff);
  FUN_01012c94(param_1 + 0x1c000,3,0x100);
  return;
}



/* @ 0x1013e50 */

void FUN_01013e50(int param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b18 + 0x20,
                 s_al_udma_iofic_m2s_error_ints_unm_01032af4 + 0x20,0xbb);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s_al_udma_iofic_s2m_error_ints_unm_01032b18 + 0x20,
                 s_al_udma_iofic_m2s_error_ints_unm_01032af4 + 0x20,0xbb);
    FUN_01000458(0);
  }
  iVar1 = param_1 + 0x1e000;
  FUN_01012b84(iVar1,1,0x28);
  FUN_01012b84(param_1 + 0x1c000,3,0x28);
  FUN_01012da4(iVar1,1,0x7fffef07);
  FUN_01012c94(iVar1,1,0x7fffef07);
  FUN_01012c94(param_1 + 0x1c000,3,0x200);
  return;
}



/* @ 0x1013f3c */

int FUN_01013f3c(undefined4 *param_1,undefined4 *param_2)

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
  local_1c = s___al_udma_iofic_level_and_group__01032b68 + 0x34;
  iVar1 = FUN_01013818(param_1 + 0x10,&local_24);
  if (iVar1 == 0) {
    FUN_01013af4(param_1 + 0x10,*(undefined1 *)((int)param_2 + 9));
    local_1f = *(undefined1 *)(param_1 + 2);
    local_24 = param_1[1];
    local_1c = s_failed_to_initialize__s__error___01032ba8 + 0x20;
    local_20 = 1;
    iVar1 = FUN_01013818(param_1 + 0xb0,&local_24);
    if (iVar1 == 0) {
      FUN_01013c34(param_1 + 0xb0,*(undefined1 *)((int)param_2 + 10));
      return 0;
    }
  }
  FUN_0100ceb0(&DAT_01032ba4,local_1c,iVar1);
  FUN_01010c00(&DAT_01032ba4,local_1c,iVar1);
  return iVar1;
}



/* @ 0x1014038 */

int FUN_01014038(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  iVar1 = FUN_01012eb8(param_1 + 0x10);
  if (iVar1 == 0) {
    iVar1 = FUN_01012eb8(param_1 + 0xb0,param_2,param_4);
    if (iVar1 != 0) {
      FUN_0100ceb0(s___s___failed_to_initialize_tx_q___01032bd4 + 0x2c,*param_1,param_2,iVar1);
      FUN_01010c00(s___s___failed_to_initialize_tx_q___01032bd4 + 0x2c,*param_1,param_2,iVar1);
    }
  }
  else {
    FUN_0100ceb0(&DAT_01032bd0,*param_1,param_2,iVar1);
    FUN_01010c00(&DAT_01032bd0,*param_1,param_2,iVar1);
  }
  return iVar1;
}



/* @ 0x10140fc */

int FUN_010140fc(undefined4 *param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = FUN_010133cc(param_1 + 0x10);
  if ((iVar1 != 0) || (iVar1 = FUN_010133cc(param_1 + 0xb0,param_2), iVar1 != 0)) {
    FUN_0100ceb0(s_ssm___s___failed_to_change_state_01032c8c,*param_1,iVar1);
    FUN_01010c00(s_ssm___s___failed_to_change_state_01032c8c,*param_1,iVar1);
  }
  return iVar1;
}



/* @ 0x1014174 */

undefined4 FUN_01014174(int param_1,int param_2,int *param_3)

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



/* @ 0x101418c */

int FUN_0101418c(undefined1 *param_1,undefined1 *param_2)

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
  iVar1 = FUN_01013f3c(param_1 + 0x40,&local_1c);
  if (iVar1 != 0) {
    FUN_0100ceb0(s___s___failed_to_initialize_rx_q___01032c04 + 0x2c,iVar1);
    FUN_01010c00(s___s___failed_to_initialize_rx_q___01032c04 + 0x2c,iVar1);
  }
  iVar2 = iVar2 + 0x1e000;
  FUN_01012c94(iVar2,1,0x40);
  FUN_01012da4(iVar2,1,0x40);
  return iVar1;
}



/* @ 0x1014234 */

int FUN_01014234(undefined1 *param_1,int param_2,int param_3,int param_4,undefined1 param_5)

{
  undefined1 uVar1;
  int iVar2;
  
  uVar1 = *param_1;
  *(undefined1 *)(param_3 + 0x24) = uVar1;
  *(undefined1 *)(param_4 + 0x24) = uVar1;
  iVar2 = FUN_01014038(param_1 + 0x40);
  if (iVar2 == 0) {
    param_1[param_2 + 0x580] = param_5;
  }
  else {
    FUN_0100ceb0(s_failed_to_initialize_udma__error_01032c34 + 0x24,*(undefined4 *)(param_1 + 0x40),
                 param_2,iVar2);
    FUN_01010c00(s_failed_to_initialize_udma__error_01032c34 + 0x24,*(undefined4 *)(param_1 + 0x40),
                 param_2,iVar2);
  }
  return iVar2;
}



/* @ 0x10142bc */

int FUN_010142bc(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_010140fc(param_1 + 0x40);
  if (iVar1 != 0) {
    FUN_0100ceb0(s_ssm___s___failed_to_initialize_q_01032c5c + 0x2c,*(undefined4 *)(param_1 + 0x40),
                 iVar1);
    FUN_01010c00(s_ssm___s___failed_to_initialize_q_01032c5c + 0x2c,*(undefined4 *)(param_1 + 0x40),
                 iVar1);
  }
  return iVar1;
}



/* @ 0x101431c */

int FUN_0101431c(int param_1,int param_2)

{
  int iVar1;
  int local_c;
  
  iVar1 = FUN_01014174(param_1 + 0x40,0,&local_c);
  if (iVar1 == 0) {
    local_c = local_c + param_2 * 0x80 + 0x40;
  }
  else {
    local_c = 0;
  }
  return local_c;
}



/* @ 0x1014368 */

int FUN_01014368(int param_1,int param_2)

{
  int iVar1;
  int local_c;
  
  iVar1 = FUN_01014174(param_1 + 0x40,1,&local_c);
  if (iVar1 == 0) {
    local_c = local_c + param_2 * 0x80 + 0x40;
  }
  else {
    local_c = 0;
  }
  return local_c;
}



/* @ 0x10146d0 */

void FUN_010146d0(int *param_1,uint param_2,int param_3,uint param_4,byte param_5)

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



/* @ 0x101658c */

undefined4 FUN_0101658c(int *param_1,uint param_2,uint param_3,uint param_4,byte param_5)

{
  int iVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__eye_measure_timeout__01032fe0 + 0x18,s_0al_serdes_reg_read_01032cce + 0x12,
                 0xe0);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__eye_measure_timeout__01032fe0 + 0x18,s_0al_serdes_reg_read_01032cce + 0x12,
                 0xe0);
    FUN_01000458(0);
  }
  if (7 < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_0103302c,1,s__s__eye_measure_timeout__01032fe0 + 0x18,
                 s_0al_serdes_reg_read_01032cce + 0x12,0xe2);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_0103302c,1,s__s__eye_measure_timeout__01032fe0 + 0x18,
                 s_0al_serdes_reg_read_01032cce + 0x12,0xe2);
    FUN_01000458(0);
  }
  if (1 < param_3) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_____int_page__<__AL_SRDS_REG_PAG_01033224 + 0x28,1,
                 s__s__eye_measure_timeout__01032fe0 + 0x18,s_0al_serdes_reg_read_01032cce + 0x12,
                 0xe4);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_____int_page__<__AL_SRDS_REG_PAG_01033224 + 0x28,1,
                 s__s__eye_measure_timeout__01032fe0 + 0x18,s_0al_serdes_reg_read_01032cce + 0x12,
                 0xe4);
    FUN_01000458(0);
  }
  iVar1 = *param_1;
  *(uint *)(iVar1 + 0x110) = param_4 | param_3 << 0xc | param_2 << 0xd;
  *(uint *)(iVar1 + 0x114) = (uint)param_5;
  return 0;
}



/* @ 0x1017b8c */

void FUN_01017b8c(int param_1,int *param_2)

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



/* @ 0x10194b4 */

uint FUN_010194b4(uint param_1)

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




/* @ 0x10194d0 -- WARNING: Control flow encountered bad instruction data */

int * FUN_010194d0(int *param_1,undefined4 param_2,undefined4 param_3,int param_4,byte param_5)

{
  int iVar1;
  
  switch(param_2) {
  case 0:
  case 1:
    goto switchD_010194e0_caseD_0;
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
    break;
  case 4:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 8:
    param_4 = param_4 + 0xc00;
    break;
  default:
    FUN_0100ceb0(s__s__Unable_to_determine_the_serd_01033e60 + 0x38,
                 s_al_serdes_25g_reg_read_010332a0 + 0x14);
    FUN_01010c00(s__s__Unable_to_determine_the_serd_01033e60 + 0x38,
                 s_al_serdes_25g_reg_read_010332a0 + 0x14,param_3);
    return (int *)0xffffffff;
  }
  iVar1 = *param_1;
  param_1 = (int *)0x0;
  *(int *)(iVar1 + 0x110) = param_4;
  *(uint *)(iVar1 + 0x114) = param_5 | 0xff00;
switchD_010194e0_caseD_0:
  return param_1;
}



/* @ 0x101957c */

int * FUN_0101957c(int *param_1,undefined4 param_2,undefined4 param_3,int param_4,
                  undefined1 *param_5)

{
  int iVar1;
  
  iVar1 = *param_1;
  if (param_5 == (undefined1 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s__s__invalid_mode___d__01033204 + 0x14,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s__s__invalid_rate___d__01033288 + 0x14,0x67);
    FUN_01010c00(s_vectors_01028ea4 + 4,s__s__invalid_mode___d__01033204 + 0x14,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s__s__invalid_rate___d__01033288 + 0x14,0x67);
    param_1 = (int *)FUN_01000458(0);
  }
  switch(param_2) {
  case 0:
  case 1:
    goto switchD_010195b4_caseD_0;
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
    break;
  case 4:
  case 8:
    param_4 = param_4 + 0xc00;
    break;
  default:
    FUN_0100ceb0(s__s__Unable_to_determine_the_serd_01033e60 + 0x38,
                 s__s__invalid_rate___d__01033288 + 0x14,param_3);
    FUN_01010c00(s__s__Unable_to_determine_the_serd_01033e60 + 0x38,
                 s__s__invalid_rate___d__01033288 + 0x14,param_3);
    param_1 = (int *)0xffffffff;
    goto switchD_010195b4_caseD_0;
  }
  *(int *)(iVar1 + 0x110) = param_4;
  param_1 = (int *)0x0;
  *param_5 = (char)*(undefined4 *)(iVar1 + 0x114);
switchD_010195b4_caseD_0:
  return param_1;
}



/* @ 0x10196b4 */

void FUN_010196b4(undefined4 param_1,undefined4 param_2,undefined4 param_3,uint param_4,
                 sbyte param_5,byte param_6)

{
  int iVar1;
  uint uVar2;
  byte local_21 [5];
  
  iVar1 = FUN_0101957c(param_1,param_2,0,param_3,local_21);
  if (iVar1 == 0) {
    uVar2 = (uint)local_21[0];
    local_21[0] = (byte)(uVar2 & ~param_4) | (byte)((uint)param_6 << param_5);
    FUN_010194d0(param_1,param_2,0,param_3,uVar2 & ~param_4 | (uint)param_6 << param_5 & 0xff);
  }
  return;
}



/* @ 0x1019944 */

void FUN_01019944(undefined4 param_1,undefined1 param_2)

{
  FUN_010196b4(param_1,4,0x109,8,3,param_2);
  FUN_010196b4(param_1,4,0x11a,1,0,param_2);
  FUN_010196b4(param_1,4,0x114,1,0,param_2);
  FUN_010196b4(param_1,4,0x111,1,0,param_2);
  return;
}



/* @ 0x1019be4 */

void FUN_01019be4(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int iVar2;
  char cVar3;
  undefined4 uVar4;
  char local_21 [5];
  
  FUN_01019944(param_1,1);
  iVar1 = FUN_0101957c(param_1,4,0,0x122,local_21);
  cVar3 = local_21[0];
  if (iVar1 != 0) {
    cVar3 = '\0';
  }
  FUN_010196b4(param_1,4,0x122,0xff,0,param_2);
  if (param_3 == 0) {
    uVar4 = 3;
  }
  else {
    uVar4 = 0xc;
  }
  iVar1 = 500;
  FUN_010196b4(param_1,4,0x133,0xf,0,uVar4);
  FUN_010196b4(param_1,4,0x133,0x10,4,1);
  while ((iVar2 = FUN_0101957c(param_1,4,0,0x133,local_21), iVar2 != 0 || (-1 < local_21[0]))) {
    (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),1);
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) {
      FUN_0100ceb0(s__s__Error_on_reading_RX_valid_01034334 + 0x1c,
                   s_al_serdes_25g_rx_diag_info_get_010334e0 + 0x1c);
      FUN_01010c00(s__s__Error_on_reading_RX_valid_01034334 + 0x1c,
                   s_al_serdes_25g_rx_diag_info_get_010334e0 + 0x1c);
LAB_01019d04:
      FUN_01019944(param_1,0);
      FUN_010196b4(param_1,4,0x122,0xff,0,cVar3);
      return;
    }
  }
  FUN_010196b4(param_1,4,0x133,0x10,4,0);
  goto LAB_01019d04;
}



/* @ 0x1019f68 */

undefined4 FUN_01019f68(int *param_1,uint param_2,int param_3,uint param_4,ushort *param_5)

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
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s__s__timeout_waiting_for_pll_fcal_01034354 + 0x28,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s_al_serdes_25g_rx_leq_fsm_op_01033464 + 0x18,0x61b);
    FUN_01010c00(s_vectors_01028ea4 + 4,s__s__timeout_waiting_for_pll_fcal_01034354 + 0x28,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s_al_serdes_25g_rx_leq_fsm_op_01033464 + 0x18,0x61b);
    FUN_01000458(0);
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
  iVar1 = FUN_0101957c(param_1,8,0,0x1b1,local_29);
  if (iVar1 == 0) {
    uVar6 = (ushort)local_29[0];
  }
  if (iVar1 == 0) {
    uVar6 = uVar6 & 0xf;
  }
  *param_5 = (ushort)uVar4 & 0xff | uVar6 << 8;
  return 0;
}



/* @ 0x101a0b0 */

undefined4 FUN_0101a0b0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  char cVar3;
  byte bVar4;
  byte local_19 [5];
  
  (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),100);
  iVar1 = FUN_0101957c(param_1,4,0,0x106,local_19);
  if (iVar1 == 0) {
    cVar3 = 'd';
    bVar4 = local_19[0] & 1;
    do {
      iVar1 = FUN_0101957c(param_1,4,0,0x106,local_19);
      if (iVar1 != 0) {
        uVar2 = 0x946;
        goto LAB_0101a170;
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
LAB_0101a170:
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s_al_serdes_25g_single_iteration_d_01033500 + 0x24,uVar2);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                 s__s__Got_timeout_waiting_for_serd_01033d94 + 0x44,
                 s_al_serdes_25g_single_iteration_d_01033500 + 0x24,uVar2);
    FUN_01000458(0);
    uVar2 = 0;
  }
  return uVar2;
}



/* @ 0x101a1d8 */

int FUN_0101a1d8(int param_1,undefined4 param_2,undefined4 param_3,ushort *param_4)

{
  int iVar1;
  int iVar2;
  byte local_22;
  byte local_21 [5];
  
  iVar2 = 0x1e8481;
  FUN_010196b4(param_1,param_2,0x580,1,0,0);
  FUN_010196b4(param_1,param_2,0x581,7,0,0);
  FUN_010194d0(param_1,param_2,0,0x582,param_3);
  FUN_010196b4(param_1,param_2,0x580,1,0,1);
  while ((iVar1 = FUN_0101957c(param_1,param_2,0,0x583,local_21), iVar1 != 0 ||
         ((local_21[0] & 1) == 0))) {
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) {
      iVar2 = -0x3e;
      FUN_0100ceb0(s__s__Error_on_reading_data_high_010338d8 + 0x1c,
                   s_al_serdes_25g_rx_equalization_01033428 + 0x1c,param_3);
      FUN_01010c00(s__s__Error_on_reading_data_high_010338d8 + 0x1c,
                   s_al_serdes_25g_rx_equalization_01033428 + 0x1c,param_3);
LAB_0101a314:
      FUN_010196b4(param_1,param_2,0x580,1,0,0);
      return iVar2;
    }
    (**(code **)(param_1 + 0xe4))(*(undefined4 *)(param_1 + 0xe8),1);
  }
  iVar2 = FUN_0101957c(param_1,param_2,0,0x590,&local_22);
  if (iVar2 != 0) {
    FUN_0100ceb0(s__s__Still_need_more_samples_but_f_01033874 + 0x40,
                 s_al_serdes_25g_rx_equalization_01033428 + 0x1c);
    FUN_01010c00(s__s__Still_need_more_samples_but_f_01033874 + 0x40,
                 s_al_serdes_25g_rx_equalization_01033428 + 0x1c);
    return iVar2;
  }
  iVar2 = FUN_0101957c(param_1,param_2,0,0x591,local_21);
  if (iVar2 != 0) {
    FUN_0100ceb0(s__s__Error_on_reading_data_low_010338b8 + 0x1c,
                 s_al_serdes_25g_rx_equalization_01033428 + 0x1c);
    FUN_01010c00(s__s__Error_on_reading_data_low_010338b8 + 0x1c,
                 s_al_serdes_25g_rx_equalization_01033428 + 0x1c);
    return iVar2;
  }
  *param_4 = (ushort)local_22 | (local_21[0] & 0xf) << 8;
  iVar2 = 0;
  goto LAB_0101a314;
}



/* @ 0x101af8c */

byte FUN_0101af8c(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  byte local_9 [5];
  
  iVar1 = FUN_0101957c(param_1,param_2,0,0x38,local_9);
  if (iVar1 == 0) {
    return local_9[0] & 1;
  }
  FUN_0100ceb0(s__s__TO_waiting_for_RX_LEQ_FSM_re_010342e4 + 0x4c,
               s_al_serdes_25g_cdr_is_locked_010333f4 + 0x18);
  FUN_01010c00(s__s__TO_waiting_for_RX_LEQ_FSM_re_010342e4 + 0x4c,
               s_al_serdes_25g_cdr_is_locked_010333f4 + 0x18);
  return 0;
}



/* @ 0x101bbd4 */

void FUN_0101bbd4(undefined4 param_1,uint param_2)

{
  if ((param_2 & 3) == 0) {
    param_2 = param_2 - 1;
  }
  else {
    param_2 = FUN_010194b4(param_2);
  }
  FUN_01019944(param_1,1);
  FUN_01019be4(param_1,param_2 & 0xff,0);
  FUN_01019944(param_1,0);
  FUN_0101a0b0(param_1);
  return;
}



/* @ 0x101bc3c */

void FUN_0101bc3c(undefined4 param_1,uint param_2,uint param_3,uint *param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  
  iVar3 = 0;
  while( true ) {
    while( true ) {
      uVar2 = param_2 + param_3;
      iVar1 = FUN_0101bbd4(param_1,uVar2);
      if (iVar1 == 0) break;
      if (iVar3 == 1) {
        if ((param_2 & 3) == 0) {
          iVar3 = ~param_3 + uVar2;
          FUN_0100ceb0(s__s__al_serdes_25g_gcfsm2_read_fa_01034444 + 0x3c,
                       s_al_serdes_25g_qsample_pll_lock_c_01033528 + 0x24,param_2,iVar3);
        }
        else {
          iVar3 = FUN_010194b4(param_2);
          FUN_0100ceb0(s__s__al_serdes_25g_gcfsm2_read_fa_01034444 + 0x3c,
                       s_al_serdes_25g_qsample_pll_lock_c_01033528 + 0x24,param_2,iVar3);
        }
        FUN_01010c00(s__s__al_serdes_25g_gcfsm2_read_fa_01034444 + 0x3c,
                     s_al_serdes_25g_qsample_pll_lock_c_01033528 + 0x24,param_2,iVar3);
      }
      if (0x83 < uVar2 - 1) goto LAB_0101bd1c;
      iVar3 = 0;
      param_2 = uVar2;
    }
    iVar1 = iVar3 + 1;
    if (0x83 < uVar2 - 1) break;
    iVar3 = 1;
    param_2 = uVar2;
    if (iVar1 != 1) {
      uVar2 = uVar2 + param_3 * -2;
LAB_0101bd1c:
      *param_4 = uVar2;
      return;
    }
  }
  uVar2 = uVar2 - iVar1 * param_3;
  goto LAB_0101bd1c;
}



/* @ 0x101bd6c */

undefined4 FUN_0101bd6c(int *param_1,char *param_2)

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
  char *pcVar10;
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
    FUN_0100ceb0(&DAT_0103392c,s_al_serdes_25g_is_fw_alive_01033628 + 0x18,cVar1);
    FUN_01010c00(&DAT_0103392c,s_al_serdes_25g_is_fw_alive_01033628 + 0x18,*param_2);
    return 0xffffffea;
  }
  local_6c = 3;
  local_54 = 0x901100;
LAB_0101bdb4:
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
  (*pcVar4)(iVar3,10);
  pcVar4 = (code *)param_1[0x39];
  iVar3 = param_1[0x3a];
  *(uint *)(iVar11 + 300) = ~*(uint *)(iVar11 + 300) & 1 ^ *(uint *)(iVar11 + 300);
  (*pcVar4)(iVar3,10);
LAB_0101be34:
  if ((*(uint *)(iVar11 + 0x130) & 0x1a) != 0x1a) goto LAB_0101be20;
  iVar3 = 0;
  FUN_010196b4(param_1,0,0x717,1,0,1);
  FUN_010196b4(param_1,0,0x717,2,1,0);
  FUN_010196b4(param_1,1,0x717,1,0,1);
  FUN_010196b4(param_1,1,0x717,2,1,0);
  if (cVar1 == '\x15') {
    uVar18 = 7;
    uVar16 = 0xf;
  }
  else {
    uVar18 = 0;
    uVar16 = 7;
  }
  do {
    FUN_010196b4(param_1,iVar3,0x71a,1,0,0);
    FUN_010196b4(param_1,iVar3,0x299,0xf,0,uVar18);
    FUN_010196b4(param_1,iVar3,0x299,0xf0,4,uVar16);
    bVar22 = iVar3 != 1;
    iVar3 = 1;
  } while (bVar22);
  if (cVar1 == '\x15') {
    FUN_010196b4(param_1,4,0x116,0xf,0,6);
    FUN_010196b4(param_1,4,0x116,0xf0,4,6);
  }
  else {
    FUN_010196b4(param_1,4,5,0xf,0,9);
  }
  iVar3 = 10000;
  (*(code *)param_1[0x39])(param_1[0x3a],1);
  *(uint *)(iVar11 + 300) = ~*(uint *)(iVar11 + 300) & 2 ^ *(uint *)(iVar11 + 300);
  while ((*(uint *)(iVar11 + 0x130) & 6) != 6) {
    (*(code *)param_1[0x39])(param_1[0x3a],1);
    iVar3 = iVar3 + -1;
    if (iVar3 == 0) goto LAB_0101cf64;
  }
  if (cVar1 != '\x15') {
    iVar3 = 0;
    do {
      FUN_010196b4(param_1,iVar3,0x18c,7,0,4);
      FUN_010196b4(param_1,iVar3,0xb0,0xf,0,6);
      FUN_0101957c(param_1,iVar3,0,0x708,&local_44);
      uVar20 = local_44 & 0xcd | 0x28;
      local_44 = CONCAT31(local_44._1_3_,(char)uVar20);
      FUN_010194d0(param_1,iVar3,0,0x708,uVar20);
      FUN_010196b4(param_1,iVar3,0x203,0x18,3,3);
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
  FUN_010196b4(param_1,0,0x202,0xff,0,uVar18);
  FUN_010196b4(param_1,1,0x202,0xff,0,uVar18);
  FUN_010196b4(param_1,0,0x1a0,2,1,1);
  FUN_010196b4(param_1,1,0x1a0,2,1,1);
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
    FUN_010196b4(param_1,iVar3,0x541,0x78,3,4);
    bVar22 = iVar3 != 1;
    iVar3 = 1;
  } while (bVar22);
  iVar3 = 0;
  do {
    FUN_010196b4(param_1,iVar3,0x14,3,0,3);
    FUN_010196b4(param_1,iVar3,0x10,0xf,0,8);
    FUN_010196b4(param_1,iVar3,0x709,1,0,0);
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
    FUN_0100ceb0(s__s__Unsupported_group_mode__u_01033930 + 0x1c);
    FUN_01010c00(s__s__Unsupported_group_mode__u_01033930 + 0x1c);
  }
  else {
    FUN_01019f68(param_1,0,1,0x14,&local_4a);
    local_4a = local_4a ^ local_4a >> 8;
    local_4a = local_4a ^ local_4a >> 4;
    local_4a = local_4a ^ local_4a >> 2;
    local_4a = local_4a ^ local_4a >> 1;
    uVar20 = (uint)local_4a;
    if (uVar20 < 10) {
      uVar18 = 0xf;
      local_5c = 10;
      local_4a = 10;
LAB_0101c27c:
      FUN_010196b4(param_1,8,0x319,0xf,0,uVar18);
      FUN_010196b4(param_1,8,0x318,1,0,1);
      FUN_01019944(param_1,1);
      FUN_01019be4(param_1,0x28,1);
      FUN_01019944(param_1,0);
      FUN_0101a0b0(param_1);
    }
    else {
      if (0xd < uVar20) {
        uVar18 = 0xb;
        local_5c = 0xd;
        local_4a = 0xd;
        goto LAB_0101c27c;
      }
      FUN_010196b4(param_1,8,0x319,0xf,0,(local_4a ^ local_4a >> 1) & 0xff);
      FUN_010196b4(param_1,8,0x318,1,0,1);
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
    piVar5 = DAT_0101d188;
    while (iVar3 < local_64) {
      iVar8 = iVar8 + 1;
      if (iVar8 == 7) {
        FUN_0100ceb0(s__s__Got_timeout_waiting_for_LN_d_01033ce8 + 0x34,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,local_64);
        FUN_01010c00(s__s__Got_timeout_waiting_for_LN_d_01033ce8 + 0x34,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,local_64);
        goto LAB_0101c514;
      }
      iVar3 = *piVar5;
      piVar5 = piVar5 + 3;
    }
    FUN_01019f68(param_1,0,1,0x10,&local_4a);
    if ((local_4a & 3) == 0) {
      uVar2 = local_4a - 1;
    }
    else {
      uVar2 = FUN_010194b4();
    }
    uVar21 = (uint)uVar2;
    iVar3 = FUN_0101a0b0(param_1);
    if (puVar12 != (undefined4 *)0x0) {
      puVar12[4] = iVar3;
    }
    bVar22 = uVar21 == 0x83;
    if (uVar21 < 0x84) {
      bVar22 = iVar3 == 0;
    }
    if (bVar22) {
      uVar17 = uVar21 + 1;
      iVar3 = FUN_0101bbd4(param_1,uVar17);
      if (iVar3 != 0) goto LAB_0101cecc;
      if ((uVar2 & 0xfffe) != 0) goto LAB_0101ceb4;
    }
    else {
      uVar17 = uVar21;
      if ((uVar2 & 0xfffe) != 0 && iVar3 == 0) {
LAB_0101ceb4:
        iVar3 = FUN_0101bbd4(param_1,uVar21 - 1);
        uVar17 = uVar21 - 1;
      }
      if (iVar3 != 0) {
LAB_0101cecc:
        FUN_0101bc3c(param_1,uVar17,1,local_68);
        FUN_0101bc3c(param_1,uVar17,0xffffffff,&local_48);
        uVar21 = (int)(local_44 + local_48) / 2;
        uVar19 = local_48;
        uVar15 = local_44;
        if (puVar12 != (undefined4 *)0x0) {
          puVar12[5] = 1;
          if ((uVar17 & 3) == 0) {
LAB_0101cf20:
            uVar24 = CONCAT44(local_48,uVar17 - 1);
            uVar15 = local_44;
          }
          else {
LAB_0101c5a4:
            uVar15 = local_44;
            uVar24 = FUN_010194b4(uVar17);
          }
          iVar3 = (int)(uVar24 >> 0x20);
          puVar12[6] = (int)uVar24;
          if ((uVar24 & 0x300000000) == 0) {
            uVar25 = CONCAT44(iVar3,iVar3 + -1);
          }
          else {
            uVar25 = FUN_010194b4(iVar3);
          }
          puVar12[7] = (int)uVar25;
          if ((uVar15 & 3) == 0) {
            uVar25 = CONCAT44((int)((ulonglong)uVar25 >> 0x20),uVar15 - 1);
          }
          else {
            uVar25 = FUN_010194b4(uVar15);
          }
          puVar12[8] = (int)uVar25;
          uVar19 = (uint)((ulonglong)uVar25 >> 0x20);
        }
LAB_0101c5e0:
        iVar3 = *(int *)(iVar8 * 0xc + 0x10344a4);
        if (*(int *)(iVar8 * 0xc + 0x10344a0) == 1) {
          uVar14 = uVar15 - iVar3;
          if (0 < (int)uVar14) goto LAB_0101ce04;
          if ((uVar14 & 3) == 0) {
            iVar3 = uVar14 - 1;
          }
          else {
            iVar3 = FUN_010194b4(uVar14);
          }
          FUN_0100ceb0(s__s__error__no_valid_dosc_window_a_010339c0 + 0x28,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar14,iVar3,1);
          FUN_01010c00(s__s__error__no_valid_dosc_window_a_010339c0 + 0x28,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar14,iVar3,1);
          if ((int)local_48 < 2) {
            uVar14 = 1;
            if (0 < (int)local_44) goto LAB_0101c7c8;
          }
          else {
            uVar14 = 1;
          }
LAB_0101ce1c:
          iVar3 = FUN_010194b4(uVar14);
LAB_0101c684:
          if ((uVar21 & 3) == 0) {
            iVar8 = uVar21 - 1;
          }
          else {
            iVar8 = FUN_010194b4(uVar21);
          }
          FUN_0100ceb0(s__s__dosc_target__l__d__r__d_is_h_01033a2c + 0x40,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar14,iVar3,uVar21,iVar8)
          ;
          FUN_01010c00(s__s__dosc_target__l__d__r__d_is_h_01033a2c + 0x40,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar14,iVar3,uVar21,iVar8)
          ;
          iVar3 = FUN_0101bbd4(param_1,uVar21);
          uVar14 = uVar21;
          if (iVar3 != 0) goto LAB_0101c854;
LAB_0101c6f8:
          FUN_0100ceb0(s__s__dosc_target_inside_the_windo_01033ab8 + 0x58,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
          FUN_01010c00(s__s__dosc_target_inside_the_windo_01033ab8 + 0x58,
                       s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
          iVar3 = FUN_0101bbd4(param_1,uVar17);
          uVar14 = uVar17;
          if (puVar12 != (undefined4 *)0x0) {
LAB_0101c85c:
            if ((uVar14 & 3) == 0) {
              uVar25 = CONCAT44(iVar3,uVar14 - 1);
            }
            else {
              uVar25 = FUN_010194b4(uVar14);
            }
            iVar3 = (int)((ulonglong)uVar25 >> 0x20);
            *(undefined8 *)(puVar12 + 9) = uVar25;
          }
          if (iVar3 == 0) {
            FUN_0100ceb0(s__s__pll_is_unlocked__returning_t_01033b14 + 0x30,
                         s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
            FUN_01010c00(s__s__pll_is_unlocked__returning_t_01033b14 + 0x30,
                         s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
            goto LAB_0101c514;
          }
        }
        else {
          uVar9 = iVar3 + uVar19;
          uVar14 = uVar9;
          if ((int)uVar9 < 0x85) {
LAB_0101ce04:
            if (((int)uVar14 < (int)uVar19) || ((int)uVar15 < (int)uVar14)) {
              if ((uVar14 & 3) != 0) goto LAB_0101ce1c;
LAB_0101c680:
              iVar3 = uVar14 - 1;
              goto LAB_0101c684;
            }
          }
          else {
            if ((uVar9 & 3) == 0) {
              iVar3 = uVar9 - 1;
            }
            else {
              iVar3 = FUN_010194b4(uVar9);
            }
            uVar14 = 0x84;
            FUN_0100ceb0(s__s__dosc_target__l__d__r__d_lowe_010339ec + 0x3c,
                         s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar9,iVar3,0x84);
            FUN_01010c00(s__s__dosc_target__l__d__r__d_lowe_010339ec + 0x3c,
                         s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,uVar9,iVar3,0x84);
            if (0x84 < (int)local_48) {
              iVar3 = 0x83;
              goto LAB_0101c684;
            }
            uVar14 = 0x84;
            if ((int)local_44 < 0x84) goto LAB_0101c680;
          }
LAB_0101c7c8:
          iVar3 = FUN_0101bbd4(param_1,uVar14);
          if (iVar3 == 0 && uVar14 != uVar21) {
            do {
              FUN_0100ceb0(s__s__stepped_outside_of_window_go_01033a70 + 0x44,
                           s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
              FUN_01010c00(s__s__stepped_outside_of_window_go_01033a70 + 0x44,
                           s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
              if ((int)uVar14 < (int)uVar21) {
                uVar14 = uVar14 + 1;
              }
              else {
                uVar14 = uVar14 - 1;
              }
              iVar3 = FUN_0101bbd4(param_1,uVar14);
            } while (iVar3 == 0 && uVar14 != uVar21);
          }
          if (iVar3 == 0) goto LAB_0101c6f8;
LAB_0101c854:
          if (puVar12 != (undefined4 *)0x0) goto LAB_0101c85c;
        }
        FUN_0100ceb0(s__s__failed_to_lock_PLL_01033b48 + 0x14,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,local_64,uVar20,local_5c);
        FUN_01010c00(s__s__failed_to_lock_PLL_01033b48 + 0x14,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,local_64,uVar20,local_5c);
        if ((uVar17 & 3) == 0) {
          iVar3 = uVar17 - 1;
        }
        else {
          iVar3 = FUN_010194b4(uVar17);
        }
        if ((uVar14 & 3) == 0) {
          iVar8 = uVar14 - 1;
        }
        else {
          iVar8 = FUN_010194b4(uVar14);
        }
        if ((local_48 & 3) == 0) {
          iVar13 = local_48 - 1;
        }
        else {
          iVar13 = FUN_010194b4();
        }
        if ((local_44 & 3) == 0) {
          iVar6 = local_44 - 1;
        }
        else {
          iVar6 = FUN_010194b4();
        }
        FUN_0100ceb0(s__s__temperature__d__swing_init___01033b60 + 0x34,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,iVar3,iVar8,iVar13,iVar6);
        if ((local_48 & 3) == 0) {
          iVar13 = local_48 - 1;
        }
        else {
          iVar13 = FUN_010194b4();
        }
        if ((local_44 & 3) == 0) {
          iVar6 = local_44 - 1;
        }
        else {
          iVar6 = FUN_010194b4();
        }
        FUN_01010c00(s__s__temperature__d__swing_init___01033b60 + 0x34,
                     s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20,iVar3,iVar8,iVar13,iVar6);
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
              FUN_0100ceb0(s_Serdes25G_power_up_OK___s_mode__01033cc4 + 0x20,
                           s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24,iVar3);
              FUN_01010c00(s_Serdes25G_power_up_OK___s_mode__01033cc4 + 0x20,
                           s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24,iVar3);
              uVar18 = 0xffffffc2;
              goto LAB_0101c540;
            }
          }
          if (iVar3 == 1) break;
          iVar3 = 1;
        }
        if (cVar1 != '\x15') {
          sVar7 = 0x41c;
          iVar3 = 0;
          do {
            FUN_01019f68(param_1,iVar3,2,7,local_68);
            uVar20 = ((local_44 & 0x3ff) >> 5) + 8;
            if (0x1f < uVar20) {
              uVar20 = 0x1f;
            }
            local_44 = CONCAT22(local_44._2_2_,(short)uVar20);
            FUN_010196b4(param_1,8,sVar7,0x1f,0,uVar20);
            FUN_010196b4(param_1,8,sVar7 + -1,1,0,1);
            bVar22 = iVar3 != 1;
            sVar7 = sVar7 + 0x100;
            iVar3 = 1;
          } while (bVar22);
        }
        iVar3 = 0;
        while( true ) {
          FUN_010196b4(param_1,iVar3,0x710,0x30,4,1);
          FUN_010196b4(param_1,iVar3,0x714,4,2,1);
          FUN_010196b4(param_1,iVar3,0x715,4,2,1);
          FUN_010196b4(param_1,iVar3,0x716,0x30,4,1);
          FUN_010196b4(param_1,iVar3,0x716,0xc0,6,1);
          FUN_010196b4(param_1,iVar3,0x717,0x10,4,1);
          FUN_010196b4(param_1,iVar3,0x717,0x20,5,1);
          FUN_010196b4(param_1,iVar3,0x717,0x40,6,1);
          FUN_010196b4(param_1,iVar3,0x717,0x80,7,1);
          FUN_010196b4(param_1,iVar3,0x718,0x30,4,2);
          FUN_010196b4(param_1,iVar3,0x718,0xc0,6,2);
          FUN_010196b4(param_1,iVar3,0x724,2,1,1);
          FUN_010196b4(param_1,iVar3,0x725,2,1,1);
          FUN_010196b4(param_1,iVar3,0x726,2,1,1);
          FUN_010196b4(param_1,iVar3,0x727,2,1,1);
          FUN_010196b4(param_1,iVar3,0x728,2,1,1);
          FUN_010196b4(param_1,iVar3,0x758,1,0,1);
          if (iVar3 == 1) break;
          iVar3 = 1;
        }
        (*(code *)param_1[0x37])(param_1,local_68);
        if (local_44 != 0) {
          FUN_0100ceb0(s__s__failed_to_apply_tx_pll_wa_01033bdc + 0x1c,
                       s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14);
          FUN_01010c00(s__s__failed_to_apply_tx_pll_wa_01033bdc + 0x1c,
                       s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14,local_44);
        }
        if (local_40[0] != 0 || local_40[1] != 0) {
          FUN_0100ceb0(s__s__Got_FW_error__TOP_error_code_01033bfc + 0x24,
                       s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14);
          FUN_01010c00(s__s__Got_FW_error__TOP_error_code_01033bfc + 0x24,
                       s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14,local_40[0]);
          if (local_40[1] != 0) {
            FUN_0100ceb0(s__s__Got_FW_error__CM_error_code___01033c24 + 0x20,
                         s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14);
            FUN_01010c00(s__s__Got_FW_error__CM_error_code___01033c24 + 0x20,
                         s_al_serdes_25g_tx_pll_wa_010335a0 + 0x14,local_40[1]);
            uVar18 = 0xfffffffb;
            goto LAB_0101c540;
          }
        }
        iVar3 = 0;
        piVar5 = local_40 + 2;
        do {
          if ((*piVar5 != 0) || (piVar5[2] != 0)) {
            FUN_0100ceb0(s__s__FW_CM_error_is_critical___d__01033c48 + 0x20,
                         s_al_serdes_25g_fw_init_status_gro_010335b8 + 0x2c,iVar3);
            FUN_01010c00(s__s__FW_CM_error_is_critical___d__01033c48 + 0x20,
                         s_al_serdes_25g_fw_init_status_gro_010335b8 + 0x2c,iVar3,*piVar5);
            if (piVar5[2] != 0) {
              FUN_0100ceb0(s__s__Got_FW_error__lane__d_error_c_01033c6c + 0x28,
                           s_al_serdes_25g_fw_init_status_gro_010335b8 + 0x2c,iVar3);
              FUN_01010c00(s__s__Got_FW_error__lane__d_error_c_01033c6c + 0x28,
                           s_al_serdes_25g_fw_init_status_gro_010335b8 + 0x2c,iVar3,
                           local_40[iVar3 + 4]);
              uVar18 = 0xfffffffb;
              goto LAB_0101c540;
            }
          }
          if (iVar3 == 1) {
            pcVar10 = &DAT_01033928;
            if (cVar1 == '\x15') {
              pcVar10 = s__s__TO_waiting_for_GCFSM2_req_to_010338f8 + 0x2c;
            }
            FUN_0100ceb0(s__s__FW_lane__d_error_is_critical_01033c98 + 0x28,pcVar10);
            FUN_01010c00(s__s__FW_lane__d_error_is_critical_01033c98 + 0x28,pcVar10);
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
      iVar3 = FUN_010194b4(uVar21);
    }
    FUN_0100ceb0(s_can_t_apply_tx_pll_WA_without_te_01033950 + 0x28,
                 s_al_serdes_25g_tx_pll_wa_find_win_01033550 + 0x28,uVar21,iVar3);
    FUN_01010c00(s_can_t_apply_tx_pll_WA_without_te_01033950 + 0x28,
                 s_al_serdes_25g_tx_pll_wa_find_win_01033550 + 0x28,uVar21,iVar3);
    uVar17 = uVar21;
    do {
      if ((int)uVar21 < 0x85) {
        uVar19 = uVar21 + 4;
        uVar15 = uVar21;
        do {
          iVar3 = FUN_0101bbd4(param_1,uVar15);
          if (iVar3 != 0) {
            if (uVar15 != 0) {
              local_48 = uVar15;
              FUN_0101bc3c(param_1,uVar15,1,local_68);
              goto LAB_0101c574;
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
          iVar3 = FUN_0101bbd4(param_1,uVar15);
          if (iVar3 != 0) {
            local_44 = uVar15;
            FUN_0101bc3c(param_1,uVar15,0xffffffff,&local_48);
LAB_0101c574:
            uVar17 = (int)(local_48 + local_44) / 2;
            uVar19 = local_48;
            uVar21 = uVar17;
            uVar15 = local_44;
            if (puVar12 != (undefined4 *)0x0) {
              puVar12[5] = 1;
              if ((uVar17 & 3) == 0) goto LAB_0101cf20;
              goto LAB_0101c5a4;
            }
            goto LAB_0101c5e0;
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
    FUN_0100ceb0(s__s__init__l__d__r__d_outside_win_0103397c + 0x40,
                 s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
    FUN_01010c00(s__s__init__l__d__r__d_outside_win_0103397c + 0x40,
                 s_al_serdes_25g_tx_pll_wa_find_win_0103357c + 0x20);
  }
LAB_0101c514:
  FUN_0100ceb0(s__s__dosc_init__d__dosc_target__d_01033b98 + 0x40,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  FUN_01010c00(s__s__dosc_init__d__dosc_target__d_01033b98 + 0x40,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  uVar18 = 0xfffffffb;
  goto LAB_0101c540;
LAB_0101be20:
  (*(code *)param_1[0x39])(param_1[0x3a],1);
  iVar8 = iVar8 + -1;
  if (iVar8 == 0) goto LAB_0101cf28;
  goto LAB_0101be34;
LAB_0101cf28:
  FUN_0100ceb0(s__s__Got_timeout_waiting_for_serd_01033d58 + 0x38,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  FUN_01010c00(s__s__Got_timeout_waiting_for_serd_01033d58 + 0x38,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  uVar18 = 0xffffffc2;
  goto LAB_0101c540;
LAB_0101cf64:
  FUN_0100ceb0(s__s__error__invalid_temperature_r_01033d20 + 0x34,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  FUN_01010c00(s__s__error__invalid_temperature_r_01033d20 + 0x34,
               s_al_serdes_25g_fw_init_status_lan_010335e8 + 0x24);
  uVar18 = 0xffffffc2;
LAB_0101c540:
  if (local_6c == 0) {
    return uVar18;
  }
  cVar1 = *param_2;
  goto LAB_0101bdb4;
}



/* @ 0x101d8c0 */

void FUN_0101d8c0(int *param_1)

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
  *(uint *)(iVar1 + 0x114) = *(uint *)(iVar1 + 0x114) & 0x80 | 8;
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
  *(undefined4 *)(iVar1 + 0x114) = 0xc;
  *(undefined4 *)(iVar1 + 0x110) = 0x8097;
  *(undefined4 *)(iVar1 + 0x114) = 0xcc;
  return;
}



/* @ 0x101da60 */

undefined4 FUN_0101da60(int *param_1,uint *param_2,int param_3,int param_4,char param_5)

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
    FUN_0100ceb0(s__s__invalid_core_clock_freq___d__010345ec + 0x20,&DAT_010344f0,param_5);
    FUN_01010c00(s__s__invalid_core_clock_freq___d__010345ec + 0x20,&DAT_010344f0,param_5);
    return 0xffffffea;
  }
  if (param_3 == 1) {
    *param_2 = *param_2 | 0x20;
  }
  else if (param_3 != 0) {
    if (param_3 != 2) {
      FUN_0100ceb0(s__s__invalid_core_clock_source____01034610 + 0x20,&DAT_010344f0,param_3);
      FUN_01010c00(s__s__invalid_core_clock_source____01034610 + 0x20,&DAT_010344f0,param_3);
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
      FUN_0100ceb0(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,&DAT_010344f0,param_4);
      FUN_01010c00(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,&DAT_010344f0,param_4);
      return 0xffffffea;
    }
    uVar2 = *param_2 | 0x300;
    *param_2 = uVar2;
  }
  *(uint *)(iVar1 + 0x128) = uVar2;
  return 0;
}




/* @ 0x101dc0c -- WARNING: Control flow encountered bad instruction data */

int * FUN_0101dc0c(int *param_1,uint *param_2,undefined4 param_3,uint param_4,byte param_5)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = *param_1;
  uVar1 = (uint)param_5;
  *param_2 = 0;
  switch(uVar1 - 1) {
  case 0:
    *param_2 = uVar1 - 1;
    if (param_4 == 1) {
      param_4 = *param_2 | 0x20;
      *param_2 = param_4;
      goto LAB_0101dcc4;
    }
    if (param_4 == 0) goto LAB_0101dcc4;
    if (param_4 == 4) {
      param_4 = *param_2 | 0x30;
      *param_2 = param_4;
switchD_0101dcc8_caseD_2:
LAB_0101dcc4:
      switch(param_3) {
      case 0:
        return param_1;
      case 1:
        *(uint *)(iVar2 + 0x128) = param_4;
        return (int *)0x0;
      case 2:
      case 3:
        goto switchD_0101dcc8_caseD_2;
      default:
        FUN_0100ceb0(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,
                     s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,param_3);
        FUN_01010c00(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,
                     s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,param_3);
        return (int *)0xffffffea;
      }
    }
    FUN_0100ceb0(s__s__invalid_core_clock_source____01034610 + 0x20,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,param_4);
    FUN_01010c00(s__s__invalid_core_clock_source____01034610 + 0x20,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,param_4);
    break;
  case 1:
  case 2:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 3:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  default:
    FUN_0100ceb0(s__s__invalid_core_clock_freq___d__010345ec + 0x20,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,uVar1);
    FUN_01010c00(s__s__invalid_core_clock_freq___d__010345ec + 0x20,
                 s_al_serdes_hssp_group_ictl_pma_va_010344f4 + 0x24,uVar1);
  }
  return (int *)0xffffffea;
}



/* @ 0x101dda8 */

void FUN_0101dda8(int *param_1,int param_2,int param_3)

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



/* @ 0x101de18 */

void FUN_0101de18(int *param_1,int param_2)

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



/* @ 0x101de40 */

undefined4 FUN_0101de40(int *param_1,int param_2)

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
      *(uint *)(iVar6 + 0x134) = *(uint *)(iVar6 + 0x134) & ~(8 >> uVar2 | 0x80000 >> uVar3);
    }
  } while (uVar5 != 4);
  return 0;
}



/* @ 0x101ec24 */

void FUN_0101ec24(int *param_1,int param_2,uint param_3,int param_4,int param_5,int param_6,
                 byte param_7)

{
  int iVar1;
  int iVar2;
  undefined4 local_34;
  
  iVar2 = *param_1;
  FUN_01007590(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf1000;
  FUN_01007590(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf0000;
  FUN_01007590(1);
  *(undefined4 *)(iVar2 + 0x134) = 0xf1000;
  FUN_01007590(1);
  iVar1 = *param_1;
  if (param_2 == 0) {
    *(undefined4 *)(iVar1 + 0x110) = 0xe065;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0xe066;
    *(undefined4 *)(iVar1 + 0x114) = 0xbf;
    *(undefined4 *)(iVar1 + 0x110) = 0xe067;
    *(undefined4 *)(iVar1 + 0x114) = 8;
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
    *(undefined4 *)(iVar1 + 0x114) = 8;
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
    *(undefined4 *)(iVar1 + 0x114) = 9;
    *(undefined4 *)(iVar1 + 0x110) = 0x8165;
    *(undefined4 *)(iVar1 + 0x114) = 3;
    *(undefined4 *)(iVar1 + 0x110) = 0x8166;
    *(undefined4 *)(iVar1 + 0x114) = 0x3e;
    *(undefined4 *)(iVar1 + 0x110) = 0x8167;
    *(undefined4 *)(iVar1 + 0x114) = 0xc;
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
  FUN_01007590(1);
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
    FUN_01007590(1);
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



/* @ 0x101f4bc */

undefined4 FUN_0101f4bc(int *param_1,int param_2,uint param_3,int param_4,int param_5)

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
  FUN_01007590(1);
  *(undefined4 *)(iVar3 + 0x134) = 0;
  FUN_01007590(1);
  *(undefined4 *)(iVar3 + 0x134) = 0x1000;
  FUN_01007590(1);
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
      *(undefined4 *)(iVar2 + 0x114) = 0xc;
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
      *(undefined4 *)(iVar2 + 0x114) = 8;
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
      *(undefined4 *)(iVar2 + 0x114) = 8;
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
      *(undefined4 *)(iVar2 + 0x114) = 9;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = 0xc;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
    }
  }
  else {
    if (param_4 != 3) {
      FUN_0100ceb0(s_al_serdes_hssp_group_cfg_eth_sgm_010345bc + 0x2c,
                   s_al_serdes_hssp_group_cfg_sata_mo_01034548 + 0x20,param_4);
      FUN_01010c00(s_al_serdes_hssp_group_cfg_eth_sgm_010345bc + 0x2c,
                   s_al_serdes_hssp_group_cfg_sata_mo_01034548 + 0x20,param_4);
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
      *(undefined4 *)(iVar2 + 0x114) = 0xc;
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
      *(undefined4 *)(iVar2 + 0x114) = 8;
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
      *(undefined4 *)(iVar2 + 0x114) = 9;
      *(undefined4 *)(iVar2 + 0x110) = 0x8165;
      *(undefined4 *)(iVar2 + 0x114) = 3;
      *(undefined4 *)(iVar2 + 0x110) = 0x8166;
      *(undefined4 *)(iVar2 + 0x114) = 0x3e;
      *(undefined4 *)(iVar2 + 0x110) = 0x8167;
      *(undefined4 *)(iVar2 + 0x114) = 0xc;
      *(undefined4 *)(iVar2 + 0x110) = 0xe2bd;
      *(undefined4 *)(iVar2 + 0x114) = 0;
    }
  }
  *(undefined4 *)(iVar2 + 0x110) = 0xe057;
  *(undefined4 *)(iVar2 + 0x114) = 0x1f;
  FUN_0101d8c0(param_1);
  *(undefined4 *)(iVar3 + 0x134) = 0x11f0;
  FUN_01007590(1);
  return 0;
}



/* @ 0x1021b64 */

undefined4 FUN_01021b64(int *param_1,byte *param_2,byte *param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  iVar1 = *param_1;
  *(undefined4 *)(iVar1 + 0x11c) = 0;
  if (param_2 == (byte *)0x0) {
    uVar3 = 0xe4;
    param_2 = (byte *)(s_al_serdes_25g_group_ictl_pma_val_0103467c + 0x24);
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



/* @ 0x1021be4 */

undefined4 FUN_01021be4(int *param_1,uint *param_2,int param_3,int param_4,char param_5)

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
    FUN_0100ceb0(s__s__invalid_core_clock_freq___d__010345ec + 0x20,
                 s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_5);
    FUN_01010c00(s__s__invalid_core_clock_freq___d__010345ec + 0x20,
                 s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_5);
    return 0xffffffea;
  }
  if (param_3 == 1) {
    *param_2 = *param_2 | 0x10;
  }
  else if (param_3 != 0) {
    if (param_3 != 2) {
      FUN_0100ceb0(s__s__invalid_core_clock_source____01034610 + 0x20,
                   s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_3);
      FUN_01010c00(s__s__invalid_core_clock_source____01034610 + 0x20,
                   s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_3);
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
      FUN_0100ceb0(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,
                   s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_4);
      FUN_01010c00(s__s__invalid_R2L_clock_source___d_01034634 + 0x20,
                   s__s__invalid_L2R_clock_source___d_01034658 + 0x20,param_4);
      return 0xffffffea;
    }
    uVar2 = *param_2 | 0x80;
    *param_2 = uVar2;
  }
  *(uint *)(iVar1 + 0x118) = uVar2;
  return 0;
}



/* @ 0x1021d94 */

void FUN_01021d94(int *param_1,int param_2,int param_3)

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
      *(uint *)(iVar6 + 0x210) = ~uVar4 & 8 ^ uVar4;
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




void FUN_01021e0c(int param_1)

{
/* @ 0x1021e0c -- WARNING: Could not recover jumptable at 0x01021e10. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(param_1 + 0xb4))();
  return;
}




void FUN_01021e14(int param_1)

{
/* @ 0x1021e14 -- WARNING: Could not recover jumptable at 0x01021e18. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(param_1 + 0xb0))();
  return;
}



/* @ 0x1021e1c */

void FUN_01021e1c(int param_1,uint *param_2)

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



/* @ 0x1021e4c */

undefined4 FUN_01021e4c(int param_1)

{
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x7c) = 4;
  return 0;
}



/* @ 0x1021e60 */

undefined4 FUN_01021e60(int *param_1)

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



/* @ 0x1021eb0 */

undefined4 FUN_01021eb0(int param_1)

{
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x7c) = 8;
  return 0;
}



/* @ 0x1021ec4 */

undefined4 FUN_01021ec4(int *param_1)

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



/* @ 0x1021f14 */

undefined4 FUN_01021f14(undefined4 param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

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
      FUN_0100ceb0(s_al_ddr_cfg_init_010426b0 + 0xc,&DAT_0104269c,iVar1);
      FUN_01010c00(s_al_ddr_cfg_init_010426b0 + 0xc,&DAT_0104269c,iVar1);
      FUN_0100ceb0(s__s__Unknown_DDR_rev__08x_010426c0 + 0x18,s_al_ddr_rev_get_010426a0 + 0xc,0xd15)
      ;
      FUN_01010c00(s__s__Unknown_DDR_rev__08x_010426c0 + 0x18,s_al_ddr_rev_get_010426a0 + 0xc,0xd15)
      ;
      return 0xfffffffb;
    }
    *(undefined1 *)(param_4 + 3) = 2;
  }
  return 0;
}



/* @ 0x1021fd8 */

uint FUN_01021fd8(int param_1)

{
  return *(uint *)(param_1 + 0x15c) >> 0x10;
}



/* @ 0x1021fe4 */

uint FUN_01021fe4(int param_1)

{
  return *(uint *)(param_1 + 0x15c) & 0xffff;
}



/* @ 0x1021ff0 */

void FUN_01021ff0(int param_1,uint param_2,uint param_3)

{
  if (*(uint *)(param_1 + 0x15c) >> 0x10 == 0) {
    *(uint *)(param_1 + 0x120) = param_2;
    return;
  }
  if ((param_2 & 0xffff) != (param_3 & 0xffff)) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_pbs_u_01042744 + 0x24,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x32);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_pbs_u_01042744 + 0x24,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x32);
    FUN_01000458(0);
  }
  if ((param_2 >> 0x10 & 0xffffff00) != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___num_cyc_spi_low____num_cyc_low_0104276c + 0x20,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x34);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___num_cyc_spi_low____num_cyc_low_0104276c + 0x20,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x34);
    FUN_01000458(0);
  }
  if ((param_3 & 0xff0000) != 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____num_cyc_hi____0xFF0000_>>_16__01042790 + 0x30,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x39);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____num_cyc_hi____0xFF0000_>>_16__01042790 + 0x30,1,
                 &DAT_01042740,s__s__d__al_ddr_cfg_init_failed__010426dc + 0x1c,0x39);
    FUN_01000458(0);
  }
  *(uint *)(param_1 + 0x120) = param_3 & 0xff000000 | param_2 & 0xffff | param_2 & 0xffff0000;
  return;
}



/* @ 0x10221ac */

void FUN_010221ac(int param_1,int param_2)

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



/* @ 0x10221cc */

void FUN_010221cc(int param_1,int *param_2)

{
  uint uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_pbs_apb_mem_config_set_01042714 + 0x18,1,&DAT_01042740,
                 s_al_pbs_axi_timeout_set_010426fc + 0x14,0x5b);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_pbs_apb_mem_config_set_01042714 + 0x18,1,&DAT_01042740,
                 s_al_pbs_axi_timeout_set_010426fc + 0x14,0x5b);
    FUN_01000458(0);
  }
  if (param_2 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_01042738,1,&DAT_01042740,
                 s_al_pbs_axi_timeout_set_010426fc + 0x14,0x5c);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_01042738,1,&DAT_01042740,
                 s_al_pbs_axi_timeout_set_010426fc + 0x14,0x5c);
    FUN_01000458(0);
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



/* @ 0x102231c */

void FUN_0102231c(int param_1)

{
  *(undefined2 *)(param_1 + 0xf4) = 0xf;
  return;
}




/* @ 0x1022328 -- WARNING: Control flow encountered bad instruction data */

char FUN_01022328(uint param_1,uint *param_2)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  undefined1 uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  
  uVar6 = param_1;
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_boots_01042834 + 0x24,1,
                 s_al_bootstrap_parse_01042820 + 0x10,s_al_bootstrap_cpu_exist_get_01042804 + 0x18,
                 0x2ce);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_boots_01042834 + 0x24,1,
                 s_al_bootstrap_parse_01042820 + 0x10,s_al_bootstrap_cpu_exist_get_01042804 + 0x18,
                 0x2ce);
    uVar6 = FUN_01000458(0);
  }
  if (param_2 == (uint *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___pbs_regfile_ptr__0104285c + 0x10,1,
                 s_al_bootstrap_parse_01042820 + 0x10,s_al_bootstrap_cpu_exist_get_01042804 + 0x18,
                 0x2cf);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___pbs_regfile_ptr__0104285c + 0x10,1,
                 s_al_bootstrap_parse_01042820 + 0x10,s_al_bootstrap_cpu_exist_get_01042804 + 0x18,
                 0x2cf);
    uVar6 = FUN_01000458(0);
  }
  uVar7 = *(uint *)(param_1 + 0x110);
  uVar2 = 100000000;
  uVar5 = 25000000;
  if ((uVar7 & 0x80000) != 0) {
    uVar5 = uVar2;
  }
  param_2[5] = uVar5;
  uVar3 = uVar5;
  switch(uVar7 & 0xf) {
  case 1:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 2:
switchD_01022378_caseD_2:
LAB_01022464:
    param_2[3] = uVar5;
    uVar5 = (uVar7 & 0x3fff) >> 0xc;
    param_2[4] = uVar6;
    uVar4 = (&DAT_010427dc)[uVar5];
    cVar1 = s___num_cyc_spi_med____0__010427c4[uVar5 + 0x14];
    uVar5 = (uVar7 & 0xfff) >> 0xb;
    uVar6 = (uVar7 & 0x3ffff) >> 0xf;
    *(byte *)(param_2 + 6) = (byte)((uVar7 << 0x14) >> 0x1f);
    *(char *)((int)param_2 + 0x19) = cVar1;
    *(undefined1 *)((int)param_2 + 0x1a) = uVar4;
    *(byte *)((int)param_2 + 0x1b) = (byte)((uVar7 << 0x11) >> 0x1f);
    param_2[0xb] = 1;
    switch(uVar6) {
    case 0:
      goto switchD_010224b0_caseD_0;
    case 1:
      break;
    case 2:
      break;
    case 3:
      goto switchD_010224b0_caseD_3;
    case 4:
      break;
    case 5:
      break;
    case 6:
      break;
    default:
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                   s_al_bootstrap_parse_01042820 + 0x10,&DAT_010427e0,0x219);
      FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                   s_al_bootstrap_parse_01042820 + 0x10,&DAT_010427e0,0x219);
      cVar1 = '\0';
    case 7:
      FUN_01000458(cVar1);
      cVar1 = '\x03';
      uVar5 = 0x1c200;
    }
    uVar6 = (uVar7 & 0x3fffff) >> 0x14;
    uVar4 = (undefined1)uVar6;
    *(char *)(param_2 + 7) = cVar1;
    param_2[8] = uVar5;
    *(byte *)(param_2 + 9) = (byte)((uVar7 << 0xd) >> 0x1f);
    if ((uVar6 != 1) && (uVar6 != 0)) {
      if (uVar6 == 3) {
        uVar4 = 2;
      }
      else {
        FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                     s_al_bootstrap_parse_01042820 + 0x10,
                     s_al_bootstrap_boot_device_get_010427e4 + 0x1c,0x267);
        FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
                     s_al_bootstrap_parse_01042820 + 0x10,
                     s_al_bootstrap_boot_device_get_010427e4 + 0x1c,0x267);
        FUN_01000458(0);
        uVar4 = 0;
      }
    }
    *(undefined1 *)((int)param_2 + 0x25) = uVar4;
    if ((uVar7 & 0x800000) == 0) {
      uVar4 = 0x50;
    }
    else {
      uVar4 = 0x57;
    }
    cVar1 = '\0';
    *(byte *)((int)param_2 + 0x26) = (byte)((uVar7 << 9) >> 0x1f);
    *(undefined1 *)((int)param_2 + 0x27) = uVar4;
    *(undefined1 *)(param_2 + 10) = 0;
switchD_010224b0_caseD_0:
    return cVar1;
  case 3:
    uVar3 = uVar2;
    break;
  case 4:
    uVar3 = uVar2;
    break;
  case 5:
    uVar3 = uVar2;
    break;
  case 6:
    uVar3 = uVar2;
    break;
  case 7:
    uVar3 = uVar2;
    break;
  case 8:
    uVar3 = uVar2;
    break;
  case 9:
    uVar3 = uVar2;
    break;
  case 10:
    uVar3 = uVar2;
    break;
  case 0xb:
    uVar3 = uVar2;
    break;
  case 0xc:
    uVar3 = uVar2;
    break;
  case 0xd:
    uVar3 = uVar2;
    break;
  case 0xe:
    uVar3 = uVar2;
    break;
  case 0xf:
    uVar3 = uVar2;
  }
  *param_2 = uVar3;
  uVar2 = ((uVar7 & 0x7f) >> 4) - 1;
  uVar6 = uVar5;
  switch(uVar2) {
  case 0:
    uVar5 = uVar2;
    goto switchD_010223d4_default;
  case 1:
    uVar5 = uVar2;
    goto switchD_010223d4_default;
  case 2:
    uVar5 = uVar2;
    goto switchD_010223d4_default;
  case 3:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 4:
    uVar5 = uVar2;
    goto switchD_010223d4_default;
  case 5:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 6:
    uVar5 = uVar2;
  default:
    goto switchD_010223d4_default;
  }
switchD_010224b0_caseD_3:
  uVar5 = (uVar7 & 0x7fff) >> 0xe;
switchD_010223d4_default:
  uVar2 = (uVar7 & 0x1ff) >> 7;
  param_2[1] = uVar5;
  if (uVar2 == 0) goto code_r0x01022410;
  uVar6 = 1500000000;
  uVar5 = (uVar7 & 0x7ff) >> 9;
  if (uVar2 == 1) {
    uVar6 = 3000000000;
  }
  param_2[2] = uVar6;
  if (uVar5 != 2) {
    if (uVar5 == 3) {
      uVar6 = 375000000;
      uVar5 = 500000000;
    }
    else if (uVar5 == 1) {
      uVar6 = 375000000;
      uVar5 = uVar6;
    }
    else {
      uVar6 = 375000000;
      uVar5 = 250000000;
    }
    goto LAB_01022464;
  }
  uVar6 = 375000000;
  uVar5 = 428000000;
  goto switchD_01022378_caseD_2;
code_r0x01022410:
  param_2[2] = uVar6;
  uVar5 = uVar6;
  goto LAB_01022464;
}



/* @ 0x1022864 */

undefined4 FUN_01022864(int *param_1)

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
        goto LAB_010228bc;
      }
    } while (uVar1 <= (uint)param_1[1]);
    FUN_0100ceb0(s__s__Timed_out__i2c_write_failed_01042914 + 0x20,
                 s_al_i2c_perform_write_01042880 + 0x14);
    FUN_01010c00(s__s__Timed_out__i2c_write_failed_01042914 + 0x20,
                 s_al_i2c_perform_write_01042880 + 0x14);
    uVar3 = 0xffffffc2;
    iVar2 = *param_1;
  }
  else {
LAB_010228bc:
    uVar3 = 0;
  }
  if ((*(uint *)(iVar2 + 0x70) & 1) != 0) {
    uVar1 = 0;
    do {
      uVar1 = uVar1 + 1;
      (*(code *)param_1[2])(param_1[3]);
      if ((*(uint *)(iVar2 + 0x70) & 1) == 0) goto LAB_01022904;
    } while (uVar1 <= (uint)param_1[1]);
    FUN_0100ceb0(s__s__Timed_out_waiting_for_stop_d_01042938 + 0x28,
                 s_al_i2c_perform_write_01042880 + 0x14);
    FUN_01010c00(s__s__Timed_out_waiting_for_stop_d_01042938 + 0x28,
                 s_al_i2c_perform_write_01042880 + 0x14);
    uVar3 = 0xffffffc2;
LAB_01022904:
    iVar2 = *param_1;
  }
  *(ushort *)(iVar2 + 0x6c) = *(ushort *)(iVar2 + 0x6c) & 0xfffe;
  return uVar3;
}



/* @ 0x1022a80 */

undefined4 FUN_01022a80(int *param_1,ushort param_2,byte *param_3,uint param_4)

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
        FUN_0100ceb0(s___i2c_regs_base__01042900 + 0x10,s___bootstrap__01042870 + 0xc);
        FUN_01010c00(s___i2c_regs_base__01042900 + 0x10,s___bootstrap__01042870 + 0xc);
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



/* @ 0x1022ce8 */

int FUN_01022ce8(int *param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4,int param_5,
                undefined1 *param_6)

{
  int iVar1;
  undefined2 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  bool bVar6;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_i2c_c_010428c4 + 0x20,1,
                 s_al_i2c_read_010428b8 + 8,s_al_i2c_init_010428ac + 8,0x13e);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_i2c_c_010428c4 + 0x20,1,
                 s_al_i2c_read_010428b8 + 8,s_al_i2c_init_010428ac + 8,0x13e);
    FUN_01000458(0);
  }
  iVar4 = *param_1;
  iVar1 = FUN_01022a80(param_1,param_2,param_4,param_3);
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
          if ((*(uint *)(iVar4 + 0x70) & 8) != 0) break;
          bVar6 = (uint)(param_1[1] * param_5) < uVar3;
          uVar3 = uVar3 + 1;
          if (bVar6) {
            FUN_01022864(param_1);
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
    iVar1 = FUN_01022864(param_1);
    return iVar1;
  }
  return iVar1;
}



/* @ 0x1022e6c */

void FUN_01022e6c(int *param_1,int param_2)

{
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_uart__010429f8 + 0x20,1,
                 s_al_uart_is_input_available_010429dc + 0x18,
                 s__s__Timed_out_waiting_for_IDLE_s_01042964 + 0x24,0x37);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_pbs_al_hal_uart__010429f8 + 0x20,1,
                 s_al_uart_is_input_available_010429dc + 0x18,
                 s__s__Timed_out_waiting_for_IDLE_s_01042964 + 0x24,0x37);
    FUN_01000458(0);
  }
  *param_1 = param_2;
  return;
}



/* @ 0x1022f00 */

undefined4 FUN_01022f00(undefined4 *param_1,int param_2,undefined1 *param_3,uint param_4)

{
  undefined4 uVar1;
  uint uVar2;
  undefined1 *puVar3;
  undefined1 *puVar4;
  uint uVar5;
  int iVar6;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_handle_init_0104298c + 0x10,
                 0x7f);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_handle_init_0104298c + 0x10,
                 0x7f);
    FUN_01000458(0);
  }
  if (param_3 == (undefined1 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_01042a38,1,s_al_uart_is_input_available_010429dc + 0x18
                 ,s_al_uart_handle_init_0104298c + 0x10,0x80);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_01042a38,1,s_al_uart_is_input_available_010429dc + 0x18
                 ,s_al_uart_handle_init_0104298c + 0x10,0x80);
    FUN_01000458(0);
  }
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___buffer__01042a3c + 8,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_handle_init_0104298c + 0x10,
                 0x81);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___buffer__01042a3c + 8,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_handle_init_0104298c + 0x10,
                 0x81);
    FUN_01000458(0);
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
          if (((uint)(param_4 != 0xffffffff) & param_4 >> 0x1f) != 0) goto LAB_01022f80;
          if (param_2 == 0) {
            return 0;
          }
          iVar6 = 0;
        }
      } while (param_4 == 0xffffffff);
      iVar6 = iVar6 + 1;
      FUN_01007590(1);
    } while (iVar6 <= (int)param_4);
LAB_01022f80:
    FUN_0100ceb0(s___bytelen__01042a48 + 8,s_al_uart_handle_init_0104298c + 0x10);
    FUN_01010c00(s___bytelen__01042a48 + 8,s_al_uart_handle_init_0104298c + 0x10);
    uVar1 = 0xffffffc2;
  }
  return uVar1;
}



/* @ 0x10233b4 */

int FUN_010233b4(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_write_010429b0 + 0xc,0x18e);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___uart_regs_base__01042a1c + 0x10,1,
                 s_al_uart_is_input_available_010429dc + 0x18,s_al_uart_write_010429b0 + 0xc,0x18e);
    FUN_01000458(0);
  }
  return 0x40 - (*(uint *)(*param_1 + 0x80) & 0xff);
}




/* @ 0x10234e8 -- WARNING: Control flow encountered bad instruction data */

undefined4 FUN_010234e8(undefined4 param_1,undefined4 param_2)

{
  switch(param_2) {
  case 0:
    return param_1;
  case 1:
    return param_1;
  case 2:
    return param_1;
  case 3:
    return param_1;
  case 4:
    return param_1;
  case 5:
    return param_1;
  case 6:
    return param_1;
  case 7:
    return param_1;
  case 8:
    return param_1;
  case 9:
    return param_1;
  case 10:
    return param_1;
  case 0xb:
    return param_1;
  case 0xc:
    return param_1;
  case 0xd:
    return param_1;
  case 0xe:
    return param_1;
  case 0xf:
    return param_1;
  case 0x10:
    return param_1;
  case 0x11:
    return param_1;
  case 0x12:
    return param_1;
  case 0x13:
    return param_1;
  case 0x14:
    return param_1;
  case 0x15:
    return param_1;
  case 0x16:
    return param_1;
  case 0x17:
    return param_1;
  case 0x18:
    return param_1;
  case 0x19:
    return param_1;
  case 0x1a:
    return param_1;
  case 0x1b:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  default:
    FUN_0100ceb0(s___dram_remap_transl_base_>___1_<_01042ac8 + 0x24);
    FUN_01010c00(s___dram_remap_transl_base_>___1_<_01042ac8 + 0x24,param_2);
    return 0xffffffea;
  }
}



/* @ 0x102383c */

undefined4 FUN_0102383c(int param_1,undefined4 param_2,uint param_3,uint param_4,uint param_5)

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
  
  iVar2 = FUN_01021fd8();
  if (iVar2 == 0) {
    uVar4 = 0x28;
  }
  else {
    uVar4 = 0x30;
  }
  if (uVar4 < param_5) {
    FUN_0100ceb0(s___log2size__01042b3c + 8,uVar4);
    FUN_01010c00(s___log2size__01042b3c + 8,uVar4);
    uVar3 = 0xffffffea;
  }
  else if (param_5 < 0xf) {
    FUN_0100ceb0(s_addr_map__max_pasw_log2size_is___01042b48 + 0x20);
    FUN_01010c00(s_addr_map__max_pasw_log2size_is___01042b48 + 0x20);
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
        FUN_0100ceb0(s_addr_map__pasw_base_has_to_be_al_01042ba0 + 0x58,uVar4);
        FUN_01010c00(s_addr_map__pasw_base_has_to_be_al_01042ba0 + 0x58,uVar4);
        uVar3 = 0xffffffea;
      }
      else {
        iVar2 = FUN_010234e8(param_1,param_2,&local_30,apuStack_2c);
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
      FUN_0100ceb0(s_addr_map__min_pasw_log2size_is_1_01042b6c + 0x30,uVar6,param_3,param_4,param_5)
      ;
      FUN_01010c00(s_addr_map__min_pasw_log2size_is_1_01042b6c + 0x30,extraout_r1,param_3,param_4,
                   param_5);
      uVar3 = 0xffffffea;
    }
  }
  return uVar3;
}



/* @ 0x1023a10 */

undefined4
FUN_01023a10(int param_1,undefined4 param_2,uint param_3,uint param_4,uint param_5,int param_6,
            byte param_7)

{
  if (3 < param_7) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_addr_map_dram_remap_set_01042a64 + 0x18,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x149);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_addr_map_dram_remap_set_01042a64 + 0x18,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x149);
    FUN_01000458(0);
  }
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14a);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14a);
    FUN_01000458(0);
  }
  if (param_4 == 0 && param_3 < 0x20000000) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___pbs_regs_base__01042a94 + 0x10,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___pbs_regs_base__01042a94 + 0x10,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14c);
    FUN_01000458(0);
  }
  if (param_6 == 0 && param_5 < 0x20000000) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___dram_remap_base_>___1_<<_29))_01042aa8 + 0x1c,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14e);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___dram_remap_base_>___1_<<_29))_01042aa8 + 0x1c,1,
                 s_addr_map__unknown_pasw__d_01042af0 + 0x18,s__s__Timed_out__01042a54 + 0xc,0x14e);
    FUN_01000458(0);
  }
  *(undefined4 *)(param_1 + 0xdc) = 1;
  *(uint *)(param_1 + 0x134) =
       (param_3 >> 0x1d | (param_4 & 0xff) << 3) << 5 | (uint)param_7 |
       (param_5 >> 0x1d | param_6 << 3) << 0x15;
  *(undefined4 *)(param_1 + 0xdc) = 0;
  return 0;
}



/* @ 0x1023c4c */

uint FUN_01023c4c(undefined4 *param_1)

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



/* @ 0x1023c98 */

undefined4 FUN_01023c98(int param_1,int param_2,int param_3,int *param_4)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x71);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_iofic_al_hal_iof_010326c4 + 0x24,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x71);
    FUN_01000458(0);
  }
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_ring_al_hal_pll__01043274 + 0x20,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x72);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_ring_al_hal_pll__01043274 + 0x20,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x72);
    FUN_01000458(0);
  }
  if (param_4 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x73);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,
                 0x73);
    FUN_01000458(0);
  }
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    uVar1 = 0;
    param_4[2] = (int)(s__s__div_val____ref_clk_bypass__01042cbc + 0x1c);
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
    FUN_0100ceb0(&DAT_0104329c,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,param_3);
    FUN_01010c00(&DAT_0104329c,s_addr_map__base___size_doesn_t_fi_01042bfc + 0x28,param_3);
    uVar1 = 0xffffffea;
  }
  return uVar1;
}



/* @ 0x1023ed4 */

undefined4 FUN_01023ed4(int param_1,undefined1 *param_2,int *param_3)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_init_01042c28 + 8,0xb8);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_init_01042c28 + 8,0xb8);
    FUN_01000458(0);
  }
  puVar4 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar2 = FUN_01023c4c(param_1);
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



/* @ 0x1023fd4 */

undefined4 FUN_01023fd4(int *param_1,uint param_2,uint *param_3)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_freq_get_01042c34 + 0xc,0x164);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_freq_get_01042c34 + 0xc,0x164);
    FUN_01000458(0);
  }
  if (param_3 == (uint *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s__s__requested_divider_too_big____01042c74 + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_freq_get_01042c34 + 0xc,0x165);
    FUN_01010c00(s_vectors_01028ea4 + 4,s__s__requested_divider_too_big____01042c74 + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_freq_get_01042c34 + 0xc,0x165);
    FUN_01000458(0);
  }
  if (0xf < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_01042ca4,1,s__s__timed_out__01043264 + 0xc,
                 s_al_pll_freq_get_01042c34 + 0xc,0x166);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_01042ca4,1,s__s__timed_out__01043264 + 0xc,
                 s_al_pll_freq_get_01042c34 + 0xc,0x166);
    FUN_01000458(0);
  }
  iVar5 = *param_1;
  uVar1 = 0;
  *param_3 = 0;
  if (((*(uint *)(iVar5 + 0x1c) & 0xfff) >> 8) - 6 < 2) {
    uVar2 = FUN_01023c4c(param_1);
    uVar3 = *(uint *)(*param_1 + ((param_2 >> 1) + 8) * 4);
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
          FUN_0100ceb0(s___chan_idx_<_16)_01042ca8 + 0x10,s_al_pll_freq_get_01042c34 + 0xc);
          FUN_01010c00(s___chan_idx_<_16)_01042ca8 + 0x10,s_al_pll_freq_get_01042c34 + 0xc);
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



/* @ 0x102421c */

undefined4
FUN_0102421c(int *param_1,uint param_2,uint param_3,int param_4,int param_5,int param_6,int param_7)

{
  int iVar1;
  uint uVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_channel_freq_get_01042c44 + 0x14,0x1a2);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s__s__timed_out__01043264 + 0xc,s_al_pll_channel_freq_get_01042c44 + 0x14,0x1a2);
    FUN_01000458(0);
  }
  if (0xf < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_01042ca4,1,s__s__timed_out__01043264 + 0xc,
                 s_al_pll_channel_freq_get_01042c44 + 0x14,0x1a3);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_01042ca4,1,s__s__timed_out__01043264 + 0xc,
                 s_al_pll_channel_freq_get_01042c44 + 0x14,0x1a3);
    FUN_01000458(0);
  }
  if ((param_3 & 0xfffffc00) != 0) {
    FUN_0100ceb0(s_al_pll_channel_div_set_01042c5c + 0x14,s_al_pll_channel_freq_get_01042c44 + 0x14,
                 param_3,0x3ff);
    FUN_01010c00(s_al_pll_channel_div_set_01042c5c + 0x14,s_al_pll_channel_freq_get_01042c44 + 0x14,
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
  FUN_01007590(3);
  iVar1 = *param_1;
  if (param_7 != 0 && 1 < ((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6) {
    do {
      FUN_01007590(1);
      iVar1 = *param_1;
      param_7 = param_7 + -1;
    } while (1 < ((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6 && param_7 != 0);
  }
  if (((*(uint *)(iVar1 + 0x1c) & 0xfff) >> 8) - 6 < 2) {
    return 0;
  }
  FUN_0100ceb0(s__s__invalid_freq___d__0104324c + 0x14,s_al_pll_channel_freq_get_01042c44 + 0x14);
  FUN_01010c00(s__s__invalid_freq___d__0104324c + 0x14,s_al_pll_channel_freq_get_01042c44 + 0x14);
  return 0xffffff8c;
}



/* @ 0x10244b8 */

undefined4 FUN_010244b8(int *param_1,int param_2,undefined1 param_3,undefined1 param_4)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,&DAT_010432dc,0x62);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,&DAT_010432dc,0x62);
    FUN_01000458(0);
  }
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___sub_timer______void___0___0104337c + 0x18,1,
                 s_al_timer_value_get_0104333c + 0x10,&DAT_010432dc,99);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___sub_timer______void___0___0104337c + 0x18,1,
                 s_al_timer_value_get_0104333c + 0x10,&DAT_010432dc,99);
    FUN_01000458(0);
  }
  *param_1 = param_2;
  *(undefined1 *)(param_1 + 1) = param_3;
  *(undefined1 *)((int)param_1 + 5) = param_4;
  return 0;
}



/* @ 0x10245d4 */

void FUN_010245d4(int *param_1,int param_2,uint param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_init_010432e0 + 0xc,0x77);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_init_010432e0 + 0xc,0x77);
    FUN_01000458(0);
  }
  if (param_3 < 3) {
    uVar1 = *(uint *)(&DAT_010432d0 + param_3 * 4);
    uVar2 = ~(*(uint *)(s__s__invalid_ref_clk_freq_enum____010432a0 + param_3 * 4 + 0x24) | 0xe);
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
      uVar1 = uVar1 | 8;
    }
  }
  *(uint *)(*param_1 + 8) = *(uint *)(*param_1 + 8) & uVar2 | uVar1;
  return;
}



/* @ 0x10246dc */

void FUN_010246dc(undefined4 *param_1,undefined4 param_2)

{
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_config_set_010432f0 + 0x10,0x8c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_config_set_010432f0 + 0x10,0x8c);
    FUN_01000458(0);
  }
  *(undefined4 *)*param_1 = param_2;
  return;
}



/* @ 0x1024774 */

void FUN_01024774(int *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_load_set_01043304 + 0x10,0xa9);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_load_set_01043304 + 0x10,0xa9);
    FUN_01000458(0);
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



/* @ 0x1024824 */

void FUN_01024824(int *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_int_enable_01043318 + 0x10,0xbf);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_int_enable_01043318 + 0x10,0xbf);
    FUN_01000458(0);
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



/* @ 0x10248d4 */

undefined4 FUN_010248d4(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_enable_0104332c + 0xc,0xf8);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__01043350 + 0x28,1,
                 s_al_timer_value_get_0104333c + 0x10,s_al_timer_enable_0104332c + 0xc,0xf8);
    FUN_01000458(0);
  }
  return *(undefined4 *)(*param_1 + 4);
}



/* @ 0x1024968 */


undefined4 FUN_01024968(int *param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x3f);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x3f);
    FUN_01000458(0);
  }
  if (param_2 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___thermal_sensor_handle__010434dc + 0x18,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x40);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___thermal_sensor_handle__010434dc + 0x18,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x40);
    FUN_01000458(0);
    iVar2 = _UndefinedInstruction;
  }
  else {
    iVar2 = param_2[1];
  }
  if (iVar2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___params__010434f8 + 8,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x41);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___params__010434f8 + 8,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x41);
    FUN_01000458(0);
    iVar2 = *param_2;
  }
  else {
    iVar2 = *param_2;
  }
  if (iVar2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___params_>thermal_sensor_reg_bas_01043504 + 0x20,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x42);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___params_>thermal_sensor_reg_bas_01043504 + 0x20,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s___timer_regs_base______void___0__01043398 + 0x20,0x42);
    FUN_01000458(0);
    iVar2 = *param_2;
  }
  *param_1 = param_2[1];
  iVar2 = FUN_01021fd8(iVar2);
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



/* @ 0x1024b90 */

void FUN_01024b90(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  local_c = 0;
  local_14 = param_3;
  local_10 = param_2;
  FUN_01024968(param_1,&local_14);
  return;
}



/* @ 0x1024bb8 */

void FUN_01024bb8(undefined4 *param_1,uint param_2)

{
  uint uVar1;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_handle_init_ex_010433bc + 0x20,0x6c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_handle_init_ex_010433bc + 0x20,0x6c);
    FUN_01000458(0);
  }
  uVar1 = *(uint *)*param_1;
  *(uint *)*param_1 = (param_2 ^ uVar1) & 0xf ^ uVar1;
  return;
}



/* @ 0x1024c60 */

void FUN_01024c60(int *param_1,int param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_trim_set_010433e0 + 0x18,0xd0);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_trim_set_010433e0 + 0x18,0xd0);
    FUN_01000458(0);
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



/* @ 0x1024d14 */

bool FUN_01024d14(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_enable_set_010433fc + 0x1c,0xe9);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_enable_set_010433fc + 0x1c,0xe9);
    FUN_01000458(0);
  }
  return (*(uint *)(*param_1 + 0xc) & 0x60000000) == 0x60000000;
}



/* @ 0x1024db8 */

void FUN_01024db8(int *param_1)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_is_ready_0104341c + 0x18,0x10c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__010434a4 + 0x34,1,
                 s_al_thermal_sensor_readout_get_01043484 + 0x1c,
                 s_al_thermal_sensor_is_ready_0104341c + 0x18,0x10c);
    FUN_01000458(0);
  }
  *(undefined4 *)(*param_1 + 8) = 2;
  return;
}



/* @ 0x1024fdc */

undefined4 FUN_01024fdc(int *param_1,int param_2,int param_3)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__0104356c + 0x28,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x33)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__0104356c + 0x28,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x33)
    ;
    FUN_01000458(0);
  }
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___otp_handle__01043598 + 0xc,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x34)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s___otp_handle__01043598 + 0xc,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x34)
    ;
    FUN_01000458(0);
  }
  if (param_3 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x35)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_al_otp_read_word_01043558 + 0x10,s___params_>pbs_regs_base__01043528 + 0x18,0x35)
    ;
    FUN_01000458(0);
  }
  *param_1 = param_2;
  param_1[1] = param_3;
  return 0;
}



/* @ 0x102515c */

undefined4 FUN_0102515c(int *param_1,uint param_2)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__0104356c + 0x28,1,
                 s_al_otp_read_word_01043558 + 0x10,s_al_otp_handle_init_01043544 + 0x10,0x45);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_services_al__0104356c + 0x28,1,
                 s_al_otp_read_word_01043558 + 0x10,s_al_otp_handle_init_01043544 + 0x10,0x45);
    FUN_01000458(0);
  }
  if (0x1e < param_2) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___otp_regs_base__010435a8 + 0x10,1,
                 s_al_otp_read_word_01043558 + 0x10,s_al_otp_handle_init_01043544 + 0x10,0x46);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___otp_regs_base__010435a8 + 0x10,1,
                 s_al_otp_read_word_01043558 + 0x10,s_al_otp_handle_init_01043544 + 0x10,0x46);
    FUN_01000458(0);
  }
  return *(undefined4 *)(*param_1 + (param_2 + 0x20) * 4);
}



/* @ 0x1025260 */

void FUN_01025260(uint *param_1,uint param_2)

{
  if (param_1 == (uint *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,s___word_idx_<_31)_010435bc + 0x10,0x16)
    ;
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,s___word_idx_<_31)_010435bc + 0x10,0x16)
    ;
    FUN_01000458(0);
  }
  *param_1 = (*(uint *)(param_2 + 0x4400) & 0xffff) >> 8;
  param_1[1] = param_2;
  return;
}



/* @ 0x1025300 */

void FUN_01025300(int *param_1,uint *param_2,int param_3)

{
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_handle_init_010435d0 + 0x18,0x26);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_handle_init_010435d0 + 0x18,0x26);
    FUN_01000458(0);
  }
  if (param_2 == (uint *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s_al_sys_fabric_core_aarch32_setup_010436a4 + 0x24,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_handle_init_010435d0 + 0x18,0x27);
    FUN_01010c00(s_vectors_01028ea4 + 4,s_al_sys_fabric_core_aarch32_setup_010436a4 + 0x24,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_handle_init_010435d0 + 0x18,0x27);
    FUN_01000458(0);
  }
  if (*param_2 < 4) {
    if (param_3 == 0) goto LAB_0102539c;
  }
  else if (param_3 != 0) goto LAB_0102539c;
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s___fabric_handle__010436cc + 0x10,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_handle_init_010435d0 + 0x18,0x2c);
  FUN_01010c00(s_vectors_01028ea4 + 4,s___fabric_handle__010436cc + 0x10,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_handle_init_010435d0 + 0x18,0x2c);
  FUN_01000458(0);
LAB_0102539c:
  *param_1 = (int)param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0x1025484 */

void FUN_01025484(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  
  if (param_1 == (undefined4 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20,0x18b);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20,0x18b);
    FUN_01000458(0);
  }
  if (*(uint *)*param_1 < 4) {
    uVar1 = ((uint *)*param_1)[1];
    *(undefined4 *)(uVar1 + 0x100) = param_2;
    *(undefined4 *)(uVar1 + 0x104) = param_3;
    return;
  }
  FUN_0100ceb0(s___core_<_4)_01043764 + 8,s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20);
  FUN_01010c00(s___core_<_4)_01043764 + 8,s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20,
               *(undefined4 *)*param_1);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20,0x195);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_cluster_handle_ini_010435ec + 0x20,0x195);
  FUN_01000458(0);
  return;
}



/* @ 0x10255e0 */

void FUN_010255e0(int *param_1,uint param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,499);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,499);
    FUN_01000458(0);
  }
  if (param_2 < 4) {
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,500);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,500);
    FUN_01000458(0);
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  if (uVar1 < 4) {
    *(uint *)(puVar2[1] + 0x24) = 1 << (param_2 + 4 & 0xff) | 3;
    return;
  }
  FUN_0100ceb0(s___core_<_4)_01043764 + 8,s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24);
  FUN_01010c00(s___core_<_4)_01043764 + 8,s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,
               *(undefined4 *)*param_1);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,0x201);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_cluster_pd_pu_time_01043610 + 0x24,0x201);
  FUN_01000458(0);
  return;
}



/* @ 0x10257a4 */

void FUN_010257a4(int *param_1,uint param_2)

{
  uint uVar1;
  uint *puVar2;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x20b);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x20b);
    FUN_01000458(0);
  }
  if (param_2 < 4) {
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x20c);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x20c);
    FUN_01000458(0);
    puVar2 = (uint *)*param_1;
    uVar1 = *puVar2;
  }
  if (uVar1 < 4) {
    uVar1 = *(uint *)(puVar2[1] + 8);
    *(uint *)(puVar2[1] + 8) = ~uVar1 & 1 << (param_2 & 0xff) ^ uVar1;
    return;
  }
  FUN_0100ceb0(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_power_on_rese_01043638 + 0x20);
  FUN_01010c00(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,
               *(undefined4 *)*param_1);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x216);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_power_on_rese_01043638 + 0x20,0x216);
  FUN_01000458(0);
  return;
}



/* @ 0x102596c */

void FUN_0102596c(int *param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x222);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x222);
    FUN_01000458(0);
  }
  if (param_2 < 4) {
    if (param_3 == 0 && param_4 == 0) {
LAB_01025a60:
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s_____fabric_handle_>ver_<__3)_&&_(_010436e0 + 100,1,
                   s____msg___0xffff_____msg__01043794 + 0x18,
                   s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x224);
      FUN_01010c00(s_vectors_01028ea4 + 4,s_____fabric_handle_>ver_<__3)_&&_(_010436e0 + 100,1,
                   s____msg___0xffff_____msg__01043794 + 0x18,
                   s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x224);
      FUN_01000458(param_3 | param_4);
      puVar2 = (uint *)*param_1;
      uVar1 = *puVar2;
      goto joined_r0x01025ac8;
    }
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x223);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x223);
    FUN_01000458(0);
    if (param_3 == 0 && param_4 == 0) goto LAB_01025a60;
  }
  puVar2 = (uint *)*param_1;
  uVar1 = *puVar2;
joined_r0x01025ac8:
  if (uVar1 < 4) {
    uVar1 = puVar2[1];
    iVar3 = uVar1 + param_2 * 0x100;
    *(uint *)(iVar3 + 0x2048) = param_4;
    *(uint *)(iVar3 + 0x204c) = param_3;
    *(undefined4 *)(uVar1 + (param_2 + 0x20) * 0x100 + 4) = 1;
    return;
  }
  FUN_0100ceb0(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_reset_deasser_0104365c + 0x20);
  FUN_01010c00(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,
               *(undefined4 *)*param_1);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x235);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_reset_deasser_0104365c + 0x20,0x235);
  FUN_01000458(0);
  return;
}



/* @ 0x1025bcc */

void FUN_01025bcc(int *param_1,uint param_2,undefined4 *param_3,undefined4 *param_4)

{
  int iVar1;
  uint uVar2;
  uint *puVar3;
  
  if (param_1 == (int *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25d);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_sys_fabric_al_ha_010437b0 + 0x34,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25d);
    FUN_01000458(0);
  }
  if (param_2 < 4) {
    if (param_3 == (undefined4 *)0x0 && param_4 == (undefined4 *)0x0) {
LAB_01025cb8:
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s_____fabric_handle_>ver_<__3)_&&_(_010436e0 + 100,1,
                   s____msg___0xffff_____msg__01043794 + 0x18,
                   s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25f);
      FUN_01010c00(s_vectors_01028ea4 + 4,s_____fabric_handle_>ver_<__3)_&&_(_010436e0 + 100,1,
                   s____msg___0xffff_____msg__01043794 + 0x18,
                   s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25f);
      FUN_01000458((uint)param_3 | (uint)param_4);
      puVar3 = (uint *)*param_1;
      uVar2 = *puVar3;
      goto joined_r0x01025d20;
    }
  }
  else {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25e);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___entry_high____entry_low__01043748 + 0x18,1,
                 s____msg___0xffff_____msg__01043794 + 0x18,
                 s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x25e);
    FUN_01000458(0);
    if (param_3 == (undefined4 *)0x0 && param_4 == (undefined4 *)0x0) goto LAB_01025cb8;
  }
  puVar3 = (uint *)*param_1;
  uVar2 = *puVar3;
joined_r0x01025d20:
  if (uVar2 < 4) {
    iVar1 = puVar3[1] + param_2 * 0x100;
    *param_4 = *(undefined4 *)(iVar1 + 0x2028);
    *param_3 = *(undefined4 *)(iVar1 + 0x202c);
    return;
  }
  FUN_0100ceb0(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20);
  FUN_01010c00(s___core_<_4)_01043764 + 8,s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,
               *(undefined4 *)*param_1);
  FUN_0100ceb0(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x269);
  FUN_01010c00(s_vectors_01028ea4 + 4,s_services_pcie_al_pci_lib_src_pci_010323cc + 0x24,1,
               s____msg___0xffff_____msg__01043794 + 0x18,
               s_al_sys_fabric_core_aarch64_setup_01043680 + 0x20,0x269);
  FUN_01000458(0);
  return;
}



/* @ 0x1025e24 */

undefined4 FUN_01025e24(int param_1,int param_2,byte *param_3)

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
  if ((*param_3 & 8) != 0) {
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



/* @ 0x1025ee0 */

undefined4
FUN_01025ee0(undefined1 *param_1,undefined1 param_2,int param_3,int param_4,undefined *param_5,
            undefined4 param_6,int param_7)

{
  if (param_1 == (undefined1 *)0x0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_io_fabric_al_hal_01043810 + 0x2c,1,
                 s_al_unit_adapter_handle_init_010437f4 + 0x18,s___handle__010437e8 + 8,0x90);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_io_fabric_al_hal_01043810 + 0x2c,1,
                 s_al_unit_adapter_handle_init_010437f4 + 0x18,s___handle__010437e8 + 8,0x90);
    FUN_01000458(0);
  }
  *param_1 = param_2;
  if (param_7 == 0) {
    param_7 = param_3;
  }
  *(int *)(param_1 + 4) = param_3;
  if (param_4 == 0) {
    param_4 = 0x1025ebc;
  }
  if (param_5 == (undefined *)0x0) {
    param_5 = &DAT_01025ecc;
  }
  *(int *)(param_1 + 8) = param_4;
  *(undefined **)(param_1 + 0xc) = param_5;
  *(undefined4 *)(param_1 + 0x10) = param_6;
  *(int *)(param_1 + 0x14) = param_7;
  return 0;
}



/* @ 0x1025fd8 */

void FUN_01025fd8(int param_1,int param_2,int param_3)

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



/* @ 0x1026048 */

void FUN_01026048(int param_1,int param_2,int param_3,int param_4,int param_5)

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



/* @ 0x1026150 */

undefined4 FUN_01026150(undefined4 param_1,int param_2,undefined4 param_3)

{
  (**(code **)(s___unit_adapter__01043840 + param_2 * 4 + 0xc))(param_1,param_3);
  return 0;
}



/* @ 0x102617c */

void FUN_0102617c(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0xd8) = param_2;
  return;
}




/* @ 0x1026184 -- WARNING: Control flow encountered bad instruction data */

uint FUN_01026184(int param_1,int param_2,int param_3)

{
  int *piVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  uint uVar6;
  byte *pbVar7;
  undefined4 extraout_r1;
  uint uVar8;
  uint uVar9;
  int *piVar10;
  uint extraout_r3;
  uint extraout_r3_00;
  int iVar11;
  undefined4 uVar12;
  int *piVar13;
  uint uVar14;
  int iVar15;
  uint uVar16;
  int *piVar17;
  uint uVar18;
  uint unaff_r10;
  uint uVar19;
  uint extraout_r12;
  uint uVar20;
  uint extraout_r12_00;
  uint uVar21;
  ulonglong uVar22;
  undefined8 uVar23;
  uint local_1f8;
  uint local_1f4;
  int local_1ec [4];
  int local_1dc;
  byte local_1d8 [4];
  undefined1 auStack_1d4 [16];
  byte local_1c4 [124];
  undefined4 local_148;
  byte local_140;
  int local_13c;
  int local_138;
  int local_134;
  int local_130;
  char local_12c;
  undefined1 local_12b;
  undefined1 local_12a;
  undefined1 auStack_120 [16];
  undefined1 auStack_110 [28];
  byte local_f4;
  int local_f0;
  int local_ec;
  int local_e8;
  int local_e4;
  undefined1 local_e0;
  undefined1 local_df;
  undefined1 local_de;
  byte local_db;
  undefined1 auStack_d4 [16];
  undefined1 auStack_c4 [28];
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
  undefined1 auStack_4c [8];
  undefined4 local_44;
  undefined1 local_40;
  undefined4 local_3c;
  undefined1 local_38;
  undefined4 local_34 [2];
  undefined1 auStack_2c [8];
  
  if (param_1 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s_al_serdes_init_clk_route_010438fc + 0x18,&DAT_01043860,0xe3);
    FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__drivers_serdes_al_hal_se_01032ffc + 0x28,1,
                 s_al_serdes_init_clk_route_010438fc + 0x18,&DAT_01043860,0xe3);
    FUN_01000458(0);
  }
  if (param_2 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_al_serdes_init_clk_route_010438fc + 0x18,&DAT_01043860,0xe4);
    FUN_01010c00(s_vectors_01028ea4 + 4,s___window_size_<_4)_01042a80 + 0x10,1,
                 s_al_serdes_init_clk_route_010438fc + 0x18,&DAT_01043860,0xe4);
    FUN_01000458(0);
  }
  if (param_3 == 0) {
    FUN_0100ceb0(s_vectors_01028ea4 + 4,&DAT_01042738,1,s_al_serdes_init_clk_route_010438fc + 0x18,
                 &DAT_01043860,0xe5);
    FUN_01010c00(s_vectors_01028ea4 + 4,&DAT_01042738,1,s_al_serdes_init_clk_route_010438fc + 0x18,
                 &DAT_01043860,0xe5);
    FUN_01000458(0);
  }
  FUN_010129dc(local_1d8,param_3,0x188);
  local_1ec[3] = param_1 + 0x2f4;
  local_1ec[1] = param_1 + 0xfc;
  local_1ec[2] = param_1 + 0x1f8;
  local_1dc = param_1 + 0x3f0;
  local_1ec[0] = param_1;
  uVar3 = FUN_01021fd8(param_2);
  uVar9 = (uint)local_1d8[0];
  if (uVar3 == 1) {
    uVar3 = *(uint *)(param_2 + 0xe8);
    uVar6 = *(uint *)(param_2 + 0x1d4);
    uVar8 = *(uint *)(param_2 + 0x228);
    uVar14 = *(uint *)(param_2 + 0x22c);
    uVar16 = *(uint *)(param_2 + 0x230);
    if (uVar9 == 1) goto LAB_010266d0;
    if (uVar9 != 3) {
      uVar4 = uVar3 & 0xffffffcc | 2;
      uVar20 = 0x20;
      goto LAB_010266cc;
    }
    uVar4 = uVar3 & 0xffffffcc | 1;
    uVar20 = 0x10;
    goto switchD_010264f8_caseD_7;
  }
LAB_01026204:
  piVar13 = local_1ec;
  uVar18 = (uint)local_1c4[4];
  iVar11 = 0;
  uVar16 = (uint)local_db;
  bVar5 = false;
  iVar15 = local_1ec[0];
  piVar10 = piVar13;
  if (uVar9 == 1) goto LAB_01026238;
  do {
    uVar3 = FUN_0101de18(iVar15,bVar5);
LAB_01026238:
    do {
      iVar11 = iVar11 + 1;
      piVar17 = piVar10 + 0x13;
      if (iVar11 == 4) {
        iVar15 = 0;
        piVar10 = piVar13;
        goto LAB_01026280;
      }
      piVar1 = piVar10 + 0x18;
      bVar5 = false;
      piVar10 = piVar17;
    } while ((char)*piVar1 == '\x01');
    iVar15 = piVar13[iVar11];
    if (iVar11 == 2) {
      bVar5 = local_12c == '\x03';
    }
  } while( true );
LAB_01026280:
  *(char *)(piVar10 + 0xb) = (char)uVar18;
  switch((char)piVar10[10]) {
  case '\0':
    break;
  case '\x01':
    goto switchD_0102628c_caseD_1;
  case '\x02':
  case '\x03':
    goto switchD_0102628c_caseD_2;
  default:
    FUN_0100ceb0(s____HAL__services_serdes_al_serde_01043918 + 0x28,
                 s_al_serdes_init_cores_01043874 + 0x14);
    uVar3 = 0xffffffea;
    FUN_01010c00(s____HAL__services_serdes_al_serde_01043918 + 0x28,
                 s_al_serdes_init_cores_01043874 + 0x14,local_1c4[iVar15 * 0x4c]);
    FUN_0100ceb0(s__s__clk_src_r2l_invalid___d___01043944 + 0x1c,
                 s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
    FUN_01010c00(s__s__clk_src_r2l_invalid___d___01043944 + 0x1c,
                 s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
    goto LAB_0102634c;
  }
switchD_0102628c_caseD_2:
  uVar18 = 0;
  iVar15 = iVar15 + 1;
  piVar10 = piVar10 + 0x13;
  if (iVar15 == 5) goto code_r0x010262b8;
  goto LAB_01026280;
code_r0x010262b8:
  iVar15 = 4;
  piVar10 = piVar13;
  do {
    while( true ) {
      cVar2 = *(char *)((int)piVar10 + 0x159);
      *(char *)((int)piVar10 + 0x15d) = (char)uVar16;
      piVar17 = piVar13;
      if (cVar2 != '\x01') break;
      iVar15 = iVar15 + -1;
      uVar16 = (uint)*(byte *)((int)piVar10 + 0x15b);
      piVar10 = piVar10 + -0x13;
      if (iVar15 == -1) goto switchD_010264f8_caseD_1;
    }
    if (cVar2 == '\0') {
      uVar16 = 0;
    }
    else if (cVar2 != '\x04') {
      FUN_0100ceb0(s__s__al_serdes_group_r2l_clk_freq_01043964 + 0x34,
                   s_al_serdes_group_r2l_clk_freq_upd_0104388c + 0x20);
      uVar3 = 0xffffffea;
      FUN_01010c00(s__s__al_serdes_group_r2l_clk_freq_01043964 + 0x34,
                   s_al_serdes_group_r2l_clk_freq_upd_0104388c + 0x20,local_1c4[iVar15 * 0x4c + 1]);
      FUN_0100ceb0(s__s__clk_src_l2r_invalid___d___0104399c + 0x1c,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
      FUN_01010c00(s__s__clk_src_l2r_invalid___d___0104399c + 0x1c,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
      goto LAB_0102634c;
    }
    iVar15 = iVar15 + -1;
    piVar10 = piVar10 + -0x13;
  } while (iVar15 != -1);
  do {
    iVar15 = 0;
    do {
      switch(*(byte *)((int)piVar17 + 0x2a) - 1) {
      case 0:
        break;
      case 1:
      case 2:
                    /* WARNING: Bad instruction - Truncating control flow here */
        halt_baddata();
      case 3:
        *(undefined1 *)(piVar13 + 0x67) = *(undefined1 *)((int)piVar17 + 0x2d);
        break;
      default:
        FUN_0100ceb0(s__s__al_serdes_group_l2r_clk_freq_010439bc + 0x34,
                     s_al_serdes_group_l2r_clk_freq_upd_010438b0 + 0x20);
        uVar3 = 0xffffffea;
        FUN_01010c00(s__s__al_serdes_group_l2r_clk_freq_010439bc + 0x34,
                     s_al_serdes_group_l2r_clk_freq_upd_010438b0 + 0x20,local_1c4[iVar15 * 0x4c + 2]
                    );
        FUN_0100ceb0(s__s__clk_src_core_invalid___d___010439f4 + 0x1c,
                     s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
        FUN_01010c00(s__s__clk_src_core_invalid___d___010439f4 + 0x1c,
                     s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,iVar15);
        goto LAB_0102634c;
      }
      iVar15 = iVar15 + 1;
      piVar17 = piVar17 + 0x13;
      piVar13 = piVar13 + 2;
    } while (iVar15 != 5);
    if ((local_1d8[0] != 1) &&
       (uVar3 = FUN_0101da60(local_1ec[0],auStack_4c,local_1c4[0],local_1c4[1],local_1c4[2]),
       uVar3 != 0)) {
      uVar12 = 0;
LAB_01027308:
      FUN_0100ceb0(s__s__al_serdes_group_core_clk_fre_01043a14 + 0x34,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,uVar12);
      FUN_01010c00(s__s__al_serdes_group_core_clk_fre_01043a14 + 0x34,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,uVar12);
LAB_0102634c:
      FUN_0100ceb0(s__s__al_serdes_init_cores_failed__01043cd8 + 0x20,&DAT_01043860);
      FUN_01010c00(s__s__al_serdes_init_cores_failed__01043cd8 + 0x20,&DAT_01043860);
      return uVar3;
    }
    if ((local_1c4[0x38] != 1) &&
       (uVar3 = FUN_0101da60(local_1ec[1],&local_44,local_1c4[0x4c],local_1c4[0x4d],local_1c4[0x4e])
       , uVar3 != 0)) {
      uVar12 = 1;
      goto LAB_01027308;
    }
    if ((local_140 != 1) &&
       (uVar3 = FUN_0101dc0c(local_1ec[2],&local_3c,local_12c,local_12b,local_12a), uVar3 != 0)) {
      uVar12 = 2;
LAB_01027340:
      FUN_0100ceb0(s__s__al_serdes_hssp_group_ictl_pm_01043a4c + 0x34,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,uVar12);
      FUN_01010c00(s__s__al_serdes_hssp_group_ictl_pm_01043a4c + 0x34,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,uVar12);
      goto LAB_0102634c;
    }
    if ((local_f4 != 1) &&
       (uVar3 = FUN_0101dc0c(local_1ec[3],local_34,local_e0,local_df,local_de), uVar3 != 0)) {
      uVar12 = 3;
      goto LAB_01027340;
    }
    uVar21 = 0x10264e8;
    uVar22 = FUN_01021be4(local_1dc,auStack_2c,local_94,local_93,local_92);
    uVar6 = (uint)(uVar22 >> 0x20);
    uVar3 = (uint)uVar22;
    if (uVar3 != 0) {
      FUN_0100ceb0(s__s__al_serdes_hssp_group_ictl_pm_01043a84 + 0x38,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,4);
      FUN_01010c00(s__s__al_serdes_hssp_group_ictl_pm_01043a84 + 0x38,
                   s_al_serdes_group_core_clk_freq_up_010438d4 + 0x24,4);
      goto LAB_0102634c;
    }
    uVar8 = (uint)local_1d8[0];
    param_2 = 5;
    uVar4 = 0;
    piVar13 = (int *)0x0;
    uVar14 = 0;
    uVar3 = 0;
    uVar19 = 0;
    uVar9 = extraout_r3;
    uVar20 = extraout_r12;
    switch(uVar8) {
    case 0:
      goto switchD_010264f8_caseD_0;
    case 1:
switchD_010264f8_caseD_1:
      break;
    case 2:
      goto switchD_010264f8_caseD_2;
    case 3:
    case 4:
      goto LAB_010268b4;
    case 5:
    case 6:
      goto LAB_010268b4;
    case 7:
    case 8:
    case 9:
    case 10:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x11:
      goto switchD_010264f8_caseD_7;
    case 0x12:
      goto switchD_010264f8_caseD_12;
    default:
      FUN_0100ceb0(s__s__al_serdes_25g_group_ictl_pma_01043ac0 + 0x34,
                   s_al_serdes_init_01043864 + 0xc);
      uVar19 = 0xffffffea;
      FUN_01010c00(s__s__al_serdes_25g_group_ictl_pma_01043ac0 + 0x34,
                   s_al_serdes_init_01043864 + 0xc,local_1d8[0]);
      goto switchD_01026640_caseD_1;
    }
  } while( true );
switchD_010264f8_caseD_7:
LAB_010266cc:
  uVar3 = uVar20 | uVar4;
LAB_010266d0:
  uVar4 = (uint)local_1c4[0x38];
  if (uVar4 != 1) {
    if (uVar4 == 6) {
      uVar20 = 0x200000;
      uVar3 = uVar3 & 0xffccccff | 0x22200;
    }
    else {
      uVar20 = 0x100000;
      uVar3 = uVar3 & 0xffccccff | 0x11100;
    }
    uVar3 = uVar20 | uVar3;
  }
  uVar20 = (uint)local_f4;
  uVar21 = (uint)local_140;
  uVar18 = uVar20 & 0xfd;
  piVar17 = (int *)(uVar21 & 0xfd);
  local_1f4 = (uint)(uVar18 == 0x10);
  unaff_r10 = (uint)(piVar17 == (int *)&DataAbort);
  local_1f8 = (uint)(uVar20 == 0x14);
  if (uVar21 == 1) {
    if (uVar20 != 1) goto LAB_01026778;
    unaff_r10 = 0;
LAB_01026814:
    uVar8 = uVar8 & 0xffccffff;
LAB_01026818:
    if ((uVar20 != 1) && (uVar8 = uVar8 & 0xefffffff, uVar20 == 0x14)) {
      uVar8 = uVar8 | 0x10000000;
    }
  }
  else {
    if (uVar21 == 9) {
      uVar3 = uVar3 & 0xccffffff | 0x22000000;
LAB_010269a0:
      uVar19 = 0x2000;
      uVar6 = uVar6 & 0xffff888c | 0x222;
    }
    else {
      uVar3 = uVar3 & 0xccffffff | 0x11000000;
      if (uVar21 == 0xf) {
        uVar19 = 0x1000;
        uVar6 = uVar6 & 0xffff888c | 0x111;
      }
      else {
        if (uVar21 != 0x13) goto LAB_010269a0;
        uVar19 = 0x4000;
        uVar6 = uVar6 & 0xffff888c | 0x442;
      }
    }
    uVar6 = uVar19 | uVar6;
    if (uVar20 != 1) {
LAB_01026778:
      if (uVar20 == 0xe) {
        uVar22 = CONCAT44(uVar6,uVar3) & 0x888cffffffffffff | 0x442000000000000;
        local_1f8 = 0x40000000;
      }
      else {
        if (uVar20 == 0x14) {
          uVar22 = CONCAT44(uVar6,uVar3) & 0x888cffffffffffff | 0x221000000000000;
          local_1f8 = 0x20000000;
          goto switchD_01026584_caseD_12;
        }
        uVar22 = CONCAT44(uVar6,uVar3) & 0x888cffffffffffff | 0x111000000000000;
        local_1f8 = 0x10000000;
      }
      goto LAB_010267a0;
    }
    unaff_r10 = (uint)(piVar17 == (int *)&DataAbort || uVar21 == 0x13);
    if (unaff_r10 != 0) goto LAB_010267bc;
LAB_01026d38:
    unaff_r10 = (uint)(uVar20 == 0x14);
    if (local_1f4 != 0 || unaff_r10 != 0) {
LAB_01026cc8:
      uVar8 = uVar8 & 0xfffffcff | 0x200;
      if (piVar17 == (int *)&DataAbort) goto LAB_01026da0;
    }
LAB_010267fc:
    if (uVar21 == 0x11) {
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
      if (uVar20 == 0x11) goto LAB_01026c20;
      uVar8 = uVar8 & 0xfeccffff;
      goto LAB_01026818;
    }
    if (uVar20 != 0x11) {
      if (uVar21 == 1) goto LAB_01026814;
LAB_01026c70:
      uVar8 = uVar8 & 0xfeccffff;
      if (uVar21 == 0x13) {
        uVar8 = uVar8 | 0x1000000;
      }
      goto LAB_01026818;
    }
LAB_01026c20:
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
    if (uVar21 != 1) goto LAB_01026c70;
    uVar8 = uVar8 & 0xefccffff;
  }
  if (uVar4 - 7 < 2) {
    uVar14 = uVar14 & 0xffffffcc | 0x22;
  }
  else if (uVar9 == 3) {
    uVar14 = uVar14 & 0xffffffcc | 0x11;
  }
  if (uVar20 - 0xc < 2) {
    uVar14 = uVar14 & 0xffffccff | 0x2200;
  }
  else {
    if (uVar21 == 9) {
      uVar14 = uVar14 & 0xffffccff | 0x1100;
    }
    if (uVar20 == 0xe) {
      uVar16 = uVar16 & 0xfffffffc | 2;
      goto LAB_0102687c;
    }
  }
  if (uVar4 == 0xe) {
    uVar16 = uVar16 & 0xfffffffc | 1;
  }
LAB_0102687c:
  *(uint *)(param_2 + 0xe8) = uVar3;
  *(uint *)(param_2 + 0x1d4) = uVar6;
  *(uint *)(param_2 + 0x228) = uVar8;
  *(uint *)(param_2 + 0x22c) = uVar14;
  *(uint *)(param_2 + 0x230) = uVar16;
switchD_010264f8_caseD_12:
  goto LAB_01026204;
switchD_010264f8_caseD_2:
LAB_010268b4:
  uVar9 = (uint)local_1d8[0];
  uVar3 = 5;
  if (2 < uVar9) {
    uVar3 = FUN_0101de40(local_1ec[0],auStack_1d4);
    uVar21 = 0x1026ca4;
    uVar22 = FUN_0101dda8(local_1ec[0],local_1c4 + 0xc,local_1c4 + 0x1c);
    uVar9 = extraout_r3_00;
    uVar20 = extraout_r12_00;
  }
  uVar12 = (undefined4)(uVar22 >> 0x20);
  uVar23 = CONCAT44(uVar12,uVar3);
  if (uVar3 != 0) {
    FUN_0100ceb0(s__s__invalid_group_A_configuratio_01043af8 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__invalid_group_A_configuratio_01043af8 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = uVar3;
    goto LAB_010268f4;
  }
  uVar8 = (uint)local_1c4[0x38];
  uVar4 = 5;
  param_2 = 0;
  switch(uVar8) {
  case 0:
    goto switchD_01026584_caseD_0;
  case 1:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 2:
    break;
  case 3:
  case 4:
  case 5:
  case 9:
  case 10:
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xf:
  case 0x10:
  case 0x11:
    goto LAB_01026f84;
  case 6:
    break;
  case 8:
    uVar23 = FUN_0101ec24(local_1ec[1],local_5c,local_44,uVar9,uVar12,local_148,uVar8);
  case 7:
    break;
  case 0xe:
    break;
  case 0x12:
switchD_01026584_caseD_12:
LAB_010267a0:
    uVar3 = (uint)uVar22;
    bVar5 = unaff_r10 == 0;
    uVar6 = local_1f8 | (uint)(uVar22 >> 0x20);
    unaff_r10 = local_1f8;
    if (bVar5 && uVar21 != 0x13) {
      if (uVar18 != 0x10) goto LAB_01026d38;
      uVar8 = uVar8 & 0xfffffffc | 2;
LAB_01026d84:
      uVar19 = uVar8 & 0xffffffcf | 0x20;
    }
    else {
LAB_010267bc:
      if (piVar17 != (int *)&DataAbort) {
        uVar8 = uVar8 & 0xfffffffc | 1;
        if (uVar18 != 0x10) {
          if (uVar20 == 0x14) goto LAB_01026cc8;
          goto LAB_010267fc;
        }
        goto LAB_01026d84;
      }
      unaff_r10 = (uint)(uVar20 == 0x14);
      uVar19 = uVar8 & 0xffffffcc | 0x11;
      if (uVar18 != 0x10 && unaff_r10 == 0) {
        uVar8 = uVar8 & 0xfffffccc | 0x111;
LAB_01026da0:
        uVar8 = uVar8 & 0xffffcfff | 0x1000;
        goto LAB_010267fc;
      }
      if (uVar18 != 0x10) {
        uVar8 = uVar8 & 0xfffffccc | 0x211;
        goto LAB_01026da0;
      }
    }
    uVar8 = uVar19 & 0xffffccff | 0x2200;
    goto LAB_010267fc;
  default:
    FUN_0100ceb0(s__s__group_A_configuration_failed_01043b20 + 0x20,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = 0xffffffea;
    FUN_01010c00(s__s__group_A_configuration_failed_01043b20 + 0x20,s_al_serdes_init_01043864 + 0xc,
                 local_1c4[0x38]);
    goto switchD_01026640_caseD_e;
  }
  if (2 < local_1c4[0x38]) {
    uVar12 = FUN_0101de40(local_1ec[1],local_1c4 + 0x3c);
    FUN_0101dda8(local_1ec[1],local_1c4 + 0x58,local_1c4 + 0x68);
    uVar23 = CONCAT44(extraout_r1,uVar12);
  }
  pbVar7 = (byte *)((ulonglong)uVar23 >> 0x20);
  uVar3 = (uint)uVar23;
  if (uVar3 != 0) {
    FUN_0100ceb0(s__s__invalid_group_B_configuratio_01043b44 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__invalid_group_B_configuratio_01043b44 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = uVar3;
switchD_01026584_caseD_0:
    goto LAB_010268f4;
  }
  switch(local_140) {
  case 0:
    uVar3 = FUN_0101f4bc(local_1ec[2],local_5c,local_3c,local_40,local_140);
  case 0x13:
    break;
  case 1:
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  case 2:
    break;
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 0xc:
  case 0xd:
  case 0xe:
    goto LAB_01027104;
  case 9:
  case 10:
    break;
  case 0xb:
    break;
  case 0xf:
    break;
  case 0x10:
    goto switchD_010265e0_caseD_10;
  case 0x11:
    goto switchD_010265e0_caseD_11;
  case 0x12:
    break;
  default:
    FUN_0100ceb0(s__s__group_B_configuration_failed_01043b6c + 0x20,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = 0xffffffea;
    FUN_01010c00(s__s__group_B_configuration_failed_01043b6c + 0x20,s_al_serdes_init_01043864 + 0xc,
                 local_140);
    goto switchD_01026640_caseD_3;
  }
LAB_01026f84:
  if (2 < local_140) {
    uVar3 = FUN_0101de40(local_1ec[2],&local_13c);
    FUN_0101dda8(local_1ec[2],auStack_120,auStack_110);
  }
  if (uVar3 != 0) {
    FUN_0100ceb0(s__s__invalid_group_C_configuratio_01043b90 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__invalid_group_C_configuratio_01043b90 + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = uVar3;
switchD_010265e0_caseD_11:
    goto LAB_010268f4;
  }
  switch(local_f4) {
  case 0:
    uVar3 = FUN_0101f4bc(local_1ec[3],local_5c,local_34[0],local_38,local_f4);
  case 0xd:
    break;
  case 1:
switchD_01026640_caseD_1:
    goto LAB_010268f4;
  case 2:
    break;
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 0xb:
  case 0xf:
  case 0x13:
switchD_01026640_caseD_3:
    goto LAB_010268f4;
  case 0xc:
    goto switchD_01026640_caseD_c;
  case 0xe:
switchD_01026640_caseD_e:
    goto LAB_010268f4;
  case 0x10:
    break;
  case 0x11:
    break;
  case 0x12:
    break;
  case 0x14:
    break;
  default:
    FUN_0100ceb0(s__s__group_C_configuration_failed_01043bb8 + 0x20,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__group_C_configuration_failed_01043bb8 + 0x20,s_al_serdes_init_01043864 + 0xc,
                 local_f4);
    if (2 < local_f4) goto LAB_01027298;
    uVar3 = 0xffffffea;
    goto LAB_01027118;
  }
LAB_01027104:
  if (2 < local_f4) {
LAB_01027298:
    uVar3 = FUN_0101de40(local_1ec[3],&local_f0);
    FUN_0101dda8(local_1ec[3],auStack_d4,auStack_c4);
  }
  if (uVar3 != 0) {
LAB_01027118:
    FUN_0100ceb0(s__s__invalid_group_D_configuratio_01043bdc + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__invalid_group_D_configuratio_01043bdc + 0x24,s_al_serdes_init_01043864 + 0xc)
    ;
    uVar19 = uVar3;
switchD_01026640_caseD_c:
    goto LAB_010268f4;
  }
  if (local_a8 == 1) {
    return 0;
  }
  FUN_01021b64(local_1dc,local_58,local_54);
  if (local_68 == (byte *)0x0) {
    if (local_a8 == 0x12) {
      uVar19 = FUN_01021e14(local_1dc);
      goto LAB_01026a54;
    }
    if (local_a8 == 0x15) {
      uVar19 = FUN_01021e0c(local_1dc);
      goto LAB_01026a54;
    }
    FUN_0100ceb0(s__s__group_D_configuration_failed_01043c04 + 0x20,s_al_serdes_init_01043864 + 0xc)
    ;
    FUN_01010c00(s__s__group_D_configuration_failed_01043c04 + 0x20,s_al_serdes_init_01043864 + 0xc,
                 local_a8);
    if (local_a8 < 3) {
      uVar19 = 0xffffffea;
    }
    else {
      uVar19 = 0xffffffea;
      FUN_01021d94(local_1dc,auStack_88,auStack_78);
    }
  }
  else {
    pbVar7 = local_68;
    if (*local_68 != local_a8) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s__s__invalid_group_E_configuratio_01043c28 + 0x24,1,
                   s_al_serdes_init_clk_route_010438fc + 0x18,s_al_serdes_init_01043864 + 0xc,0x5dd)
      ;
      FUN_01010c00(s_vectors_01028ea4 + 4,s__s__invalid_group_E_configuratio_01043c28 + 0x24,1,
                   s_al_serdes_init_clk_route_010438fc + 0x18,s_al_serdes_init_01043864 + 0xc,0x5dd)
      ;
      FUN_01000458(0);
      pbVar7 = local_68;
switchD_010265e0_caseD_10:
    }
    uVar19 = (**(code **)(local_1dc + 0xb8))(local_1dc,pbVar7);
LAB_01026a54:
    if (2 < local_a8) {
      FUN_01021d94(local_1dc,auStack_88,auStack_78);
    }
    if (uVar19 == 0) {
      return 0;
    }
  }
  FUN_0100ceb0(s___cfg_>grp_cfg_AL_SRDS_GRP_E__mo_01043c50 + 0x60,s_al_serdes_init_01043864 + 0xc);
  FUN_01010c00(s___cfg_>grp_cfg_AL_SRDS_GRP_E__mo_01043c50 + 0x60,s_al_serdes_init_01043864 + 0xc);
LAB_010268f4:
  FUN_0100ceb0(s__s__group_E_configuration_failed_01043cb4 + 0x20,&DAT_01043860);
  FUN_01010c00(s__s__group_E_configuration_failed_01043cb4 + 0x20,&DAT_01043860);
switchD_010264f8_caseD_0:
  uVar3 = uVar19;
switchD_0102628c_caseD_1:
  return uVar3;
}



/* @ 0x10274e8 */

void FUN_010274e8(int param_1,int param_2)

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



/* @ 0x1027508 */

void FUN_01027508(int param_1,int param_2)

{
  if (param_2 != 0) {
    *(undefined4 *)(param_1 + 0x4000) = 1;
    *(undefined4 *)(param_1 + 0x5000) = 1;
  }
  *(undefined4 *)(param_1 + 4) = 7;
  return;
}



/* @ 0x1027530 */

void FUN_01027530(int param_1)

{
  *(uint *)(param_1 + 0xd0) = *(uint *)(param_1 + 0xd0) & 0xbfffffff;
  return;
}



/* @ 0x1027540 */

void FUN_01027540(int param_1)

{
  *(undefined4 *)(param_1 + 0x4000) = 0;
  *(undefined4 *)(param_1 + 0x5000) = 0;
  *(undefined4 *)(param_1 + 4) = 0;
  return;
}



/* @ 0x102755c */

void FUN_0102755c(int param_1)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr9;
  
  *DAT_010275b8 = 1;
  do {
  } while (*DAT_010275bc != 2);
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



/* @ 0x10275c0 */

char * FUN_010275c0(uint param_1)

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
    if (uVar2 == 8) {
      pcVar1 = s_UBOOT_ENV_01043e38 + 8;
    }
    else if (uVar2 < 9) {
      if (uVar2 == 3) {
        pcVar1 = &DAT_01043e04;
      }
      else if (uVar2 < 4) {
        if (uVar2 == 1) {
          pcVar1 = s_BOOT_MODE_01043df0 + 8;
        }
        else if (uVar2 < 2) {
          pcVar1 = s_al_flash_obj_data_load_01043dd8 + 0x14;
        }
        else {
          pcVar1 = &DAT_01043e00;
        }
      }
      else if (uVar2 == 5) {
        pcVar1 = &DAT_01043e1c;
      }
      else if (uVar2 < 5) {
        pcVar1 = &DAT_01043e0c;
      }
      else if (uVar2 == 6) {
        pcVar1 = &DAT_01043e24;
      }
      else {
        pcVar1 = s_UBOOT_SCRIPT_01043e28 + 0xc;
        if (uVar2 != 7) {
          pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
        }
      }
    }
    else if (uVar2 == 0xc) {
      pcVar1 = &DAT_01043e60;
    }
    else if (uVar2 < 0xd) {
      if (uVar2 == 10) {
        pcVar1 = s_KERNEL_01043e54 + 4;
      }
      else if (uVar2 < 0xb) {
        pcVar1 = s_UBOOT_ENV_RED_01043e44 + 0xc;
      }
      else {
        pcVar1 = s_PRE_BOOT_V2_01043e70 + 8;
      }
    }
    else if (uVar2 == 0xe) {
      pcVar1 = &DAT_01043e14;
    }
    else if (uVar2 < 0xe) {
      pcVar1 = s_PRE_BOOT_01043e64 + 8;
    }
    else if (uVar2 == 0x100) {
      pcVar1 = &DAT_01043e80;
    }
    else {
      pcVar1 = (char *)0x1043e88;
      if (uVar2 != 0x101) {
        pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
      }
    }
  }
  else if (uVar2 == 0x240) {
    pcVar1 = s_APP_2_CFG_01043eec + 8;
  }
  else if (uVar2 < 0x241) {
    if (uVar2 == 0x211) {
      pcVar1 = &DAT_01043ec0;
    }
    else if (uVar2 < 0x212) {
      if (uVar2 == 0x201) {
        pcVar1 = s_BOOT_APP_01043ea0 + 8;
      }
      else if (uVar2 == 0x210) {
        pcVar1 = s_BOOT_APP_CFG_01043eac + 0xc;
      }
      else {
        pcVar1 = (char *)0x1043e9c;
        if (uVar2 != 0x200) {
          pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
        }
      }
    }
    else if (uVar2 == 0x221) {
      pcVar1 = &DAT_01043ed4;
    }
    else if (uVar2 < 0x222) {
      pcVar1 = (char *)0x1043ecc;
      if (uVar2 != 0x220) {
        pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
      }
    }
    else if (uVar2 == 0x230) {
      pcVar1 = &DAT_01043ee0;
    }
    else {
      pcVar1 = (char *)0x1043ee8;
      if (uVar2 != 0x231) {
        pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
      }
    }
  }
  else if (uVar2 == 0x500) {
    pcVar1 = &DAT_01043f18;
  }
  else if (uVar2 < 0x501) {
    if (uVar2 == 0x300) {
      pcVar1 = &DAT_01043f08;
    }
    else if (uVar2 == 0x400) {
      pcVar1 = s_CRASH_DUMP_01043f0c + 8;
    }
    else {
      pcVar1 = (char *)0x1043efc;
      if (uVar2 != 0x241) {
        pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
      }
    }
  }
  else if (uVar2 == 0x601) {
    pcVar1 = &DAT_01043f30;
  }
  else if (uVar2 < 0x602) {
    pcVar1 = s_SERDES_25G_FW_01043f1c + 0xc;
    if (uVar2 != 0x600) {
      pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
    }
  }
  else if (uVar2 == 0x602) {
    pcVar1 = &DAT_01043f38;
  }
  else {
    pcVar1 = s_Unable_to_read_Board_RevID_from_E_0103166c + 0x1c;
    if (uVar2 != 0x700) {
      pcVar1 = s_expected_0x_X__read_0x_X_01043fbc + 0x18;
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
        FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__services_flash_contents__01043fdc + 0x30,1,
                     &DAT_01043fd8,s__s__al_serdes_init_clk_route_fai_01043cfc + 0x24,0x74);
        FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__services_flash_contents__01043fdc + 0x30,1,
                     &DAT_01043fd8,s__s__al_serdes_init_clk_route_fai_01043cfc + 0x24,0x74);
        FUN_01000458(0);
      }
    }
    if (0x27 < uVar3) {
      FUN_0100ceb0(s_vectors_01028ea4 + 4,s____HAL__services_flash_contents__01043fdc + 0x30,1,
                   &DAT_01043fd8,s__s__al_serdes_init_clk_route_fai_01043cfc + 0x24,0x77);
      FUN_01010c00(s_vectors_01028ea4 + 4,s____HAL__services_flash_contents__01043fdc + 0x30,1,
                   &DAT_01043fd8,s__s__al_serdes_init_clk_route_fai_01043cfc + 0x24,0x77);
      FUN_01000458(0);
    }
    pcVar1 = (char *)&DAT_01049d60;
    *(undefined1 *)((int)&DAT_01049d60 + uVar2 + 1) = 0;
  }
  return pcVar1;
}



/* @ 0x1027a3c */

int FUN_01027a3c(code *param_1,int param_2,int *param_3,uint *param_4)

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
            if (iVar1 != 0) goto LAB_01027b80;
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
          FUN_0100ceb0(s__s__unable_to_find_stage_2_at_of_01044020 + 0x28,
                       s_al_flash_obj_id_to_str_01043d24 + 0x14);
          FUN_01010c00(s__s__unable_to_find_stage_2_at_of_01044020 + 0x28,
                       s_al_flash_obj_id_to_str_01043d24 + 0x14);
          return -5;
        }
        goto LAB_01027b80;
      }
    }
    iVar1 = -5;
  }
  else {
LAB_01027b80:
    FUN_0100ceb0(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_obj_id_to_str_01043d24 + 0x14);
    FUN_01010c00(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_obj_id_to_str_01043d24 + 0x14);
  }
  return iVar1;
}



/* @ 0x1027bcc */

undefined4 FUN_01027bcc(undefined4 param_1,int param_2,int param_3,uint param_4,int *param_5)

{
  int iVar1;
  uint uVar2;
  undefined1 auStack_20 [4];
  undefined1 auStack_1c [4];
  
  if (param_4 != 0) {
    uVar2 = 0;
    do {
      iVar1 = FUN_01027a3c(param_1,param_2,auStack_20,auStack_1c);
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



/* @ 0x1027c58 */

int FUN_01027c58(code *param_1,int param_2,int param_3,int param_4,uint param_5,uint *param_6,
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
        if (iVar2 != 0) goto LAB_01027d1c;
        uVar4 = param_5;
        if (local_68 == param_3) goto LAB_01027d90;
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
LAB_01027d90:
        if (uVar4 < local_74) {
          *param_6 = uVar4;
          FUN_010129dc(param_7,&local_68,0x20);
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
LAB_01027d1c:
    iVar1 = iVar2;
    FUN_0100ceb0(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_toc_validate_01043d3c + 0x14);
    FUN_01010c00(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_toc_validate_01043d3c + 0x14);
  }
  return iVar1;
}



/* @ 0x1027dec */

void FUN_01027dec(void)

{
  FUN_01027c58();
  return;
}



/* @ 0x1027e14 */

int FUN_01027e14(int param_1,int param_2,code *param_3,int param_4,uint *param_5,uint *param_6)

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
          if (iVar1 != 0) goto LAB_01027eb8;
          uVar2 = local_40[0] & 0xfffffff;
          bVar6 = uVar2 != 0;
          if (uVar2 != 1) {
            bVar6 = uVar2 != 0xc;
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
      FUN_0100ceb0(&DAT_0104401c,s_al_flash_toc_find_id_with_fallba_01043d54 + 0x20,uVar5);
      FUN_01010c00(&DAT_0104401c,s_al_flash_toc_find_id_with_fallba_01043d54 + 0x20,uVar5);
    }
    else {
LAB_01027eb8:
      FUN_0100ceb0(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                   s_al_flash_toc_find_id_with_fallba_01043d54 + 0x20);
      FUN_01010c00(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                   s_al_flash_toc_find_id_with_fallba_01043d54 + 0x20);
    }
  }
  else {
    iVar1 = 0;
    *param_5 = (uVar5 & 0xffff) >> 8;
    *param_6 = (uVar5 & 0xffffff) >> 0x10;
  }
  return iVar1;
}



/* @ 0x1027f70 */

int FUN_01027f70(code *param_1,undefined4 param_2,int *param_3)

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
        FUN_010129dc(param_3,&local_58,0x48);
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
        FUN_0100ceb0(s__s__device_read_failed__01043f6c + 0x18,
                     s_al_flash_toc_stage2_active_insta_01043d78 + 0x34);
        FUN_01010c00(s__s__device_read_failed__01043f6c + 0x18,
                     s_al_flash_toc_stage2_active_insta_01043d78 + 0x34);
        iVar1 = -5;
        FUN_0100ceb0(s__s__flash_obj_header_contains_in_01043f88 + 0x30,0xb9ec7,local_58);
        FUN_01010c00(s__s__flash_obj_header_contains_in_01043f88 + 0x30,0xb9ec7,local_58);
      }
    }
    else {
      iVar1 = -5;
    }
  }
  else {
    FUN_0100ceb0(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_toc_stage2_active_insta_01043d78 + 0x34);
    FUN_01010c00(s__s__data_checksum_validation_fai_01043f44 + 0x24,
                 s_al_flash_toc_stage2_active_insta_01043d78 + 0x34);
  }
  return iVar1;
}



/* @ 0x10280d8 */

int FUN_010280d8(code *param_1,int param_2,byte *param_3)

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
      FUN_0100ceb0(&DAT_01043f40,s_al_flash_obj_header_read_and_val_01043db0 + 0x24);
      FUN_01010c00(&DAT_01043f40,s_al_flash_obj_header_read_and_val_01043db0 + 0x24);
      return -5;
    }
  }
  FUN_0100ceb0(s__s__data_checksum_validation_fai_01043f44 + 0x24,
               s_al_flash_obj_header_read_and_val_01043db0 + 0x24);
  FUN_01010c00(s__s__data_checksum_validation_fai_01043f44 + 0x24,
               s_al_flash_obj_header_read_and_val_01043db0 + 0x24);
  return iVar1;
}



/* @ 0x10281f0 */

void FUN_010281f0(undefined4 *param_1,undefined1 *param_2,int param_3,int param_4,undefined4 param_5
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



/* @ 0x1028414 */

void FUN_01028414(int param_1,int param_2)

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



/* @ 0x1028d30 */

undefined8 FUN_01028d30(int param_1,int param_2,int param_3,int param_4)

{
  undefined8 uVar1;
  
  if ((param_4 == 0) && (param_3 == 0)) {
    if (param_2 != 0 || param_1 != 0) {
      param_2 = -1;
      param_1 = -1;
    }
    return CONCAT44(param_2,param_1);
  }
  uVar1 = FUN_01028d60();
  return uVar1;
}



/* @ 0x1028d60 */

undefined8 FUN_01028d60(uint param_1,uint param_2,uint param_3,uint param_4,uint *param_5)

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
          if (uVar4 == 0) goto LAB_01028e10;
        }
        bVar12 = param_1 < uVar8;
        uVar5 = param_1 - uVar8;
        param_1 = uVar5 * 2 + 1;
        param_2 = ((param_2 - uVar9) - (uint)bVar12) * 2 + (uint)CARRY4(uVar5,uVar5) +
                  (uint)(0xfffffffe < uVar5 * 2);
        uVar4 = uVar4 - 1;
        uVar5 = param_2;
      } while (uVar4 != 0);
LAB_01028e10:
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



/* @ 0x102eff8 */

void FUN_0102eff8(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0x102f0f0 */

void FUN_0102f0f0(undefined4 param_1)

{
  undefined4 in_cr0;
  undefined4 in_cr14;
  
  coprocessor_moveto(0xf,0,0,param_1,in_cr14,in_cr0);
  return;
}



/* @ 0x102f12c */

void FUN_0102f12c(undefined4 param_1,code *param_2)

{
  (*param_2)();
  return;
}



/* @ 0x102f2fc */

void FUN_0102f2fc(void)

{
  FUN_0102f324();
  FUN_0102f5f4(DAT_0102f320);
  FUN_0102f614();
  FUN_0102fb80(0xf0000000);
  FUN_0102fb9a();
  return;
}



/* @ 0x102f324 */

void FUN_0102f324(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined1 uStack_25;
  undefined1 auStack_24 [4];
  int local_20;
  undefined1 auStack_1c [20];
  
  iVar1 = DAT_0102f384;
  FUN_0102f76c(DAT_0102f388,DAT_0102f384);
  iVar2 = FUN_0102f98c(DAT_0102f390,DAT_0102f394,*(int *)(iVar1 + 0x14) != DAT_0102f38c,auStack_1c);
  if (((iVar2 == 0) && (iVar2 = FUN_0102f9ec(auStack_1c,&uStack_25,auStack_24), iVar2 == 0)) &&
     (iVar2 = FUN_0102fa24(auStack_1c,0,&local_20), iVar2 == 0)) {
    *(int *)(iVar1 + 4) = local_20 * 1000;
  }
  uVar3 = *(uint *)(iVar1 + 4) >> 4;
  FUN_0102f0f0(uVar3);
  *(uint *)(DAT_0102f398 + 0x1004) = uVar3;
  return;
}



/* @ 0x102f39c */

uint FUN_0102f39c(uint param_1,int param_2,uint param_3,int param_4,char *param_5,uint param_6,
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
      if (iVar3 < 10) {
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



/* @ 0x102f446 */

uint FUN_0102f446(int param_1,uint param_2)

{
  uint uVar1;
  undefined1 *puVar2;
  int *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_0102fc68();
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



/* @ 0x102f486 */

undefined4 FUN_0102f486(undefined1 param_1)

{
  undefined1 *puVar1;
  undefined4 *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_0102fc4c();
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



/* @ 0x102f4b4 */

undefined4 FUN_0102f4b4(undefined4 param_1,uint param_2,byte *param_3,int *param_4)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  byte bVar5;
  byte *pbVar6;
  undefined1 auStack_30 [24];
  
LAB_0102f4c4:
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
        goto LAB_0102f5e6;
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
        if (9 < bVar5) {
          bVar5 = 0;
        }
      }
      else if (uVar1 - 0x31 < 9) {
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
LAB_0102f574:
      param_4 = param_4 + 1;
      uVar3 = FUN_0102f39c(iVar2,uVar3,10,0,auStack_30,bVar5,uVar4);
      FUN_0102f446(auStack_30,uVar3);
    }
    if (uVar1 < 0x65) {
      if (uVar1 == 0x58) goto LAB_0102f588;
      if (uVar1 != 99) {
        if (uVar1 == 0) {
          return 0;
        }
        goto LAB_0102f5e6;
      }
      FUN_0102f486((char)*param_4);
LAB_0102f5dc:
      param_4 = param_4 + 1;
      goto LAB_0102f4c4;
    }
    if (uVar1 == 0x73) {
      for (iVar2 = 0; *(char *)(*param_4 + iVar2) != '\0'; iVar2 = iVar2 + 1) {
      }
      FUN_0102f446();
      goto LAB_0102f5dc;
    }
    if (uVar1 < 0x74) {
      if (uVar1 != 0x70) goto LAB_0102f5e6;
      bVar5 = 8;
LAB_0102f588:
      iVar2 = *param_4;
      param_4 = param_4 + 1;
      uVar3 = FUN_0102f39c(iVar2,1,0x10,uVar1 == 0x58,auStack_30,bVar5,uVar4);
      FUN_0102f446(auStack_30,uVar3);
    }
    else {
      if (uVar1 == 0x75) {
        uVar3 = 1;
        iVar2 = *param_4;
        goto LAB_0102f574;
      }
      if (uVar1 == 0x78) goto LAB_0102f588;
LAB_0102f5e6:
      FUN_0102f486(uVar1);
      param_3 = pbVar6;
    }
  } while( true );
}



/* @ 0x102f5f4 */

void FUN_0102f5f4(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_0102f4b4(0,0xffffffff,param_1,&uStack_c,param_1,&uStack_c,param_3);
  return;
}



/* @ 0x102f614 */

void FUN_0102f614(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  
  FUN_0102f730();
  iVar2 = 0;
  do {
    iVar3 = iVar2 + 1;
    FUN_0102fac8(*DAT_0102f660,iVar2,0,DAT_0102f664);
    puVar1 = DAT_0102f668;
    iVar2 = iVar3;
  } while (iVar3 != 4);
  DAT_0102f668[2] = 0;
  puVar1[3] = 0;
  puVar1[4] = 0;
  puVar1[5] = 0;
  puVar1[6] = 0;
  puVar1[7] = 0;
  puVar1[8] = 0;
  puVar1[9] = 0;
  puVar1[1] = 0;
  *puVar1 = DAT_0102f66c;
  return;
}



/* @ 0x102f730 */

void FUN_0102f730(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
  piVar1 = DAT_0102f75c;
  if (*DAT_0102f75c == 0) {
    FUN_0102fa88(DAT_0102f760,DAT_0102f764,param_3,DAT_0102f75c,param_4);
    iVar3 = 0;
    do {
      iVar2 = DAT_0102f768 + iVar3;
      iVar3 = iVar3 + 8;
      FUN_0102fa98(iVar2,DAT_0102f760,0);
    } while (iVar3 != 0x20);
    *piVar1 = 1;
  }
  return;
}



/* @ 0x102f76c */

void FUN_0102f76c(int param_1,undefined4 *param_2)

{
  bool bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined1 uVar5;
  undefined1 uVar6;
  uint uVar7;
  
  uVar4 = *(uint *)(param_1 + 0x110);
  uVar2 = DAT_0102f8d4;
  if ((uVar4 & 0x80000) == 0) {
    uVar2 = DAT_0102f8d0;
  }
  param_2[5] = uVar2;
  uVar3 = DAT_0102f910;
  switch(uVar4 & 0xf) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_0102f8d8;
    break;
  case 2:
    uVar3 = DAT_0102f8dc;
    break;
  case 3:
    uVar3 = DAT_0102f8e0;
    break;
  case 4:
    uVar3 = DAT_0102f8e4;
    break;
  case 5:
    uVar3 = DAT_0102f8e8;
    break;
  case 6:
    uVar3 = DAT_0102f8ec;
    break;
  case 7:
    uVar3 = DAT_0102f8f0;
    break;
  case 8:
    uVar3 = DAT_0102f8f4;
    break;
  case 9:
    uVar3 = DAT_0102f8f8;
    break;
  case 10:
    uVar3 = DAT_0102f8fc;
    break;
  case 0xb:
    uVar3 = DAT_0102f900;
    break;
  case 0xc:
    uVar3 = DAT_0102f904;
    break;
  case 0xd:
    uVar3 = DAT_0102f908;
    break;
  case 0xe:
    uVar3 = DAT_0102f90c;
  }
  *param_2 = uVar3;
  uVar3 = DAT_0102f914;
  switch((uVar4 & 0x7f) >> 4) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_0102f918;
    break;
  case 2:
    uVar3 = DAT_0102f91c;
    break;
  case 3:
    uVar3 = DAT_0102f920;
    break;
  case 4:
    uVar3 = DAT_0102f924;
    break;
  case 5:
    uVar3 = DAT_0102f928;
    break;
  case 6:
    uVar3 = DAT_0102f92c;
  }
  uVar7 = (uVar4 & 0x1ff) >> 7;
  param_2[1] = uVar3;
  if (uVar7 == 0) {
    bVar1 = true;
    uVar3 = uVar2;
  }
  else {
    bVar1 = false;
    uVar3 = DAT_0102f930;
    if (uVar7 != 1) {
      uVar3 = DAT_0102f8e0;
    }
  }
  param_2[2] = uVar3;
  if ((bVar1) ||
     (((uVar7 = (uVar4 & 0x7ff) >> 9, uVar3 = DAT_0102f938, uVar2 = DAT_0102f934, uVar7 != 2 &&
       (uVar3 = DAT_0102f93c, uVar7 != 3)) && (uVar3 = DAT_0102f940, uVar7 != 0)))) {
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
    uVar6 = *(undefined1 *)(DAT_0102f944 + uVar7);
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



/* @ 0x102f948 */

uint FUN_0102f948(int *param_1)

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



/* @ 0x102f97a */

bool FUN_0102f97a(int param_1)

{
  return ((*(uint *)(param_1 + 0x1c) & 0xfff) >> 8) - 6 < 2;
}



/* @ 0x102f98c */

undefined4 FUN_0102f98c(undefined4 param_1,undefined4 param_2,int param_3,undefined4 *param_4)

{
  undefined4 uVar1;
  
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    param_4[4] = DAT_0102f9d8;
    param_4[2] = DAT_0102f9dc;
    uVar1 = 0x14;
  }
  else {
    if (param_3 == 0) {
      param_4[4] = 25000;
      uVar1 = DAT_0102f9e0;
    }
    else {
      if (param_3 != 2) {
        FUN_0102f5f4(DAT_0102f9e8,DAT_0102f9e4);
        return 0xffffffea;
      }
      param_4[4] = DAT_0102f9d0;
      uVar1 = DAT_0102f9d4;
    }
    param_4[2] = uVar1;
    uVar1 = 0x13;
  }
  param_4[3] = uVar1;
  return 0;
}



/* @ 0x102f9ec */

undefined4 FUN_0102f9ec(int param_1,undefined1 *param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  
  puVar3 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar1 = FUN_0102f948();
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



/* @ 0x102fa24 */

uint FUN_0102fa24(int *param_1,uint param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  *param_3 = 0;
  iVar1 = FUN_0102f97a(*param_1);
  if (iVar1 == 0) {
    return 0;
  }
  uVar2 = FUN_0102f948(param_1);
  uVar3 = *(uint *)(*param_1 + ((param_2 >> 1) + 8) * 4);
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
      FUN_0102f5f4(DAT_0102fa80,DAT_0102fa84);
      return 0xfffffffb;
    }
    *param_3 = uVar2 / (uVar3 & 0x3ff);
  }
  return uVar4;
}



/* @ 0x102fa88 */

void FUN_0102fa88(uint *param_1,uint param_2)

{
  uint uVar1;
  
  uVar1 = *(uint *)(param_2 + 0x4400);
  param_1[1] = param_2;
  *param_1 = (uVar1 & 0xffff) >> 8;
  return;
}



/* @ 0x102fa98 */

void FUN_0102fa98(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  *param_1 = param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0x102fac8 */

void FUN_0102fac8(int *param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  if (*(uint *)*param_1 < 4) {
    iVar1 = ((uint *)*param_1)[1] + param_2 * 0x100;
    *(undefined4 *)(iVar1 + 0x2028) = param_4;
    *(undefined4 *)(iVar1 + 0x202c) = param_3;
    return;
  }
  FUN_0102f5f4(DAT_0102faf4,DAT_0102faf8);
  return;
}



/* @ 0x102fafc */

void FUN_0102fafc(int param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  puVar1 = DAT_0102fb70;
  if (param_1 != 0) {
    puVar1 = DAT_0102fb74;
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
  puVar1 = DAT_0102fb7c;
  if (param_1 == 0) {
    puVar1 = DAT_0102fb78;
  }
  *puVar1 = *puVar1 | 1;
  return;
}



/* @ 0x102fb80 */

void FUN_0102fb80(int param_1)

{
  *(undefined4 *)(param_1 + 0x90008) = 1;
  FUN_0102fafc(0);
  FUN_0102fafc(1);
  return;
}



/* @ 0x102fb9a */

void FUN_0102fb9a(void)

{
  return;
}



/* @ 0x102fc4c */

uint FUN_0102fc4c(uint param_1)

{
  FUN_0102fc7c(0,param_1 & 0xff);
  if (param_1 == 10) {
    FUN_0102fc7c(0,0xd);
  }
  return param_1;
}



/* @ 0x102fc68 */

void FUN_0102fc68(int param_1)

{
  char *pcVar1;
  
  pcVar1 = (char *)(param_1 + -1);
  while( true ) {
    pcVar1 = pcVar1 + 1;
    if (*pcVar1 == '\0') break;
    FUN_0102fc4c();
  }
  return;
}



/* @ 0x102fc7c */

void FUN_0102fc7c(int param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  
  puVar1 = *(undefined4 **)(DAT_0102fc90 + param_1 * 4);
  do {
  } while (-1 < (int)(puVar1[5] << 0x19));
  *puVar1 = param_2;
  return;
}



