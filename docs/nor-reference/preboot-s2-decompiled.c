/* @ 0xf2200020  FUN_f2200020 */

/* WARNING: Control flow encountered bad instruction data */

void s2_entry_a32_stub(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  uint uVar2;
  
  uVar2 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar2 | 0x1000);
  InstructionSynchronizationBarrier(0xf);
  uVar1 = FUN_f2200078();
  s2_main(uVar1,param_2);
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



/* @ 0xf220005c  FUN_f220005c */

int cpu_affinity_id(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return ((uVar1 & 0xff00) >> 6) + (uVar1 & 0xff);
}



/* @ 0xf2200078  FUN_f2200078 */

void FUN_f2200078(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0xf2200098  FUN_f2200098 */

void memcpy32_unrolled(undefined4 *param_1,undefined4 *param_2,int param_3)

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



/* @ 0xf22000b4  FUN_f22000b4 */

void memset_unrolled(undefined4 *param_1,undefined4 param_2,int param_3)

{
  do {
    *param_1 = param_2;
    param_1[1] = param_2;
    param_1[2] = param_2;
    param_1[3] = param_2;
    param_1[4] = param_2;
    param_1[5] = param_2;
    param_1[6] = param_2;
    param_1[7] = param_2;
    param_1 = param_1 + 8;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  return;
}



/* @ 0xf22000ec  FUN_f22000ec */

void FUN_f22000ec(code *UNRECOVERED_JUMPTABLE,undefined4 param_2)

{
  coproc_moveto_Invalidate_Entire_Instruction(param_2);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
                    /* WARNING: Could not recover jumptable at 0xf22000fc. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0xf2200100  FUN_f2200100 */

void s_freq_helper_a(int param_1)

{
  for (; param_1 != 0; param_1 = param_1 + -1) {
  }
  return;
}



/* @ 0xf2200114  FUN_f2200114 */

void s_freq_helper_b(undefined4 param_1)

{
  undefined4 in_cr0;
  undefined4 in_cr14;
  
  coprocessor_moveto(0xf,0,0,param_1,in_cr14,in_cr0);
  return;
}



/* @ 0xf220011c  thunk_EXT_FUN_ffff0020 */

void thunk_EXT_FUN_ffff0020(void)

{
                    /* WARNING: Could not recover jumptable at 0xf220011e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_f2200120)();
  return;
}



/* @ 0xf2200124  FUN_f2200124 */

void s2_main(void)

{
  int iVar1;
  int iVar2;
  ushort uVar3;
  code *pcVar4;
  undefined4 uVar5;
  int local_88;
  int local_84;
  undefined4 local_80;
  undefined4 local_7c;
  undefined1 auStack_78 [16];
  int local_68;
  undefined1 auStack_58 [48];
  undefined4 local_28;
  undefined4 local_20;
  
  iVar2 = DAT_f2200280;
  bootstrap_parse_nb_pll_init();
  FUN_f22014c8();
  FUN_f22002bc();
  pcVar4 = DAT_f220028c;
  if (*(char *)(iVar2 + 0x1c) != '\0') {
    iVar1 = s_rec_present_check();
    rec_read_dd();
    if (*(char *)(iVar1 + 0x24) == -0x23) {
      uVar3 = (ushort)*(byte *)(iVar1 + 0x25);
    }
    else {
      uVar3 = (short)(*(uint *)(iVar2 + 0xc) / DAT_f2200284) + 1U & 0xfffe;
    }
    *(ushort *)(DAT_f2200288 + 0x14) = uVar3;
    pcVar4 = DAT_f220028c;
    if (*(char *)(iVar2 + 0x1c) != '\0') {
      pcVar4 = DAT_f2200298;
    }
  }
  ddr_bringup_orchestrator();
  if (*(char *)(iVar2 + 0x1c) != '\0') {
    *DAT_f2200290 = *DAT_f2200290 | 2;
  }
  iVar2 = FUN_f22041b4(pcVar4,0,0x20000,0x10,&local_80);
  if (iVar2 != 0) {
    s_al_err_printf(DAT_f2200294);
    goto LAB_f220019a;
  }
  iVar2 = FUN_f220437e(DAT_f220029c,pcVar4,local_80,&local_84);
  if (iVar2 != 0) {
    s_al_err_printf(DAT_f22002a0);
    local_84 = 0;
  }
  iVar2 = al_flash_toc_find_id_with_fallback(pcVar4,local_80,local_84 << 0x1c | 0xd,0xd,0,&local_88,auStack_78);
  if (iVar2 == 0) {
    if (local_88 < 0) goto LAB_f22001d8;
    iVar2 = FUN_f22044a0(pcVar4,local_68,0x1000000,0x1000,auStack_58);
    if (iVar2 != 0) {
      s_al_err_printf(DAT_f22002a8);
      goto LAB_f220019a;
    }
    (*pcVar4)(&DAT_00021000 + local_68,&local_7c,4);
    (*pcVar4)(local_68 + 0x21004,0x1000000,local_7c);
    uVar5 = 0x1000000;
  }
  else {
    s_al_err_printf(DAT_f22002a4);
LAB_f22001d8:
    iVar2 = al_flash_toc_find_id_with_fallback(pcVar4,local_80,local_84 << 0x1c | 4,4,0,&local_88,auStack_78);
    if (iVar2 != 0) {
      s_al_err_printf(DAT_f22002a4);
      goto LAB_f220019a;
    }
    if (local_88 < 0) {
      s_al_err_printf(DAT_f22002b0);
      goto LAB_f220019a;
    }
    iVar2 = al_flash_obj_header_read_and_validate(pcVar4,local_68,auStack_58);
    if (iVar2 != 0) {
      s_al_err_printf(DAT_f22002b4);
      goto LAB_f220019a;
    }
    iVar2 = al_flash_obj_data_load(pcVar4,local_68,local_28);
    uVar5 = local_20;
    if (iVar2 != 0) {
      s_al_err_printf(DAT_f22002b8);
      goto LAB_f220019a;
    }
  }
  s_al_err_printf(DAT_f22002ac);
  FUN_f22000ec(uVar5);
LAB_f220019a:
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* @ 0xf22002bc  FUN_f22002bc */

void FUN_f22002bc(void)

{
  s_al_err_printf(DAT_f22002c4);
  return;
}



/* @ 0xf22002c8  FUN_f22002c8 */

void bootstrap_parse_nb_pll_init(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined1 uStack_25;
  undefined1 auStack_24 [4];
  int local_20;
  undefined1 auStack_1c [20];
  
  iVar1 = DAT_f2200328;
  FUN_f2203b58(DAT_f220032c,DAT_f2200328);
  iVar2 = al_pll_init(DAT_f2200334,DAT_f2200338,*(int *)(iVar1 + 0x14) != DAT_f2200330,auStack_1c);
  if (((iVar2 == 0) && (iVar2 = FUN_f2203e18(auStack_1c,&uStack_25,auStack_24), iVar2 == 0)) &&
     (iVar2 = al_pll_channel_freq_get(auStack_1c,0,&local_20), iVar2 == 0)) {
    *(int *)(iVar1 + 4) = local_20 * 1000;
  }
  uVar3 = *(uint *)(iVar1 + 4) >> 4;
  s_freq_helper_b(uVar3);
  *(uint *)(DAT_f220033c + 0x1004) = uVar3;
  return;
}



/* @ 0xf2200340  FUN_f2200340 */

void s_freq_helper_c(uint param_1)

{
  *(uint *)(DAT_f2200358 + 4) = param_1;
  s_freq_helper_b(param_1 >> 4);
  *(uint *)(DAT_f220035c + 0x1004) = param_1 >> 4;
  return;
}



/* @ 0xf22003b0  FUN_f22003b0 */

void rec_read_aa(void)

{
  int iVar1;
  undefined2 uVar2;
  
  if (*DAT_f220039c == '\0') {
    uVar2 = 0x400;
  }
  else {
    uVar2 = CONCAT11(*DAT_f22003a0,DAT_f22003a0[-1]);
  }
  iVar1 = i2c_eeprom_read(*(undefined1 *)(DAT_f22003a8 + 0x27),uVar2,7,DAT_f22003a4);
  if (iVar1 != 0) {
    s_al_err_printf(DAT_f22003ac);
    return;
  }
  return;
}



/* @ 0xf22003b8  FUN_f22003b8 */

void rec_read_bb(void)

{
  int iVar1;
  uint uVar2;
  
  if (*DAT_f220039c == '\0') {
    uVar2 = 0x400;
  }
  else {
    uVar2 = (uint)CONCAT11(*DAT_f22003a0,DAT_f22003a0[-1]);
  }
  iVar1 = i2c_eeprom_read(*(undefined1 *)(DAT_f22003a8 + 0x27),uVar2 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT,
                       3,DAT_f22003a4 + AL_I2C_INTR_MASK_GEN_CALL_SHIFT);
  if (iVar1 != 0) {
    s_al_err_printf(DAT_f22003ac);
    return;
  }
  return;
}



/* @ 0xf22003c0  FUN_f22003c0 */

void rec_read_cc(void)

{
  int iVar1;
  uint uVar2;
  
  if (*DAT_f220039c == '\0') {
    uVar2 = 0x400;
  }
  else {
    uVar2 = (uint)CONCAT11(*DAT_f22003a0,DAT_f22003a0[-1]);
  }
  iVar1 = i2c_eeprom_read(*(undefined1 *)(DAT_f22003a8 + 0x27),uVar2 + 0xe,0x16,DAT_f22003a4 + 0xe);
  if (iVar1 != 0) {
    s_al_err_printf(DAT_f22003ac);
    return;
  }
  return;
}



/* @ 0xf22003c8  FUN_f22003c8 */

void rec_read_dd(void)

{
  int iVar1;
  uint uVar2;
  
  if (*DAT_f220039c == '\0') {
    uVar2 = 0x400;
  }
  else {
    uVar2 = (uint)CONCAT11(*DAT_f22003a0,DAT_f22003a0[-1]);
  }
  iVar1 = i2c_eeprom_read(*(undefined1 *)(DAT_f22003a8 + 0x27),uVar2 + 0x24,2,DAT_f22003a4 + 0x24);
  if (iVar1 != 0) {
    s_al_err_printf(DAT_f22003ac);
    return;
  }
  return;
}



/* @ 0xf22003d0  FUN_f22003d0 */

undefined4 s_rec_present_check(void)

{
  return DAT_f22003d4;
}



/* @ 0xf22003d8  FUN_f22003d8 */

void ddr_bringup_orchestrator(void)

{
  byte bVar1;
  longlong lVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  char *pcVar9;
  undefined1 uVar10;
  uint uVar11;
  uint uVar12;
  int iVar13;
  undefined4 *puVar14;
  uint uVar15;
  int iVar16;
  
  iVar13 = DAT_f22005dc;
  shared_params_write_magic();
  iVar3 = s_shared_params_write_size();
  iVar8 = DAT_f22005e0;
  *(undefined1 *)(iVar3 + 4) = 0;
  *(undefined8 *)(iVar3 + 8) = 0;
  s_cfg_get_helper();
  al_ddr_cfg_init(DAT_f22005ec,DAT_f22005e4,DAT_f22005e8,iVar13);
  *(uint *)(iVar13 + 0x6c) = *(uint *)(iVar8 + 4) / DAT_f22005f0;
  s_orch_helper(iVar13 + 0x104);
  ddr_freq_change_according_to_spd(DAT_f22005f4);
  if ((*(char *)(iVar8 + 0x19) == '\0') && (*DAT_f22005f8 != '\0')) {
    bVar1 = *DAT_f22005fc;
    uVar11 = bVar1 & 4;
    if (-1 < (int)((uint)bVar1 << 0x1c)) {
      if ((bVar1 & 4) == 0) goto LAB_f220043a;
      goto LAB_f2200448;
    }
    if ((bVar1 & 4) == 0) goto LAB_f220043c;
  }
  else {
LAB_f220043a:
    uVar11 = 1;
LAB_f220043c:
    if (*(char *)(iVar13 + 0x10) == '\x01') {
      *(undefined1 *)(iVar13 + 0x10) = 0;
    }
    if (uVar11 != 0) {
LAB_f2200448:
      *(undefined4 *)(iVar13 + 0x24) = 0;
    }
  }
  puVar14 = DAT_f22005f4;
  ddr_build_addrmap(DAT_f22005f4,DAT_f2200600);
  set_dram_impedance_ctrl_from_eeprom(*(undefined4 *)(iVar13 + 0x18),*(undefined4 *)(iVar13 + 0x14),DAT_f2200604);
  dram_voltage_gpio(puVar14[9]);
  pcVar9 = (char *)*puVar14;
  uVar11 = *(uint *)(pcVar9 + 4);
  if (pcVar9[0x18] == '\x01') {
    iVar16 = puVar14[5];
  }
  else {
    iVar16 = 0;
  }
  uVar12 = puVar14[2] + puVar14[4] + puVar14[3] + iVar16;
  uVar4 = uVar11 << (uVar12 & 0xff);
  if (*pcVar9 == '\x01') {
    iVar16 = 6;
  }
  else if (*pcVar9 == '\0') {
    iVar16 = 5;
  }
  else {
    iVar16 = 4;
  }
  uVar10 = 1;
  uVar15 = 1 << (iVar16 - 3U & 0xff);
  lVar2 = (ulonglong)uVar15 * (ulonglong)uVar4;
  uVar5 = (uint)lVar2;
  uVar7 = *(uint *)(iVar8 + 4);
  *(uint *)(iVar13 + 0xdc) = (uint)(*(int *)(iVar13 + 0x24) != 0);
  iVar8 = (int)((ulonglong)DAT_f2200608 / ((ulonglong)uVar7 / 1000));
  if (iVar8 == 0x753) goto LAB_f220052c;
  if (iVar8 != 0x5dc) {
    if (iVar8 == 0x4e2) {
      uVar10 = 3;
      goto LAB_f220052c;
    }
    if ((iVar8 == 0x442) || (iVar8 == 0x42f)) {
      uVar10 = 4;
      goto LAB_f220052c;
    }
    if (iVar8 - 0x3a9U < 0x85) {
      uVar10 = 5;
      goto LAB_f220052c;
    }
    if (iVar8 - 0x341U < 0x68) {
      uVar10 = 6;
      goto LAB_f220052c;
    }
    s_al_err_printf(DAT_f220060c);
  }
  uVar10 = 2;
LAB_f220052c:
  iVar8 = DAT_f22005dc;
  iVar16 = 0;
  *(undefined1 *)(iVar13 + 0x70) = uVar10;
  do {
    iVar16 = iVar16 + 1;
    iVar6 = al_ddr_init(iVar8);
    if (iVar6 == 0) {
      if (iVar16 != 1) {
        s_al_err_printf(DAT_f2200614,0,iVar16);
      }
      goto LAB_f22005be;
    }
  } while (iVar16 != 1000);
  s_al_err_printf(DAT_f2200610,0);
LAB_f22005be:
  if (iVar6 == 0) {
    uVar7 = *(uint *)(iVar3 + 8);
    *(uint *)(iVar3 + 8) = uVar5 + uVar7;
    *(uint *)(iVar3 + 0xc) =
         (int)((ulonglong)lVar2 >> 0x20) +
         uVar4 * ((int)uVar15 >> 0x1f) +
         uVar15 * (0 << (uVar12 & 0xff) | uVar11 << (uVar12 - 0x20 & 0xff) |
                  uVar11 >> (0x20 - uVar12 & 0xff)) + *(int *)(iVar3 + 0xc) +
         (uint)CARRY4(uVar5,uVar7);
    *(char *)(iVar3 + 4) = (char)iVar16 + -1 + *(char *)(iVar3 + 4);
    if (*(int *)(iVar13 + 0xdc) != 0) {
      if (*(char *)(iVar13 + 0x10) == '\x01') {
        iVar13 = 6;
      }
      else if (*(char *)(iVar13 + 0x10) == '\0') {
        iVar13 = 5;
      }
      else {
        iVar13 = 4;
      }
      puVar14 = (undefined4 *)0x0;
      do {
        *puVar14 = 0;
        puVar14 = (undefined4 *)((int)puVar14 + (4 << (iVar13 - 2U & 0xff)));
      } while (puVar14 < (undefined4 *)0x2000000);
    }
    return;
  }
  s_al_err_printf(DAT_f2200618,0);
  return;
}



/* @ 0xf220061c  FUN_f220061c */

void s_orch_helper(undefined4 *param_1)

{
  *param_1 = DAT_f2200624;
  return;
}



/* @ 0xf2200628  FUN_f2200628 */

int ddr_freq_change_according_to_spd(int *param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  undefined4 uVar8;
  int iVar9;
  int local_48;
  int local_44;
  int local_40;
  undefined1 auStack_3c [24];
  
  uVar1 = DAT_f2200810;
  iVar5 = DAT_f220080c;
  uVar2 = 0;
  do {
    ddr_init_spd_get(uVar2,&local_48,&local_44);
    if (local_48 == 0) {
      s_al_err_printf(DAT_f22007c4,DAT_f22007c0);
      return -0x13;
    }
    if (local_44 == 0) {
      iVar9 = 1;
    }
    else {
      iVar9 = 2;
    }
    while (iVar3 = al_ddr_spd_parse(local_48,(int)((ulonglong)uVar1 /
                                              ((ulonglong)*(uint *)(iVar5 + 4) / 1000)),param_1,
                                &local_40), iVar3 == 0) {
      if (local_40 == 0) {
        iVar5 = *param_1;
        *(int *)(iVar5 + 8) = iVar9;
        *(int *)(iVar5 + 4) = iVar9 * *(int *)(iVar5 + 4);
        return 0;
      }
      uVar6 = param_1[8];
      s_al_err_printf(DAT_f22007d0,DAT_f22007cc,uVar6);
      uVar7 = DAT_f22007dc;
      if ((((0x442 < uVar6) && (uVar7 = DAT_f22007f4, 0x4e2 < uVar6)) &&
          (uVar7 = DAT_f22007e4, 0x5dc < uVar6)) && (uVar7 = DAT_f22007e8, 0x753 < uVar6)) {
        iVar3 = -0x16;
        s_al_err_printf(DAT_f22007d4,DAT_f22007cc,uVar6);
        goto LAB_f22006d0;
      }
      s_al_err_printf(DAT_f22007d0,DAT_f22007e0,uVar7);
      if (uVar7 != DAT_f22007e4) {
        if (DAT_f22007e4 <= uVar7) {
          if (uVar7 == DAT_f22007f4) {
            uVar8 = 1;
            uVar2 = 3;
          }
          else {
            if (uVar7 != DAT_f22007dc) goto LAB_f22006f4;
            uVar8 = 3;
            uVar2 = 0x1e;
          }
          goto LAB_f2200732;
        }
        if (uVar7 == DAT_f22007e8) {
          uVar8 = 1;
          uVar2 = uVar8;
          goto LAB_f2200732;
        }
LAB_f22006f4:
        iVar3 = -0x16;
        s_al_err_printf(DAT_f22007ec,DAT_f22007e0,uVar7);
LAB_f2200702:
        s_al_err_printf(DAT_f22007f0,DAT_f22007cc);
LAB_f22006d0:
        s_al_err_printf(DAT_f22007d8,DAT_f22007c0);
        return iVar3;
      }
      uVar8 = 3;
      uVar2 = 0x11;
LAB_f2200732:
      iVar3 = al_pll_init(DAT_f22007f8,0,1,auStack_3c);
      uVar4 = DAT_f22007fc;
      if (((iVar3 != 0) ||
          (iVar3 = al_pll_freq_set(auStack_3c,uVar2,1000), uVar4 = DAT_f2200800, iVar3 != 0)) ||
         (iVar3 = al_pll_channel_div_set(auStack_3c,0,uVar8,0,0,1,1000), uVar4 = DAT_f2200804, iVar3 != 0)) {
        s_al_err_printf(uVar4,DAT_f22007e0);
        goto LAB_f2200702;
      }
      s_freq_helper_a(&DAT_00009c40);
      s_freq_helper_c(uVar7);
      *(uint *)param_1[1] = *(uint *)(iVar5 + 4) / DAT_f2200808;
    }
    s_al_err_printf(DAT_f22007c8);
    uVar2 = 1;
  } while( true );
}



/* @ 0xf2200814  FUN_f2200814 */

void s_cfg_get_helper(void)

{
  return;
}



/* @ 0xf2200816  FUN_f2200816 */

void ddr_build_addrmap(undefined4 *param_1,undefined1 *param_2)

{
  char cVar1;
  char cVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  char *pcVar7;
  int iVar8;
  char *pcVar9;
  
  pcVar7 = (char *)*param_1;
  if (*pcVar7 == '\x01') {
    iVar4 = 6;
  }
  else if (*pcVar7 == '\0') {
    iVar4 = 5;
  }
  else {
    iVar4 = 4;
  }
  if (pcVar7[0x18] == '\x01') {
    if (iVar4 != 6) {
      *param_2 = (char)iVar4;
      if (iVar4 != 5) {
        param_2[1] = 5;
      }
    }
    uVar6 = param_1[5];
    uVar5 = 9 - iVar4;
    if (uVar6 != 0) {
      param_2[0xd] = 6;
      if (1 < uVar6) {
        param_2[0xe] = 7;
      }
    }
    iVar4 = uVar6 + 6;
    for (; (int)uVar6 < 2; uVar6 = uVar6 + 1) {
      param_2[uVar6 + 0xd] = 0xff;
    }
  }
  else {
    uVar5 = 3;
  }
  uVar6 = param_1[3];
  for (; cVar1 = (char)iVar4, uVar5 < uVar6; uVar5 = uVar5 + 1) {
    param_2[uVar5 - 3] = cVar1;
    iVar4 = iVar4 + 1;
  }
  for (; (int)uVar5 < AL_I2C_TAR_10BIT_ADDR_SHIFT + 1; uVar5 = uVar5 + 1) {
    param_2[uVar5 - 3] = 0xff;
  }
  if (*(int *)(pcVar7 + 4) == 4) {
    param_2[0x21] = cVar1;
    iVar4 = iVar4 + 2;
    param_2[0x22] = cVar1 + '\x01';
    goto LAB_f2200882;
  }
  if (*(int *)(pcVar7 + 4) == 2) {
    if (*(int *)(pcVar7 + 8) == 2) {
      param_2[0x21] = 0xff;
      param_2[0x22] = cVar1;
    }
    else {
      if (*(int *)(pcVar7 + 8) != 1) goto LAB_f2200904;
      param_2[0x21] = cVar1;
      param_2[0x22] = 0xff;
    }
    iVar4 = iVar4 + 1;
  }
  else {
LAB_f2200904:
    param_2[0x21] = 0xff;
    param_2[0x22] = 0xff;
  }
LAB_f2200882:
  pcVar9 = param_2 + 0xf;
  iVar8 = iVar4;
  pcVar7 = pcVar9;
  while( true ) {
    uVar5 = iVar8 - iVar4;
    cVar1 = (char)iVar8;
    if (0x10 < iVar8) break;
    *pcVar7 = cVar1;
    iVar8 = iVar8 + 1;
    pcVar7 = pcVar7 + 1;
  }
  iVar4 = param_1[4];
  pcVar7 = param_2 + 10;
  for (iVar8 = 0; iVar8 != iVar4; iVar8 = iVar8 + 1) {
    *pcVar7 = cVar1 + (char)iVar8;
    pcVar7 = pcVar7 + 1;
  }
  cVar2 = (char)iVar4;
  for (; iVar4 < 3; iVar4 = iVar4 + 1) {
    (param_2 + 10)[iVar4] = -1;
  }
  uVar6 = param_1[2];
  cVar3 = (char)uVar5;
  for (; uVar5 < uVar6; uVar5 = uVar5 + 1) {
    pcVar9[uVar5] = ((cVar1 + cVar2) - cVar3) + (char)uVar5;
  }
  for (; (int)uVar5 < 0x12; uVar5 = uVar5 + 1) {
    pcVar9[uVar5] = -1;
  }
  *(undefined4 *)(param_2 + 0x24) = 0;
  return;
}



/* @ 0xf220093c  FUN_f220093c */

void ddr_init_spd_get(int param_1,undefined4 *param_2,undefined4 *param_3)

{
  char cVar1;
  char cVar2;
  undefined4 uVar3;
  char *pcVar4;
  int *piVar5;
  char *pcVar6;
  int iVar7;
  undefined4 uVar8;
  undefined4 *extraout_r2;
  undefined4 *puVar9;
  undefined4 uVar10;
  undefined4 *puVar11;
  
  pcVar4 = DAT_f2200a30;
  piVar5 = DAT_f2200a2c;
  puVar9 = param_3;
  puVar11 = param_2;
  uVar3 = DAT_f2200a54;
  if (param_1 == 0) {
    *DAT_f2200a2c = 0x50;
    *pcVar4 = 'T';
    pcVar4[2] = '\0';
    pcVar4[3] = '\0';
    pcVar4[4] = 'U';
    pcVar4[6] = '\0';
    pcVar4[7] = '\0';
    pcVar6 = (char *)s_rec_present_check();
    rec_read_aa();
    if (*pcVar6 == -0x56) {
      cVar1 = pcVar6[1];
      cVar2 = pcVar6[3];
      *pcVar4 = cVar1;
      if (cVar1 == -1) {
        *pcVar4 = *(char *)(DAT_f2200a40 + 0x27);
      }
      cVar1 = pcVar6[6];
      *(ushort *)(pcVar4 + 2) = CONCAT11(cVar2,pcVar6[2]);
      cVar2 = pcVar6[4];
      pcVar4[4] = cVar2;
      if (cVar2 == -1) {
        pcVar4[4] = *(char *)(DAT_f2200a40 + 0x27);
      }
      puVar9 = (undefined4 *)(uint)CONCAT11(cVar1,pcVar6[5]);
      *(ushort *)(pcVar4 + 6) = CONCAT11(cVar1,pcVar6[5]);
      uVar3 = DAT_f2200a54;
      pcVar4 = DAT_f2200a30;
    }
    else {
      s_al_err_printf(DAT_f2200a34);
      s_al_err_printf(DAT_f2200a38);
      puVar9 = extraout_r2;
      uVar3 = DAT_f2200a54;
      pcVar4 = DAT_f2200a30;
    }
  }
  while( true ) {
    if (0x58 < *piVar5) {
      s_al_err_printf(DAT_f2200a3c);
      *param_2 = 0;
      *param_3 = 0;
      return;
    }
    s_al_err_printf(uVar3,*pcVar4,puVar9,*piVar5,param_1,puVar11);
    uVar10 = DAT_f2200a44;
    uVar8 = DAT_f2200a50;
    if ((*pcVar4 != '\0') &&
       (iVar7 = i2c_eeprom_read(*pcVar4,*(undefined2 *)(pcVar4 + 2),0x100,DAT_f2200a44),
       uVar8 = DAT_f2200a48, iVar7 == 0)) break;
    s_al_err_printf(uVar8);
    s_al_err_printf(DAT_f2200a4c);
    iVar7 = *piVar5;
    *pcVar4 = (char)iVar7;
    pcVar4[2] = '\0';
    pcVar4[3] = '\0';
    *piVar5 = iVar7 + 1;
    puVar9 = (undefined4 *)0x0;
  }
  if ((pcVar4[4] == '\0') ||
     (iVar7 = i2c_eeprom_read(pcVar4[4],*(undefined2 *)(pcVar4 + 6),1,&stack0xffffffdf), iVar7 != 0)) {
    *param_2 = uVar10;
    uVar10 = 0;
  }
  else {
    *param_2 = uVar10;
  }
  *param_3 = uVar10;
  return;
}



/* @ 0xf2200a58  FUN_f2200a58 */

void set_dram_impedance_ctrl_from_eeprom(uint param_1,uint param_2,undefined1 *param_3)

{
  char cVar1;
  bool bVar2;
  bool bVar3;
  int iVar4;
  undefined1 uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined1 uVar9;
  undefined1 uVar10;
  char local_36 [8];
  undefined1 local_2e;
  undefined1 local_2d;
  undefined1 local_2c;
  undefined1 local_2b;
  undefined1 local_2a;
  undefined1 local_29;
  undefined1 local_28;
  undefined1 local_27;
  undefined1 local_26;
  undefined1 local_25;
  undefined1 local_24;
  undefined1 local_23;
  
  if (param_1 < 2) {
    uVar5 = 3;
  }
  else {
    uVar5 = 5;
  }
  uVar9 = 0;
  *param_3 = 1;
  param_3[1] = uVar5;
  if (param_1 < 2) {
    uVar5 = 0;
  }
  else {
    uVar5 = 2;
  }
  uVar10 = 0;
  param_3[2] = uVar5;
  if (param_1 < 2) {
    *(undefined2 *)(param_3 + 0xc) = 1;
    *(undefined2 *)(param_3 + 0xe) = 2;
    *(undefined2 *)(param_3 + 0x10) = 0;
    *(undefined2 *)(param_3 + 0x12) = 0;
    *(undefined2 *)(param_3 + 0x14) = 0;
    *(undefined2 *)(param_3 + 0x16) = 0;
    *(undefined2 *)(param_3 + 0x18) = 0;
    *(undefined2 *)(param_3 + 0x1a) = 0;
  }
  else {
    *(undefined2 *)(param_3 + 0xc) = 5;
    *(undefined2 *)(param_3 + 0xe) = AL_I2C_INTR_MASK_START_DET_SHIFT;
    *(undefined2 *)(param_3 + 0x12) = AL_I2C_INTR_MASK_START_DET_SHIFT;
    *(undefined2 *)(param_3 + 0x18) = 1;
    *(undefined2 *)(param_3 + 0x10) = 5;
    *(undefined2 *)(param_3 + 0x14) = 4;
    *(undefined2 *)(param_3 + 0x16) = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    *(undefined2 *)(param_3 + 0x1a) = 2;
  }
  *(undefined2 *)(param_3 + 0x1c) = 0xa0a;
  *(undefined2 *)(param_3 + 0x1e) = 0x808;
  *(undefined2 *)(param_3 + 0x20) = 1;
  *(undefined2 *)(param_3 + 4) = 0x1928;
  *(undefined2 *)(param_3 + 6) = 0xb0d;
  *(undefined2 *)(param_3 + 8) = 0xd0d;
  *(undefined2 *)(param_3 + 10) = 0x707;
  iVar4 = s_rec_present_check();
  rec_read_cc();
  iVar6 = DAT_f2200ce4;
  local_36[0] = *(char *)(iVar4 + 0x10);
  local_36[1] = *(undefined1 *)(iVar4 + 0x11);
  local_36[2] = *(undefined1 *)(iVar4 + 0x12);
  local_36[3] = *(undefined1 *)(iVar4 + 0x13);
  local_36[4] = *(undefined1 *)(iVar4 + 0x14);
  local_36[5] = *(undefined1 *)(iVar4 + 0x15);
  local_36[6] = *(undefined1 *)(iVar4 + 0x16);
  local_36[7] = *(undefined1 *)(iVar4 + 0x17);
  local_2e = *(undefined1 *)(iVar4 + 0x18);
  local_2d = *(undefined1 *)(iVar4 + 0x19);
  local_2c = *(undefined1 *)(iVar4 + 0x1a);
  local_2b = *(undefined1 *)(iVar4 + 0x1b);
  local_2a = *(undefined1 *)(iVar4 + 0x1c);
  local_29 = *(undefined1 *)(iVar4 + 0x1d);
  local_28 = *(undefined1 *)(iVar4 + 0x1e);
  local_27 = *(undefined1 *)(iVar4 + 0x1f);
  local_26 = *(undefined1 *)(iVar4 + 0x20);
  local_25 = *(undefined1 *)(iVar4 + 0x21);
  local_24 = *(undefined1 *)(iVar4 + 0x22);
  local_23 = *(undefined1 *)(iVar4 + 0x23);
  if (*(char *)(iVar4 + 0xe) != -0x34) {
    return;
  }
  if (*(char *)(iVar4 + 0xf) == '\x01') {
    *(undefined2 *)(param_3 + 0x20) = 0x100;
  }
  else {
    *(undefined2 *)(param_3 + 0x20) = 1;
  }
  iVar4 = param_1 - 1;
  iVar8 = param_2 / param_1 - 1;
  iVar7 = 0;
  do {
    if (*(char *)(iVar6 + iVar7 * 2) ==
        local_36[iVar4 * AL_I2C_INTR_MASK_START_DET_SHIFT + iVar8 * 5]) {
      param_3[1] = *(undefined1 *)(iVar6 + iVar7 * 2 + 1);
      goto LAB_f2200b9c;
    }
    iVar7 = iVar7 + 1;
  } while (iVar7 != AL_I2C_INTR_MASK_START_DET_SHIFT);
  s_al_err_printf(DAT_f2200cec,DAT_f2200ce8);
LAB_f2200b9c:
  cVar1 = local_36[iVar4 * AL_I2C_INTR_MASK_START_DET_SHIFT + iVar8 * 5 + 1];
  iVar6 = 0;
  if (cVar1 == '\0') {
LAB_f2200bea:
    param_3[2] = *(undefined1 *)(DAT_f2200cf0 + iVar6 * 2 + 1);
  }
  else {
    if (cVar1 == '\x02') {
      iVar6 = 1;
      goto LAB_f2200bea;
    }
    if (cVar1 == '\x04') {
      iVar6 = 2;
      goto LAB_f2200bea;
    }
    if (cVar1 == '\x01') {
      iVar6 = 3;
      goto LAB_f2200bea;
    }
    if (cVar1 == -0x67) {
      iVar6 = 4;
      goto LAB_f2200bea;
    }
    s_al_err_printf(DAT_f2200cf8,DAT_f2200ce8);
  }
  cVar1 = local_36[iVar4 * AL_I2C_INTR_MASK_START_DET_SHIFT + iVar8 * 5 + 2];
  if (cVar1 == '\x06') {
    iVar6 = 0;
LAB_f2200c30:
    *param_3 = *(undefined1 *)(DAT_f2200cfc + iVar6 * 2 + 1);
  }
  else {
    if (cVar1 == '\a') {
      iVar6 = 1;
      goto LAB_f2200c30;
    }
    if (cVar1 == '\x05') {
      iVar6 = 2;
      goto LAB_f2200c30;
    }
    s_al_err_printf(DAT_f2200cf4,DAT_f2200ce8);
  }
  iVar6 = 0;
  do {
    if (*(char *)(DAT_f2200d00 + iVar6 * 2) ==
        local_36[iVar4 * AL_I2C_INTR_MASK_START_DET_SHIFT + iVar8 * 5 + 3]) {
      bVar2 = true;
      uVar9 = *(undefined1 *)(DAT_f2200d00 + iVar6 * 2 + 1);
      goto LAB_f2200c62;
    }
    iVar6 = iVar6 + 1;
  } while (iVar6 != 0xf);
  bVar2 = false;
  s_al_err_printf(DAT_f2200d08,DAT_f2200ce8);
LAB_f2200c62:
  iVar6 = 0;
  do {
    if (*(char *)(DAT_f2200d04 + iVar6 * 2) == local_36[iVar4 * 10 + iVar8 * 5 + 4]) {
      uVar10 = *(undefined1 *)(DAT_f2200d04 + iVar6 * 2 + 1);
      bVar3 = true;
      goto LAB_f2200c8a;
    }
    iVar6 = iVar6 + 1;
  } while (iVar6 != AL_I2C_INTR_MASK_GEN_CALL_SHIFT);
  s_al_err_printf(DAT_f2200d0c,DAT_f2200ce8);
  bVar3 = false;
LAB_f2200c8a:
  if (bVar2) {
    param_3[0x1e] = uVar9;
  }
  if (bVar3) {
    param_3[0x1c] = uVar10;
    if (!bVar2) goto LAB_f2200c9e;
  }
  else if (!bVar2) {
    return;
  }
  param_3[0x1f] = uVar9;
  if (!bVar3) {
    return;
  }
LAB_f2200c9e:
  param_3[0x1d] = uVar10;
  return;
}



/* @ 0xf2200d10  FUN_f2200d10 */

void dram_voltage_gpio(undefined1 param_1)

{
  byte bVar1;
  int iVar2;
  uint *puVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  
  iVar2 = s_rec_present_check();
  rec_read_bb();
  if (*(char *)(iVar2 + 0xb) == -0x45) {
    bVar1 = *(byte *)(iVar2 + 0xc);
    uVar4 = (uint)bVar1;
    if (uVar4 != 0xff) {
      if (0x37 < uVar4) {
        s_al_err_printf(DAT_f2200db8);
        return;
      }
      uVar6 = (uint)*(byte *)(iVar2 + 0xd);
      switch(param_1) {
      case 1:
      case 4:
        s_al_err_printf(DAT_f2200dbc);
      case 0:
        iVar2 = 1;
        break;
      case 5:
        s_al_err_printf(DAT_f2200dc8);
      default:
        iVar2 = 2;
      }
      uVar7 = uVar4 & 7;
      uVar4 = uVar4 - 0x28 & 0xff;
      puVar3 = DAT_f2200dc0;
      if (7 < uVar4) {
        puVar3 = (uint *)((uint)(bVar1 >> 3) * 0x1000 + -0x2778c00);
      }
      *puVar3 = 1 << uVar7 | *puVar3;
      if (iVar2 != 1) {
        uVar6 = 1 - uVar6;
      }
      uVar5 = DAT_f2200dc4;
      if (7 < uVar4) {
        uVar5 = (uint)(bVar1 >> 3);
      }
      if (7 < uVar4) {
        uVar5 = uVar5 << AL_I2C_TAR_10BIT_ADDR_SHIFT;
      }
      if (7 < uVar4) {
        uVar5 = uVar5 + 0xfd887000;
      }
      *(uint *)((4 << uVar7) + uVar5) = uVar6 << uVar7;
    }
  }
  return;
}



/* @ 0xf2200dcc  FUN_f2200dcc */

uint jedec_crc16(byte *param_1,int param_2)

{
  byte *pbVar1;
  byte *pbVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  
  uVar4 = 0;
  pbVar1 = param_1;
  while (pbVar1 != param_1 + param_2) {
    pbVar2 = pbVar1 + 1;
    uVar4 = uVar4 ^ (uint)*pbVar1 << 8;
    iVar3 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    do {
      uVar5 = uVar4 & 0x8000;
      uVar4 = uVar4 << 1;
      if (uVar5 != 0) {
        uVar4 = uVar4 ^ 0x1021;
      }
      iVar3 = iVar3 + -1;
      pbVar1 = pbVar2;
    } while (iVar3 != 0);
  }
  return uVar4 & 0xffff;
}



/* @ 0xf2200df8  FUN_f2200df8 */

undefined4 ddr3_cwl_from_tck(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  
  if (param_2 < 0x9c4) {
    if (param_2 < 0x753) {
      if (param_2 < 0x5dc) {
        if (param_2 < 0x4e2) {
          if (param_2 < 0x42e) {
            if (param_2 < 0x3a7) {
              if (param_2 < 0x341) {
                if (param_2 < 0x2ee) {
                  s_al_err_printf(DAT_f2200e70,DAT_f2200e6c,param_3,0x2ed,param_4);
                  return 0xfffffffb;
                }
                uVar1 = AL_I2C_TAR_10BIT_ADDR_SHIFT;
              }
              else {
                uVar1 = AL_I2C_INTR_MASK_GEN_CALL_SHIFT;
              }
            }
            else {
              uVar1 = AL_I2C_INTR_MASK_START_DET_SHIFT;
            }
          }
          else {
            uVar1 = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
          }
        }
        else {
          uVar1 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
        }
      }
      else {
        uVar1 = 7;
      }
    }
    else {
      uVar1 = 6;
    }
  }
  else {
    uVar1 = 5;
  }
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x3c) = uVar1;
  return 0;
}



/* @ 0xf2200e74  FUN_f2200e74 */

undefined4 ddr4_cwl_from_tck(int param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  
  if (param_2 < 0x4e2) {
    if (param_2 < 0x42e) {
      if (param_2 < 0x3a7) {
        if (param_2 < 0x341) {
          if (param_2 < 0x2ee) {
            if (param_2 < 0x2a9) {
              s_al_err_printf(DAT_f2200ed4,DAT_f2200ed0,param_3,0x2ed,param_4);
              return 0xfffffffb;
            }
            uVar1 = 0x10;
          }
          else {
            uVar1 = 0xe;
          }
        }
        else {
          uVar1 = AL_I2C_TAR_10BIT_ADDR_SHIFT;
        }
      }
      else {
        uVar1 = AL_I2C_INTR_MASK_GEN_CALL_SHIFT;
      }
    }
    else {
      uVar1 = AL_I2C_INTR_MASK_START_DET_SHIFT;
    }
  }
  else {
    uVar1 = AL_I2C_INTR_MASK_STOP_DET_SHIFT;
  }
  *(undefined4 *)(*(int *)(param_1 + 4) + 0x3c) = uVar1;
  return 0;
}



/* @ 0xf2200ed8  FUN_f2200ed8 */

undefined4 ddr_cl_solve(int param_1,uint param_2,int param_3,uint param_4)

{
  uint uVar1;
  
  for (uVar1 = (param_3 + (param_4 - 1)) / param_4;
      ((1 << (uVar1 & 0xff) & param_2) == 0 && (uVar1 < 0x20)); uVar1 = uVar1 + 1) {
  }
  if (uVar1 == 0x20) {
    s_al_err_printf(DAT_f2200f28,DAT_f2200f24,0x20,param_4,param_4);
  }
  else {
    if (uVar1 * param_4 < 0x4e21) {
      *(uint *)(*(int *)(param_1 + 4) + 0x38) = uVar1;
      return 0;
    }
    s_al_err_printf(DAT_f2200f2c,DAT_f2200f24,uVar1,uVar1 * param_4,param_4);
  }
  return 0xfffffffb;
}



/* @ 0xf2200f30  FUN_f2200f30 */

int al_ddr3_spd_parse(int param_1,uint param_2,int *param_3,undefined4 *param_4)

{
  byte bVar1;
  undefined1 *puVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  byte bVar6;
  undefined1 uVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  
  uVar4 = DAT_f2201128;
  uVar5 = DAT_f2201124;
  if ((*(byte *)(param_1 + 7) & 7) != 0) {
    puVar2 = (undefined1 *)*param_3;
    *(uint *)(puVar2 + 4) = ((*(byte *)(param_1 + 7) & 0x3f) >> 3) + 1;
    bVar1 = *(byte *)(param_1 + 8);
    bVar6 = bVar1 & 7;
    if (bVar6 == 2) {
      uVar7 = 0;
    }
    else if (bVar6 == 3) {
      uVar7 = 1;
    }
    else {
      if (bVar6 != 1) {
        s_al_err_printf(DAT_f2201130,DAT_f2201124);
        return -0x16;
      }
      uVar7 = 2;
    }
    *puVar2 = uVar7;
    *(undefined4 *)(puVar2 + 0xc) = 0;
    *(undefined4 *)(puVar2 + 0x10) = 0;
    uVar5 = (uint)*(byte *)(param_1 + 3);
    uVar4 = DAT_f2201134;
    if ((uVar5 & 0xf) < 0xe) {
      uVar8 = 1 << (uVar5 & 0xf);
      if ((uVar8 & 0x35dc) == 0) {
        if ((uVar8 & 0xa22) == 0) goto LAB_f2200f46;
        *(undefined4 *)(puVar2 + 0xc) = 1;
      }
      else if ((int)((uint)*(byte *)(param_1 + 0x3f) << 0x1f) < 0) {
        *(undefined4 *)(puVar2 + 0x10) = 1;
      }
      *(uint *)(puVar2 + 0x14) = (uint)((bVar1 & 0x18) != 0);
      bVar1 = *(byte *)(param_1 + 5);
      param_3[2] = ((bVar1 & 0x3f) >> 3) + AL_I2C_TAR_10BIT_ADDR_SHIFT;
      param_3[3] = (bVar1 & 7) + AL_I2C_INTR_MASK_STOP_DET_SHIFT;
      param_3[4] = ((*(byte *)(param_1 + 4) & 0x7f) >> 4) + 3;
      uVar8 = ((uint)*(byte *)(param_1 + 10) * 1000) / (uint)*(byte *)(param_1 + 0xb);
      param_3[6] = uVar8;
      uVar5 = ((uint)(*(byte *)(param_1 + 9) >> 4) * 10) / (*(byte *)(param_1 + 9) & 0xf);
      param_3[7] = uVar5;
      uVar5 = uVar8 * *(byte *)(param_1 + 0xc) +
              (int)((int)*(char *)(param_1 + 0x22) * uVar5) / AL_I2C_INTR_MASK_START_DET_SHIFT;
      param_3[8] = uVar5;
      if ((param_2 < uVar5) && (s_al_err_printf(DAT_f220112c), param_4 != (undefined4 *)0x0)) {
        *param_4 = 1;
        return 0;
      }
      iVar3 = ddr_cl_solve(param_3,(uint)*(ushort *)(param_1 + 0xe) << 4,
                           param_3[6] * (uint)*(byte *)(param_1 + 0x10) +
                           ((int)*(char *)(param_1 + 0x23) * param_3[7]) /
                           AL_I2C_INTR_MASK_START_DET_SHIFT,param_2);
      uVar4 = DAT_f2201138;
      if ((iVar3 == 0) && (iVar3 = ddr3_cwl_from_tck(param_3,param_2), uVar4 = DAT_f220113c, iVar3 == 0))
      {
        iVar9 = param_3[1];
        iVar10 = param_3[6];
        *(uint *)(iVar9 + 0x34) = iVar10 * (uint)*(byte *)(param_1 + 0x11);
        iVar3 = param_3[7];
        *(uint *)(iVar9 + 0x14) =
             iVar10 * (uint)*(byte *)(param_1 + 0x12) +
             (iVar3 * *(char *)(param_1 + 0x24)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        iVar10 = param_3[6];
        *(uint *)(iVar9 + 0x18) = iVar10 * (uint)*(byte *)(param_1 + 0x13);
        *(uint *)(iVar9 + 0x1c) =
             iVar10 * (uint)*(byte *)(param_1 + 0x14) +
             (iVar3 * *(char *)(param_1 + 0x25)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        bVar1 = *(byte *)(param_1 + 0x15);
        iVar10 = param_3[6];
        *(uint *)(iVar9 + 0xc) =
             iVar10 * ((bVar1 & 0xf) << AL_I2C_INTR_MASK_ACTIVITY_SHIFT |
                      (uint)*(byte *)(param_1 + 0x16));
        *(uint *)(iVar9 + 0x10) =
             iVar10 * ((bVar1 & 0xf0) << 4 | (uint)*(byte *)(param_1 + 0x17)) +
             (*(char *)(param_1 + 0x26) * iVar3) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar9 + 0x20) = (uint)*(ushort *)(param_1 + 0x18) * param_3[6];
        *(uint *)(iVar9 + 0x2c) = (uint)*(byte *)(param_1 + 0x1a) * param_3[6];
        *(uint *)(iVar9 + 0x30) = (uint)*(byte *)(param_1 + 0x1b) * param_3[6];
        *(uint *)(iVar9 + 8) =
             param_3[6] *
             ((*(byte *)(param_1 + 0x1c) & 0xf) << AL_I2C_INTR_MASK_ACTIVITY_SHIFT |
             (uint)*(byte *)(param_1 + 0x1d));
        param_3[9] = (uint)*(byte *)(param_1 + 6);
        return 0;
      }
      s_al_err_printf(uVar4,DAT_f2201124);
      return iVar3;
    }
  }
LAB_f2200f46:
  s_al_err_printf(uVar4,uVar5);
  return -0x16;
}



/* @ 0xf2201140  FUN_f2201140 */

int al_ddr4_spd_parse(int param_1,int param_2,int *param_3,undefined4 *param_4)

{
  byte bVar1;
  undefined1 *puVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  byte bVar6;
  undefined1 uVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  
  bVar1 = *(byte *)(param_1 + 0xc);
  uVar3 = DAT_f22013d0;
  switch(bVar1 & 7) {
  case 0:
    iVar9 = *param_3;
    uVar7 = 0;
    break;
  case 1:
    uVar7 = 1;
    iVar9 = *param_3;
    break;
  case 2:
    uVar7 = 2;
    iVar9 = *param_3;
    break;
  case 3:
    uVar3 = DAT_f22013cc;
    uVar4 = DAT_f22013c4;
    goto LAB_f220124e;
  default:
    goto switchD_f2201156_default;
  }
  puVar2 = (undefined1 *)*param_3;
  *(undefined1 *)(iVar9 + 0x19) = uVar7;
  *(uint *)(puVar2 + 4) = ((bVar1 & 0x3f) >> 3) + 1;
  bVar1 = *(byte *)(param_1 + 0xd);
  bVar6 = bVar1 & 7;
  if (bVar6 == 2) {
    uVar7 = 0;
  }
  else if (bVar6 == 3) {
    uVar7 = 1;
  }
  else {
    uVar3 = DAT_f22013d4;
    if (bVar6 != 1) {
switchD_f2201156_default:
      s_al_err_printf(uVar3,DAT_f22013c4);
      return -0x16;
    }
    uVar7 = 2;
  }
  *puVar2 = uVar7;
  *(undefined4 *)(puVar2 + 0xc) = 0;
  *(undefined4 *)(puVar2 + 0x10) = 0;
  uVar4 = (uint)*(byte *)(param_1 + 3);
  uVar3 = DAT_f22013d8;
  if ((uVar4 & 0xf) < 0xe) {
    uVar8 = 1 << (uVar4 & 0xf);
    if ((uVar8 & 0x324c) == 0) {
      if ((uVar8 & 0x132) == 0) goto LAB_f220124e;
      *(undefined4 *)(puVar2 + 0xc) = 1;
      bVar6 = *(byte *)(param_1 + 0x88);
    }
    else {
      bVar6 = *(byte *)(param_1 + 0x83);
    }
    if ((int)((uint)bVar6 << 0x1f) < 0) {
      *(undefined4 *)(puVar2 + 0x10) = 1;
    }
    *(uint *)(puVar2 + 0x14) = (uint)((bVar1 & 0x18) != 0);
    bVar1 = *(byte *)(param_1 + 5);
    param_3[2] = ((bVar1 & 0x3f) >> 3) + AL_I2C_TAR_10BIT_ADDR_SHIFT;
    param_3[3] = (bVar1 & 7) + AL_I2C_INTR_MASK_STOP_DET_SHIFT;
    bVar1 = *(byte *)(param_1 + 4);
    param_3[4] = ((bVar1 & 0x3f) >> 4) + 2;
    param_3[5] = (uint)(bVar1 >> 6);
    uVar4 = (uint)*(byte *)(param_1 + 0x11);
    uVar3 = DAT_f22013dc;
    if (uVar4 == 0) {
      param_3[6] = 0x7d;
      param_3[7] = AL_I2C_INTR_MASK_START_DET_SHIFT;
      uVar4 = (short)(ushort)*(byte *)(param_1 + 0x12) * 0x7d + (int)*(char *)(param_1 + 0x7d);
      param_3[8] = uVar4;
      if ((param_2 + 1U < uVar4) && (s_al_err_printf(DAT_f22013e0), param_4 != (undefined4 *)0x0)) {
        *param_4 = 1;
        return 0;
      }
      iVar9 = ddr_cl_solve(param_3,((uint)*(byte *)(param_1 + 0x15) <<
                                    AL_I2C_INTR_MASK_ACTIVITY_SHIFT |
                                    (uint)*(byte *)(param_1 + 0x15) << 0x10 |
                                   (uint)*(byte *)(param_1 + 0x14)) << 7,
                           param_3[6] * (uint)*(byte *)(param_1 + 0x18) +
                           ((int)*(char *)(param_1 + 0x7b) * param_3[7]) /
                           AL_I2C_INTR_MASK_START_DET_SHIFT,param_2);
      uVar3 = DAT_f22013c8;
      if ((iVar9 == 0) && (iVar9 = ddr4_cwl_from_tck(param_3,param_2), uVar3 = DAT_f22013e4, iVar9 == 0))
      {
        iVar5 = param_3[7];
        iVar10 = param_3[1];
        iVar9 = param_3[6];
        *(uint *)(iVar10 + 0x44) =
             iVar9 * (uint)*(byte *)(param_1 + 0x28) +
             (iVar5 * *(char *)(param_1 + 0x75)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar10 + 0x14) =
             iVar9 * (uint)*(byte *)(param_1 + 0x19) +
             (iVar5 * *(char *)(param_1 + 0x7a)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar10 + 0x48) =
             iVar9 * (uint)*(byte *)(param_1 + 0x26) +
             (iVar5 * *(char *)(param_1 + 0x77)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar10 + 0x18) =
             iVar9 * (uint)*(byte *)(param_1 + 0x27) +
             (iVar5 * *(char *)(param_1 + 0x76)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar10 + 0x1c) =
             iVar9 * (uint)*(byte *)(param_1 + 0x1a) +
             (iVar5 * *(char *)(param_1 + 0x79)) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        bVar1 = *(byte *)(param_1 + 0x1b);
        iVar9 = param_3[6];
        *(uint *)(iVar10 + 0xc) =
             iVar9 * ((bVar1 & 0xf) << AL_I2C_INTR_MASK_ACTIVITY_SHIFT |
                     (uint)*(byte *)(param_1 + 0x1c));
        *(uint *)(iVar10 + 0x10) =
             iVar9 * ((bVar1 & 0xf0) << 4 | (uint)*(byte *)(param_1 + 0x1d)) +
             (*(char *)(param_1 + 0x78) * iVar5) / AL_I2C_INTR_MASK_START_DET_SHIFT;
        *(uint *)(iVar10 + 0x20) = (uint)*(ushort *)(param_1 + 0x1e) * param_3[6];
        *(uint *)(iVar10 + 0x24) = (uint)*(ushort *)(param_1 + 0x20) * param_3[6];
        iVar9 = *param_3;
        *(uint *)(iVar10 + 0x28) = (uint)*(ushort *)(param_1 + 0x22) * param_3[6];
        *(uint *)(iVar10 + 8) =
             param_3[6] *
             ((*(byte *)(param_1 + 0x24) & 0xf) << AL_I2C_INTR_MASK_ACTIVITY_SHIFT |
             (uint)*(byte *)(param_1 + 0x25));
        param_3[9] = (uint)*(byte *)(param_1 + 0xb);
        s_spd4_timing_get(iVar9 + 0x20,param_1 + 0x3c,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
        s_spd4_timing_get(*param_3 + 0x28,param_1 + 0x46,AL_I2C_INTR_MASK_ACTIVITY_SHIFT);
        s_spd4_timing_get(*param_3 + 0x30,param_1 + 0x44,2);
        *(undefined4 *)(*param_3 + 0x1c) = 1;
        return 0;
      }
      s_al_err_printf(uVar3,DAT_f22013c4);
      return iVar9;
    }
  }
LAB_f220124e:
  s_al_err_printf(uVar3,uVar4);
  return -0x16;
}



/* @ 0xf22013e8  FUN_f22013e8 */

int al_ddr_spd_parse(char *param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined4 *puVar5;
  
  if (param_4 != (undefined4 *)0x0) {
    *param_4 = 0;
  }
  if (*param_1 < 0) {
    uVar3 = 0x75;
  }
  else {
    uVar3 = 0x7e;
  }
  puVar5 = param_4;
  uVar1 = jedec_crc16(param_1,uVar3,param_3,(int)*param_1,param_4);
  uVar4 = (uVar1 & 0xffff) >> 8;
  uVar3 = DAT_f22014a8;
  if (((uint)(byte)param_1[0x7e] == (uVar1 & 0xff)) && ((byte)param_1[0x7f] == uVar4)) {
    uVar1 = jedec_crc16(param_1 + 0x80,0x7e,(uint)(byte)param_1[0x7f],uVar4,puVar5);
    uVar3 = DAT_f22014a8;
    if (((uint)(byte)param_1[0xfe] == (uVar1 & 0xff)) &&
       ((uint)(byte)param_1[0xff] == (uVar1 & 0xffff) >> 8)) {
      if (param_1[2] == AL_I2C_INTR_MASK_GEN_CALL_SHIFT) {
        *(undefined1 *)(*param_3 + 0x18) = 0;
        iVar2 = al_ddr3_spd_parse(param_1,param_2,param_3,param_4);
        uVar3 = DAT_f22014a4;
      }
      else {
        uVar3 = DAT_f22014b0;
        if (param_1[2] != AL_I2C_TAR_10BIT_ADDR_SHIFT) goto LAB_f220146c;
        *(undefined1 *)(*param_3 + 0x18) = 1;
        iVar2 = al_ddr4_spd_parse(param_1,param_2,param_3,param_4);
        uVar3 = DAT_f22014ac;
      }
      if (iVar2 == 0) {
        return 0;
      }
      s_al_err_printf(uVar3,DAT_f22014a0);
      return iVar2;
    }
  }
LAB_f220146c:
  s_al_err_printf(uVar3,DAT_f22014a0);
  return -0x16;
}



/* @ 0xf22014b4  FUN_f22014b4 */

undefined4 FUN_f22014b4(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  s_spd4_timing_get(param_2,param_1 + -0x8000000,param_3,param_2,param_4);
  return 0;
}



/* @ 0xf22014c8  FUN_f22014c8 */

/* WARNING: Removing unreachable block (ram,0xf2201746) */

undefined4 FUN_f22014c8(void)

{
  int *piVar1;
  uint uVar2;
  
  piVar1 = DAT_f2201788;
  uVar2 = *(uint *)(DAT_f22014d8 + 0xc) / 0x1c2000;
  *(undefined4 *)(*DAT_f2201788 + 0xc) = 0x80;
  *(uint *)(*piVar1 + 4) = uVar2 >> AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  *(uint *)*piVar1 = uVar2 & 0xff;
  *(undefined4 *)(*piVar1 + 0xc) = 3;
  *(undefined4 *)(*piVar1 + 8) = 1;
  return 0;
}



/* @ 0xf22014dc  FUN_f22014dc */

uint s_printf_emit(uint param_1)

{
  s_printf_helper(0,param_1 & 0xff);
  if (param_1 == AL_I2C_INTR_MASK_START_DET_SHIFT) {
    s_printf_helper(0,0xd);
  }
  return param_1;
}



/* @ 0xf22014f8  FUN_f22014f8 */

void s_printf_putc(int param_1)

{
  char *pcVar1;
  
  pcVar1 = (char *)(param_1 + -1);
  while( true ) {
    pcVar1 = pcVar1 + 1;
    if (*pcVar1 == '\0') break;
    s_printf_emit();
  }
  return;
}



/* @ 0xf220150a  FUN_f220150a */

void s_printf_fmt_num(uint param_1,int param_2,uint param_3,int param_4,char *param_5,uint param_6,
                 uint param_7)

{
  bool bVar1;
  char cVar2;
  char *pcVar3;
  char *pcVar4;
  char cVar5;
  uint uVar6;
  bool bVar7;
  
  if (((int)param_1 < 0) && (param_2 == 0)) {
    param_1 = -param_1;
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  pcVar4 = param_5;
  if (param_4 == 0) {
    cVar5 = 'W';
  }
  else {
    cVar5 = '7';
  }
  do {
    uVar6 = param_1 - param_3 * (param_1 / param_3);
    cVar2 = (char)uVar6;
    if (uVar6 < 10) {
      cVar2 = cVar2 + '0';
    }
    else {
      cVar2 = cVar2 + cVar5;
    }
    bVar7 = param_3 <= param_1;
    pcVar3 = pcVar4 + 1;
    *pcVar4 = cVar2;
    param_1 = param_1 / param_3;
    pcVar4 = pcVar3;
  } while (bVar7);
  for (; (uint)((int)pcVar3 - (int)param_5) < param_6; pcVar3 = pcVar3 + 1) {
    *pcVar3 = '0';
  }
  for (; (uint)((int)pcVar3 - (int)param_5) < param_7; pcVar3 = pcVar3 + 1) {
    *pcVar3 = ' ';
  }
  pcVar4 = pcVar3;
  if (bVar1) {
    pcVar4 = pcVar3 + 1;
    *pcVar3 = '-';
  }
  *pcVar4 = '\0';
  pcVar3 = param_5 + ((uint)((int)pcVar4 - (int)param_5) >> 1);
  for (; pcVar3 != param_5; param_5 = param_5 + 1) {
    cVar5 = *param_5;
    pcVar4 = pcVar4 + -1;
    *param_5 = *pcVar4;
    *pcVar4 = cVar5;
  }
  return;
}



/* @ 0xf220159c  FUN_f220159c */

uint s_printf_fmt_str(undefined1 *param_1,uint param_2)

{
  uint uVar1;
  undefined1 *puVar2;
  undefined1 *puVar3;
  int *in_r12;
  
  if (in_r12[2] == 0) {
    s_printf_putc();
  }
  else {
    uVar1 = (*in_r12 + -1) - (in_r12[1] - in_r12[2]);
    if (param_2 <= uVar1) {
      uVar1 = param_2;
    }
    param_2 = uVar1;
    puVar2 = param_1 + param_2;
    for (; puVar3 = (undefined1 *)in_r12[1], param_1 != puVar2; param_1 = param_1 + 1) {
      in_r12[1] = (int)(puVar3 + 1);
      *puVar3 = *param_1;
    }
    *puVar3 = 0;
  }
  return param_2;
}



/* @ 0xf22015e4  FUN_f22015e4 */

undefined4 s_printf_pad(undefined1 param_1)

{
  undefined1 *puVar1;
  undefined4 *in_r12;
  
  if (in_r12[2] == 0) {
    s_printf_emit();
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



/* @ 0xf2201618  FUN_f2201618 */

void s_vprintf_core(undefined4 param_1,uint param_2,byte *param_3,uint *param_4)

{
  int iVar1;
  undefined1 *puVar2;
  uint uVar3;
  undefined4 uVar4;
  byte bVar5;
  undefined4 uVar6;
  uint uVar7;
  bool bVar8;
  undefined1 auStack_28 [24];
  
  do {
    while( true ) {
      uVar7 = (uint)*param_3;
      if (uVar7 == 0) {
        return;
      }
      if (param_2 < 2) {
        return;
      }
      if (uVar7 == 0x25) break;
      param_3 = param_3 + 1;
LAB_f22016fc:
      s_printf_pad(uVar7);
    }
    uVar7 = (uint)param_3[1];
    if (uVar7 == 0x30) {
      if (param_3[2] == 0) {
        return;
      }
      bVar5 = param_3[2] - 0x30;
      uVar7 = (uint)param_3[3];
      uVar3 = 0;
      param_3 = param_3 + 4;
      if (AL_I2C_INTR_MASK_START_DET_SHIFT - 1 < bVar5) {
        bVar5 = 0;
      }
    }
    else {
      bVar5 = 0;
      if (uVar7 - 0x31 < AL_I2C_INTR_MASK_ACTIVITY_SHIFT + 1) {
        uVar3 = uVar7 - 0x30 & 0xff;
        uVar7 = (uint)param_3[2];
        param_3 = param_3 + 3;
      }
      else {
        param_3 = param_3 + 2;
        uVar3 = 0;
      }
    }
    if (uVar7 != 100) {
      if (uVar7 < 0x65) {
        if (uVar7 == 0x58) goto LAB_f22016ae;
        if (uVar7 == 99) {
          uVar7 = *param_4 & 0xff;
          param_4 = param_4 + 1;
        }
        else if (uVar7 == 0) {
          return;
        }
      }
      else {
        if (uVar7 == 0x73) {
          puVar2 = (undefined1 *)*param_4;
          for (iVar1 = 0; puVar2[iVar1] != '\0'; iVar1 = iVar1 + 1) {
          }
          goto LAB_f22016ec;
        }
        if (uVar7 < 0x74) {
          if (uVar7 == 0x70) {
            bVar5 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
LAB_f22016ae:
            bVar8 = uVar7 == 0x58;
            uVar6 = 0x10;
LAB_f22016cc:
            uVar4 = 1;
            goto LAB_f22016dc;
          }
        }
        else {
          if (uVar7 == 0x75) {
            bVar8 = false;
            uVar6 = AL_I2C_INTR_MASK_START_DET_SHIFT;
            goto LAB_f22016cc;
          }
          if (uVar7 == 0x78) goto LAB_f22016ae;
        }
      }
      goto LAB_f22016fc;
    }
    bVar8 = false;
    uVar6 = AL_I2C_INTR_MASK_START_DET_SHIFT;
    uVar4 = 0;
LAB_f22016dc:
    iVar1 = s_printf_fmt_num(*param_4,uVar4,uVar6,bVar8,auStack_28,bVar5,uVar3);
    puVar2 = auStack_28;
LAB_f22016ec:
    param_4 = param_4 + 1;
    s_printf_fmt_str(puVar2,iVar1);
  } while( true );
}



/* @ 0xf2201716  FUN_f2201716 */

void s_al_err_printf(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  s_vprintf_core(0,0xffffffff,param_1,&uStack_c,param_1,&uStack_c,param_3);
  return;
}



/* @ 0xf220178c  FUN_f220178c */

void s_printf_helper(int param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  
  puVar1 = *(undefined4 **)(DAT_f22017a0 + param_1 * 4);
  do {
  } while (-1 < (int)(puVar1[5] << 0x19));
  *puVar1 = param_2;
  return;
}



/* @ 0xf22017a4  FUN_f22017a4 */

undefined4 * s_spd4_timing_get(undefined4 *param_1,undefined4 *param_2,uint param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined1 *puVar3;
  int iVar4;
  
  puVar1 = param_1;
  puVar2 = param_2;
  if ((((uint)param_1 ^ (uint)param_2) & 3) == 0) {
    for (; param_1 = puVar1, puVar2 = param_2, param_3 != 0; param_3 = param_3 - 1) {
      if (((uint)puVar1 & 3) == 0) {
        if (0x1f < (int)param_3) {
          iVar4 = (int)param_3 >> 5;
          param_3 = param_3 & 0x1f;
          param_1 = puVar1 + iVar4 * 8;
          puVar2 = param_2 + iVar4 * 8;
          memcpy32_unrolled(puVar1,param_2,iVar4,param_2,param_4);
        }
        for (; 3 < (int)param_3; param_3 = param_3 - 4) {
          *param_1 = *puVar2;
          puVar2 = puVar2 + 1;
          param_1 = param_1 + 1;
        }
        break;
      }
      *(undefined1 *)puVar1 = *(undefined1 *)param_2;
      param_2 = (undefined4 *)((int)param_2 + 1);
      puVar1 = (undefined4 *)((int)puVar1 + 1);
    }
  }
  puVar3 = (undefined1 *)((int)puVar2 + -1);
  puVar2 = (undefined4 *)(param_3 + (int)param_1);
  for (; param_1 != puVar2; param_1 = (undefined4 *)((int)param_1 + 1)) {
    puVar3 = puVar3 + 1;
    *(undefined1 *)param_1 = *puVar3;
  }
  return param_1;
}



/* @ 0xf220180e  FUN_f220180e */

void s_ddr_train_step(uint *param_1,uint param_2,uint param_3)

{
  int iVar1;
  uint *puVar2;
  uint uVar3;
  
  while( true ) {
    if (param_3 == 0) goto LAB_f220184a;
    if (((uint)param_1 & 3) == 0) break;
    param_3 = param_3 - 1;
    *(char *)param_1 = (char)param_2;
    param_1 = (uint *)((int)param_1 + 1);
  }
  uVar3 = param_2 << 0x10 | param_2 << 0x18 | param_2 | param_2 << 8;
  if (0x1f < (int)param_3) {
    iVar1 = (int)param_3 >> 5;
    param_3 = param_3 & 0x1f;
    memset_unrolled(param_1,uVar3,iVar1);
    param_1 = param_1 + iVar1 * 8;
  }
  for (; 3 < (int)param_3; param_3 = param_3 - 4) {
    *param_1 = uVar3;
    param_1 = param_1 + 1;
  }
LAB_f220184a:
  puVar2 = (uint *)((int)param_1 + param_3);
  for (; puVar2 != param_1; param_1 = (uint *)((int)param_1 + 1)) {
    *(char *)param_1 = (char)param_2;
  }
  return;
}



/* @ 0xf220186a  FUN_f220186a */

undefined4 s_ddr_delay_step(uint *param_1,uint param_2,uint param_3,int param_4)

{
  while( true ) {
    if ((*param_1 & param_2) == param_3) {
      return 0;
    }
    if (param_4 == 0) break;
    param_4 = param_4 + -1;
    s_udelay(1);
  }
  return 0xffffff60;
}



/* @ 0xf2201894  FUN_f2201894 */

int al_ddr_mode_register_set(int param_1,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *(int *)(param_1 + 4);
  iVar1 = s_ddr_delay_step(iVar2 + 0x18,1,0,&DAT_00001388);
  if (iVar1 == 0) {
    *(uint *)(iVar2 + 0x10) =
         ((param_3 << AL_I2C_TAR_10BIT_ADDR_SHIFT | param_2 << 4) ^ *(uint *)(iVar2 + 0x10)) &
         0xf0f1 ^ *(uint *)(iVar2 + 0x10);
    *(undefined4 *)(iVar2 + 0x14) = param_4;
    *(uint *)(iVar2 + 0x10) = ~*(uint *)(iVar2 + 0x10) & 0x80000000 ^ *(uint *)(iVar2 + 0x10);
  }
  else {
    s_al_err_printf(DAT_f22018f0,DAT_f22018ec);
  }
  return iVar1;
}



/* @ 0xf22018f4  FUN_f22018f4 */

undefined4 al_ddr_cfg_init(undefined4 param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

{
  undefined1 uVar1;
  int iVar2;
  
  *param_4 = param_1;
  param_4[1] = param_2;
  param_4[2] = param_3;
  iVar2 = *param_3;
  if (iVar2 == DAT_f2201930) {
    uVar1 = 1;
  }
  else {
    if (iVar2 != DAT_f2201934) {
      s_al_err_printf(DAT_f220193c,DAT_f2201938,iVar2,param_4,param_4);
      s_al_err_printf(DAT_f2201944,DAT_f2201940,0xd15);
      return 0xfffffffb;
    }
    uVar1 = 2;
  }
  *(undefined1 *)(param_4 + 3) = uVar1;
  return 0;
}



/* @ 0xf2201948  FUN_f2201948 */

void s_ddr_train_step2(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 8);
  if (*(char *)(param_1 + 0xc) != '\x01') {
    if (*(char *)(param_1 + 0xc) == '\x02') {
      *(uint *)(iVar1 + 0x28) = *(uint *)(iVar1 + 0x28) & 0xfffffffe;
    }
    return;
  }
  *(uint *)(iVar1 + 0xc) = *(uint *)(iVar1 + 0xc) & 0xfbffffff;
  return;
}



/* @ 0xf2201968  FUN_f2201968 */

undefined4 al_ddr_ctrl_wait_for_normal_operating_mode(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 extraout_r2;
  uint uVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0x1389;
  iVar3 = *(int *)(param_1 + 4);
  while( true ) {
    uVar1 = *(uint *)(iVar3 + 4) & 7;
    if (uVar1 == 1) {
      return 0;
    }
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) break;
    s_udelay(1);
    param_3 = extraout_r2;
  }
  s_al_err_printf(DAT_f220199c,DAT_f2201998,param_3,uVar1,param_4);
  return 0xffffff60;
}



/* @ 0xf22019a0  FUN_f22019a0 */

undefined4 al_ddr_phy_wait_for_init_done(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = 0x1389;
  s_udelay(1);
  while( true ) {
    if (*(int *)(param_1 + 0x30) << 0x1f < 0) {
      s_udelay(1);
      if ((DAT_f22019e8 & *(uint *)(param_1 + 0x30)) == 0) {
        uVar1 = 0;
      }
      else {
        uVar1 = 0xfffffffb;
      }
      return uVar1;
    }
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) break;
    s_udelay(1);
  }
  s_al_err_printf(DAT_f22019f0,DAT_f22019ec);
  return 0xffffff60;
}



/* @ 0xf22019f4  FUN_f22019f4 */

void s_ddr_train_step3(int param_1,uint param_2,undefined4 param_3)

{
  *(uint *)(param_1 + 0x200) = *(uint *)(param_1 + 0x200) & ~param_2;
  *(undefined4 *)(param_1 + 4) = param_3;
  DataMemoryBarrier(0x1f);
  al_ddr_phy_wait_for_init_done();
  return;
}



/* @ 0xf2201a10  FUN_f2201a10 */

undefined4 al_ddr_phy_vt_calc_disable(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  char cVar1;
  int iVar2;
  int iVar3;
  
  cVar1 = *(char *)(param_1 + 0xc);
  iVar2 = *(int *)(param_1 + 8);
  if (cVar1 == '\x01') {
    iVar3 = 0x1389;
    *(uint *)(iVar2 + 0xc) = ~*(uint *)(iVar2 + 0xc) & 0x4000000 ^ *(uint *)(iVar2 + 0xc);
    while (-1 < *(int *)(iVar2 + 0x1c) << 1) {
      iVar3 = iVar3 + -1;
      if (iVar3 == 0) {
LAB_f2201a74:
        s_al_err_printf(DAT_f2201a8c,DAT_f2201a84);
        return 0xffffff60;
      }
      s_udelay(1);
    }
  }
  else {
    if (cVar1 != '\x02') {
      s_al_err_printf(DAT_f2201a88,DAT_f2201a84,cVar1,param_4,param_4);
      return 0xffffffea;
    }
    iVar3 = 0x1389;
    *(uint *)(iVar2 + 0x28) = ~*(uint *)(iVar2 + 0x28) & 1 ^ *(uint *)(iVar2 + 0x28);
    while (-1 < *(int *)(iVar2 + 0x34) << 1) {
      iVar3 = iVar3 + -1;
      if (iVar3 == 0) goto LAB_f2201a74;
      s_udelay(1);
    }
  }
  return 0;
}



/* @ 0xf2201a90  FUN_f2201a90 */

int al_ddr_init(int *param_1)

{
  byte bVar1;
  char cVar2;
  ushort uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  uint uVar13;
  undefined4 uVar14;
  int *piVar15;
  undefined4 uVar16;
  undefined4 uVar17;
  int iVar18;
  uint uVar19;
  uint uVar20;
  int iVar21;
  int iVar22;
  int iVar23;
  uint uVar24;
  int iVar25;
  uint uVar26;
  int iVar27;
  uint uVar28;
  uint uVar29;
  uint uVar30;
  int iVar31;
  int iVar32;
  uint uVar33;
  char *pcVar34;
  uint uVar35;
  uint uVar36;
  uint uVar37;
  uint uVar38;
  int iVar39;
  uint *puVar40;
  int *piVar41;
  int iVar42;
  int iVar43;
  int iVar44;
  uint uVar45;
  int iVar46;
  uint uVar47;
  byte *pbVar48;
  uint uVar49;
  uint *puVar50;
  uint uVar51;
  uint uVar52;
  uint uVar53;
  uint uVar54;
  uint *puVar55;
  uint uVar56;
  bool bVar57;
  int *local_164;
  uint local_160;
  uint local_15c;
  int *local_158;
  uint local_154;
  uint local_150;
  int local_140;
  int local_138;
  uint local_134;
  uint local_118;
  uint local_10c;
  uint local_108;
  uint local_104;
  int local_100;
  int local_f0;
  int local_ec;
  char acStack_a4 [12];
  uint local_98 [10];
  uint local_70 [11];
  int local_44;
  uint local_40;
  int local_3c;
  int local_38;
  int iStack_34;
  uint local_30;
  int iStack_2c;
  
  iVar42 = *param_1;
  *(undefined4 *)(iVar42 + 0x38) = 0x3f;
  s_udelay(100);
  *(uint *)(iVar42 + 0x38) = *(uint *)(iVar42 + 0x38) & 0xffffffe6;
  DataMemoryBarrier(0x1f);
  bVar1 = *(byte *)(param_1 + 4);
  if (param_1[0x3e] == 0) {
    *(undefined2 *)(param_1 + 0x52) = 0x101;
    bVar57 = bVar1 == 1;
    *(bool *)((int)param_1 + 0x14a) = bVar1 < 2;
    *(bool *)((int)param_1 + 0x14b) = bVar1 < 2;
    *(bool *)(param_1 + 0x53) = bVar57;
    *(bool *)((int)param_1 + 0x14d) = bVar57;
    *(bool *)((int)param_1 + 0x14e) = bVar57;
    *(bool *)((int)param_1 + 0x14f) = bVar57;
    *(char *)(param_1 + 0x54) = (char)param_1[9];
  }
  else {
    piVar15 = param_1 + 0x52;
    do {
      piVar41 = (int *)((int)piVar15 + 1);
      *(undefined1 *)piVar15 = 0;
      piVar15 = piVar41;
    } while ((int *)((int)param_1 + 0x151) != piVar41);
    *(undefined1 *)((int)param_1 + 0x149) = 1;
    *(undefined1 *)((int)param_1 + 0x14b) = 1;
    *(bool *)((int)param_1 + 0x14d) = bVar1 == 0;
    *(bool *)((int)param_1 + 0x14e) = bVar1 == 0;
    *(char *)(param_1 + 0x53) = (char)param_1[9];
  }
  iVar42 = *param_1;
  *(uint *)(iVar42 + 0x38) = ~*(uint *)(iVar42 + 0x38) & 0x40 ^ *(uint *)(iVar42 + 0x38);
  *(uint *)(iVar42 + 0x38) = *(uint *)(iVar42 + 0x38) & 0xffffffbf;
  uVar8 = DAT_f2201d0c;
  if ((param_1[6] == 2) && (param_1[5] == 2)) {
    iVar39 = 5;
  }
  else {
    iVar39 = (1 << (param_1[5] & 0xffU)) + -1;
  }
  cVar2 = (char)param_1[10];
  puVar40 = (uint *)param_1[1];
  if (cVar2 == '\0') {
    iVar43 = 0;
  }
  else {
    iVar43 = param_1[0x3c];
  }
  uVar35 = (uint)*(byte *)((int)param_1 + 0x29);
  if (uVar35 - 1 < 2) {
    uVar36 = (uint)(param_1[0x40] == 0);
  }
  else {
    uVar36 = 0;
  }
  uVar37 = (uint)*(byte *)(param_1 + 0x1c);
  if (iVar43 == 0) {
    uVar51 = 0;
  }
  else {
    uVar51 = (uint)*(ushort *)(DAT_f2201d04 + uVar37 * 2);
  }
  uVar45 = param_1[0x1b];
  if (cVar2 == '\x01') {
    local_100 = 0;
    if (bVar1 != 0) {
      local_100 = 1;
    }
LAB_f2201b82:
    local_f0 = 0x21;
    local_104 = 0x200;
    local_10c = 0x80;
  }
  else {
    if (cVar2 != '\0') {
      local_100 = 1;
      goto LAB_f2201b82;
    }
    uVar19 = uVar45 * 0x280 + 999;
    if (DAT_f2201d14 < uVar19) {
      uVar19 = uVar19 / 1000;
    }
    else {
      uVar19 = 0x200;
    }
    local_f0 = (uVar19 + 0x1f >> 5) + 1;
    local_104 = uVar45 * 0x140 + 999;
    if (DAT_f2201d18 < local_104) {
      local_104 = local_104 / 1000;
    }
    else {
      local_104 = 0x100;
    }
    local_10c = uVar45 * 0x50 + 999;
    if (local_10c < 65000) {
      local_100 = 1;
      local_10c = 0x40;
    }
    else {
      local_10c = local_10c / 1000;
      local_100 = 1;
    }
  }
  if (cVar2 == '\x01') {
    iVar18 = 15000;
  }
  else {
    iVar18 = param_1[0x28];
  }
  uVar19 = (uVar45 * iVar18 + 999999) / DAT_f2201d08;
  iVar18 = param_1[0x3b];
  if (((iVar18 == 0) || (uVar36 == 0)) || (cVar2 != '\x01')) {
    if (cVar2 != '\0') {
      local_134 = 0;
      goto LAB_f2201c04;
    }
    local_118 = uVar45 * param_1[0x26] + 999999;
    local_134 = 0;
    if (DAT_f2201d0c < local_118) {
      local_118 = local_118 / DAT_f2202058;
    }
    else {
      local_118 = 4;
    }
  }
  else {
    local_134 = uVar45 * 0xea6 + 999999;
    if (DAT_f2201d10 < local_134) {
      local_134 = local_134 / DAT_f2201d08;
    }
    else {
      local_134 = 5;
    }
LAB_f2201c04:
    uVar20 = uVar45 * 0x1d4c + 999999;
    if (DAT_f2201d0c < uVar20) {
      uVar20 = uVar20 / DAT_f2201d08;
    }
    else {
      uVar20 = 4;
    }
    if (iVar18 == 0) {
      local_118 = 0;
    }
    else if (uVar36 == 0) {
      local_118 = 0;
    }
    else {
      local_118 = uVar45 * 0xea6 + 999999;
      if (DAT_f2201d10 < local_118) {
        local_118 = local_118 / DAT_f2201d08;
      }
      else {
        local_118 = 5;
      }
    }
    local_118 = local_118 + uVar20;
  }
  uVar20 = uVar45 * 0x9c4 + 999999;
  if (DAT_f220205c < uVar20) {
    uVar20 = uVar20 / DAT_f2202058;
  }
  else {
    uVar20 = 2;
  }
  if (iVar18 == 0) {
    uVar52 = 0;
  }
  else if (uVar36 == 0) {
    uVar52 = 0;
  }
  else {
    uVar52 = uVar45 * 0xea6 + 999999;
    if (DAT_f2202060 < uVar52) {
      uVar52 = uVar52 / DAT_f2202058;
    }
    else {
      uVar52 = 5;
    }
  }
  if (cVar2 == '\x01') {
    uVar4 = uVar45 * 0x1d4c + 999999;
    if (DAT_f2201d0c < uVar4) goto LAB_f2201db2;
    uVar4 = 4;
  }
  else {
    uVar4 = uVar45 * param_1[0x27] + 999999;
LAB_f2201db2:
    uVar4 = uVar4 / DAT_f2202058;
  }
  if (param_1[0x3f] == 1) {
    if (uVar45 < 0x3a7) {
      iVar5 = 2;
    }
    else {
      iVar5 = 3;
    }
  }
  else {
    iVar5 = 0;
  }
  iVar5 = param_1[0x29] + iVar5;
  param_1[0x29] = iVar5;
  iVar46 = DAT_f2202470;
  iVar21 = param_1[0x2b];
  iVar22 = param_1[0x2a];
  iVar23 = iVar5 - iVar22;
  if (cVar2 == '\0') {
    uVar6 = (uVar45 * *(ushort *)(DAT_f2202064 + uVar37 * 4 + 2) + 999999) / DAT_f2202058;
    uVar24 = (uint)*(ushort *)(DAT_f2202064 + uVar37 * 4);
    if (uVar6 < uVar24) {
      uVar6 = uVar24;
    }
  }
  else {
    uVar6 = uVar45 * 5000 + 999999;
    if (DAT_f2202454 < uVar6) {
      uVar6 = uVar6 / DAT_f2202058;
    }
    else {
      uVar6 = 3;
    }
  }
  iVar25 = param_1[0x1d];
  uVar56 = uVar45 * param_1[0x1e] + 999999;
  uVar24 = uVar56 / DAT_f2202058;
  if (cVar2 == '\0') {
    uVar26 = (uVar45 * *(ushort *)(uVar37 * 4 + DAT_f2202068 + 2) + 999999) / DAT_f2202058;
    uVar7 = (uint)*(ushort *)(DAT_f2202068 + uVar37 * 4);
    if (uVar26 < uVar7) {
      uVar26 = uVar7;
    }
  }
  else {
    uVar26 = (uVar45 * *(ushort *)(uVar37 * 4 + DAT_f2202458 + 2) + 999999) / DAT_f2202058;
    uVar7 = (uint)*(ushort *)(DAT_f2202458 + uVar37 * 4);
    if (uVar26 < uVar7) {
      uVar26 = uVar7;
    }
    uVar26 = uVar26 + uVar51;
  }
  uVar7 = (uVar45 * param_1[0x1f] + 999999) / DAT_f2202058;
  iVar27 = (uVar45 * param_1[0x20] + 999999) / DAT_f2202058 - iVar21;
  uVar28 = param_1[0x21] * uVar45 + 999999;
  if (uVar8 < uVar28) {
    uVar28 = uVar28 / DAT_f2202058;
  }
  else {
    uVar28 = 4;
  }
  uVar53 = param_1[0x2d] * uVar45 + 999999;
  if (uVar8 < uVar53) {
    uVar53 = uVar53 / DAT_f2202058;
  }
  else {
    uVar53 = 4;
  }
  uVar8 = uVar45 * param_1[0x22] + 999999;
  uVar29 = uVar8 / DAT_f2202058;
  local_140 = param_1[7];
  if (cVar2 == '\0') {
    uVar9 = AL_I2C_TAR_10BIT_ADDR_SHIFT;
  }
  else {
    uVar9 = 0x18;
  }
  uVar30 = (uVar45 * 0xf + 999) / 1000;
  if (uVar30 < uVar9) {
    uVar30 = uVar9;
  }
  uVar9 = uVar51;
  if (iVar43 == 0) {
    uVar9 = 0;
  }
  iVar31 = local_140 + uVar30 + uVar9;
  if (cVar2 == '\0') {
    iVar32 = 4;
  }
  else {
    iVar32 = iVar31;
    if (iVar43 == 0) {
      iVar32 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
  }
  uVar9 = uVar45 * AL_I2C_INTR_MASK_START_DET_SHIFT + 999;
  if (uVar9 < 6000) {
    uVar9 = 5;
  }
  else {
    uVar9 = uVar9 / 1000;
  }
  local_ec = local_140;
  if (param_1[0x39] == 1) {
    if (cVar2 == '\0') {
      local_140 = 0;
    }
    local_ec = 0;
  }
  uVar33 = (param_1[0x23] * uVar45 + 999999) / DAT_f2202058;
  uVar49 = (uVar45 * 10000 + param_1[0x23] * uVar45 + 999999) / DAT_f2202058;
  uVar30 = uVar49;
  if ((cVar2 == '\0') && (uVar49 < 5)) {
    uVar30 = 5;
  }
  uVar49 = uVar49 + 0x1f >> 5;
  uVar47 = (uVar45 * (param_1[0x25] + 10000) + 999999) / DAT_f2202058 + 0x1f >> 5;
  if (cVar2 == '\0') {
    uVar10 = 0x200;
  }
  else {
    uVar10 = (uint)*(ushort *)(DAT_f220206c + uVar37 * 2);
  }
  uVar54 = (uVar45 * (DAT_f2202058 / uVar45) * 5 + 999999) / DAT_f2202058;
  iVar43 = DAT_f2202070 * uVar45;
  if (uVar54 < uVar30) {
    uVar54 = uVar30;
  }
  iVar44 = uVar45 * DAT_f2202074;
  iVar11 = param_1[0x3a];
  if (cVar2 == '\0') {
    local_138 = iVar23;
    if (iVar18 == 0) {
      uVar30 = 4;
    }
    else {
      uVar30 = 5;
    }
LAB_f2202248:
    uVar12 = uVar30;
    local_154 = uVar30;
  }
  else {
    if (iVar5 == iVar22) {
      local_138 = 0;
    }
    else {
      local_138 = iVar23 + -1;
    }
    local_154 = (uVar45 * param_1[0x2c] + 999999) / DAT_f2202058;
    if (iVar18 == 0) {
      uVar12 = 4;
    }
    else {
      uVar30 = 5;
      uVar12 = 5;
      if (local_154 == 4) goto LAB_f2202248;
    }
  }
  uVar30 = (uint)*(ushort *)(DAT_f2202078 + uVar37 * 2);
  if (bVar1 == 1) {
    uVar13 = 0x16;
  }
  else if (bVar1 == 0) {
    uVar13 = 0x15;
  }
  else {
    uVar13 = 0x12;
  }
  param_1[0x4e] = uVar51;
  param_1[0x50] = uVar33;
  uVar38 = (uint)*(ushort *)(iVar46 + uVar37 * 2);
  uVar37 = ((uVar49 - uVar47) * 0x10 - uVar13) * 2;
  if ((int)uVar37 < (int)uVar38) {
    uVar37 = uVar38;
  }
  param_1[0x42] = uVar29;
  param_1[0x43] = uVar24;
  param_1[0x44] = uVar7;
  param_1[0x45] = uVar28;
  param_1[0x46] = iVar27;
  param_1[0x47] = uVar4;
  param_1[0x48] = local_118;
  param_1[0x49] = local_138;
  param_1[0x4f] = local_154;
  param_1[0x51] = uVar10;
  puVar40[200] = 0;
  puVar40[0x6c] = puVar40[0x6c] & 0xfffffffe;
  if (uVar35 == 0) {
    uVar24 = 0;
  }
  else if (uVar35 == 1) {
    uVar24 = 0x40000000;
  }
  else {
    uVar24 = 0x80000000;
  }
  if (bVar1 == 2) {
    uVar29 = 0x2000;
  }
  else if (bVar1 == 0) {
    uVar29 = 0x1000;
  }
  else {
    uVar29 = 0;
  }
  if (cVar2 == '\0') {
    uVar38 = 1;
  }
  else {
    uVar38 = 0x10;
  }
  puVar55 = (uint *)param_1[0x41];
  *puVar40 = *puVar40 ^
             ((iVar39 << 0x18 | uVar38 | uVar24 | uVar29 | 0x600) ^ *puVar40) & DAT_f220244c;
  if ((((puVar55 != (uint *)0x0) && (cVar2 == '\0')) && ((param_1[8] != 0 || (param_1[7] != 0)))) &&
     (uVar14 = DAT_f2202450, uVar24 = *puVar55, *puVar55 != 1)) goto switchD_f220308a_default;
  puVar40[0x14] = puVar40[0x14] ^ (puVar40[0x14] ^ (*puVar55 - 1) * 0x10) & 0x1f0;
  puVar40[0x19] =
       puVar40[0x19] ^
       ((uVar33 + 1 >> 1 | (((uVar45 * 7000 + 999) / 1000 + 0x1f >> 5) + 1 >> 1) << 0x10) ^
       puVar40[0x19]) & 0xfff03ff;
  if (param_1[0x37] == 0) {
    uVar24 = 0;
  }
  else {
    uVar24 = 4;
  }
  if (param_1[0x38] == 0) {
    uVar29 = 0;
  }
  else {
    uVar29 = 0x10;
  }
  puVar40[0x1c] = uVar24 | uVar29 | puVar40[0x1c] & 0xffffffe8;
  if (param_1[0x3c] == 0) {
    uVar24 = 0;
    if (param_1[0x3b] != 0) {
      uVar24 = 1;
    }
  }
  else {
    uVar24 = 1;
  }
  puVar40[0x30] = uVar24 | puVar40[0x30] & 0xfffffffe;
  if (param_1[0x3b] == 0) {
    uVar24 = 0;
  }
  else {
    uVar24 = 0x10;
  }
  if (param_1[0x3c] != 0) {
    uVar24 = uVar24 | 1;
  }
  puVar40[0x31] = puVar40[0x31] & 0xfffffc6e | uVar24 | 0x180;
  puVar40[0x32] =
       puVar40[0x32] ^ DAT_f220245c & ((uVar13 | (uVar37 + 1 >> 1) << 0x10 | 0x500) ^ puVar40[0x32])
  ;
  if (cVar2 == '\x01') {
    puVar40[0xa0] = param_1[0xc];
    puVar40[0xa1] = param_1[0xd];
    puVar40[0xa2] = param_1[0xe];
    puVar40[0xa3] = param_1[0xf];
    puVar40[0xa4] = (uint)*(ushort *)(param_1 + 0x10);
  }
  uVar24 = puVar40[0xa5];
  uVar37 = uVar24;
  if (param_1[0xb] == 0) {
    uVar37 = uVar24 ^ 1;
  }
  puVar40[0xa5] = uVar37 & 1 ^ uVar24;
  puVar40[0x34] =
       (((((iVar43 + 999U) / 1000 + 0x3ff >> 10) + 1 >> 1) + 1 |
        (((uVar54 + 0x3ff >> 10) + 2 >> 1) + 1) * 0x10000) ^ puVar40[0x34]) & 0x3ff03ff ^
       puVar40[0x34];
  puVar40[0x35] =
       puVar40[0x35] ^
       (puVar40[0x35] ^
       ((((iVar44 + 999U) / 1000 + 0x3ff >> AL_I2C_INTR_MASK_START_DET_SHIFT) + 1 >> 1) + 1) *
       0x10000) & 0xff0000;
  uVar24 = param_1[0x29];
  uVar14 = DAT_f2202464;
  if (cVar2 == '\0') {
    if (AL_I2C_INTR_MASK_STOP_DET_SHIFT < uVar24 - 5) goto switchD_f220308a_default;
    uVar37 = (uint)*(ushort *)(DAT_f2202460 + (uVar24 - 5) * 2);
    switch(uVar19) {
    case 5:
      uVar37 = uVar37 | 0x200;
      break;
    case 6:
      uVar37 = uVar37 | 0x400;
      break;
    case 7:
      uVar37 = uVar37 | 0x600;
      break;
    case 8:
      uVar37 = uVar37 | 0x800;
      break;
    default:
switchD_f22023b4_caseD_9:
      uVar14 = DAT_f2202d58;
      uVar24 = uVar19;
      goto switchD_f220308a_default;
    case 10:
      uVar37 = uVar37 | 0xa00;
      break;
    case 0xc:
      uVar37 = uVar37 | 0xc00;
      break;
    case 0xe:
      uVar37 = uVar37 | 0xe00;
      break;
    case 0x10:
      break;
    }
    uVar24 = (uint)*(byte *)(param_1 + 0x2e);
    if (uVar24 == 0) {
      uVar29 = 0;
    }
    else {
      uVar14 = DAT_f2202468;
      if (uVar24 != 1) goto switchD_f220308a_default;
      uVar29 = 2;
    }
    uVar24 = (uint)*(byte *)((int)param_1 + 0xb9);
    switch(uVar24) {
    case 0:
      break;
    case 1:
      uVar29 = uVar29 | 0x40;
      break;
    case 2:
      uVar29 = uVar29 | 4;
      break;
    case 3:
      uVar29 = uVar29 | 0x44;
      break;
    case 4:
      uVar29 = uVar29 | 0x204;
      break;
    case 5:
      uVar29 = uVar29 | 0x200;
      break;
    default:
switchD_f220241a_default:
      uVar14 = DAT_f2202d5c;
      goto switchD_f220308a_default;
    }
    uVar24 = param_1[0x2b];
    if (uVar24 == 1) {
      uVar29 = uVar29 | 8;
    }
    else if (uVar24 != 0) {
      uVar14 = DAT_f220246c;
      if (uVar24 != 2) goto switchD_f220308a_default;
      uVar29 = uVar29 | 0x10;
    }
    puVar40[0x37] = uVar29 | uVar37 << 0x10;
    uVar24 = param_1[0x2a];
    if (7 < uVar24 - 5) {
switchD_f2202ce8_caseD_d:
      uVar14 = DAT_f2202d60;
      goto switchD_f220308a_default;
    }
    uVar33 = (uint)*(byte *)(DAT_f22024bc + (uVar24 - 5));
    uVar24 = (uint)*(byte *)((int)param_1 + 0xba);
    if (uVar24 == 1) {
      uVar33 = uVar33 | 0x400;
    }
    else if (uVar24 != 0) {
      if (uVar24 != 2) goto switchD_f2202d06_caseD_2;
      uVar33 = uVar33 | 0x200;
    }
    puVar40[0x38] = uVar33 << 0x10;
    uVar54 = 0;
    local_160 = 0;
    local_108 = 0;
  }
  else {
    if (uVar24 < (uint)param_1[0x2a]) {
      s_al_err_printf(DAT_f2202d50);
      return -0x16;
    }
    if (0xf < uVar24 - 9) goto switchD_f220308a_default;
    uVar37 = (uint)*(ushort *)(DAT_f2202d54 + (uVar24 - 9) * 2);
    switch(uVar19) {
    case 10:
      break;
    default:
      goto switchD_f22023b4_caseD_9;
    case 0xc:
      uVar37 = uVar37 | 0x200;
      break;
    case 0xe:
      uVar37 = uVar37 | 0x400;
      break;
    case 0x10:
      uVar37 = uVar37 | 0x600;
      break;
    case 0x12:
      uVar37 = uVar37 | 0x800;
      break;
    case 0x14:
      uVar37 = uVar37 | 0xa00;
      break;
    case 0x18:
      uVar37 = uVar37 | 0xc00;
    }
    uVar24 = (uint)*(byte *)(param_1 + 0x2e);
    if (uVar24 == 1) {
      uVar29 = 1;
    }
    else {
      uVar14 = DAT_f2202468;
      if (uVar24 != 2) goto switchD_f220308a_default;
      uVar29 = 3;
    }
    uVar24 = (uint)*(byte *)((int)param_1 + 0xb9);
    switch(uVar24) {
    case 0:
      break;
    case 1:
      uVar29 = uVar29 | 0x200;
      break;
    case 2:
      uVar29 = uVar29 | 0x100;
      break;
    case 3:
      uVar29 = uVar29 | 0x300;
      break;
    default:
      goto switchD_f220241a_default;
    case 6:
      uVar29 = uVar29 | 0x400;
      break;
    case 7:
      uVar29 = uVar29 | 0x500;
      break;
    case 8:
      uVar29 = uVar29 | 0x600;
      break;
    case 9:
      uVar29 = uVar29 | 0x700;
    }
    uVar24 = param_1[0x2b];
    if (uVar24 == 1) {
      uVar29 = uVar29 | 8;
    }
    else if (uVar24 != 0) {
      uVar14 = DAT_f220246c;
      if (uVar24 != 2) goto switchD_f220308a_default;
      uVar29 = uVar29 | 0x10;
    }
    puVar40[0x37] = uVar29 | uVar37 << 0x10;
    uVar24 = param_1[0x2a];
    if (param_1[0x3b] == 0) {
      uVar33 = 0;
    }
    else {
      uVar33 = 0x1000;
    }
    switch(uVar24) {
    case 9:
      break;
    case 10:
      uVar33 = uVar33 | 8;
      break;
    case 0xb:
      uVar33 = uVar33 | 0x10;
      break;
    case 0xc:
      uVar33 = uVar33 | 0x18;
      break;
    default:
      goto switchD_f2202ce8_caseD_d;
    case 0xe:
      uVar33 = uVar33 | 0x20;
      break;
    case 0x10:
      uVar33 = uVar33 | 0x28;
      break;
    case 0x12:
      uVar33 = uVar33 | 0x30;
    }
    uVar24 = (uint)*(byte *)((int)param_1 + 0xba);
    switch(uVar24) {
    case 0:
      break;
    case 1:
      uVar33 = uVar33 | 0x200;
      break;
    default:
switchD_f2202d06_caseD_2:
      uVar14 = DAT_f22024c0;
      goto switchD_f220308a_default;
    case 3:
      uVar33 = uVar33 | 0x400;
      break;
    case 4:
      uVar33 = uVar33 | 0x600;
    }
    uVar54 = uVar30 - 4 & 0xffff;
    uVar14 = DAT_f2202ea0;
    uVar24 = uVar30;
    if (2 < uVar54) {
switchD_f220308a_default:
      s_al_err_printf(uVar14,uVar24);
      return -0x16;
    }
    local_108 = (uint)*(ushort *)(DAT_f2202e9c + uVar54 * 2);
    puVar40[0x38] = local_108 | uVar33 << 0x10;
    if (param_1[0x40] == 0) {
      uVar30 = 0;
    }
    else {
      uVar30 = 0x800;
    }
    if (param_1[0x3f] == 0) {
      uVar24 = 0;
    }
    else {
      uVar24 = 0x1000;
    }
    uVar30 = uVar30 | uVar24 | uVar36 << 10;
    uVar24 = param_1[0x4e];
    uVar54 = uVar30 | 0x200;
    if (uVar24 == 4) {
      uVar54 = uVar30 | 0x201;
    }
    else if (uVar24 == 5) {
      uVar54 = uVar30 | 0x202;
    }
    else {
      uVar14 = DAT_f2202ea4;
      if (uVar24 != 0) goto switchD_f220308a_default;
    }
    uVar14 = DAT_f2202ea8;
    uVar24 = (uint)*(byte *)((int)param_1 + 0xbb);
    switch((uint)*(byte *)((int)param_1 + 0xbb)) {
    case 0:
      break;
    case 1:
      uVar54 = uVar54 | 0x40;
      break;
    case 2:
      uVar54 = uVar54 | 0x80;
      break;
    case 3:
      uVar54 = uVar54 | 0xc0;
      break;
    case 4:
      uVar54 = uVar54 | 0x100;
      break;
    case 5:
      uVar54 = uVar54 | 0x140;
      break;
    case 6:
      uVar54 = uVar54 | 0x180;
      break;
    case 7:
      uVar54 = uVar54 | 0x1c0;
      break;
    default:
      goto switchD_f220308a_default;
    }
    puVar40[0x3a] = uVar54;
    local_160 = (uint)*(byte *)((int)param_1 + 0xbd);
    uVar14 = DAT_f2202eac;
    uVar24 = local_154;
    switch(local_154) {
    case 4:
      break;
    case 5:
      local_160 = local_160 | 0x400;
      break;
    case 6:
      local_160 = local_160 | 0x800;
      break;
    case 7:
      local_160 = local_160 | 0xc00;
      break;
    case 8:
      local_160 = local_160 | 0x1000;
      break;
    default:
      goto switchD_f220308a_default;
    }
    puVar40[0x3b] = puVar40[0x3b] & 0xffff | local_160 << 0x10;
  }
  uVar24 = puVar40[0x39];
  *(short *)(param_1 + 0x4a) = (short)uVar37;
  *(short *)((int)param_1 + 0x12a) = (short)uVar29;
  *(short *)(param_1 + 0x4b) = (short)uVar33;
  *(short *)((int)param_1 + 0x12e) = (short)local_108;
  *(undefined2 *)(param_1 + 0x4c) = 0;
  *(short *)((int)param_1 + 0x132) = (short)uVar54;
  *(short *)(param_1 + 0x4d) = (short)local_160;
  puVar40[0x39] = (uVar24 ^ local_f0 << 0x10) & 0xff0000 ^ uVar24;
  if (param_1[8] != 0) {
    puVar40[0x3c] = ~puVar40[0x3c] & 3 ^ puVar40[0x3c];
  }
  if (((param_1[7] != 0) && (puVar40[0x3c] = ~puVar40[0x3c] & 1 ^ puVar40[0x3c], param_1[7] != 0))
     && (cVar2 == '\x01')) {
    if ((char)param_1[0x19] == -1) {
      uVar24 = 4;
    }
    else {
      uVar24 = 0xc;
    }
    if (uVar35 == 2) {
      uVar30 = 0;
    }
    else {
      uVar30 = 0x10;
    }
    puVar40[0x3c] = uVar24 | uVar30 | puVar40[0x3c] & 0xffffffe3;
  }
  if ((short)param_1[10] == 0x201) {
    puVar40[0x3c] = ~puVar40[0x3c] & 0x20 ^ puVar40[0x3c];
  }
  iVar39 = iVar22 + iVar21 + uVar51;
  iVar43 = iVar21 + uVar51 + iVar5 + local_ec;
  iVar5 = (uVar47 + 2 >> 1) + 1;
  puVar40[0x3d] = puVar40[0x3d] & 0xfffff00f | 0x770;
  uVar51 = DAT_f2202914;
  puVar40[0x40] =
       ((uVar56 / DAT_f2202914 + 1 | ((uVar45 * DAT_f2202910) / 0xfa000 - 1 >> 1) << 8 |
         ((iVar25 * uVar45 + 999999) / DAT_f2202934 + 1 >> 1) << 0x10 |
        ((iVar39 + 4 + uVar19 + local_134 >> 1) + 1) * 0x1000000) ^ puVar40[0x40]) & 0x7f3f7f3f ^
       puVar40[0x40];
  if (uVar4 < 4) {
    uVar4 = iVar21 + 4;
  }
  else {
    uVar4 = iVar21 + uVar4;
  }
  puVar40[0x41] =
       DAT_f2202918 &
       ((uVar7 + 1 >> 1 | ((uVar4 >> 1) + 1) * 0x100 | (uVar26 + 1 >> 1) << 0x10) ^ puVar40[0x41]) ^
       puVar40[0x41];
  puVar40[0x42] =
       ((local_118 + iVar39 + 7 >> 1 | (iVar23 + AL_I2C_INTR_MASK_STOP_DET_SHIFT >> 1) << 8 |
         (iVar43 + 1U >> 1) << 0x10 | ((uint)(iVar39 + 1 + local_140) >> 1) << 0x18) ^ puVar40[0x42]
       ) & 0x3f3f1f3f ^ puVar40[0x42];
  puVar40[0x43] =
       ((iVar31 + 1U >> 1 | (iVar32 + 1U >> 1) << 0xc) ^ puVar40[0x43]) & DAT_f2202938 ^
       puVar40[0x43];
  puVar40[0x44] =
       puVar40[0x44] ^
       DAT_f220291c &
       ((uVar8 / uVar51 + 1 | (iVar27 + 1U >> 1) << 0x18 | (uVar28 + 1 >> 1) << 8 |
        (local_154 + 1 >> 1) << 0x10) ^ puVar40[0x44]);
  puVar40[0x45] =
       DAT_f2202920 &
       ((uVar6 + 1 >> 1 | (uVar6 + 2 >> 1) << 8 | (uVar9 + 1 >> 1) << 0x18 |
        (uVar9 + 2 >> 1) << 0x10) ^ puVar40[0x45]) ^ puVar40[0x45];
  puVar40[0x48] =
       puVar40[0x48] ^
       (((uVar49 + 2 >> 1) + 1 | iVar5 * 0x1000000 | iVar5 * 0x10000 |
        (((uVar10 >> 5) + 2 >> 1) + 1) * 0x100) ^ puVar40[0x48]) & 0x7f7f7f7f;
  if (cVar2 == '\x01') {
    puVar40[0x49] =
         (((uVar53 + 1 >> 1) << 8 | uVar52 + uVar20 + iVar39 + 7 >> 1 | (uVar12 + 1 >> 1) << 0x10) ^
         puVar40[0x49]) & 0x70f3f ^ puVar40[0x49];
  }
  puVar40[0x60] =
       puVar40[0x60] ^
       ((local_10c + 1 >> 1 | (local_104 + 1 >> 1) << 0x10) ^ puVar40[0x60]) & 0x3ff03ff;
  puVar40[100] = puVar40[100] ^
                 DAT_f2202924 &
                 (((iVar43 + -4) * 0x10000 | (param_1[7] + 4) * 0x1000000 | iVar39 + -2 + local_ec |
                  0x808200U) ^ puVar40[100]);
  puVar40[0x65] =
       puVar40[0x65] ^ ((param_1[7] + 3U | (param_1[7] + 3U) * 0x100) ^ puVar40[0x65]) & 0xf0f;
  uVar8 = DAT_f2202928;
  puVar40[0x68] = ~puVar40[0x68] & 0x80000000 ^ puVar40[0x68];
  if (iVar11 == 0) {
    uVar8 = DAT_f220292c;
  }
  puVar40[0x6a] = DAT_f2202930 & puVar40[0x6a] | uVar8;
  uVar8 = uVar36;
  if (param_1[0x3f] != 0) {
    uVar8 = uVar36 | 4;
  }
  if (param_1[0x40] != 0) {
    uVar8 = uVar8 | 2;
  }
  pbVar48 = (byte *)(param_1 + 0x11);
  puVar40[0x70] = uVar8;
  iVar39 = -6;
  puVar50 = local_98;
  do {
    if (bVar1 == 1) {
      uVar8 = (uint)*pbVar48;
LAB_f2202880:
      if (uVar8 == 0xff) goto LAB_f2202e96;
      uVar8 = uVar8 + iVar39;
    }
    else {
      if (bVar1 == 0) {
        uVar8 = (uint)pbVar48[1];
        goto LAB_f2202880;
      }
      if (iVar39 != -0xe) {
        uVar8 = (uint)pbVar48[2];
        goto LAB_f2202880;
      }
LAB_f2202e96:
      uVar8 = 0xf;
    }
    iVar39 = iVar39 + -1;
    pbVar48 = pbVar48 + 1;
    *puVar50 = uVar8;
    puVar50 = puVar50 + 1;
  } while (iVar39 != -0xf);
  iVar39 = -10;
  uVar8 = (uint)*(byte *)((int)param_1 + 0x51);
  if (*(byte *)((int)param_1 + 0x4e) == 0xff) {
    uVar51 = 0x1f;
  }
  else {
    uVar51 = *(byte *)((int)param_1 + 0x4e) - 6;
  }
  if (*(byte *)((int)param_1 + 0x4f) == 0xff) {
    iVar43 = 0x1f;
  }
  else {
    iVar43 = *(byte *)((int)param_1 + 0x4f) - 7;
  }
  if (*(byte *)(param_1 + 0x14) == 0xff) {
    iVar5 = 0x1f;
  }
  else {
    iVar5 = *(byte *)(param_1 + 0x14) + -AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
  }
  uVar45 = 0xfffffff1;
  if (uVar8 != 0xff) {
    uVar45 = uVar8 - 6;
  }
  if (uVar8 == 0xff) {
    uVar45 = 0x1f;
  }
  if (*(byte *)((int)param_1 + 0x52) == 0xff) {
    iVar46 = 0x1f;
  }
  else {
    iVar46 = *(byte *)((int)param_1 + 0x52) - 7;
  }
  if (*(byte *)((int)param_1 + 0x65) == 0xff) {
    uVar8 = 0x1f;
  }
  else {
    uVar8 = *(byte *)((int)param_1 + 0x65) + -AL_I2C_INTR_MASK_START_DET_SHIFT;
  }
  if (*(byte *)((int)param_1 + 0x66) == 0xff) {
    iVar21 = 0x1f;
  }
  else {
    iVar21 = *(byte *)((int)param_1 + 0x66) - 0xb;
  }
  pbVar48 = (byte *)((int)param_1 + 0x53);
  puVar50 = local_70;
  do {
    if (*pbVar48 == 0xff) {
      uVar19 = 0xf;
    }
    else {
      uVar19 = (uint)*pbVar48 + iVar39;
    }
    iVar39 = iVar39 + -1;
    *puVar50 = uVar19;
    pbVar48 = pbVar48 + 1;
    puVar50 = puVar50 + 1;
  } while (iVar39 != -0x1c);
  puVar40[0x80] = ((uVar8 | iVar21 << 8) ^ puVar40[0x80]) & 0x1f1f ^ puVar40[0x80];
  puVar40[0x81] =
       puVar40[0x81] ^
       ((iVar43 << AL_I2C_INTR_MASK_ACTIVITY_SHIFT | iVar5 << 0x10 | uVar51) ^ puVar40[0x81]) &
       0x1f1f1f;
  puVar40[0x82] =
       puVar40[0x82] ^
       ((local_98[2] << 0x10 | local_98[3] << 0x18 | local_98[0] | local_98[1] << 8) ^ puVar40[0x82]
       ) & 0xf0f0f0f;
  puVar40[0x83] =
       puVar40[0x83] ^
       ((local_98[6] << 0x10 | local_98[7] << 0x18 | local_98[4] | local_98[5] << 8) ^ puVar40[0x83]
       ) & 0xf0f0f0f;
  puVar40[0x84] = puVar40[0x84] ^ ((local_98[8] | 0xf00) ^ puVar40[0x84]) & 0xf0f;
  puVar40[0x85] =
       puVar40[0x85] ^
       ((local_70[6] << 0x10 | local_44 << 0x18 | local_70[0] | local_70[1] << 8) ^ puVar40[0x85]) &
       0xf0f0f0f;
  puVar40[0x86] =
       puVar40[0x86] ^
       ((local_38 << 0x10 | iStack_34 << 0x18 | local_40 | local_3c << 8) ^ puVar40[0x86]) &
       0xf0f0f0f;
  puVar40[0x87] = ((local_30 | iStack_2c << 8) ^ puVar40[0x87]) & 0xf0f ^ puVar40[0x87];
  *(uint *)(&DAT_00004820 + iVar42) =
       local_70[5] << 0x12 | local_70[3] << 6 | local_70[2] | local_70[4] << 0xc | 0x80000000;
  uVar3 = *(ushort *)(param_1 + 0x33);
  puVar40[0x88] = ((uVar45 | iVar46 << 8) ^ puVar40[0x88]) & 0x1f1f ^ puVar40[0x88];
  puVar40[0x90] =
       puVar40[0x90] ^
       (((iVar18 + 6) * 0x1000000 | (iVar18 + 6) * 0x100 | local_138 << 2) ^ puVar40[0x90]) &
       DAT_f2202d44;
  puVar40[0x91] =
       (uint)*(ushort *)(param_1 + 0x31) |
       (uint)*(ushort *)((int)param_1 + 0xc6) << AL_I2C_INTR_MASK_ACTIVITY_SHIFT | (uint)uVar3 << 4
       | (uint)*(ushort *)((int)param_1 + 0xce) << 0xc | (uint)*(ushort *)(param_1 + 0x32) << 0x10 |
       (uint)*(ushort *)(param_1 + 0x34) << 0x14 | (uint)*(ushort *)((int)param_1 + 0xca) << 0x18 |
       (uint)*(ushort *)((int)param_1 + 0xd2) << 0x1c;
  *(uint *)(iVar42 + 0xa8) =
       *(uint *)(iVar42 + 0xa8) ^ (0x3f - puVar55[2] ^ *(uint *)(iVar42 + 0xa8)) & 0x7f;
  if (puVar55[3] == 0) {
    uVar8 = 0;
  }
  else {
    uVar8 = 4;
  }
  puVar40[0x94] =
       DAT_f2202d48 &
       ((puVar55[2] << AL_I2C_INTR_MASK_ACTIVITY_SHIFT | puVar55[1] << 0x18 | uVar8) ^ puVar40[0x94]
       ) ^ puVar40[0x94];
  puVar40[0x95] = (puVar55[4] ^ puVar40[0x95]) & 0xff ^ puVar40[0x95];
  puVar40[0x97] = puVar40[0x97] ^ ((puVar55[7] | puVar55[6] << 0x18) ^ puVar40[0x97]) & 0xff00ffff;
  puVar40[0x99] = puVar40[0x99] ^ ((puVar55[10] | puVar55[9] << 0x18) ^ puVar40[0x99]) & 0xff00ffff;
  puVar40[0x9b] =
       puVar40[0x9b] ^ ((puVar55[0xd] | puVar55[0xc] << 0x18) ^ puVar40[0x9b]) & 0xff00ffff;
  if (param_1[0x3d] != 0) {
    puVar40[0xcc] = puVar40[0xcc] & 0xfefffefc | 1;
  }
  if ((cVar2 == '\x01') && (bVar1 == 0)) {
    if (uVar36 == 0) {
      s_al_err_printf(DAT_f2202d4c);
      return -0x16;
    }
    puVar40[0x100] = ~puVar40[0x100] & 0x100 ^ puVar40[0x100];
    puVar40[0xc0] = puVar40[0xc0] & 0xfffffffe;
  }
  iVar39 = param_1[2];
  puVar40[0x101] = (puVar40[0x101] ^ local_100 << 0xb) & 0x800 ^ puVar40[0x101];
  puVar40[0x104] = puVar40[0x104] & 0xffc00000 | 0x18;
  puVar40[0x105] = puVar40[0x105] & 0xffc00000 | 8;
  *(undefined4 *)(&DAT_00004830 + iVar42) = 0x61e;
  *(undefined4 *)(&DAT_00004834 + iVar42) = AL_I2C_TAR_10BIT_ADDR_SHIFT;
  *(undefined4 *)(&DAT_00004838 + iVar42) = 0x79e;
  *(undefined4 *)(&DAT_0000483c + iVar42) = 0x70c;
  puVar40[0x125] = DAT_f22030c0;
  if ((param_1[6] == 2) && (param_1[5] == 2)) {
    iVar42 = 5;
  }
  else {
    iVar42 = (1 << (param_1[5] & 0xffU)) + -1;
  }
  uVar8 = param_1[0x1b];
  bVar57 = uVar8 < 900;
  uVar36 = uVar8 * 0xc418;
  if (bVar57) {
    uVar8 = *(uint *)(iVar39 + 0x80);
  }
  if (bVar57) {
    *(uint *)(iVar39 + 0x80) = uVar8 & 0xffe7ffff | 0x80000;
  }
  if (param_1[0x3e] != 0) {
    *(undefined4 *)(iVar39 + 0xc4) = 1;
  }
  if (uVar35 == 0) {
    uVar8 = 0x80000000;
  }
  else {
    uVar8 = 0x40000000;
  }
  *(uint *)(iVar39 + 0x88) = *(uint *)(iVar39 + 0x88) & 0x3fffffff | uVar8;
  if (cVar2 == '\x01') {
    *(uint *)(iVar39 + 0x88) = *(uint *)(iVar39 + 0x88) & 0xffffe01f | 0x1000;
  }
  *(uint *)(iVar39 + 0x90) = *(uint *)(iVar39 + 0x90) & 0xffa3fffe | 0x440000;
  if (param_1[8] == 0) {
    uVar8 = 0x10000000;
  }
  else {
    uVar8 = 0x38000000;
  }
  if (param_1[7] != 0) {
    uVar8 = uVar8 | 0x8000000;
  }
  if (cVar2 == '\0') {
    uVar8 = uVar8 | 3;
  }
  else {
    uVar8 = uVar8 | 4;
  }
  if (uVar35 == 2) {
    uVar8 = uVar8 | 0x40000000;
  }
  *(uint *)(iVar39 + 0x100) =
       (uVar8 ^ *(uint *)(iVar39 + 0x100)) & 0x78000307 ^ *(uint *)(iVar39 + 0x100);
  if (cVar2 == '\x01') {
    uVar37 = uVar37 & 0xfffffeff;
  }
  *(uint *)(iVar39 + 0x180) = uVar37;
  *(uint *)(iVar39 + 0x184) = uVar29;
  *(uint *)(iVar39 + 0x188) = uVar33;
  *(uint *)(iVar39 + 0x18c) = local_108;
  *(undefined4 *)(iVar39 + 400) = 0;
  *(uint *)(iVar39 + 0x194) = uVar54;
  *(uint *)(iVar39 + 0x198) = local_160;
  *(uint *)(iVar39 + 4) = *(uint *)(iVar39 + 4) & 0xfffffffd;
  *(uint *)(iVar39 + 0x680) = *(uint *)(iVar39 + 0x680) & 0xffffc7ff;
  s_udelay(1000);
  iVar18 = DAT_f22030d0;
  uVar35 = DAT_f22030cc;
  uVar8 = DAT_f22030c8;
  iVar5 = 0;
  iVar43 = param_1[10];
  pbVar48 = (byte *)((int)param_1 + 0xbe);
  while( true ) {
    if ((char)iVar43 == '\0') {
      uVar24 = (uint)pbVar48[0x16];
      uVar14 = DAT_f22030c4;
      if (AL_I2C_INTR_MASK_START_DET_SHIFT < uVar24) goto switchD_f220308a_default;
      uVar37 = (uint)*(byte *)(iVar18 + uVar24);
      uVar24 = (uint)pbVar48[0x18];
      uVar14 = DAT_f2203544;
      switch(uVar24) {
      case 0:
        uVar37 = uVar37 | 0x10;
        break;
      case 1:
        uVar37 = uVar37 | 0x20;
        break;
      case 2:
        uVar37 = uVar37 | 0x30;
        break;
      case 3:
        uVar37 = uVar37 | 0x40;
        break;
      case 4:
        uVar37 = uVar37 | 0x50;
        break;
      case 5:
        uVar37 = uVar37 | 0x60;
        break;
      case 6:
        uVar37 = uVar37 | 0x70;
        break;
      case 7:
        uVar37 = uVar37 | 0x80;
        break;
      case 8:
        uVar37 = uVar37 | 0x90;
        break;
      case 9:
        uVar37 = uVar37 | 0xa0;
        break;
      case 10:
        uVar37 = uVar37 | 0xb0;
        break;
      case 0xb:
        uVar37 = uVar37 | 0xc0;
        break;
      case 0xc:
        uVar37 = uVar37 | 0xd0;
        break;
      case 0xd:
        uVar37 = uVar37 | 0xe0;
        break;
      case 0xe:
        uVar37 = uVar37 | 0xf0;
        break;
      default:
        goto switchD_f220308a_default;
      }
      iVar46 = iVar39 + iVar5 * 0x10;
      *(uint *)(iVar46 + 0x684) = uVar37 | *(uint *)(iVar46 + 0x684) & 0xffffff00;
    }
    else {
      *(uint *)(iVar39 + 0xc0) = *(uint *)(iVar39 + 0xc0) & 0xfffffffd;
      *(uint *)(iVar39 + 0x680) = *(uint *)(iVar39 + 0x680) & 0xf8fff8ff | uVar8;
      iVar46 = iVar39 + iVar5 * 0x10;
      uVar37 = *(uint *)(iVar46 + 0x684);
      *(uint *)(iVar46 + 0x684) =
           (((uint)pbVar48[2] << AL_I2C_TAR_10BIT_ADDR_SHIFT | (uint)*pbVar48 << 8 |
             (uint)pbVar48[4] << 0x10 | iVar5 << 0x16) ^ uVar37) & uVar35 ^ uVar37;
    }
    pbVar48 = pbVar48 + 1;
    if (iVar5 != 0) break;
    iVar5 = 1;
  }
  *(uint *)(iVar39 + 0x680) = *(uint *)(iVar39 + 0x680) & 0xffffc7ff | 0x800;
  s_udelay(1000);
  piVar15 = param_1 + 0x31;
  for (uVar8 = 0; (int)uVar8 < param_1[5]; uVar8 = uVar8 + 1) {
    iVar43 = *piVar15;
    *(uint *)(iVar39 + 0x4dc) = uVar8 | uVar8 << 0x10;
    *(uint *)(iVar39 + 0x98) = CONCAT22((short)iVar43,(short)piVar15[2]);
    piVar15 = (int *)((int)piVar15 + 2);
  }
  if (param_1[0x39] != 0) {
    if (*(char *)((int)param_1 + 0x29) == '\0') {
      uVar8 = 0;
    }
    else {
      uVar8 = 0x1000;
    }
    if (param_1[0x3f] == 0) {
      uVar35 = 0x8000;
    }
    else {
      uVar35 = 0xc000;
    }
    *(uint *)(iVar39 + 0x200) = *(uint *)(iVar39 + 0x200) & 0xffff2fbf | uVar8 | uVar35 | 0x40;
    iVar43 = param_1[10];
    *(uint *)(iVar39 + 0x204) =
         (*(uint *)(iVar39 + 0x204) ^ iVar42 << 0x10) & 0xf0000 ^ *(uint *)(iVar39 + 0x204);
    if ((char)iVar43 == '\x01') {
      iVar42 = 0;
      do {
        uVar8 = (uint)*(byte *)(param_1 + 0x2f);
        iVar43 = iVar39 + iVar42 * 0x100;
        iVar42 = iVar42 + 1;
        *(uint *)(iVar43 + 0x714) =
             ((uVar8 << 0x10 | uVar8 << 8 | uVar8 | uVar8 << 0x18) ^ *(uint *)(iVar43 + 0x714)) &
             0x3f3f3f3f ^ *(uint *)(iVar43 + 0x714);
        uVar8 = (uint)*(byte *)(param_1 + 0x2f);
        *(uint *)(iVar43 + 0x720) =
             ((uVar8 << 0x10 | uVar8 << 8 | uVar8 | uVar8 << 0x18) ^ *(uint *)(iVar43 + 0x720)) &
             0x3f3f3f3f ^ *(uint *)(iVar43 + 0x720);
      } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
      *(uint *)(iVar39 + 0x524) = *(uint *)(iVar39 + 0x524) & 0xffffffc0 | 9;
      *(uint *)(iVar39 + 0x528) = *(uint *)(iVar39 + 0x528) & 0xf7ffffff;
      *(uint *)(iVar39 + 0x52c) = *(uint *)(iVar39 + 0x52c) & 0xfff0ffb | 0xf004;
    }
  }
  *(int *)(iVar39 + 0x110) =
       param_1[0x43] << 0x10 | param_1[0x42] << 8 | param_1[0x47] | param_1[0x45] << 0x18;
  uVar8 = DAT_f2203524;
  uVar35 = param_1[0x49];
  if (0x1f < uVar35) {
    uVar35 = 0x20;
  }
  *(uint *)(iVar39 + 0xc4) =
       ((uVar35 << AL_I2C_INTR_MASK_STOP_DET_SHIFT | 0x100) ^ *(uint *)(iVar39 + 0xc4)) & 0x7f00 ^
       *(uint *)(iVar39 + 0xc4);
  uVar35 = DAT_f2203528;
  *(uint *)(iVar39 + 0x11c) =
       uVar8 & ((param_1[0x51] << 0x10 | 0x20000000U) ^ *(uint *)(iVar39 + 0x11c)) ^
       *(uint *)(iVar39 + 0x11c);
  *(uint *)(iVar39 + 0x120) =
       uVar35 & (*(uint *)(iVar39 + 0x120) ^ param_1[0x50] << 0x10) ^ *(uint *)(iVar39 + 0x120);
  *(int *)(iVar39 + 0x124) = param_1[0x44] << 0x10 | param_1[0x46] << 8 | param_1[0x48];
  uVar35 = *(uint *)(iVar39 + 0x140);
  uVar8 = uVar35;
  if (param_1[7] != 0) {
    uVar8 = uVar35 ^ 1;
  }
  *(uint *)(iVar39 + 0x140) = uVar8 & 1 ^ uVar35;
  if ((param_1[7] != 0) && ((char)param_1[10] == '\x01')) {
    if ((char)param_1[0x19] == -1) {
      *(uint *)(iVar39 + 0x144) =
           ~*(uint *)(iVar39 + 0x144) & 0x10000000 ^ *(uint *)(iVar39 + 0x144);
      *(uint *)(iVar39 + 0x154) = *(uint *)(iVar39 + 0x154) & 0xfffffff0 | 8;
    }
    if (param_1[8] != 0) {
      *(uint *)(iVar39 + 0x154) = *(uint *)(iVar39 + 0x154) & 0xff0fffff | 0x800000;
    }
  }
  *(uint *)(iVar39 + 0x14) = *(uint *)(iVar39 + 0x14) & 0xfffffe4f | 0x80;
  *(uint *)(iVar39 + 0x18) =
       (uVar36 / 1000 ^ *(uint *)(iVar39 + 0x18)) & 0x3ffff ^ *(uint *)(iVar39 + 0x18);
  *(undefined4 *)(iVar39 + 4) = 0x73;
  DataMemoryBarrier(0x1f);
  al_ddr_phy_wait_for_init_done(iVar39);
  *(uint *)(iVar39 + 0x680) = ~*(uint *)(iVar39 + 0x680) & 0x8000000 ^ *(uint *)(iVar39 + 0x680);
  *(uint *)(iVar39 + 0x680) = *(uint *)(iVar39 + 0x680) & 0xf7ffffff;
  *(undefined4 *)(iVar39 + 4) = DAT_f220352c;
  DataMemoryBarrier(0x1f);
  al_ddr_phy_wait_for_init_done(iVar39);
  if (param_1[0x39] == 0) {
    al_ddr_phy_vt_calc_disable(param_1);
    uVar14 = DAT_f2203530;
    iVar39 = param_1[2];
    iVar42 = 0;
    do {
      uVar35 = param_1[5];
      iVar43 = iVar39 + iVar42 * 0x100;
      for (uVar8 = 0; uVar35 != uVar8; uVar8 = uVar8 + 1) {
        *(uint *)(iVar39 + 0x4dc) = uVar8 | uVar8 << 0x10;
        *(undefined4 *)(iVar43 + 0x788) = uVar14;
        *(undefined **)(iVar43 + 0x780) = &DAT_00320032;
      }
      iVar42 = iVar42 + 1;
    } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
    s_ddr_train_step2(param_1);
  }
  uVar35 = DAT_f2203538;
  uVar8 = DAT_f2203534;
  local_164 = param_1 + 0x52;
  iVar42 = 0;
  iVar39 = param_1[2];
  piVar15 = local_164;
  do {
    if ((char)*piVar15 == '\0') {
      iVar43 = iVar39 + iVar42 * 0x100;
      uVar36 = *(uint *)(iVar43 + 0x700);
      *(uint *)(iVar43 + 0x700) = (uVar36 ^ uVar8) & uVar35 ^ uVar36;
    }
    iVar42 = iVar42 + 1;
    piVar15 = (int *)((int)piVar15 + 1);
  } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
  DataMemoryBarrier(0x1f);
  *(uint *)(iVar39 + 4) = ~*(uint *)(iVar39 + 4) & 0x20000000 ^ *(uint *)(iVar39 + 4);
  *(uint *)(iVar39 + 4) = *(uint *)(iVar39 + 4) & 0xdfffffff;
  *(uint *)(iVar39 + 0x1c) = ~*(uint *)(iVar39 + 0x1c) & 0xe000 ^ *(uint *)(iVar39 + 0x1c);
  *(uint *)(*param_1 + 0x38) = *(uint *)(*param_1 + 0x38) & 0xffffffd9;
  DataMemoryBarrier(0x1f);
  iVar42 = param_1[1];
  if ((param_1[7] != 0) && ((char)param_1[10] == '\x01')) {
    *(uint *)(iVar42 + 0x10) = ~*(uint *)(iVar42 + 0x10) & 8 ^ *(uint *)(iVar42 + 0x10);
  }
  *(uint *)(iVar42 + 0x1b0) = ~*(uint *)(iVar42 + 0x1b0) & 1 ^ *(uint *)(iVar42 + 0x1b0);
  DataMemoryBarrier(0x1f);
  if ((param_1[7] != 0) && ((char)param_1[10] == '\x01')) {
    s_udelay(AL_I2C_INTR_MASK_START_DET_SHIFT);
    if ((((char)param_1[0x19] == -1) && (iVar39 = al_ddr_mode_register_set(param_1,1,7,0x88), iVar39 != 0)) ||
       ((param_1[8] != 0 && (iVar39 = al_ddr_mode_register_set(param_1,1,7,0xd8), iVar39 != 0)))) {
      s_al_err_printf(DAT_f2203540,DAT_f220353c);
      return iVar39;
    }
    *(uint *)(iVar42 + 0x10) = *(uint *)(iVar42 + 0x10) & 0xfffffff7;
  }
  al_ddr_ctrl_wait_for_normal_operating_mode(param_1);
  if (param_1[0x39] == 0) goto LAB_f2203576;
  if ((param_1[6] == 2) && (param_1[5] == 2)) {
    uVar8 = 5;
  }
  else {
    uVar8 = (1 << (param_1[5] & 0xffU)) - 1;
  }
  s_ddr_train_step(local_98,0,0x24);
  s_ddr_train_step(local_70,0,0x24);
  iVar42 = param_1[1];
  iVar39 = param_1[2];
  *(uint *)(iVar42 + 0x1b0) = *(uint *)(iVar42 + 0x1b0) & 0xfffffffe;
  *(uint *)(iVar42 + 0x60) = ~*(uint *)(iVar42 + 0x60) & 1 ^ *(uint *)(iVar42 + 0x60);
  *(uint *)(iVar42 + 0x180) = ~*(uint *)(iVar42 + 0x180) & 0x80000000 ^ *(uint *)(iVar42 + 0x180);
  DataMemoryBarrier(0x1f);
  s_udelay(5);
  al_ddr_phy_vt_calc_disable(param_1);
  iVar42 = s_ddr_train_step3(param_1[2],0,0xe01);
  uVar14 = DAT_f2203a38;
  if (iVar42 == 0) {
LAB_f2203642:
    uVar36 = 0;
    *(uint *)(iVar39 + 0x10) = *(uint *)(iVar39 + 0x10) & 0xfbffffff;
    *(uint *)(iVar39 + 0x1c) = *(uint *)(iVar39 + 0x1c) & 0xffffffe7 | 8;
    *(uint *)(iVar39 + 0x10) = ~*(uint *)(iVar39 + 0x10) & 0x4000000 ^ *(uint *)(iVar39 + 0x10);
    uVar35 = *(uint *)(iVar39 + 0x204);
    do {
      if ((1 << (uVar36 & 0xff) & uVar8) >> (uVar36 & 0xff) != 0) {
        iVar42 = param_1[2];
        *(uint *)(iVar39 + 0x204) =
             (*(uint *)(iVar39 + 0x204) ^ uVar36 << 0xc) & 0x3000 ^ *(uint *)(iVar39 + 0x204);
        iVar42 = s_ddr_train_step3(iVar42,0,&DAT_00010001);
        if (iVar42 != 0) goto LAB_f22036ce;
        iVar42 = 0;
        piVar15 = local_164;
        do {
          if ((char)*piVar15 != '\0') {
            uVar37 = (*(uint *)(iVar39 + (iVar42 + 7) * 0x100) & 0xffffff) >> 0x14;
            uVar51 = *(uint *)(iVar39 + iVar42 * 0x100 + 0x71c);
            if (local_98[iVar42] < uVar37) {
              local_98[iVar42] = uVar37;
            }
            uVar37 = (uVar51 & 0x3fffffff) >> 0x1a;
            if (local_70[iVar42] < uVar37) {
              local_70[iVar42] = uVar37;
            }
          }
          iVar42 = iVar42 + 1;
          piVar15 = (int *)((int)piVar15 + 1);
        } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
      }
      uVar36 = uVar36 + 1;
    } while (uVar36 != 4);
    iVar42 = 0;
    piVar15 = local_164;
    do {
      if ((char)*piVar15 != '\0') {
        iVar43 = iVar39 + iVar42 * 0x100;
        *(uint *)(iVar43 + 0x700) =
             (*(uint *)(iVar43 + 0x700) ^ local_98[iVar42] << 0x14) & 0xf00000 ^
             *(uint *)(iVar43 + 0x700);
        *(uint *)(iVar43 + 0x71c) =
             (*(uint *)(iVar43 + 0x71c) ^ local_70[iVar42] << 0x1a) & 0x3c000000 ^
             *(uint *)(iVar43 + 0x71c);
      }
      iVar42 = iVar42 + 1;
      piVar15 = (int *)((int)piVar15 + 1);
    } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
    iVar42 = 0;
LAB_f22036ce:
    *(uint *)(iVar39 + 0x204) =
         (uVar35 ^ *(uint *)(iVar39 + 0x204)) & 0x3000 ^ *(uint *)(iVar39 + 0x204);
    if ((iVar42 != 0) ||
       (iVar43 = s_ddr_train_step3(param_1[2],0,&DAT_0000f001), uVar14 = DAT_f2203a24, iVar42 = iVar43,
       iVar43 != 0)) goto LAB_f220384e;
    if ((char)param_1[10] == '\x01') {
      iVar42 = param_1[2];
      *(uint *)(iVar39 + 0x400) = ~*(uint *)(iVar39 + 0x400) & 0x60000 ^ *(uint *)(iVar39 + 0x400);
      *(undefined **)(iVar39 + 0x42c) = &DAT_a5a5a5a5;
      *(uint *)(iVar39 + 0x41c) = *(uint *)(iVar39 + 0x41c) & 0xffff000f | 0x80;
      uVar8 = *(uint *)(iVar39 + 0x200);
      iVar42 = s_ddr_train_step3(iVar42,0xf0000000,uVar14);
      *(uint *)(iVar39 + 0x200) =
           (uVar8 ^ *(uint *)(iVar39 + 0x200)) & 0xf0000000 ^ *(uint *)(iVar39 + 0x200);
      if (iVar42 != 0) goto LAB_f220384e;
    }
  }
  else {
    if ((*(uint *)(iVar39 + 0x30) & 0xc00000) != 0) {
      if ((param_1[6] == 2) && (param_1[5] == 2)) {
        uVar35 = 5;
      }
      else {
        uVar35 = (1 << (param_1[5] & 0xffU)) - 1;
      }
      iVar42 = 0;
      iVar43 = param_1[2];
      pcVar34 = acStack_a4;
      piVar15 = local_164;
      do {
        iVar18 = *piVar15;
        *pcVar34 = '\0';
        if ((char)iVar18 != '\0') {
          iVar18 = iVar42 * 0x100;
          if (*(int *)(iVar43 + iVar18 + 2000) != 0) {
            *pcVar34 = '\x01';
          }
          iVar5 = iVar43 + iVar18;
          if (*(int *)(iVar5 + 0x7dc) == 0) {
            if (*pcVar34 == '\0') {
              *(uint *)(iVar5 + 0x700) = *(uint *)(iVar5 + 0x700) & 0xfffffffe;
              goto LAB_f22037d6;
            }
          }
          else {
            *pcVar34 = '\x01';
          }
          iVar18 = iVar18 + iVar43;
          *(undefined **)(iVar18 + 0x750) = &DAT_20202020;
          *(undefined **)(iVar18 + 0x754) = &DAT_20202020;
          *(undefined4 *)(iVar18 + 0x758) = uVar14;
        }
LAB_f22037d6:
        iVar42 = iVar42 + 1;
        pcVar34 = pcVar34 + 1;
        piVar15 = (int *)((int)piVar15 + 1);
      } while (iVar42 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
      *(uint *)(iVar43 + 0x10) = *(uint *)(iVar43 + 0x10) & 0xfbffffff;
      s_udelay(1);
      iVar42 = param_1[2];
      *(uint *)(iVar43 + 0x10) = ~*(uint *)(iVar43 + 0x10) & 0x4000000 ^ *(uint *)(iVar43 + 0x10);
      iVar42 = s_ddr_train_step3(iVar42,0,0xc01);
      local_160 = 0;
      local_158 = local_164;
      do {
        if ((char)*local_158 != '\0') {
          iVar18 = local_160 * 0x100 + iVar43;
          if (acStack_a4[local_160] == '\0') {
            *(uint *)(iVar18 + 0x700) = ~*(uint *)(iVar18 + 0x700) & 1 ^ *(uint *)(iVar18 + 0x700);
          }
          else {
            uVar36 = 0;
            *(undefined4 *)(iVar18 + 0x750) = 0;
            *(undefined4 *)(iVar18 + 0x754) = 0;
            *(undefined4 *)(iVar18 + 0x758) = 0;
            do {
              if ((1 << (uVar36 & 0xff) & uVar35) >> (uVar36 & 0xff) != 0) {
                *(uint *)(iVar43 + 0x4dc) = uVar36 | uVar36 << 0x10;
                uVar37 = *(uint *)(iVar18 + 0x788) & 0xff;
                uVar51 = (*(uint *)(iVar18 + 0x788) & 0xffffff) >> 0x10;
                uVar19 = *(uint *)(iVar18 + 0x7c0) & 0x1f;
                local_15c = (*(uint *)(iVar18 + 0x7c0) & 0x1fff) >> 8;
                local_150 = uVar37 - 0x20;
                uVar45 = (*(uint *)(iVar18 + 0x7e0) & 0x3ffffff) >> 0x11;
                if (uVar37 < 0x20) {
                  local_150 = uVar45 + (local_150 & 0xff);
                  if (uVar19 == 0) {
                    iVar42 = -5;
                    s_al_err_printf(DAT_f2203a34,local_160);
                  }
                  uVar19 = uVar19 - 1 & 0xff;
                }
                local_150 = local_150 & 0xff;
                uVar37 = uVar51 - 0x20 & 0xff;
                if (uVar51 < 0x20) {
                  uVar37 = uVar37 + uVar45 & 0xff;
                  if (local_15c == 0) {
                    iVar42 = -5;
                    s_al_err_printf(DAT_f2203a30,local_160);
                  }
                  local_15c = local_15c - 1 & 0xff;
                }
                *(uint *)(iVar18 + 0x788) = *(uint *)(iVar18 + 0x788) & 0xfffffe00 | local_150;
                *(uint *)(iVar18 + 0x7c0) =
                     (uVar19 ^ *(uint *)(iVar18 + 0x7c0)) & 0x1f ^ *(uint *)(iVar18 + 0x7c0);
                *(uint *)(iVar18 + 0x788) =
                     (*(uint *)(iVar18 + 0x788) ^ uVar37 << 0x10) & 0x1ff0000 ^
                     *(uint *)(iVar18 + 0x788);
                *(uint *)(iVar18 + 0x7c0) =
                     (*(uint *)(iVar18 + 0x7c0) ^ local_15c << 8) & 0x1f00 ^
                     *(uint *)(iVar18 + 0x7c0);
              }
              uVar36 = uVar36 + 1;
            } while (uVar36 != 4);
          }
        }
        local_160 = local_160 + 1;
        local_158 = (int *)((int)local_158 + 1);
      } while (local_160 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
      if (iVar42 == 0) goto LAB_f2203642;
    }
LAB_f220384e:
    uVar14 = DAT_f2203a3c;
    iVar18 = 0;
    iVar39 = param_1[2];
    do {
      if ((char)*local_164 != '\0') {
        iVar43 = iVar39 + iVar18 * 0x100;
        iVar5 = *(int *)(iVar43 + 0x7e0);
        iVar46 = *(int *)(iVar43 + 0x7e8);
        uVar35 = *(uint *)(iVar43 + 0x7ec);
        uVar8 = *(uint *)(iVar43 + 2000);
        uVar37 = *(uint *)(iVar43 + 0x7d4);
        uVar36 = *(uint *)(iVar43 + 0x7dc);
        s_al_err_printf(DAT_f2203a28,iVar18);
        uVar16 = DAT_f2203a2c;
        if ((iVar5 << 0x19 < 0) ||
           ((((uVar17 = DAT_f2203b2c, (uVar8 & 0xffff) == 0 &&
              (uVar17 = DAT_f2203b34, (uVar37 & 0xffff) == 0)) &&
             (uVar17 = DAT_f2203b38, (uVar36 & 0xffff) == 0)) &&
            ((((uVar16 = DAT_f2203b3c, iVar46 << AL_I2C_INTR_MASK_GEN_CALL_SHIFT < 0 ||
               (uVar16 = DAT_f2203b40, iVar46 << 0x1f < 0)) ||
              ((uVar16 = DAT_f2203b44, iVar46 << 0x1d < 0 ||
               ((uVar16 = DAT_f2203b48, iVar46 << 0x1b < 0 ||
                (uVar16 = DAT_f2203b4c, iVar46 << 0x19 < 0)))))) ||
             ((uVar17 = DAT_f2203b50, (uVar35 & 0xfffff) >> 0x10 == 0 &&
              (uVar16 = uVar14, uVar17 = DAT_f2203b54, (uVar35 & 0xfff) >> 8 == 0)))))))) {
          s_al_err_printf(uVar16);
        }
        else {
          s_al_err_printf(uVar17);
        }
        s_al_err_printf(DAT_f2203b30);
      }
      iVar18 = iVar18 + 1;
      iVar43 = iVar42;
      local_164 = (int *)((int)local_164 + 1);
    } while (iVar18 != AL_I2C_INTR_MASK_STOP_DET_SHIFT);
  }
  s_ddr_train_step2(param_1);
  iVar42 = param_1[1];
  *(uint *)(iVar42 + 0x1b0) = ~*(uint *)(iVar42 + 0x1b0) & 1 ^ *(uint *)(iVar42 + 0x1b0);
  *(uint *)(iVar42 + 0x60) = *(uint *)(iVar42 + 0x60) & 0xfffffffe;
  *(uint *)(iVar42 + 0x180) = *(uint *)(iVar42 + 0x180) & 0x7fffffff;
  DataMemoryBarrier(0x1f);
  al_ddr_ctrl_wait_for_normal_operating_mode();
  if (iVar43 != 0) {
    return iVar43;
  }
LAB_f2203576:
  iVar42 = param_1[2];
  *(uint *)(iVar42 + 0x90) = *(uint *)(iVar42 + 0x90) & 0xff23ffda | 0xc40021;
  *(uint *)(iVar42 + 0x10) = *(uint *)(iVar42 + 0x10) & 0xfbffffff;
  *(uint *)(iVar42 + 0x1c) = *(uint *)(iVar42 + 0x1c) & 0xffffffe7;
  *(uint *)(iVar42 + 0x10) = ~*(uint *)(iVar42 + 0x10) & 0x4000000 ^ *(uint *)(iVar42 + 0x10);
  *(undefined4 *)(param_1[1] + 0x490) = 1;
  return 0;
}



/* @ 0xf2203b58  FUN_f2203b58 */

void FUN_f2203b58(int param_1,undefined4 *param_2)

{
  undefined4 uVar1;
  uint uVar2;
  undefined1 uVar3;
  undefined1 uVar4;
  undefined4 uVar5;
  undefined *puVar6;
  bool bVar7;
  uint uVar8;
  
  uVar2 = *(uint *)(param_1 + 0x110);
  uVar1 = DAT_f2203cd4;
  if ((uVar2 & 0x80000) != 0) {
    uVar1 = DAT_f2203cd0;
  }
  param_2[5] = uVar1;
  uVar5 = DAT_f2203cd8;
  switch(uVar2 & 0xf) {
  case 0:
    uVar5 = uVar1;
    break;
  case 1:
    uVar5 = DAT_f2203cdc;
    break;
  case 2:
    uVar5 = DAT_f2203ce0;
    break;
  case 3:
    uVar5 = DAT_f2203ce4;
    break;
  case 4:
    uVar5 = DAT_f2203ce8;
    break;
  case 5:
    uVar5 = DAT_f2203cec;
    break;
  case 6:
    uVar5 = DAT_f2203cf0;
    break;
  case 7:
    uVar5 = DAT_f2203cf4;
    break;
  case 8:
    uVar5 = DAT_f2203cf8;
    break;
  case 9:
    uVar5 = DAT_f2203cfc;
    break;
  case 10:
    uVar5 = DAT_f2203d00;
    break;
  case 0xb:
    uVar5 = DAT_f2203d04;
    break;
  case 0xc:
    uVar5 = DAT_f2203d08;
    break;
  case 0xd:
    uVar5 = DAT_f2203d0c;
    break;
  case 0xe:
    uVar5 = DAT_f2203d10;
  }
  *param_2 = uVar5;
  uVar5 = DAT_f2203d3c;
  switch((uVar2 & 0x7f) >> 4) {
  case 0:
    uVar5 = uVar1;
    break;
  case 1:
    uVar5 = DAT_f2203d24;
    break;
  case 2:
    uVar5 = DAT_f2203d28;
    break;
  case 3:
    uVar5 = DAT_f2203d2c;
    break;
  case 4:
    uVar5 = DAT_f2203d30;
    break;
  case 5:
    uVar5 = DAT_f2203d34;
    break;
  case 6:
    uVar5 = DAT_f2203d38;
  }
  uVar8 = (uVar2 & 0x1ff) >> 7;
  param_2[1] = uVar5;
  if (uVar8 == 0) {
    bVar7 = true;
    uVar5 = uVar1;
  }
  else {
    bVar7 = false;
    uVar5 = DAT_f2203d14;
    if (uVar8 != 1) {
      uVar5 = DAT_f2203ce4;
    }
  }
  param_2[2] = uVar5;
  uVar5 = uVar1;
  if ((((!bVar7) &&
       (uVar8 = (uVar2 & 0x7ff) >> 9, uVar1 = DAT_f2203d40, uVar5 = DAT_f2203d18, uVar8 != 2)) &&
      (uVar1 = DAT_f2203d44, uVar8 != 3)) && (uVar1 = DAT_f2203d18, uVar8 == 0)) {
    uVar1 = DAT_f2203d48;
  }
  param_2[3] = uVar1;
  uVar4 = 1;
  param_2[4] = uVar5;
  *(byte *)(param_2 + 6) = (byte)((uVar2 << 0x14) >> 0x1f);
  uVar8 = (uVar2 & 0x3fff) >> 0xc;
  param_2[0xb] = 1;
  if (uVar8 == 2) {
    uVar3 = 0;
  }
  else if ((uVar8 == 3) || (uVar8 == 1)) {
    uVar3 = 1;
    uVar4 = 0;
  }
  else {
    uVar3 = 1;
  }
  *(undefined1 *)((int)param_2 + 0x19) = uVar4;
  *(undefined1 *)((int)param_2 + 0x1a) = uVar3;
  *(byte *)((int)param_2 + 0x1b) = (byte)((uVar2 << 0x11) >> 0x1f);
  uVar8 = (uVar2 & 0x3ffff) >> 0xf;
  if (uVar8 == 7) {
    puVar6 = &DAT_0001c200;
    uVar4 = 3;
  }
  else {
    uVar4 = *(undefined1 *)(DAT_f2203d1c + uVar8);
    puVar6 = *(undefined **)(DAT_f2203d20 + uVar8 * 4);
  }
  *(undefined1 *)(param_2 + 7) = uVar4;
  param_2[8] = puVar6;
  *(byte *)(param_2 + 9) = (byte)((uVar2 << 0xd) >> 0x1f);
  uVar8 = (uVar2 & 0x3fffff) >> 0x14;
  uVar4 = 1;
  if (uVar8 != 1) {
    if (uVar8 == 3) {
      uVar4 = 2;
    }
    else {
      uVar4 = 0;
    }
  }
  *(undefined1 *)((int)param_2 + 0x25) = uVar4;
  *(byte *)((int)param_2 + 0x26) = (byte)((uVar2 << 9) >> 0x1f);
  if ((uVar2 & 0x800000) == 0) {
    uVar4 = 0x50;
  }
  else {
    uVar4 = 0x57;
  }
  *(undefined1 *)((int)param_2 + 0x27) = uVar4;
  *(undefined1 *)(param_2 + 10) = 0;
  return;
}



/* @ 0xf2203d4c  FUN_f2203d4c */

uint FUN_f2203d4c(int *param_1)

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
      iVar4 = ((uVar3 & 0xfffffff) >> 0x18) + 1;
      return ((uVar3 & 0x1fff) * uVar1 + uVar1) / (((uVar3 & 0x3fffff) >> 0x10) * iVar4 + iVar4);
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}



/* @ 0xf2203d7e  FUN_f2203d7e */

bool s_pll_reg_a(int param_1)

{
  return ((*(uint *)(param_1 + 0x1c) & 0xfff) >> 8) - 6 < 2;
}



/* @ 0xf2203d90  FUN_f2203d90 */

void s_pll_reg_b(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  
  param_1 = param_1 + (param_2 >> 1) * 4;
  uVar1 = *(uint *)(param_1 + 0x20);
  if ((param_2 & 1) == 0) {
    param_3 = uVar1 & 0xffff0000 | param_3;
  }
  else {
    param_3 = uVar1 & 0xffff | param_3 << 0x10;
  }
  *(uint *)(param_1 + 0x20) = param_3;
  return;
}



/* @ 0xf2203db0  FUN_f2203db0 */

undefined4 al_pll_init(undefined4 param_1,undefined4 param_2,int param_3,undefined4 *param_4)

{
  undefined4 uVar1;
  
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    param_4[4] = DAT_f2203e04;
    param_4[2] = DAT_f2203e08;
    uVar1 = 0x14;
  }
  else {
    if ((char)param_3 == '\0') {
      param_4[4] = &DAT_000061a8;
      param_4[2] = DAT_f2203e0c;
      param_4[3] = 0x13;
      return 0;
    }
    if (param_3 != 2) {
      s_al_err_printf(DAT_f2203e14,DAT_f2203e10);
      return 0xffffffea;
    }
    param_4[4] = DAT_f2203dfc;
    param_4[2] = DAT_f2203e00;
    uVar1 = 0x13;
  }
  param_4[3] = uVar1;
  return 0;
}



/* @ 0xf2203e18  FUN_f2203e18 */

undefined4 FUN_f2203e18(int param_1,undefined1 *param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  
  puVar3 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar1 = FUN_f2203d4c();
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



/* @ 0xf2203e50  FUN_f2203e50 */

undefined4 al_pll_freq_set(int *param_1,uint param_2,int param_3,undefined4 param_4)

{
  undefined4 uVar1;
  uint uVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  byte *pbVar7;
  
  puVar3 = (uint *)*param_1;
  pbVar7 = (byte *)param_1[2];
  for (iVar4 = 0; iVar4 < param_1[3]; iVar4 = iVar4 + 1) {
    if (*pbVar7 == param_2) goto LAB_f2203e80;
    pbVar7 = pbVar7 + 0x18;
  }
  if (param_1[3] == iVar4) {
    s_al_err_printf(DAT_f2203edc,DAT_f2203ee0,param_2,iVar4,param_4);
    uVar1 = 0xffffffea;
  }
  else {
LAB_f2203e80:
    puVar3[6] = puVar3[6] & 0x7fffffff;
    uVar5 = *(int *)(pbVar7 + 0x10) << 0x18 | *(int *)(pbVar7 + 0xc) << 0x10 | *(uint *)(pbVar7 + 8)
    ;
    uVar2 = *(uint *)(pbVar7 + 0x14);
    *puVar3 = uVar5;
    puVar3[1] = puVar3[1] & 0xfffff000 | uVar2;
    *puVar3 = uVar5 | 0x80000000;
    while( true ) {
      iVar4 = *(int *)(*param_1 + 0x1c);
      if ((iVar4 < 0) || (param_3 == 0)) break;
      param_3 = param_3 + -1;
      s_udelay(1);
    }
    iVar6 = *(int *)(*param_1 + 0x1c);
    if (iVar6 < 0) {
      uVar1 = 0;
    }
    else {
      s_al_err_printf(DAT_f2203ee4,DAT_f2203ee0,iVar4,iVar6,param_4);
      uVar1 = 0xffffffc4;
    }
  }
  return uVar1;
}



/* @ 0xf2203ee8  FUN_f2203ee8 */

undefined4 al_pll_channel_freq_get(int *param_1,uint param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  uint uVar4;
  
  iVar1 = *param_1;
  *param_3 = 0;
  iVar1 = s_pll_reg_a(iVar1);
  uVar3 = 0;
  if (iVar1 != 0) {
    uVar2 = FUN_f2203d4c(param_1);
    uVar4 = *(uint *)(*param_1 + ((param_2 >> 1) + 8) * 4);
    if ((param_2 & 1) == 0) {
      uVar4 = uVar4 & 0xffff;
    }
    else {
      uVar4 = uVar4 >> 0x10;
    }
    if ((uVar4 & 0x3ff) == 0) {
      uVar3 = 0;
      if ((short)uVar4 < 0) {
        *param_3 = uVar2;
      }
      else {
        *param_3 = 0;
      }
    }
    else if ((short)uVar4 < 0) {
      s_al_err_printf(DAT_f2203f4c,DAT_f2203f48);
      uVar3 = 0xfffffffb;
    }
    else {
      *param_3 = uVar2 / (uVar4 & 0x3ff);
      uVar3 = 0;
    }
  }
  return uVar3;
}



/* @ 0xf2203f50  FUN_f2203f50 */

undefined4
al_pll_channel_div_set(undefined4 *param_1,undefined4 param_2,uint param_3,int param_4,int param_5,int param_6
            ,int param_7)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 extraout_r1;
  
  if ((param_3 & 0xfffffc00) == 0) {
    param_3 = param_3 & 0xffff;
    if (param_4 != 0) {
      param_3 = param_3 | 0x800;
    }
    if (param_5 == 0) {
      param_3 = param_3 | 0x4000;
    }
    s_pll_reg_b(*param_1,param_2,param_3 | 0x1000);
    if (param_6 != 0) {
      s_pll_reg_b(*param_1,extraout_r1,param_3 | 0x3000);
      s_udelay(3);
      for (; (iVar2 = s_pll_reg_a(*param_1), iVar2 == 0 && (param_7 != 0)); param_7 = param_7 + -1)
      {
        s_udelay(1);
      }
      iVar2 = s_pll_reg_a(*param_1);
      if (iVar2 == 0) {
        s_al_err_printf(DAT_f2203fe0,DAT_f2203fd8);
        return 0xffffffc4;
      }
    }
    uVar1 = 0;
  }
  else {
    s_al_err_printf(DAT_f2203fdc,DAT_f2203fd8,param_3,0x3ff);
    uVar1 = 0xffffffea;
  }
  return uVar1;
}



/* @ 0xf2203fe4  FUN_f2203fe4 */

int FUN_f2203fe4(byte *param_1,int param_2)

{
  int iVar1;
  byte *pbVar2;
  
  pbVar2 = param_1 + param_2;
  iVar1 = 0;
  for (; param_1 != pbVar2; param_1 = param_1 + 1) {
    iVar1 = iVar1 + (uint)*param_1;
  }
  return iVar1;
}



/* @ 0xf2203ff8  FUN_f2203ff8 */

int _pre_boot_validate(code *param_1,int param_2,uint param_3,int param_4,undefined4 param_5,uint param_6,
                undefined4 *param_7)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  int local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  int local_2c;
  
  iVar2 = (*param_1)(param_2 + param_3,&local_70,0x48);
  if (iVar2 == 0) {
    iVar2 = FUN_f2203fe4(&local_70,0x44);
    iVar1 = local_48;
    if (iVar2 == local_2c) {
      iVar5 = 0;
      iVar4 = param_2;
      uVar6 = param_3;
      do {
        uVar3 = param_6;
        if (uVar6 <= param_6) {
          uVar3 = uVar6;
        }
        iVar2 = (*param_1)(iVar4,param_5);
        if (iVar2 != 0) goto LAB_f22040f4;
        iVar4 = iVar4 + uVar3;
        iVar2 = FUN_f2203fe4(param_5,uVar3);
        uVar6 = uVar6 - uVar3;
        iVar5 = iVar5 + iVar2;
      } while (uVar6 != 0);
      param_2 = param_2 + param_4;
      for (param_3 = iVar1 - param_3; param_3 != 0; param_3 = param_3 - uVar6) {
        uVar6 = param_6;
        if (param_3 <= param_6) {
          uVar6 = param_3;
        }
        iVar2 = (*param_1)(param_2,param_5,uVar6);
        if (iVar2 != 0) goto LAB_f22040f4;
        param_2 = param_2 + uVar6;
        iVar2 = FUN_f2203fe4(param_5,uVar6);
        iVar5 = iVar5 + iVar2;
      }
      iVar2 = (*param_1)(param_2,&local_74,4);
      if (iVar2 != 0) goto LAB_f22040f4;
      if (local_74 == iVar5) {
        s_spd4_timing_get(param_7,&local_70,0x48);
        *param_7 = local_70;
        param_7[1] = local_6c;
        param_7[2] = local_68;
        param_7[3] = local_64;
        param_7[4] = local_60;
        param_7[10] = local_48;
        param_7[0xb] = local_44;
        param_7[0xc] = local_40;
        param_7[0xd] = local_3c;
        param_7[0xe] = local_38;
        param_7[0xf] = local_34;
        return 0;
      }
      s_al_err_printf(DAT_f2204104,DAT_f2204100);
    }
    iVar2 = -5;
  }
  else {
LAB_f22040f4:
    s_al_err_printf(DAT_f2204108,DAT_f2204100);
  }
  return iVar2;
}



/* @ 0xf220410c  FUN_f220410c */

int al_flash_toc_validate(code *param_1,int param_2,int *param_3,uint *param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int local_58;
  int local_54 [2];
  uint local_4c;
  int local_44;
  undefined1 auStack_40 [36];
  
  iVar1 = (*param_1)(param_2,local_54,0x14);
  if (iVar1 == 0) {
    param_2 = param_2 + 0x14;
    if ((local_54[0] == DAT_f22041a4) && (iVar1 = FUN_f2203fe4(local_54,0x10), iVar1 == local_44)) {
      *param_4 = local_4c;
      *param_3 = local_4c * 0x20 + 0x18;
      iVar3 = 0;
      for (uVar2 = 0; uVar2 < *param_4; uVar2 = uVar2 + 1) {
        iVar1 = (*param_1)(param_2,auStack_40,0x20);
        if (iVar1 != 0) goto LAB_f2204194;
        iVar1 = FUN_f2203fe4(auStack_40,0x20);
        param_2 = param_2 + 0x20;
        iVar3 = iVar3 + iVar1;
      }
      iVar1 = (*param_1)(param_2,&local_58,4);
      if (iVar1 != 0) goto LAB_f2204194;
      if (local_58 == iVar3) {
        return 0;
      }
      s_al_err_printf(DAT_f22041ac,DAT_f22041a8);
    }
    iVar1 = -5;
  }
  else {
LAB_f2204194:
    s_al_err_printf(DAT_f22041b0,DAT_f22041a8);
  }
  return iVar1;
}



/* @ 0xf22041b4  FUN_f22041b4 */

undefined4 FUN_f22041b4(undefined4 param_1,int param_2,int param_3,int param_4,int *param_5)

{
  int iVar1;
  int iVar2;
  undefined4 uStack_20;
  int iStack_1c;
  
  iVar2 = 0;
  uStack_20 = param_1;
  iStack_1c = param_2;
  while( true ) {
    if (iVar2 == param_4) {
      return 0xffffffea;
    }
    iVar1 = al_flash_toc_validate(param_1,param_2,&uStack_20,&iStack_1c);
    if (iVar1 == 0) break;
    param_2 = param_2 + param_3;
    iVar2 = iVar2 + 1;
  }
  *param_5 = param_2;
  return 0;
}



/* @ 0xf22041ec  FUN_f22041ec */

int al_flash_toc_find_id_with_fallback(code *param_1,int param_2,int param_3,int param_4,uint param_5,uint *param_6,
                int *param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined1 auStack_7c [8];
  uint local_74;
  int local_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  undefined4 uStack_4c;
  int local_48;
  undefined4 uStack_44;
  undefined4 uStack_40;
  int iStack_3c;
  int local_38;
  int iStack_34;
  int iStack_30;
  undefined4 uStack_2c;
  
  iVar1 = (*param_1)(param_2,auStack_7c,0x14);
  iVar2 = iVar1;
  if (iVar1 == 0) {
    uVar4 = 0xffffffff;
    iVar3 = param_2 + param_5 * 0x20 + 0x14;
    for (; param_5 < local_74; param_5 = param_5 + 1) {
      iVar2 = (*param_1)(iVar3,&local_68);
      if (iVar2 != 0) goto LAB_f22042b0;
      iVar3 = iVar3 + 0x20;
      if (local_68 == param_3) goto LAB_f2204282;
      if (local_68 == param_4) {
        local_48 = local_68;
        uStack_44 = uStack_64;
        uStack_40 = uStack_60;
        iStack_3c = local_5c;
        local_38 = local_58;
        iStack_34 = local_54;
        iStack_30 = local_50;
        uStack_2c = uStack_4c;
        uVar4 = param_5;
      }
    }
    if (-1 < (int)uVar4) {
      local_68 = local_48;
      uStack_64 = uStack_44;
      uStack_60 = uStack_40;
      local_5c = iStack_3c;
      local_58 = local_38;
      local_54 = iStack_34;
      local_50 = iStack_30;
      uStack_4c = uStack_2c;
      param_5 = uVar4;
      if (uVar4 < local_74) {
LAB_f2204282:
        *param_6 = param_5;
        s_spd4_timing_get(param_7,&local_68);
        *param_7 = local_68;
        param_7[3] = local_5c;
        param_7[4] = local_58;
        param_7[5] = local_54;
        param_7[6] = local_50;
        return 0;
      }
    }
    *param_6 = 0xffffffff;
    iVar2 = iVar1;
  }
  else {
LAB_f22042b0:
    s_al_err_printf(DAT_f22042c0,DAT_f22042bc);
  }
  return iVar2;
}



/* @ 0xf22042c4  FUN_f22042c4 */

int al_flash_toc_stage2_active_instance_get_with_fallback(int param_1,int param_2,code *param_3,int param_4,uint *param_5,uint *param_6)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  undefined1 auStack_5c [8];
  int local_54;
  uint local_48 [4];
  uint local_38;
  
  uVar2 = *(uint *)(param_1 + param_2);
  if ((uVar2 & 1) == 0) {
    iVar1 = (*param_3)(param_4,auStack_5c,0x14);
    if (iVar1 == 0) {
      param_4 = param_4 + 0x14;
      for (iVar3 = 0; iVar3 != local_54; iVar3 = iVar3 + 1) {
        iVar1 = (*param_3)(param_4,local_48,0x20);
        if (iVar1 != 0) goto LAB_f2204354;
        param_4 = param_4 + 0x20;
        if ((((local_48[0] & 0xfffffff) == 1) ||
            ((local_48[0] & 0xfffffff) + -AL_I2C_TAR_10BIT_ADDR_SHIFT < 2)) && (local_38 == uVar2))
        {
          *param_5 = local_48[0] >> 0x1c;
          *param_6 = local_48[0] >> 0x1c;
          return 0;
        }
      }
      iVar1 = -0x16;
      s_al_err_printf(DAT_f2204364,DAT_f2204360,uVar2);
    }
    else {
LAB_f2204354:
      s_al_err_printf(DAT_f2204368,DAT_f2204360);
    }
  }
  else {
    iVar1 = 0;
    *param_5 = (uVar2 & 0xffff) >> 8;
    *param_6 = (uVar2 & 0xffffff) >> 0x10;
  }
  return iVar1;
}



/* @ 0xf220436c  FUN_f220436c */

void FUN_f220436c(void)

{
  al_flash_toc_stage2_active_instance_get_with_fallback();
  return;
}



/* @ 0xf220437e  FUN_f220437e */

void FUN_f220437e(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  FUN_f220436c(param_1,0,param_2,param_3,param_4,param_2,param_3);
  return;
}



/* @ 0xf2204394  FUN_f2204394 */

int al_flash_obj_header_read_and_validate(code *param_1,undefined4 param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  int local_48;
  int local_30;
  int local_2c;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_14;
  
  iVar1 = (*param_1)(param_2,&local_58,0x48);
  if (iVar1 == 0) {
    iVar2 = FUN_f2203fe4(&local_58,0x44);
    iVar1 = DAT_f2204424;
    if (iVar2 == local_14) {
      if (local_58 == DAT_f2204424) {
        s_spd4_timing_get(param_3,&local_58,0x48);
        *param_3 = local_58;
        param_3[1] = local_54;
        param_3[2] = local_50;
        param_3[3] = local_4c;
        param_3[4] = local_48;
        param_3[10] = local_30;
        param_3[0xb] = local_2c;
        param_3[0xc] = local_28;
        param_3[0xd] = local_24;
        param_3[0xe] = local_20;
        param_3[0xf] = local_1c;
        return 0;
      }
      s_al_err_printf(DAT_f2204428,DAT_f220441c);
      s_al_err_printf(DAT_f220442c,iVar1,local_58);
    }
    iVar1 = -5;
  }
  else {
    s_al_err_printf(DAT_f2204420,DAT_f220441c);
  }
  return iVar1;
}



/* @ 0xf2204430  FUN_f2204430 */

int al_flash_obj_data_load(code *param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  int local_64;
  undefined1 auStack_60 [40];
  int local_38;
  
  iVar1 = (*param_1)(param_2,auStack_60,0x48);
  if (iVar1 == 0) {
    iVar1 = (*param_1)(param_2 + 0x48,param_3,local_38);
    if ((iVar1 == 0) && (iVar1 = (*param_1)(param_2 + 0x48 + local_38,&local_64), iVar1 == 0)) {
      iVar1 = FUN_f2203fe4(param_3,local_38);
      if (local_64 == iVar1) {
        return 0;
      }
      s_al_err_printf(DAT_f2204498,DAT_f2204494);
      return -5;
    }
  }
  s_al_err_printf(DAT_f220449c,DAT_f2204494);
  return iVar1;
}



/* @ 0xf22044a0  FUN_f22044a0 */

void FUN_f22044a0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5)

{
  _pre_boot_validate(param_1,param_2,0x20000,&DAT_00021000,param_3,param_4,param_5,param_4);
  return;
}



/* @ 0xf22044b8  FUN_f22044b8 */

void shared_params_write_magic(void)

{
  *DAT_f22044c0 = DAT_f22044c4;
  return;
}



/* @ 0xf22044c8  FUN_f22044c8 */

undefined4 s_shared_params_write_size(void)

{
  return DAT_f22044cc;
}



/* @ 0xf22044d0  FUN_f22044d0 */

void i2c_eeprom_read(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  ushort local_2c [2];
  undefined1 auStack_28 [20];
  
  if (param_2 < 0x100) {
    uVar2 = 1;
    local_2c[0] = CONCAT11(local_2c[0]._1_1_,(char)param_2);
  }
  else {
    local_2c[0] = (ushort)((param_2 & 0xff) << 8) | (ushort)(param_2 >> 8) & 0xff;
    uVar2 = 2;
  }
  iVar1 = s_i2c_xfer_a(auStack_28,DAT_f2204514,DAT_f2204510);
  if (iVar1 == 0) {
    s_i2c_xfer_b(auStack_28,param_1,uVar2,local_2c,param_3,param_4);
  }
  return;
}



/* @ 0xf2204518  FUN_f2204518 */

void s_udelay(uint param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  
  for (; param_1 != 0; param_1 = param_1 - uVar5) {
    uVar5 = param_1;
    if (9999 < param_1) {
      uVar5 = 10000;
    }
    uVar3 = (uVar5 * (*(uint *)(DAT_f2204564 + 4) / 16000)) / 1000;
    uVar2 = *DAT_f2204568;
    while (0 < (int)uVar3) {
      uVar1 = *DAT_f2204568;
      if (uVar1 < uVar2) {
        iVar4 = 1 - uVar1;
      }
      else {
        iVar4 = -uVar1;
      }
      uVar3 = uVar3 + uVar2 + iVar4;
      uVar2 = uVar1;
    }
  }
  return;
}



/* @ 0xf2204574  FUN_f2204574 */

undefined4 al_i2c_xfer_finish(int *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar4 = *param_1;
  uVar2 = 0;
  while (iVar1 = (uint)*(ushort *)(iVar4 + 0x34) << 0x16, -1 < iVar1) {
    if ((uint)param_1[1] < uVar2) {
      uVar3 = 0xffffff60;
      s_al_err_printf(DAT_f22045f0,DAT_f22045ec);
      goto LAB_f2204588;
    }
    uVar2 = uVar2 + 1;
    (*(code *)param_1[2])(param_1[3],1,iVar1,(code *)param_1[2],param_4);
  }
  uVar3 = 0;
LAB_f2204588:
  iVar1 = *param_1;
  uVar2 = 0;
  iVar4 = *(int *)(iVar1 + 0x70);
  do {
    if (-1 < iVar4 << 0x1f) {
LAB_f2204592:
      *(ushort *)(*param_1 + 0x6c) = *(ushort *)(*param_1 + 0x6c) & 0xfffe;
      return uVar3;
    }
    if ((uint)param_1[1] < uVar2) {
      uVar3 = 0xffffff60;
      s_al_err_printf(DAT_f22045f4,DAT_f22045ec);
      goto LAB_f2204592;
    }
    uVar2 = uVar2 + 1;
    (*(code *)param_1[2])(param_1[3],1);
    iVar4 = *(int *)(iVar1 + 0x70);
  } while( true );
}



/* @ 0xf2204660  FUN_f2204660 */

/* WARNING: Removing unreachable block (ram,0xf2204620) */
/* WARNING: Removing unreachable block (ram,0xf2204630) */

undefined4 al_i2c_perform_write(int *param_1,ushort param_2,byte *param_3,int param_4)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  
  iVar3 = *param_1;
  *(ushort *)(iVar3 + 4) = param_2 | *(ushort *)(iVar3 + 4) & 0xfc00;
  *(ushort *)(iVar3 + 0x6c) = *(ushort *)(iVar3 + 0x6c) & 0xfffe | 1;
  if (param_4 == 0) {
    return 0;
  }
  uVar4 = 1;
  iVar3 = *param_1;
  do {
    uVar2 = 0;
    while( true ) {
      if (param_4 == 0) {
        return 0;
      }
      if (*(int *)(iVar3 + 0x70) << 0x1e < 0) break;
      if ((uint)param_1[1] < uVar2) {
        s_al_err_printf(DAT_f220465c,DAT_f2204658);
        return 0xffffff60;
      }
      uVar2 = uVar2 + 1;
      (*(code *)param_1[2])(param_1[3],1,(code *)param_1[2],param_1[1],uVar4);
    }
    bVar1 = *param_3;
    param_3 = param_3 + 1;
    *(ushort *)(iVar3 + 0x10) = (ushort)bVar1;
    param_4 = param_4 + -1;
  } while( true );
}



/* @ 0xf2204694  FUN_f2204694 */

undefined4 s_i2c_xfer_a(int *param_1,char *param_2,ushort *param_3)

{
  char cVar1;
  byte bVar2;
  ushort uVar3;
  ushort uVar4;
  
  uVar4 = param_3[0x36];
  cVar1 = param_2[0x18];
  *param_1 = (int)param_3;
  param_3[0x36] = uVar4 & 0xfffe;
  if (*param_2 == '\x01') {
    bVar2 = param_2[0x19];
    if (cVar1 == '\x01') {
      uVar4 = 0x1000;
    }
    else {
      uVar4 = 0;
    }
    param_3[2] = uVar4;
    if (cVar1 == '\x01') {
      uVar4 = bVar2 | 0x71;
    }
    else {
      uVar4 = bVar2 | 0x61;
    }
  }
  else {
    param_3[4] = *(ushort *)(param_2 + 0x10);
    if (cVar1 == '\x01') {
      uVar4 = AL_I2C_INTR_MASK_ACTIVITY_SHIFT;
    }
    else {
      uVar4 = 0;
    }
  }
  *param_3 = uVar4;
  if (*(int *)(param_2 + 4) == 0) {
    uVar4 = 0;
    uVar3 = 0;
  }
  else {
    uVar4 = *(ushort *)(param_2 + 8);
    uVar3 = *(ushort *)(param_2 + 0xc);
  }
  param_3[0x1c] = uVar4;
  param_3[0x1e] = uVar3;
  param_1[1] = *(int *)(param_2 + 0x14);
  param_1[2] = DAT_f22046f8;
  param_1[3] = 0;
  return 0;
}



/* @ 0xf22046fc  FUN_f22046fc */

int s_i2c_xfer_b(int *param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4,int param_5,
                undefined1 *param_6)

{
  int iVar1;
  int iVar2;
  undefined2 uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  
  iVar5 = *param_1;
  iVar1 = al_i2c_perform_write(param_1,param_2,param_4,param_3);
  iVar2 = param_5;
  iVar6 = param_5;
  if (iVar1 != 0) {
    return iVar1;
  }
  do {
    uVar4 = 0;
    while( true ) {
      if (iVar6 == 0) {
        iVar2 = al_i2c_xfer_finish(param_1);
        return iVar2;
      }
      if ((iVar2 != 0) && ((*(uint *)(iVar5 + 0x70) & 0x12) == 2)) {
        bVar7 = iVar2 == 1;
        iVar2 = iVar2 + -1;
        if (bVar7) {
          uVar3 = 0x300;
        }
        else {
          uVar3 = 0x100;
        }
        *(undefined2 *)(iVar5 + 0x10) = uVar3;
      }
      if ((int)(*(uint *)(iVar5 + 0x70) << 0x1c) < 0) break;
      if ((uint)(param_1[1] * param_5) < uVar4) {
        al_i2c_xfer_finish(param_1);
        return -0xa0;
      }
      uVar4 = uVar4 + 1;
      (*(code *)param_1[2])(param_1[3],1);
    }
    *param_6 = (char)*(undefined2 *)(iVar5 + 0x10);
    param_6 = param_6 + 1;
    iVar6 = iVar6 + -1;
  } while( true );
}



