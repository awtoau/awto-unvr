/* @ 0xfbff4200  FUN_fbff4200 */

void FUN_fbff4200(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  uint uVar2;
  
  uVar2 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar2 | 0x1000);
  InstructionSynchronizationBarrier(0xf);
  uVar1 = FUN_fbff4284();
  FUN_fbff4588(uVar1,param_2);
  return;
}



/* @ 0xfbff4278  FUN_fbff4278 */

uint FUN_fbff4278(void)

{
  uint uVar1;
  undefined4 in_cr0;
  
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  return uVar1 & 0xf;
}



/* @ 0xfbff4284  FUN_fbff4284 */

void FUN_fbff4284(void)

{
  uint uVar1;
  
  uVar1 = coproc_movefrom_Coprocessor_Access_Control();
  coproc_moveto_Coprocessor_Access_Control(uVar1 | 0xff00000);
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  return;
}



/* @ 0xfbff42f4  FUN_fbff42f4 */

void FUN_fbff42f4(int param_1)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr9;
  
  *DAT_fbff434c = 1;
  do {
  } while (*DAT_fbff4350 != 2);
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



/* @ 0xfbff437c  FUN_fbff437c */

void FUN_fbff437c(undefined4 param_1)

{
  undefined4 in_cr0;
  undefined4 in_cr14;
  
  coprocessor_moveto(0xf,0,0,param_1,in_cr14,in_cr0);
  return;
}



/* @ 0xfbff43b8  FUN_fbff43b8 */

void FUN_fbff43b8(undefined4 param_1,code *param_2)

{
  (*param_2)();
  return;
}



/* @ 0xfbff4588  FUN_fbff4588 */

void FUN_fbff4588(void)

{
  FUN_fbff45b0();
  FUN_fbff4880(DAT_fbff45ac);
  FUN_fbff48a0();
  FUN_fbff4e0c(0xf0000000);
  FUN_fbff4e26();
  return;
}



/* @ 0xfbff45b0  FUN_fbff45b0 */

void FUN_fbff45b0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined1 uStack_25;
  undefined1 auStack_24 [4];
  int local_20;
  undefined1 auStack_1c [20];
  
  iVar1 = DAT_fbff4610;
  FUN_fbff49f8(DAT_fbff4614,DAT_fbff4610);
  iVar2 = FUN_fbff4c18(DAT_fbff461c,DAT_fbff4620,*(int *)(iVar1 + 0x14) != DAT_fbff4618,auStack_1c);
  if (((iVar2 == 0) && (iVar2 = FUN_fbff4c78(auStack_1c,&uStack_25,auStack_24), iVar2 == 0)) &&
     (iVar2 = FUN_fbff4cb0(auStack_1c,0,&local_20), iVar2 == 0)) {
    *(int *)(iVar1 + 4) = local_20 * 1000;
  }
  uVar3 = *(uint *)(iVar1 + 4) >> 4;
  FUN_fbff437c(uVar3);
  *(uint *)(DAT_fbff4624 + 0x1004) = uVar3;
  return;
}



/* @ 0xfbff4628  FUN_fbff4628 */

uint FUN_fbff4628(uint param_1,int param_2,uint param_3,int param_4,char *param_5,uint param_6,
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



/* @ 0xfbff46d2  FUN_fbff46d2 */

uint FUN_fbff46d2(int param_1,uint param_2)

{
  uint uVar1;
  undefined1 *puVar2;
  int *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_fbff4ef4();
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



/* @ 0xfbff4712  FUN_fbff4712 */

undefined4 FUN_fbff4712(undefined1 param_1)

{
  undefined1 *puVar1;
  undefined4 *in_r12;
  
  if (in_r12[2] == 0) {
    FUN_fbff4ed8();
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



/* @ 0xfbff4740  FUN_fbff4740 */

undefined4 FUN_fbff4740(undefined4 param_1,uint param_2,byte *param_3,int *param_4)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  byte bVar5;
  byte *pbVar6;
  undefined1 auStack_30 [24];
  
LAB_fbff4750:
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
        goto LAB_fbff4872;
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
LAB_fbff4800:
      param_4 = param_4 + 1;
      uVar3 = FUN_fbff4628(iVar2,uVar3,10,0,auStack_30,bVar5,uVar4);
      FUN_fbff46d2(auStack_30,uVar3);
    }
    if (uVar1 < 0x65) {
      if (uVar1 == 0x58) goto LAB_fbff4814;
      if (uVar1 != 99) {
        if (uVar1 == 0) {
          return 0;
        }
        goto LAB_fbff4872;
      }
      FUN_fbff4712((char)*param_4);
LAB_fbff4868:
      param_4 = param_4 + 1;
      goto LAB_fbff4750;
    }
    if (uVar1 == 0x73) {
      for (iVar2 = 0; *(char *)(*param_4 + iVar2) != '\0'; iVar2 = iVar2 + 1) {
      }
      FUN_fbff46d2();
      goto LAB_fbff4868;
    }
    if (uVar1 < 0x74) {
      if (uVar1 != 0x70) goto LAB_fbff4872;
      bVar5 = 8;
LAB_fbff4814:
      iVar2 = *param_4;
      param_4 = param_4 + 1;
      uVar3 = FUN_fbff4628(iVar2,1,0x10,uVar1 == 0x58,auStack_30,bVar5,uVar4);
      FUN_fbff46d2(auStack_30,uVar3);
    }
    else {
      if (uVar1 == 0x75) {
        uVar3 = 1;
        iVar2 = *param_4;
        goto LAB_fbff4800;
      }
      if (uVar1 == 0x78) goto LAB_fbff4814;
LAB_fbff4872:
      FUN_fbff4712(uVar1);
      param_3 = pbVar6;
    }
  } while( true );
}



/* @ 0xfbff4880  FUN_fbff4880 */

void FUN_fbff4880(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_fbff4740(0,0xffffffff,param_1,&uStack_c,param_1,&uStack_c,param_3);
  return;
}



/* @ 0xfbff48a0  FUN_fbff48a0 */

void FUN_fbff48a0(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  
  FUN_fbff49bc();
  iVar2 = 0;
  do {
    iVar3 = iVar2 + 1;
    FUN_fbff4d54(*DAT_fbff48ec,iVar2,0,DAT_fbff48f0);
    puVar1 = DAT_fbff48f4;
    iVar2 = iVar3;
  } while (iVar3 != 4);
  DAT_fbff48f4[2] = 0;
  puVar1[3] = 0;
  puVar1[4] = 0;
  puVar1[5] = 0;
  puVar1[6] = 0;
  puVar1[7] = 0;
  puVar1[8] = 0;
  puVar1[9] = 0;
  puVar1[1] = 0;
  *puVar1 = DAT_fbff48f8;
  return;
}



/* @ 0xfbff48fc  FUN_fbff48fc */

void FUN_fbff48fc(uint param_1)

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
  
  uVar6 = DAT_fbff49b0;
  iVar7 = param_1 + 1;
  FUN_fbff49bc();
  iVar1 = DAT_fbff49b4;
  uVar6 = uVar6 & param_1;
  uVar4 = *(uint *)(DAT_fbff49b4 + iVar7 * 8);
  if ((int)uVar6 < 0) {
    uVar6 = (uVar6 - 1 | 0xfffffffc) + 1;
  }
  while( true ) {
    iVar2 = DAT_fbff49b4;
    UNRECOVERED_JUMPTABLE = *(code **)(iVar1 + iVar7 * 8 + 4);
    uVar3 = *(undefined4 *)(DAT_fbff49b8 + ((int)param_1 / 4) * 4);
    if ((UNRECOVERED_JUMPTABLE != (code *)0x0) && ((uVar4 & 4) == 0)) break;
    FUN_fbff4d2c(uVar3,uVar6,3);
    WaitForInterrupt();
  }
  uVar3 = FUN_fbff4d2c(uVar3,uVar6,0);
  uVar6 = *(uint *)(iVar2 + 4);
  uVar8 = CONCAT44(uVar6 << 0x1d,uVar3);
  if ((int)(uVar6 << 0x1d) < 0) {
    uVar6 = uVar6 & 0xfffffffb;
    *(uint *)(iVar2 + 4) = uVar6;
    FUN_fbff4e26();
    uVar8 = FUN_fbff42f4(1);
  }
  iVar2 = DAT_fbff49b4;
  uVar3 = (undefined4)((ulonglong)uVar8 >> 0x20);
  iVar5 = uVar6 << 0x1f;
  if (-1 < iVar5) {
    uVar4 = *(uint *)(iVar1 + iVar7 * 8);
    if ((uVar4 & 2) == 0) {
      uVar8 = FUN_fbff4e28();
      iVar5 = *(int *)(iVar2 + iVar7 * 8);
      *(uint *)(iVar2 + iVar7 * 8) = uVar4 | 2;
    }
    FUN_fbff4eb0((int)uVar8,(int)((ulonglong)uVar8 >> 0x20),iVar5);
    FUN_fbff5058();
    FUN_fbff4ff4();
    FUN_fbff4fa0();
    uVar3 = extraout_r1;
    if ((int)(uVar6 << 0x1e) < 0) {
      FUN_fbff4f8c();
      uVar3 = extraout_r1_00;
    }
  }
  coproc_moveto_Invalidate_Entire_Instruction(uVar3);
  InstructionSynchronizationBarrier(0xf);
  DataSynchronizationBarrier(0xf);
                    /* WARNING: Could not recover jumptable at 0xfbff4364. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0xfbff49bc  FUN_fbff49bc */

void FUN_fbff49bc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
  piVar1 = DAT_fbff49e8;
  if (*DAT_fbff49e8 == 0) {
    FUN_fbff4d14(DAT_fbff49ec,DAT_fbff49f0,param_3,DAT_fbff49e8,param_4);
    iVar3 = 0;
    do {
      iVar2 = DAT_fbff49f4 + iVar3;
      iVar3 = iVar3 + 8;
      FUN_fbff4d24(iVar2,DAT_fbff49ec,0);
    } while (iVar3 != 0x20);
    *piVar1 = 1;
  }
  return;
}



/* @ 0xfbff49f8  FUN_fbff49f8 */

void FUN_fbff49f8(int param_1,undefined4 *param_2)

{
  bool bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined1 uVar5;
  undefined1 uVar6;
  uint uVar7;
  
  uVar4 = *(uint *)(param_1 + 0x110);
  uVar2 = DAT_fbff4b60;
  if ((uVar4 & 0x80000) == 0) {
    uVar2 = DAT_fbff4b5c;
  }
  param_2[5] = uVar2;
  uVar3 = DAT_fbff4b9c;
  switch(uVar4 & 0xf) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_fbff4b64;
    break;
  case 2:
    uVar3 = DAT_fbff4b68;
    break;
  case 3:
    uVar3 = DAT_fbff4b6c;
    break;
  case 4:
    uVar3 = DAT_fbff4b70;
    break;
  case 5:
    uVar3 = DAT_fbff4b74;
    break;
  case 6:
    uVar3 = DAT_fbff4b78;
    break;
  case 7:
    uVar3 = DAT_fbff4b7c;
    break;
  case 8:
    uVar3 = DAT_fbff4b80;
    break;
  case 9:
    uVar3 = DAT_fbff4b84;
    break;
  case 10:
    uVar3 = DAT_fbff4b88;
    break;
  case 0xb:
    uVar3 = DAT_fbff4b8c;
    break;
  case 0xc:
    uVar3 = DAT_fbff4b90;
    break;
  case 0xd:
    uVar3 = DAT_fbff4b94;
    break;
  case 0xe:
    uVar3 = DAT_fbff4b98;
  }
  *param_2 = uVar3;
  uVar3 = DAT_fbff4ba0;
  switch((uVar4 & 0x7f) >> 4) {
  case 0:
    uVar3 = uVar2;
    break;
  case 1:
    uVar3 = DAT_fbff4ba4;
    break;
  case 2:
    uVar3 = DAT_fbff4ba8;
    break;
  case 3:
    uVar3 = DAT_fbff4bac;
    break;
  case 4:
    uVar3 = DAT_fbff4bb0;
    break;
  case 5:
    uVar3 = DAT_fbff4bb4;
    break;
  case 6:
    uVar3 = DAT_fbff4bb8;
  }
  uVar7 = (uVar4 & 0x1ff) >> 7;
  param_2[1] = uVar3;
  if (uVar7 == 0) {
    bVar1 = true;
    uVar3 = uVar2;
  }
  else {
    bVar1 = false;
    uVar3 = DAT_fbff4bbc;
    if (uVar7 != 1) {
      uVar3 = DAT_fbff4b6c;
    }
  }
  param_2[2] = uVar3;
  if ((bVar1) ||
     (((uVar7 = (uVar4 & 0x7ff) >> 9, uVar3 = DAT_fbff4bc4, uVar2 = DAT_fbff4bc0, uVar7 != 2 &&
       (uVar3 = DAT_fbff4bc8, uVar7 != 3)) && (uVar3 = DAT_fbff4bcc, uVar7 != 0)))) {
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
    uVar6 = *(undefined1 *)(DAT_fbff4bd0 + uVar7);
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



/* @ 0xfbff4bd4  FUN_fbff4bd4 */

uint FUN_fbff4bd4(int *param_1)

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



/* @ 0xfbff4c06  FUN_fbff4c06 */

bool FUN_fbff4c06(int param_1)

{
  return ((*(uint *)(param_1 + 0x1c) & 0xfff) >> 8) - 6 < 2;
}



/* @ 0xfbff4c18  FUN_fbff4c18 */

undefined4 FUN_fbff4c18(undefined4 param_1,undefined4 param_2,int param_3,undefined4 *param_4)

{
  undefined4 uVar1;
  
  *param_4 = param_1;
  param_4[1] = param_2;
  if (param_3 == 1) {
    param_4[4] = DAT_fbff4c64;
    param_4[2] = DAT_fbff4c68;
    uVar1 = 0x14;
  }
  else {
    if (param_3 == 0) {
      param_4[4] = &DAT_000061a8;
      uVar1 = DAT_fbff4c6c;
    }
    else {
      if (param_3 != 2) {
        FUN_fbff4880(DAT_fbff4c74,DAT_fbff4c70);
        return 0xffffffea;
      }
      param_4[4] = DAT_fbff4c5c;
      uVar1 = DAT_fbff4c60;
    }
    param_4[2] = uVar1;
    uVar1 = 0x13;
  }
  param_4[3] = uVar1;
  return 0;
}



/* @ 0xfbff4c78  FUN_fbff4c78 */

undefined4 FUN_fbff4c78(int param_1,undefined1 *param_2,int *param_3)

{
  int iVar1;
  int iVar2;
  undefined1 *puVar3;
  
  puVar3 = *(undefined1 **)(param_1 + 8);
  *param_2 = 0;
  iVar1 = FUN_fbff4bd4();
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



/* @ 0xfbff4cb0  FUN_fbff4cb0 */

uint FUN_fbff4cb0(int *param_1,uint param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  *param_3 = 0;
  iVar1 = FUN_fbff4c06(*param_1);
  if (iVar1 == 0) {
    return 0;
  }
  uVar2 = FUN_fbff4bd4(param_1);
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
      FUN_fbff4880(DAT_fbff4d0c,DAT_fbff4d10);
      return 0xfffffffb;
    }
    *param_3 = uVar2 / (uVar3 & 0x3ff);
  }
  return uVar4;
}



/* @ 0xfbff4d14  FUN_fbff4d14 */

void FUN_fbff4d14(uint *param_1,uint param_2)

{
  uint uVar1;
  
  uVar1 = *(uint *)(&DAT_00004400 + param_2);
  param_1[1] = param_2;
  *param_1 = (uVar1 & 0xffff) >> 8;
  return;
}



/* @ 0xfbff4d24  FUN_fbff4d24 */

void FUN_fbff4d24(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  *param_1 = param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0xfbff4d2c  FUN_fbff4d2c */

void FUN_fbff4d2c(int *param_1,int param_2,undefined4 param_3)

{
  uint uVar1;
  
  uVar1 = *(uint *)*param_1;
  if (uVar1 < 4) {
    *(undefined4 *)(&DAT_00002020 + ((uint *)*param_1)[1] + param_2 * 0x100) = param_3;
    return;
  }
  FUN_fbff4880(DAT_fbff4d4c,DAT_fbff4d50,uVar1);
  return;
}



/* @ 0xfbff4d54  FUN_fbff4d54 */

void FUN_fbff4d54(int *param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  if (*(uint *)*param_1 < 4) {
    iVar1 = ((uint *)*param_1)[1] + param_2 * 0x100;
    *(undefined4 *)(&DAT_00002028 + iVar1) = param_4;
    *(undefined4 *)(&DAT_0000202c + iVar1) = param_3;
    return;
  }
  FUN_fbff4880(DAT_fbff4d80,DAT_fbff4d84);
  return;
}



/* @ 0xfbff4d88  FUN_fbff4d88 */

void FUN_fbff4d88(int param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  puVar1 = DAT_fbff4dfc;
  if (param_1 != 0) {
    puVar1 = DAT_fbff4e00;
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
    *(undefined **)(iVar4 + iVar3 + -0xffffc00) = &DAT_80808080;
  }
  puVar1 = DAT_fbff4e08;
  if (param_1 == 0) {
    puVar1 = DAT_fbff4e04;
  }
  *puVar1 = *puVar1 | 1;
  return;
}



/* @ 0xfbff4e0c  FUN_fbff4e0c */

void FUN_fbff4e0c(int param_1)

{
  *(undefined4 *)(&DAT_00090008 + param_1) = 1;
  FUN_fbff4d88(0);
  FUN_fbff4d88(1);
  return;
}



/* @ 0xfbff4e26  FUN_fbff4e26 */

void FUN_fbff4e26(void)

{
  return;
}



/* @ 0xfbff4e28  FUN_fbff4e28 */

void FUN_fbff4e28(void)

{
  undefined4 *puVar1;
  uint *puVar2;
  int iVar3;
  
  puVar2 = DAT_fbff4ea4;
  puVar1 = DAT_fbff4ea0;
  *(undefined4 *)(DAT_fbff4e9c + 0x80) = 0xffffffff;
  *puVar1 = &DAT_80808080;
  puVar1[1] = &DAT_80808080;
  puVar1[2] = &DAT_80808080;
  puVar1[3] = &DAT_80808080;
  puVar1[4] = &DAT_80808080;
  puVar1[5] = &DAT_80808080;
  puVar1[6] = &DAT_80808080;
  puVar1[7] = &DAT_80808080;
  puVar1[-0x400ff] = 0xff;
  *puVar2 = *puVar2 & 0xfffffd82 | 0x79;
  iVar3 = DAT_fbff4ea8;
  *(undefined4 *)(DAT_fbff4ea8 + 0x80) = 0xffffffff;
  *(undefined **)(iVar3 + 0x400) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x404) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x408) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x40c) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x410) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x414) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x418) = &DAT_80808080;
  *(undefined **)(iVar3 + 0x41c) = &DAT_80808080;
  *DAT_fbff4eac = 0xff;
  *(uint *)(iVar3 + 0x1000) = *(uint *)(iVar3 + 0x1000) & 0xfffffd82 | 0x78;
  return;
}



/* @ 0xfbff4eb0  FUN_fbff4eb0 */

void FUN_fbff4eb0(void)

{
  int iVar1;
  
  iVar1 = FUN_fbff4278();
  *(undefined4 *)(iVar1 * 0x20000 + -0xfd7ffec) = 0;
  do {
  } while (*(int *)(iVar1 * 0x20000 + -0xfd7ffec) << 0x1d < 0);
  *DAT_fbff4ed4 = 0xff;
  return;
}



/* @ 0xfbff4ed8  FUN_fbff4ed8 */

uint FUN_fbff4ed8(uint param_1)

{
  FUN_fbff4f08(0,param_1 & 0xff);
  if (param_1 == 10) {
    FUN_fbff4f08(0,0xd);
  }
  return param_1;
}



/* @ 0xfbff4ef4  FUN_fbff4ef4 */

void FUN_fbff4ef4(int param_1)

{
  char *pcVar1;
  
  pcVar1 = (char *)(param_1 + -1);
  while( true ) {
    pcVar1 = pcVar1 + 1;
    if (*pcVar1 == '\0') break;
    FUN_fbff4ed8();
  }
  return;
}



/* @ 0xfbff4f08  FUN_fbff4f08 */

void FUN_fbff4f08(int param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  
  puVar1 = *(undefined4 **)(DAT_fbff4f1c + param_1 * 4);
  do {
  } while (-1 < (int)(puVar1[5] << 0x19));
  *puVar1 = param_2;
  return;
}



/* @ 0xfbff4f8c  FUN_fbff4f8c */

void FUN_fbff4f8c(void)

{
  software_hvc(0);
  return;
}



/* @ 0xfbff4fa0  FUN_fbff4fa0 */

void FUN_fbff4fa0(void)

{
  uint uVar1;
  
  coproc_movefrom_Control();
  uVar1 = coproc_movefrom_Secure_Configuration();
  coproc_moveto_Secure_Configuration(uVar1 | 1);
  InstructionSynchronizationBarrier(0xf);
                    /* WARNING: Could not recover jumptable at 0xfbff4ff0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_fbff50a4)(DAT_fbff50a4,uVar1 | 1);
  return;
}



/* @ 0xfbff4ff4  FUN_fbff4ff4 */

void FUN_fbff4ff4(void)

{
  uint uVar1;
  uint uVar2;
  undefined4 in_cr0;
  undefined4 in_cr12;
  
  coprocessor_moveto(0xf,0,1,0xfbff4f40,in_cr12,in_cr0);
  software_smc(0);
  uVar1 = coproc_movefrom_Identification_registers(5,in_cr0);
  coprocessor_moveto(0xf,0,1,DAT_fbff50ac,in_cr12,in_cr0);
  uVar2 = coproc_movefrom_Secure_Configuration();
  coproc_moveto_Secure_Configuration(DAT_fbff50b0 | uVar2 & 0xfffffd80);
  coprocessor_moveto(0xf,4,0,DAT_fbff50b4,in_cr12,in_cr0);
  coproc_moveto_Secure_Configuration(DAT_fbff50b0 & 0xfffffffe | uVar2 & 0xfffffd80);
                    /* WARNING: Could not recover jumptable at 0xfbff5050. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_fbff50b8)(uVar1 & 0xf,DAT_fbff50b8,DAT_fbff50b4);
  return;
}



/* @ 0xfbff5058  FUN_fbff5058 */

void FUN_fbff5058(undefined4 param_1)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 in_cr15;
  
  uVar1 = coproc_movefrom_Auxiliary_Control();
  coproc_moveto_Auxiliary_Control(uVar1 | 0x80000000);
  FUN_fbff4284(param_1,uVar1 | 0x80000000);
  uVar1 = coproc_movefrom_NonSecure_Access_Control();
  coproc_moveto_NonSecure_Access_Control(uVar1 | 0x60c00);
  uVar1 = coprocessor_movefromRt(0xf,1,in_cr15);
  uVar2 = coprocessor_movefromRt2(0xf,1,in_cr15);
  coprocessor_moveto2(0xf,1,uVar1 | 0x40,uVar2,in_cr15);
  return;
}



