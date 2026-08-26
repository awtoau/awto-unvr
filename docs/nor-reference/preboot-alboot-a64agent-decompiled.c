/* @ 0x1029248  FUN_01029248 */

long FUN_01029248(void)

{
  ulong uVar1;
  
  uVar1 = mpidr_el1;
  return ((uVar1 & 0xff00) >> 6) + (uVar1 & 0xff);
}



/* @ 0x1029268  FUN_01029268 */

void FUN_01029268(uint param_1,long param_2,long param_3)

{
  bool bVar1;
  uint uVar2;
  int iVar3;
  undefined8 uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  ulong uVar8;
  ulong uVar9;
  ulong uVar10;
  ulong uVar11;
  
  if (param_3 != 0) {
    uVar9 = sctlr_el1;
    sctlr_el1 = uVar9 & 0xfffffffffffffffb;
    InstructionSynchronizationBarrier();
    DataSynchronizationBarrier(3,3,0);
  }
  uVar9 = 0;
  do {
    csselr_el1 = uVar9;
    InstructionSynchronizationBarrier();
    uVar4 = ccsidr_el1;
    uVar5 = (uint)uVar4;
    uVar2 = (uVar5 & 7) + 4;
    uVar7 = uVar5 >> 3 & 0x3ff;
    uVar6 = (uint)LZCOUNT(uVar7);
    uVar11 = (ulong)(uint)(1 << (ulong)(uVar6 & 0x1f));
    uVar8 = (ulong)(uVar7 << (ulong)(uVar6 & 0x1f));
    do {
      iVar3 = 1 << (ulong)uVar2;
      uVar7 = (uVar5 >> 0xd & 0x7fff) << (ulong)uVar2;
      do {
        uVar10 = (ulong)((uint)uVar9 | (uint)uVar8 | uVar7);
        if (param_2 == 0) {
          DC_ISW(uVar10);
        }
        else {
          DC_CISW(uVar10);
        }
        bVar1 = iVar3 <= (int)uVar7;
        uVar7 = uVar7 - iVar3;
      } while (bVar1);
      bVar1 = (long)uVar11 <= (long)uVar8;
      uVar8 = uVar8 - uVar11;
    } while (bVar1);
    uVar2 = (uint)uVar9 + 2;
    uVar9 = (ulong)uVar2;
    DataSynchronizationBarrier(3,3,0);
  } while ((int)uVar2 <= (int)(param_1 & 0xfffffffe));
  return;
}



/* @ 0x1029398  FUN_01029398 */

undefined8 FUN_01029398(void)

{
  undefined8 uVar1;
  
  uVar1 = cntpct_el0;
  return uVar1;
}



/* @ 0x10293a0  FUN_010293a0 */

void FUN_010293a0(undefined8 param_1,undefined8 param_2,undefined8 *param_3)

{
  undefined8 local_20;
  undefined8 uStack_18;
  undefined8 local_10;
  undefined8 uStack_8;
  
  FUN_0102a684(0);
  FUN_0102a9c8(&DAT_0102e000,param_1);
  FUN_0102a6a0(0);
  local_20 = *param_3;
  uStack_18 = param_3[1];
  local_10 = param_3[2];
  uStack_8 = param_3[3];
  FUN_0102a994(0x102a818,0,param_2,&local_20);
  FUN_0102a684(0);
  FUN_0102a9c8(&DAT_0102e000,0x102e2b6);
  FUN_0102a6a0(0);
  return;
}



/* @ 0x1029434  FUN_01029434 */

void FUN_01029434(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                 undefined8 param_5,undefined8 param_6,undefined8 param_7,undefined8 param_8)

{
  undefined1 *local_100;
  undefined1 *puStack_f8;
  undefined1 *local_f0;
  undefined8 uStack_e8;
  undefined1 auStack_40 [8];
  undefined8 local_38;
  undefined8 local_30;
  undefined8 local_28;
  undefined8 local_20;
  undefined8 local_18;
  undefined8 local_10;
  undefined8 local_8;
  
  local_f0 = auStack_40;
  uStack_e8 = 0xffffff80ffffffc8;
  local_100 = (undefined1 *)register0x00000008;
  puStack_f8 = (undefined1 *)register0x00000008;
  local_38 = param_2;
  local_30 = param_3;
  local_28 = param_4;
  local_20 = param_5;
  local_18 = param_6;
  local_10 = param_7;
  local_8 = param_8;
  FUN_010293a0(&DAT_0102e003,param_1,&local_100);
  return;
}



/* @ 0x10294c8  FUN_010294c8 */


void FUN_010294c8(long param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar1 = FUN_01029248();
  uVar2 = FUN_01029248();
  FUN_01029434(&DAT_0102e00c,uVar2);
  if (param_1 != 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e025,param_1);
    FUN_0102a6a0(0);
  }
  FUN_0102a680(0x80ff);
  if (_DAT_0102ea48 == 0) {
    FUN_0102a308(0,1,0x100);
    _DAT_0102ea48 = 1;
  }
  FUN_0102a684(0);
  FUN_0102a9c8(&DAT_0102e045);
  FUN_0102a6a0(0);
  FUN_0102a684(0);
  FUN_0102a9c8(&LAB_0102e068);
  FUN_0102a6a0(0);
  do {
    do {
      iVar3 = FUN_0102a8e0(uVar1);
    } while (iVar3 == 0);
    FUN_0102a800(uVar1);
  } while( true );
}



/* @ 0x1029668  FUN_01029668 */

ulong FUN_01029668(ulong param_1)

{
  return param_1 >> 8 & 0xff;
}



/* @ 0x1029670  FUN_01029670 */

undefined1 FUN_01029670(undefined1 param_1)

{
  return param_1;
}



/* @ 0x1029678  FUN_01029678 */

void FUN_01029678(void)

{
  long lVar1;
  ulong uVar2;
  undefined8 uVar3;
  int iVar4;
  undefined4 uVar5;
  
  uVar2 = mpidr_el1;
  iVar4 = FUN_01029668(uVar2 & 0xffffffff);
  uVar5 = FUN_01029670(uVar2 & 0xffffffff);
  lVar1 = ((long)iVar4 + 1) * 0x10 + 0x102eb40;
  FUN_0102d158(lVar1,uVar5,3);
  FUN_0102d684();
  uVar3 = sctlr_el3;
  FUN_01029268(1,1);
  uVar2 = UnkSytemRegRead(3,1,0xf,2,1);
  UnkSytemRegWrite(3,1,0xf,2,1,uVar2 & 0xffffffffffffffbf);
  InstructionSynchronizationBarrier();
  DataSynchronizationBarrier(3,3,0);
  WaitForInterrupt();
  UnkSytemRegWrite(3,1,0xf,2,1,uVar2);
  sctlr_el3 = uVar3;
  FUN_0102d7d0(uVar2,uVar2 & 0xffffffffffffffbf);
  FUN_0102d158(lVar1,uVar5,0);
  return;
}



/* @ 0x1029940  FUN_01029940 */


void FUN_01029940(void)

{
  long lVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  ulong uVar7;
  long lVar8;
  long lVar9;
  ulong uVar10;
  long lVar11;
  code *pcVar12;
  undefined1 auStack_20 [16];
  undefined1 auStack_10 [16];
  
  uVar10 = sctlr_el3;
  sctlr_el3 = uVar10 | 0x1000;
  uVar10 = mpidr_el1;
  iVar4 = FUN_01029668(uVar10 & 0xffffffff);
  lVar8 = (long)iVar4;
  iVar5 = FUN_01029670(uVar10 & 0xffffffff);
  uVar10 = mpidr_el1;
  FUN_01029668(uVar10 & 0xffffffff);
  uVar6 = FUN_01029670(uVar10 & 0xffffffff);
  FUN_0102cea4(auStack_20,&DAT_f0070000);
  FUN_0102cf20(auStack_10,auStack_20,0);
  FUN_0102d158(auStack_10,uVar6,3);
  while ((iVar5 != 0 || iVar4 != 0 && ((_DAT_0102eb60 == 0 || (_DAT_0102eb64 == 0))))) {
    WaitForEvent();
  }
  lVar11 = (long)iVar5;
  if (_DAT_0102eb60 == 0) {
    FUN_0102cea4(0x102eb40,&DAT_f0070000);
    FUN_0102cf20(0x102eb50,0x102eb40,0);
    FUN_01029f2c(0x8ffff000,0x84000000,&LAB_010295c4);
    FUN_01029f2c(0x8fffffff,0x82000000,0x102950c);
    FUN_01029f2c(0x8fffffff,0x82000001,0x1029ba0);
    FUN_01029f2c(0x8fffffff,0x82000003,0x1029e20);
    FUN_01029f2c(0x8fffffff,0x82000004,0x1029d7c);
    uVar2 = DAT_fbff40f8;
    uVar3 = DAT_fbff40fc;
    uVar10 = (ulong)uVar2;
    pcVar12 = (code *)(ulong)uVar3;
    DataSynchronizationBarrier(3,3,0);
    _DAT_0102eb60 = 1;
  }
  else {
    pcVar12 = (code *)0x0;
    uVar10 = *(ulong *)((lVar11 + lVar8 * 4 + 1) * 0x40 + 0x102eb48);
  }
  if (uVar10 == 0) {
    FUN_0102d158((lVar8 + 1) * 0x10 + 0x102eb40,iVar5,3);
    WaitForInterrupt();
  }
  lVar1 = (lVar8 + 1) * 0x10 + 0x102eb40;
  FUN_0102d158(lVar1,iVar5,0);
  if (_DAT_0102eb64 == 0) {
    FUN_0102d6c0(0xf0000000);
    DataSynchronizationBarrier(3,3,0);
    _DAT_0102eb64 = 1;
    DataSynchronizationBarrier(3,3,0);
    InstructionSynchronizationBarrier();
    SendEvent();
  }
  lVar9 = (lVar11 + lVar8 * 4 + 1) * 0x40;
  if (*(int *)(lVar9 + 0x102eb40) == 0) {
    FUN_0102d6f4();
    *(undefined4 *)(lVar9 + 0x102eb40) = 1;
  }
  while ((iVar5 != 0 && (*(int *)((lVar8 + 8) * 4 + 0x102eb48) == 0))) {
    WaitForEvent();
  }
  lVar9 = (lVar8 + 8) * 4;
  if (*(int *)(lVar9 + 0x102eb48) == 0) {
    FUN_0102d04c(lVar1,0x10);
    FUN_0102d6f0();
    FUN_0102d810(1);
    *(undefined4 *)(lVar9 + 0x102eb48) = 1;
  }
  *(undefined4 *)((lVar11 + lVar8 * 4 + 1) * 0x40 + 0x102eb44) = 2;
  uVar7 = FUN_0102d040();
  cntfrq_el0 = uVar7 & 0xffffffff;
  FUN_0102d7d0(uVar7 & 0xffffffff);
  FUN_0102a0e4();
  FUN_0102a168();
  FUN_0102a12c();
  if (pcVar12 != (code *)0x0) {
    (*pcVar12)();
    DataMemoryBarrier(3,3);
    FUN_0102c270(&DAT_fd882000);
  }
  FUN_0102a07c(uVar10,*(undefined8 *)((lVar11 + lVar8 * 4 + 1) * 0x40 + 0x102eb50));
  return;
}



/* @ 0x1029c48  FUN_01029c48 */

void FUN_01029c48(long param_1,int param_2,uint param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  long lVar3;
  
  uVar1 = param_2 * param_3 & 0x1f;
  lVar3 = (ulong)(param_2 * param_3 >> 5) * 4;
  uVar2 = (1 << (ulong)(param_3 & 0x1f)) + -1 << (ulong)uVar1;
  *(uint *)(param_1 + lVar3) =
       *(uint *)(param_1 + lVar3) & (uVar2 ^ 0xffffffff) | uVar2 & param_4 << (ulong)uVar1;
  return;
}



/* @ 0x1029c84  FUN_01029c84 */

void FUN_01029c84(void)

{
  uint uVar1;
  ulong uVar2;
  long lVar3;
  
  uVar2 = UnkSytemRegRead(3,0,0xc,8,0);
  uVar1 = (uint)uVar2 & 0x3ff;
  if (uVar1 != 0x3ff) {
    lVar3 = 0;
    do {
      if (*(uint *)(lVar3 * 4 + 0x102ea60) == uVar1) {
        (**(code **)((long)(int)lVar3 * 8 + 0x102ea88))(uVar2 & 0xffffffff,0);
        break;
      }
      lVar3 = lVar3 + 1;
    } while (lVar3 != 10);
    UnkSytemRegWrite(3,0,0xc,8,1,uVar2 & 0xffffffff);
  }
  return;
}



/* @ 0x1029cf8  FUN_01029cf8 */

undefined8 FUN_01029cf8(undefined8 param_1,undefined8 param_2,uint param_3,ulong param_4)

{
  ulong uVar1;
  int iVar2;
  long lVar3;
  undefined4 uVar4;
  undefined1 *puVar5;
  
  if ((param_4 & 1) == 0) {
    lVar3 = 0;
    do {
      if (*(uint *)(lVar3 * 4 + 0x102ea60) == param_3) {
        *(undefined4 *)((long)(int)lVar3 * 4 + 0x102ea60) = 0;
        *(undefined8 *)((long)(int)lVar3 * 8 + 0x102ea88) = 0;
        return 0;
      }
      lVar3 = lVar3 + 1;
    } while (lVar3 != 10);
  }
  else {
    lVar3 = 0;
    do {
      if (*(uint *)(lVar3 * 4 + 0x102ea60) == param_3) {
        *(undefined8 *)((long)(int)lVar3 * 8 + 0x102ea88) = param_2;
        return 0;
      }
      lVar3 = lVar3 + 1;
    } while (lVar3 != 10);
    lVar3 = 0;
    do {
      if (*(int *)(lVar3 * 4 + 0x102ea60) == 0) {
        *(uint *)((long)(int)lVar3 * 4 + 0x102ea60) = param_3;
        *(undefined8 *)((long)(int)lVar3 * 8 + 0x102ea88) = param_2;
        break;
      }
      lVar3 = lVar3 + 1;
    } while (lVar3 != 10);
  }
  uVar1 = daif;
  daif = uVar1 & 0xffffffffffffffbf;
  puVar5 = &DAT_f0200000;
  if ((int)param_3 < 0x20) {
    iVar2 = FUN_01029248(uVar1 & 0xffffffffffffffbf);
    puVar5 = (undefined1 *)((long)((iVar2 + 0x14) * 0x20000) + 0xf0010000);
  }
  lVar3 = (ulong)(param_3 >> 5) * 4;
  iVar2 = 1 << (ulong)(param_3 & 0x1f);
  *(int *)(puVar5 + lVar3 + 0x180) = iVar2;
  FUN_01029c48(puVar5 + 0x80,param_3,1,0);
  FUN_01029c48(puVar5 + 0x400,param_3,8,0);
  if ((param_4 & 1) == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e284,1,0x102e218,0x102e290,0x53);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar4 = 3;
  if ((param_4 & 2) == 0) {
    uVar4 = 1;
  }
  *(undefined8 *)(puVar5 + (long)(int)(param_3 << 3) + 0x6000) = 0;
  FUN_01029c48(puVar5 + 0xc00,param_3,2,uVar4);
  *(int *)(puVar5 + lVar3 + 0x100) = iVar2;
  return 0;
}



/* @ 0x1029ee8  FUN_01029ee8 */

undefined8 FUN_01029ee8(long param_1)

{
  if (*(code **)(param_1 + 0xad8) == (code *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(0x102e2a8);
    FUN_0102a6a0(0);
    do {
                    /* WARNING: Do nothing block with infinite loop */
    } while( true );
  }
  (**(code **)(param_1 + 0xad8))(0);
  return 0;
}



/* @ 0x1029f2c  FUN_01029f2c */

undefined8 FUN_01029f2c(undefined4 param_1,undefined4 param_2,undefined8 param_3)

{
  ulong uVar1;
  
  uVar1 = 0;
  do {
    if (*(int *)(uVar1 * 0x10 + 0x102ec80) == 0) {
      uVar1 = -(uVar1 >> 0x1f & 1) & 0xfffffff000000000 | (uVar1 & 0xffffffff) << 4;
      *(undefined4 *)(uVar1 + 0x102ec80) = param_1;
      *(undefined4 *)(uVar1 + 0x102ec84) = param_2;
      *(undefined8 *)(uVar1 + 0x102ec88) = param_3;
      return 0;
    }
    uVar1 = uVar1 + 1;
  } while (uVar1 != 10);
  return 0xffffffff;
}



/* @ 0x102a07c  FUN_0102a07c */

void FUN_0102a07c(code *UNRECOVERED_JUMPTABLE,undefined8 param_2)

{
                    /* WARNING: Could not recover jumptable at 0x0102a090. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)(param_2,0,0,0);
  return;
}



/* @ 0x102a0e4  FUN_0102a0e4 */

long FUN_0102a0e4(void)

{
  ulong uVar1;
  long lVar2;
  
  lVar2 = currentel;
  if (lVar2 != 0xc && lVar2 != 0xd) {
    return lVar2;
  }
  scr_el3 = DAT_0102a180;
  actlr_el2 = DAT_0102a188;
  actlr_el3 = DAT_0102a188;
  vbar_el3 = DAT_0102a190;
  uVar1 = UnkSytemRegRead(3,1,0xf,2,1);
  UnkSytemRegWrite(3,1,0xf,2,1,uVar1 | 0x40);
  UnkSytemRegWrite(3,6,0xc,0xc,5,0xf);
  return 0xf;
}



/* @ 0x102a12c  FUN_0102a12c */

void FUN_0102a12c(void)

{
  spsr_el3 = 0x3c9;
  elr_el3 = 0x102a160;
  FUN_01029248(&LAB_0102a160);
  ExceptionReturn();
  return;
}



/* @ 0x102a168  FUN_0102a168 */

void FUN_0102a168(void)

{
  return;
}



/* @ 0x102a308  FUN_0102a308 */


undefined8 FUN_0102a308(int param_1,int param_2,uint param_3)

{
  undefined8 uVar1;
  
  uVar1 = 0xffffffea;
  if (param_1 < 0x11) {
    if (param_2 == 0) {
      UnkSytemRegWrite(3,0,0xc,0xb,5,(ulong)param_3 + (long)(param_1 << 0x18));
    }
    else {
      _DAT_f0009f00 = param_1 + param_3 * 0x10000;
    }
    uVar1 = 0;
  }
  return uVar1;
}



/* @ 0x102a348  FUN_0102a348 */


undefined4 FUN_0102a348(void)

{
  undefined4 uVar1;
  
  uVar1 = _DAT_0102eae0;
  if (_DAT_0102eb04 == 2) {
    uVar1 = _DAT_0102eae8;
  }
  return uVar1;
}



/* @ 0x102a370  FUN_0102a370 */


ulong FUN_0102a370(void)

{
  uint uVar1;
  ulong uVar2;
  
  if (_DAT_0102eb04 == 0) {
    uVar2 = FUN_01029398();
  }
  else if (_DAT_0102eb04 == 1) {
    uVar2 = (ulong)*(uint *)(_DAT_0102ea40 + 0x1008);
  }
  else {
    uVar1 = FUN_0102ce38(0x102eaf0);
    uVar2 = (ulong)~uVar1;
  }
  return uVar2;
}



/* @ 0x102a3bc  FUN_0102a3bc */


void FUN_0102a3bc(undefined4 param_1,undefined4 param_2,ulong param_3,undefined4 param_4)

{
  _DAT_0102eb04 = param_1;
  _DAT_0102eb00 = param_2;
  _DAT_0102eae0 = (uint)(param_3 >> 4) & 0xfffffff;
  _DAT_0102eae8 = param_4;
  _DAT_0102eae4 = 1;
  return;
}



/* @ 0x102a3f0  FUN_0102a3f0 */


void FUN_0102a3f0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  if (_DAT_0102eae4 != 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e2da,1,0x102e2c0,0x102e358,0xe3);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  FUN_0102a3bc(param_1,param_2,param_3,param_4);
  if (param_1 == 2) {
    iVar1 = FUN_0102cafc(0x102eaf0,&DAT_fd890000,0,0);
    if (iVar1 != 0) {
      FUN_0102a684(0);
      FUN_0102a9c8(&DAT_0102e0c1,0x102e2f2,iVar1,0x102e2c0,0x102e348,0x7e);
      FUN_0102a6a0(0);
      FUN_010294c8(0);
    }
    FUN_0102cdb0(0x102eaf0,0);
    FUN_0102cbcc(0x102eaf0,1,0,0);
    FUN_0102ccb8(0x102eaf0,0xffffffff);
    FUN_0102cd28(0x102eaf0,0);
    FUN_0102cdb0(0x102eaf0,1);
  }
  _DAT_0102eae4 = 1;
  return;
}



/* @ 0x102a54c  FUN_0102a54c */


undefined8 FUN_0102a54c(uint param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  
  if (_DAT_0102eae4 != 1) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e32e,1,0x102e2c0,0x102e2b8,0xfb);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = FUN_0102a348();
  for (; param_1 != 0; param_1 = param_1 - uVar5) {
    uVar5 = param_1;
    if (10000 < param_1) {
      uVar5 = 10000;
    }
    uVar4 = (uVar5 * (uVar1 / 1000)) / 1000;
    uVar2 = FUN_0102a370();
    while( true ) {
      if ((int)uVar4 < 1) break;
      uVar3 = FUN_0102a370();
      if (uVar3 < uVar2) {
        uVar4 = uVar2 + 1 + (uVar4 - uVar3);
        uVar2 = uVar3;
      }
      else {
        uVar4 = (uVar4 + uVar2) - uVar3;
        uVar2 = uVar3;
      }
    }
  }
  return 0;
}



/* @ 0x102a63c  FUN_0102a63c */

void FUN_0102a63c(int param_1)

{
  FUN_0102a76c(0,param_1);
  if (param_1 == 10) {
    FUN_0102a76c(0,0xd);
    return;
  }
  return;
}



/* @ 0x102a680  FUN_0102a680 */

void FUN_0102a680(void)

{
  return;
}



/* @ 0x102a684  FUN_0102a684 */

undefined8 FUN_0102a684(ulong param_1)

{
  do {
  } while (*(int *)(&DAT_f0070000 + ((param_1 & 0xffffffff) + 0x500) * 4) != 0);
  return 1;
}



/* @ 0x102a6a0  FUN_0102a6a0 */

undefined8 FUN_0102a6a0(ulong param_1)

{
  *(undefined4 *)(&DAT_f0070000 + ((param_1 & 0xffffffff) + 0x500) * 4) = 0;
  return 1;
}



/* @ 0x102a6b8  FUN_0102a6b8 */

undefined8 FUN_0102a6b8(int param_1,undefined4 param_2,undefined4 param_3)

{
  long lVar1;
  
  if (3 < param_1) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e3c0,1,0x102e3a0,0x102e408,0x70);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  FUN_0102c6d8((long)param_1 * 0x10 + 0x102edd8,*(undefined8 *)((long)param_1 * 8 + 0x102e3e8));
  lVar1 = (long)param_1 * 0x28;
  *(undefined4 *)(lVar1 + 0x102ed44) = param_2;
  *(undefined4 *)(lVar1 + 0x102ed48) = param_3;
  return 0;
}



/* @ 0x102a76c  FUN_0102a76c */

void FUN_0102a76c(uint param_1,undefined1 param_2)

{
  ulong uVar1;
  undefined1 local_1;
  
  local_1 = param_2;
  if (3 < (int)param_1) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e3c0,1,0x102e3a0,0x102e368,0x7e);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = -(ulong)(param_1 >> 0x1f) & 0xfffffff000000000 | (ulong)param_1 << 4;
  if (*(long *)(uVar1 + 0x102edd8) != 0) {
    FUN_0102c914(uVar1 + 0x102edd8,1,&local_1,0xffffffff);
  }
  return;
}



/* @ 0x102a800  FUN_0102a800 */

undefined1 FUN_0102a800(int param_1)

{
  undefined1 local_1;
  
  if (3 < param_1) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e3c0,1,0x102e3a0,0x102e378,0x8b);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (*(long *)((long)param_1 * 0x10 + 0x102edd8) == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e3cc,1,0x102e3a0,0x102e378,0x8c);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  FUN_0102c744((long)param_1 * 0x10 + 0x102edd8,1,&local_1,0xffffffff);
  return local_1;
}



/* @ 0x102a8e0  FUN_0102a8e0 */

undefined8 FUN_0102a8e0(uint param_1)

{
  undefined8 uVar1;
  ulong uVar2;
  
  if (3 < (int)param_1) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e3c0,1,0x102e3a0,0x102e388,0x96);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar2 = -(ulong)(param_1 >> 0x1f) & 0xfffffff000000000 | (ulong)param_1 << 4;
  if (*(long *)(uVar2 + 0x102edd8) != 0) {
    uVar1 = FUN_0102caec(uVar2 + 0x102edd8);
    return uVar1;
  }
  return 0;
}



/* @ 0x102a970  FUN_0102a970 */

undefined4 FUN_0102a970(undefined4 param_1)

{
  FUN_0102a63c();
  return param_1;
}



/* @ 0x102a994  FUN_0102a994 */

undefined8 FUN_0102a994(undefined8 param_1,long param_2)

{
  if (param_2 == 0) {
    FUN_0102afdc();
  }
  return 0;
}



/* @ 0x102a9c8  FUN_0102a9c8 */

void FUN_0102a9c8(undefined8 param_1,undefined8 param_2,undefined8 param_3,undefined8 param_4,
                 undefined8 param_5,undefined8 param_6,undefined8 param_7,undefined8 param_8)

{
  undefined1 *local_100;
  undefined1 *puStack_f8;
  undefined1 *local_f0;
  undefined8 uStack_e8;
  undefined1 auStack_40 [8];
  undefined8 local_38;
  undefined8 local_30;
  undefined8 local_28;
  undefined8 local_20;
  undefined8 local_18;
  undefined8 local_10;
  undefined8 local_8;
  
  local_f0 = auStack_40;
  uStack_e8 = 0xffffff80ffffffc8;
  local_100 = (undefined1 *)register0x00000008;
  puStack_f8 = (undefined1 *)register0x00000008;
  local_38 = param_2;
  local_30 = param_3;
  local_28 = param_4;
  local_20 = param_5;
  local_18 = param_6;
  local_10 = param_7;
  local_8 = param_8;
  FUN_0102afdc(0x102a818,0,param_1,&local_100,0);
  return;
}



/* @ 0x102aa64  FUN_0102aa64 */

void FUN_0102aa64(code *UNRECOVERED_JUMPTABLE,long *param_2,undefined1 param_3,int *param_4)

{
  if (param_2 != (long *)0x0) {
    if (param_4 == (int *)0x0) {
      *(undefined1 *)*param_2 = param_3;
      *param_2 = *param_2 + 1;
    }
    else if (*param_4 != 0) {
      if (*param_4 == 1) {
        *(undefined1 *)*param_2 = 0;
      }
      else {
        *(undefined1 *)*param_2 = param_3;
      }
      *param_2 = *param_2 + 1;
      *param_4 = *param_4 + -1;
    }
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x0102aac8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)();
  return;
}



/* @ 0x102aad0  FUN_0102aad0 */

undefined8 FUN_0102aad0(byte *param_1)

{
  byte bVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;
  int iVar10;
  uint uVar11;
  int iVar12;
  byte *pbVar13;
  undefined1 uVar14;
  int iVar15;
  undefined1 local_8 [8];
  
  FUN_0102c210(local_8,0,8);
  iVar10 = 1;
LAB_0102ab64:
  do {
    bVar1 = *param_1;
    if (bVar1 == 0) {
      return 0;
    }
    param_1 = param_1 + 1;
    bVar2 = false;
    bVar8 = false;
    bVar5 = false;
    bVar4 = false;
    bVar3 = false;
    bVar7 = false;
    bVar6 = false;
    iVar15 = iVar10;
  } while (bVar1 != 0x25);
LAB_0102ab84:
  do {
    iVar10 = iVar15;
    uVar11 = (uint)*param_1;
    param_1 = param_1 + 1;
LAB_0102ab8c:
    if (uVar11 == 99) {
      local_8[iVar10] = 4;
      goto LAB_0102af2c;
    }
    iVar15 = iVar10;
    if (99 < uVar11) {
      if (uVar11 != 0x6f) {
        if (uVar11 < 0x70) {
          if (uVar11 == 0x69) {
LAB_0102ae48:
            if (bVar2) {
              local_8[iVar10] = 0x16;
              goto LAB_0102ae9c;
            }
            if (!bVar6) {
              if (bVar7) {
                local_8[iVar10] = 0x14;
              }
              else if (bVar3) {
                local_8[iVar10] = 10;
              }
              else if (bVar4) {
                local_8[iVar10] = 7;
              }
              else if (bVar5) {
                local_8[iVar10] = 1;
              }
              else if (bVar8) {
                local_8[iVar10] = 0x19;
              }
              else {
                local_8[iVar10] = 4;
              }
              goto LAB_0102ae9c;
            }
            goto LAB_0102af58;
          }
          if (uVar11 < 0x6a) {
            if (uVar11 == 100) goto LAB_0102ae48;
            if (uVar11 != 0x68) goto LAB_0102afb4;
            if (*param_1 == 0x68) {
              param_1 = param_1 + 1;
              bVar8 = true;
            }
            else {
              bVar5 = true;
            }
            goto LAB_0102ab84;
          }
          if (uVar11 == 0x6c) {
            if (*param_1 == 0x6c) {
              param_1 = param_1 + 1;
              goto LAB_0102ae1c;
            }
            bVar4 = true;
            goto LAB_0102ab84;
          }
          if (uVar11 != 0x6e) {
            if (uVar11 != 0x6a) goto LAB_0102afb4;
            bVar2 = true;
            goto LAB_0102ab84;
          }
          if (bVar3) {
            uVar14 = 0xc;
LAB_0102af04:
            local_8[iVar10] = uVar14;
          }
          else {
            if (bVar4) {
              uVar14 = 9;
              goto LAB_0102af04;
            }
            if (bVar5) {
              uVar14 = 3;
              goto LAB_0102af04;
            }
            if (bVar6) {
              uVar14 = 0x12;
              goto LAB_0102af04;
            }
            if (bVar7) {
              uVar14 = 0x15;
              goto LAB_0102af04;
            }
            if (bVar2) {
              local_8[iVar10] = 0x18;
            }
            else {
              local_8[iVar10] = 6;
            }
          }
          goto LAB_0102ae9c;
        }
        if (uVar11 == 0x74) {
          bVar6 = true;
          goto LAB_0102ab84;
        }
        if (uVar11 < 0x75) {
          if (uVar11 != 0x71) {
            if (uVar11 < 0x71) {
              local_8[iVar10] = 0x10;
            }
            else {
              if (uVar11 != 0x73) goto LAB_0102afb4;
              local_8[iVar10] = 0xf;
            }
LAB_0102af2c:
            iVar10 = iVar10 + 1;
            goto LAB_0102ab64;
          }
LAB_0102ae1c:
          bVar3 = true;
          goto LAB_0102ab84;
        }
        if (uVar11 != 0x78) {
          if (uVar11 != 0x7a) {
            bVar9 = uVar11 == 0x75;
LAB_0102acc4:
            if (!bVar9) goto LAB_0102afb4;
            goto LAB_0102af38;
          }
          bVar7 = true;
          goto LAB_0102ab84;
        }
      }
LAB_0102af38:
      if (bVar2) {
        local_8[iVar10] = 0x17;
      }
      else if (bVar6) {
LAB_0102af58:
        local_8[iVar10] = 0x11;
      }
      else if (bVar7) {
        local_8[iVar10] = 0x13;
      }
      else if (bVar3) {
        local_8[iVar10] = 0xb;
      }
      else if (bVar4) {
        local_8[iVar10] = 8;
      }
      else if (bVar5) {
        local_8[iVar10] = 2;
      }
      else if (bVar8) {
        local_8[iVar10] = 0x1a;
      }
      else {
        local_8[iVar10] = 5;
      }
LAB_0102ae9c:
      iVar10 = iVar10 + 1;
      goto LAB_0102ab64;
    }
    if (uVar11 != 0x2e) {
      if (uVar11 < 0x2f) {
        if (uVar11 == 0x27) goto LAB_0102ab84;
        if (uVar11 < 0x28) {
          if ((uVar11 == 0x20) || (uVar11 == 0x23)) goto LAB_0102ab84;
        }
        else {
          if ((uVar11 == 0x2b) || (uVar11 == 0x2d)) goto LAB_0102ab84;
          if (uVar11 == 0x2a) {
            iVar12 = 0;
            pbVar13 = param_1;
            while( true ) {
              bVar1 = *pbVar13;
              uVar11 = bVar1 - 0x30;
              if (9 < uVar11) break;
              if ((0xccccccc < iVar12) || ((int)(-0x7fffffd1 - (uint)bVar1) < iVar12 * 10)) {
                return 0xffffffff;
              }
              iVar12 = iVar12 * 10 + uVar11;
              pbVar13 = pbVar13 + 1;
            }
            goto LAB_0102ad54;
          }
        }
LAB_0102afb4:
        if (uVar11 == 0) {
          return 0;
        }
        goto LAB_0102ab64;
      }
      if (uVar11 == 0x44) {
        bVar4 = true;
        goto LAB_0102ae48;
      }
      if (0x44 < uVar11) {
        if (uVar11 == 0x55) {
          bVar4 = true;
        }
        else if (uVar11 != 0x58) {
          bVar4 = true;
          bVar9 = uVar11 == 0x4f;
          goto LAB_0102acc4;
        }
        goto LAB_0102af38;
      }
      if (uVar11 == 0x30) goto LAB_0102ab84;
      if ((uVar11 < 0x30) || (0x39 < uVar11)) goto LAB_0102afb4;
      iVar15 = 0;
      pbVar13 = param_1;
      while( true ) {
        if ((int)(-0x7fffffd1 - uVar11) < iVar15 * 10) {
          return 0xffffffff;
        }
        iVar15 = iVar15 * 10 + (uVar11 - 0x30);
        param_1 = pbVar13 + 1;
        uVar11 = (uint)*pbVar13;
        if (9 < uVar11 - 0x30) break;
        pbVar13 = param_1;
        if (0xccccccc < iVar15) {
          return 0xffffffff;
        }
      }
      if (uVar11 == 0x24) goto LAB_0102ab84;
      goto LAB_0102ab8c;
    }
    uVar11 = (uint)*param_1;
    param_1 = param_1 + 1;
    if (uVar11 != 0x2a) {
      while (uVar11 - 0x30 < 10) {
        uVar11 = (uint)*param_1;
        param_1 = param_1 + 1;
      }
      goto LAB_0102ab8c;
    }
    iVar12 = 0;
    pbVar13 = param_1;
    while( true ) {
      bVar1 = *pbVar13;
      uVar11 = bVar1 - 0x30;
      if (9 < uVar11) break;
      if (0xccccccc < iVar12) {
        return 0xffffffff;
      }
      if ((int)(-0x7fffffd1 - (uint)bVar1) < iVar12 * 10) {
        return 0xffffffff;
      }
      iVar12 = iVar12 * 10 + uVar11;
      pbVar13 = pbVar13 + 1;
    }
LAB_0102ad54:
    if (bVar1 == 0x24) {
      param_1 = pbVar13 + 1;
      local_8[iVar12] = 4;
    }
    else {
      local_8[iVar10] = 4;
      iVar15 = iVar10 + 1;
    }
  } while( true );
}



/* @ 0x102afdc  FUN_0102afdc */

int FUN_0102afdc(undefined8 param_1,undefined8 param_2,byte *param_3,undefined8 *param_4,
                undefined8 param_5)

{
  undefined1 *puVar1;
  byte bVar2;
  bool bVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;
  bool bVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  undefined2 *puVar15;
  undefined1 *puVar16;
  long *plVar17;
  int *piVar18;
  uint *puVar19;
  char *pcVar20;
  ulong uVar21;
  uint uVar22;
  uint uVar23;
  uint uVar24;
  long lVar25;
  uint uVar26;
  byte *pbVar27;
  byte *pbVar28;
  uint *puVar29;
  uint *puVar30;
  byte *pbVar31;
  char *pcVar32;
  long lVar33;
  int iVar34;
  int iVar35;
  int iVar36;
  undefined1 *puVar37;
  uint uVar38;
  ulong uVar39;
  ulong uVar40;
  long local_130;
  ulong local_118;
  int local_f4;
  undefined8 local_f0;
  undefined8 uStack_e8;
  undefined8 local_e0;
  undefined8 uStack_d8;
  undefined1 auStack_cc [4];
  undefined8 local_c8;
  undefined8 uStack_c0;
  undefined8 local_b8;
  undefined8 uStack_b0;
  undefined1 local_a8 [64];
  char local_68 [98];
  char local_6 [6];
  
  puVar37 = (undefined1 *)0x0;
  uVar40 = 1;
  local_c8 = *param_4;
  uStack_c0 = param_4[1];
  local_b8 = param_4[2];
  uStack_b0 = param_4[3];
  puVar29 = (uint *)*param_4;
  lVar33 = param_4[1];
  iVar34 = *(int *)(param_4 + 3);
  FUN_0102c210(auStack_cc,0,4);
  local_f4 = 0;
  pbVar28 = param_3;
  pbVar27 = param_3;
LAB_0102b05c:
  pbVar31 = pbVar28;
  if (*pbVar31 != 0) goto code_r0x0102b064;
  if (pbVar31 == pbVar27) {
    return local_f4;
  }
  bVar3 = false;
  goto LAB_0102b08c;
code_r0x0102b064:
  pbVar28 = pbVar31 + 1;
  if (*pbVar31 == 0x25) {
    bVar3 = true;
    if (pbVar31 != pbVar27) {
LAB_0102b08c:
      lVar25 = (long)pbVar31 - (long)pbVar27;
      if ((lVar25 < 0) || (0x7fffffff - local_f4 < lVar25)) {
        return -1;
      }
      for (; pbVar27 != pbVar31; pbVar27 = pbVar27 + 1) {
        FUN_0102aa64(param_1,param_2,*pbVar27,param_5);
      }
      local_f4 = local_f4 + (int)lVar25;
      if (!bVar3) {
        return local_f4;
      }
    }
    pbVar27 = pbVar27 + 1;
    uVar23 = 0xffffffff;
    uVar12 = 0;
    uVar14 = 0;
    bVar4 = false;
    bVar10 = false;
    bVar7 = false;
    bVar6 = false;
    bVar9 = false;
    bVar8 = false;
    bVar3 = false;
    bVar5 = false;
    uVar38 = 0;
LAB_0102b0fc:
    do {
      uVar24 = uVar38;
      uVar26 = (uint)*pbVar27;
      pbVar27 = pbVar27 + 1;
      uVar22 = uVar12;
      uVar11 = uVar23;
LAB_0102b108:
      uVar23 = uVar11;
      uVar12 = uVar22;
      iVar36 = (int)uVar40;
      puVar19 = puVar29;
      pbVar28 = pbVar27;
      if (uVar26 == 99) {
        if (puVar37 == (undefined1 *)0x0) {
          iVar35 = iVar34;
          if (iVar34 < 0) {
            iVar35 = iVar34 + 8;
            puVar30 = (uint *)(lVar33 + iVar34);
            iVar34 = iVar35;
            if (0 < iVar35) goto LAB_0102b5e8;
          }
          else {
LAB_0102b5e8:
            puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
            puVar30 = puVar29;
            iVar34 = iVar35;
          }
          local_68[0] = (char)*puVar30;
        }
        else {
          local_68[0] = puVar37[-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3];
        }
        uVar40 = (ulong)(iVar36 + 1);
LAB_0102bdac:
        pcVar32 = local_68;
        local_118 = 1;
LAB_0102bdbc:
        uVar24 = 0;
        uVar26 = 0;
        uVar23 = 0;
        puVar29 = puVar19;
        uVar38 = uVar14;
        goto LAB_0102bdc0;
      }
      uVar38 = uVar24;
      if (99 < uVar26) {
        if (uVar26 == 0x6f) {
LAB_0102b840:
          iVar35 = iVar34;
          if (bVar4) {
LAB_0102b90c:
            if (puVar37 == (undefined1 *)0x0) {
              if (-1 < iVar34) {
LAB_0102b938:
                uVar40 = (long)puVar29 + 0xb;
                goto LAB_0102b93c;
              }
              iVar35 = iVar34 + 8;
              puVar30 = (uint *)(lVar33 + iVar34);
              iVar34 = iVar35;
              if (0 < iVar35) goto LAB_0102b938;
              goto LAB_0102b944;
            }
            uVar39 = (ulong)*(uint *)(puVar37 +
                                     (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
          }
          else {
            if ((((!bVar5) && (!bVar3)) && (!bVar8)) && (!bVar9)) {
              if (!bVar6) {
                if (!bVar7) goto LAB_0102b90c;
                if (puVar37 != (undefined1 *)0x0) {
                  uVar39 = (ulong)(byte)puVar37[-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3
                                               ];
                  goto LAB_0102b948;
                }
                if (iVar34 < 0) {
                  iVar35 = iVar34 + 8;
                  puVar30 = (uint *)(lVar33 + iVar34);
                  iVar34 = iVar35;
                  if (0 < iVar35) goto LAB_0102b8f8;
                }
                else {
LAB_0102b8f8:
                  puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                  puVar30 = puVar29;
                  iVar34 = iVar35;
                }
                uVar39 = (ulong)(byte)*puVar30;
                puVar29 = puVar19;
                goto LAB_0102b948;
              }
              if (puVar37 != (undefined1 *)0x0) {
                uVar39 = (ulong)*(ushort *)
                                 (puVar37 + (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
                goto LAB_0102b948;
              }
              if (iVar34 < 0) {
                iVar35 = iVar34 + 8;
                puVar30 = (uint *)(lVar33 + iVar34);
                iVar34 = iVar35;
                if (0 < iVar35) goto LAB_0102b8b8;
              }
              else {
LAB_0102b8b8:
                puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                puVar30 = puVar29;
                iVar34 = iVar35;
              }
              uVar39 = (ulong)(ushort)*puVar30;
              puVar29 = puVar19;
              goto LAB_0102b948;
            }
            if (puVar37 != (undefined1 *)0x0) {
              uVar39 = *(ulong *)(puVar37 + (long)iVar36 * 8);
              goto LAB_0102b948;
            }
            if (iVar34 < 0) {
              iVar35 = iVar34 + 8;
              puVar30 = (uint *)(lVar33 + iVar34);
              iVar34 = iVar35;
              if (iVar35 < 1) goto LAB_0102b944;
            }
            uVar40 = (long)puVar29 + 0xf;
LAB_0102b93c:
            puVar19 = (uint *)(uVar40 & 0xfffffffffffffff8);
            puVar30 = puVar29;
            iVar34 = iVar35;
LAB_0102b944:
            uVar39 = (ulong)*puVar30;
            puVar29 = puVar19;
          }
LAB_0102b948:
          uVar40 = (ulong)(iVar36 + 1);
          uVar26 = 0;
          iVar36 = 0;
LAB_0102bca4:
          uVar24 = 0;
        }
        else {
          if (0x6f < uVar26) {
            if (uVar26 == 0x74) {
              bVar8 = true;
            }
            else {
              if (uVar26 < 0x75) {
                if (uVar26 == 0x71) goto LAB_0102b59c;
                if (uVar26 < 0x71) {
                  if (puVar37 == (undefined1 *)0x0) {
                    if (iVar34 < 0) {
                      iVar35 = iVar34 + 8;
                      puVar19 = (uint *)(lVar33 + iVar34);
                      puVar30 = puVar29;
                      iVar34 = iVar35;
                      if (0 < iVar35) goto LAB_0102b984;
                    }
                    else {
LAB_0102b984:
                      puVar30 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                      puVar19 = puVar29;
                    }
                    uVar39 = (ulong)*puVar19;
                    puVar29 = puVar30;
                  }
                  else {
                    uVar39 = *(ulong *)(puVar37 + (long)iVar36 * 8);
                  }
                  uVar40 = (ulong)(iVar36 + 1);
                  uVar26 = 0x78;
                  local_130 = 0x102e418;
                  goto LAB_0102bca0;
                }
                if (uVar26 != 0x73) goto LAB_0102bda0;
                uVar40 = (ulong)(iVar36 + 1);
                if (puVar37 == (undefined1 *)0x0) {
                  iVar36 = iVar34;
                  if (iVar34 < 0) {
                    iVar36 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar36;
                    if (0 < iVar36) goto LAB_0102b9e0;
                  }
                  else {
LAB_0102b9e0:
                    puVar19 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                    iVar34 = iVar36;
                  }
                  pcVar32 = *(char **)puVar30;
                }
                else {
                  pcVar32 = *(char **)(puVar37 + (long)iVar36 * 8);
                }
                if (pcVar32 == (char *)0x0) {
                  pcVar32 = (char *)0x102e448;
                }
                if (uVar23 == 0xffffffff) {
                  local_118 = FUN_0102c1f4(pcVar32);
                  if (0x7fffffff < local_118) {
                    return -1;
                  }
                }
                else {
                  lVar25 = FUN_0102c22c(pcVar32,0,(long)(int)uVar23);
                  uVar38 = (int)lVar25 - (int)pcVar32;
                  if (lVar25 == 0) {
                    uVar38 = uVar23;
                  }
                  local_118 = (ulong)uVar38;
                }
                local_118 = local_118 & 0xffffffff;
                goto LAB_0102bdbc;
              }
              if (uVar26 == 0x78) {
                local_130 = 0x102e418;
LAB_0102bb88:
                iVar35 = iVar34;
                if (bVar4) {
LAB_0102bc58:
                  if (puVar37 == (undefined1 *)0x0) {
                    if (-1 < iVar34) {
LAB_0102bc80:
                      uVar40 = (long)puVar29 + 0xb;
                      goto LAB_0102bc84;
                    }
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102bc80;
                    goto LAB_0102bc8c;
                  }
                  uVar39 = (ulong)*(uint *)(puVar37 +
                                           (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
                }
                else {
                  if (((!bVar5) && (!bVar3)) && ((!bVar8 && (!bVar9)))) {
                    if (!bVar6) {
                      if (!bVar7) goto LAB_0102bc58;
                      if (puVar37 != (undefined1 *)0x0) {
                        uVar39 = (ulong)(byte)puVar37[-(uVar40 >> 0x1f) & 0xfffffff800000000 |
                                                      uVar40 << 3];
                        goto LAB_0102bc90;
                      }
                      if (iVar34 < 0) {
                        iVar35 = iVar34 + 8;
                        puVar30 = (uint *)(lVar33 + iVar34);
                        iVar34 = iVar35;
                        if (0 < iVar35) goto LAB_0102bc44;
                      }
                      else {
LAB_0102bc44:
                        puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                        puVar30 = puVar29;
                      }
                      uVar39 = (ulong)(byte)*puVar30;
                      puVar29 = puVar19;
                      goto LAB_0102bc90;
                    }
                    if (puVar37 != (undefined1 *)0x0) {
                      uVar39 = (ulong)*(ushort *)
                                       (puVar37 +
                                       (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
                      goto LAB_0102bc90;
                    }
                    if (iVar34 < 0) {
                      iVar35 = iVar34 + 8;
                      puVar30 = (uint *)(lVar33 + iVar34);
                      iVar34 = iVar35;
                      if (0 < iVar35) goto LAB_0102bc04;
                    }
                    else {
LAB_0102bc04:
                      puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                      puVar30 = puVar29;
                    }
                    uVar39 = (ulong)(ushort)*puVar30;
                    puVar29 = puVar19;
                    goto LAB_0102bc90;
                  }
                  if (puVar37 != (undefined1 *)0x0) {
                    uVar39 = *(ulong *)(puVar37 + (long)iVar36 * 8);
                    goto LAB_0102bc90;
                  }
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (iVar35 < 1) goto LAB_0102bc8c;
                  }
                  uVar40 = (long)puVar29 + 0xf;
LAB_0102bc84:
                  puVar19 = (uint *)(uVar40 & 0xfffffffffffffff8);
                  puVar30 = puVar29;
                  iVar34 = iVar35;
LAB_0102bc8c:
                  uVar39 = (ulong)*puVar30;
                  puVar29 = puVar19;
                }
LAB_0102bc90:
                uVar40 = (ulong)(iVar36 + 1);
                if ((!bVar10) || ((int)uVar39 == 0)) {
                  uVar26 = 0;
                }
LAB_0102bca0:
                iVar36 = 2;
                goto LAB_0102bca4;
              }
              if (uVar26 != 0x7a) {
                if (uVar26 == 0x75) goto LAB_0102ba50;
                goto LAB_0102bda0;
              }
              bVar9 = true;
            }
            goto LAB_0102b0fc;
          }
          if (uVar26 != 0x69) {
            if (0x69 < uVar26) {
              if (uVar26 == 0x6c) {
                if (*pbVar27 == 0x6c) {
                  pbVar27 = pbVar27 + 1;
LAB_0102b59c:
                  bVar5 = true;
                }
                else {
                  bVar3 = true;
                }
                goto LAB_0102b0fc;
              }
              if (uVar26 != 0x6e) {
                if (uVar26 != 0x6a) goto LAB_0102bda0;
                bVar4 = true;
                goto LAB_0102b0fc;
              }
              if ((bVar5) || (bVar3)) {
LAB_0102b7c4:
                if (puVar37 == (undefined1 *)0x0) {
                  iVar35 = iVar34;
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102b7e4;
                  }
                  else {
LAB_0102b7e4:
                    puVar19 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                    iVar34 = iVar35;
                  }
                  plVar17 = *(long **)puVar30;
                }
                else {
                  plVar17 = *(long **)(puVar37 + (long)iVar36 * 8);
                }
                *plVar17 = (long)local_f4;
              }
              else if (bVar6) {
                if (puVar37 == (undefined1 *)0x0) {
                  iVar35 = iVar34;
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102b760;
                  }
                  else {
LAB_0102b760:
                    puVar19 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                    iVar34 = iVar35;
                  }
                  puVar15 = *(undefined2 **)puVar30;
                }
                else {
                  puVar15 = *(undefined2 **)(puVar37 + (long)iVar36 * 8);
                }
                *puVar15 = (undefined2)local_f4;
              }
              else if (bVar7) {
                if (puVar37 == (undefined1 *)0x0) {
                  iVar35 = iVar34;
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102b7a0;
                  }
                  else {
LAB_0102b7a0:
                    puVar19 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                    iVar34 = iVar35;
                  }
                  puVar16 = *(undefined1 **)puVar30;
                }
                else {
                  puVar16 = *(undefined1 **)(puVar37 + (long)iVar36 * 8);
                }
                *puVar16 = (undefined1)local_f4;
              }
              else {
                if ((bVar8) || (bVar9)) goto LAB_0102b7c4;
                if (puVar37 == (undefined1 *)0x0) {
                  iVar35 = iVar34;
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102b820;
                  }
                  else {
LAB_0102b820:
                    puVar19 = (uint *)((long)puVar29 + 0xfU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                    iVar34 = iVar35;
                  }
                  piVar18 = *(int **)puVar30;
                }
                else {
                  piVar18 = *(int **)(puVar37 + (long)iVar36 * 8);
                }
                *piVar18 = local_f4;
              }
              uVar40 = (ulong)(iVar36 + 1);
              puVar29 = puVar19;
              break;
            }
            if (uVar26 != 100) {
              if (uVar26 != 0x68) goto LAB_0102bda0;
              if (*pbVar27 == 0x68) {
                pbVar27 = pbVar27 + 1;
                bVar7 = true;
              }
              else {
                bVar6 = true;
              }
              goto LAB_0102b0fc;
            }
          }
LAB_0102b60c:
          iVar35 = iVar34;
          if (bVar4) {
LAB_0102b6dc:
            if (puVar37 == (undefined1 *)0x0) {
              if (-1 < iVar34) {
LAB_0102b704:
                uVar40 = (long)puVar29 + 0xb;
                goto LAB_0102b708;
              }
              iVar35 = iVar34 + 8;
              puVar19 = (uint *)(lVar33 + iVar34);
              iVar34 = iVar35;
              if (0 < iVar35) goto LAB_0102b704;
              goto LAB_0102b710;
            }
            uVar39 = (ulong)*(uint *)(puVar37 +
                                     (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
          }
          else {
            if ((((!bVar5) && (!bVar3)) && (!bVar8)) && (!bVar9)) {
              if (!bVar6) {
                if (!bVar7) goto LAB_0102b6dc;
                if (puVar37 != (undefined1 *)0x0) {
                  uVar39 = (ulong)(char)puVar37[-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3
                                               ];
                  goto LAB_0102b714;
                }
                if (iVar34 < 0) {
                  iVar35 = iVar34 + 8;
                  puVar19 = (uint *)(lVar33 + iVar34);
                  puVar30 = puVar29;
                  iVar34 = iVar35;
                  if (0 < iVar35) goto LAB_0102b6c8;
                }
                else {
LAB_0102b6c8:
                  puVar30 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                  puVar19 = puVar29;
                  iVar34 = iVar35;
                }
                uVar39 = (ulong)(char)*puVar19;
                puVar29 = puVar30;
                goto LAB_0102b714;
              }
              if (puVar37 != (undefined1 *)0x0) {
                uVar39 = (ulong)*(short *)(puVar37 +
                                          (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
                goto LAB_0102b714;
              }
              if (iVar34 < 0) {
                iVar35 = iVar34 + 8;
                puVar19 = (uint *)(lVar33 + iVar34);
                puVar30 = puVar29;
                iVar34 = iVar35;
                if (0 < iVar35) goto LAB_0102b688;
              }
              else {
LAB_0102b688:
                puVar30 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                puVar19 = puVar29;
                iVar34 = iVar35;
              }
              uVar39 = (ulong)(short)*puVar19;
              puVar29 = puVar30;
              goto LAB_0102b714;
            }
            if (puVar37 != (undefined1 *)0x0) {
              uVar39 = *(ulong *)(puVar37 + (long)iVar36 * 8);
              goto LAB_0102b714;
            }
            if (iVar34 < 0) {
              iVar35 = iVar34 + 8;
              puVar19 = (uint *)(lVar33 + iVar34);
              iVar34 = iVar35;
              if (iVar35 < 1) goto LAB_0102b710;
            }
            uVar40 = (long)puVar29 + 0xf;
LAB_0102b708:
            puVar19 = puVar29;
            puVar29 = (uint *)(uVar40 & 0xfffffffffffffff8);
            iVar34 = iVar35;
LAB_0102b710:
            uVar39 = (ulong)*puVar19;
          }
LAB_0102b714:
          uVar40 = (ulong)(iVar36 + 1);
          if ((int)uVar39 < 0) {
            uVar39 = (ulong)(uint)-(int)uVar39;
            uVar24 = 0x2d;
          }
          uVar26 = 0;
          iVar36 = 1;
        }
        uVar38 = uVar14 & 0xffffff7f;
        if (uVar23 == 0xffffffff) {
          uVar38 = uVar14;
        }
        if ((uVar23 != 0) || (pcVar32 = local_6 + 2, (int)uVar39 != 0)) {
          if (iVar36 == 1) {
            pcVar20 = local_6 + 2;
            while( true ) {
              pcVar32 = pcVar20 + -1;
              if ((uint)uVar39 < 10) break;
              uVar14 = (uint)uVar39 / 10;
              *pcVar32 = (char)uVar39 + (char)uVar14 * -10 + '0';
              uVar39 = (ulong)uVar14;
              pcVar20 = pcVar32;
            }
            pcVar20[-1] = (char)uVar39 + '0';
          }
          else if (iVar36 == 2) {
            pcVar32 = local_6 + 2;
            do {
              uVar21 = uVar39 & 0xf;
              uVar39 = uVar39 >> 4 & 0xfffffff;
              pcVar32 = pcVar32 + -1;
              *pcVar32 = *(char *)(local_130 + uVar21);
            } while ((int)uVar39 != 0);
          }
          else {
            pcVar32 = local_6 + 2;
            do {
              pcVar20 = pcVar32;
              uVar22 = (uint)uVar39;
              uVar14 = uVar22 >> 3;
              uVar39 = (ulong)uVar14;
              iVar36 = (uVar22 & 7) + 0x30;
              pcVar32 = pcVar20 + -1;
              *pcVar32 = (char)iVar36;
            } while (uVar14 != 0);
            if ((bVar10) && (iVar36 != 0x30)) {
              pcVar32 = pcVar20 + -2;
              pcVar20[-2] = '0';
            }
          }
        }
        uVar14 = ((int)local_6 + 2) - (int)pcVar32;
        local_118 = (ulong)uVar14;
        if (100 < (int)uVar14) {
          return -1;
        }
LAB_0102bdc0:
        uVar14 = (uint)local_118;
        if ((int)(uint)local_118 < (int)uVar23) {
          uVar14 = uVar23;
        }
        if (uVar24 != 0) {
          uVar14 = uVar14 + 1;
        }
        uVar22 = uVar14 + 2;
        if (uVar26 == 0) {
          uVar22 = uVar14;
        }
        if ((uVar38 == 0) && (iVar35 = uVar12 - uVar22, iVar36 = iVar35, 0 < iVar35)) {
          for (; 0x10 < iVar36; iVar36 = iVar36 + -0x10) {
            lVar25 = 0;
            do {
              FUN_0102aa64(param_1,param_2,*(undefined1 *)(lVar25 + 0x102e450),param_5);
              lVar25 = lVar25 + 1;
            } while (lVar25 != 0x10);
          }
          puVar16 = &DAT_0102f451;
          do {
            FUN_0102aa64(param_1,param_2,puVar16[-1],param_5);
            puVar16 = puVar16 + 1;
          } while (puVar16 !=
                   (undefined1 *)((ulong)((iVar35 + (iVar35 - 1U >> 4) * -0x10) - 1) + 0x102f452));
        }
        if (uVar24 != 0) {
          FUN_0102aa64(param_1,param_2,uVar24,param_5);
        }
        if (uVar26 != 0) {
          FUN_0102aa64(param_1,param_2,0x30,param_5);
          FUN_0102aa64(param_1,param_2,uVar26,param_5);
        }
        if ((uVar38 == 0x80) && (iVar36 = uVar12 - uVar22, iVar35 = iVar36, 0 < iVar36)) {
          for (; 0x10 < iVar35; iVar35 = iVar35 + -0x10) {
            lVar25 = 0;
            do {
              FUN_0102aa64(param_1,param_2,(&DAT_0102e438)[lVar25],param_5);
              lVar25 = lVar25 + 1;
            } while (lVar25 != 0x10);
          }
          puVar16 = &DAT_0102e439;
          do {
            FUN_0102aa64(param_1,param_2,puVar16[-1],param_5);
            puVar16 = puVar16 + 1;
          } while (puVar16 !=
                   (undefined1 *)((ulong)((iVar36 + (iVar36 - 1U >> 4) * -0x10) - 1) + 0x102e43a));
        }
        iVar36 = uVar23 - (uint)local_118;
        iVar35 = iVar36;
        if (0 < iVar36) {
          for (; 0x10 < iVar35; iVar35 = iVar35 + -0x10) {
            lVar25 = 0;
            do {
              FUN_0102aa64(param_1,param_2,(&DAT_0102e438)[lVar25],param_5);
              lVar25 = lVar25 + 1;
            } while (lVar25 != 0x10);
          }
          puVar16 = &DAT_0102f439;
          do {
            FUN_0102aa64(param_1,param_2,puVar16[-1],param_5);
            puVar16 = puVar16 + 1;
          } while (puVar16 !=
                   (undefined1 *)((ulong)((iVar36 + (iVar36 - 1U >> 4) * -0x10) - 1) + 0x102f43a));
        }
        for (uVar39 = 0; uVar39 != local_118; uVar39 = uVar39 + 1) {
          FUN_0102aa64(param_1,param_2,pcVar32[uVar39],param_5);
        }
        if (((uVar38 >> 2 & 1) != 0) && (iVar35 = uVar12 - uVar22, iVar36 = iVar35, 0 < iVar35)) {
          for (; 0x10 < iVar36; iVar36 = iVar36 + -0x10) {
            lVar25 = 0;
            do {
              puVar16 = &DAT_0102f450 + lVar25;
              lVar25 = lVar25 + 1;
              FUN_0102aa64(param_1,param_2,*puVar16,param_5);
            } while (lVar25 != 0x10);
          }
          puVar16 = &DAT_0102f451;
          do {
            puVar1 = puVar16 + -1;
            puVar16 = puVar16 + 1;
            FUN_0102aa64(param_1,param_2,*puVar1,param_5);
          } while (puVar16 !=
                   (undefined1 *)((ulong)((iVar35 + (iVar35 - 1U >> 4) * -0x10) - 1) + 0x102f452));
        }
        if ((int)uVar22 < (int)uVar12) {
          uVar22 = uVar12;
        }
        if (0x7fffffff - local_f4 < (int)uVar22) {
          return -1;
        }
        local_f4 = local_f4 + uVar22;
        break;
      }
      if (uVar26 == 0x2e) {
        uVar26 = (uint)*pbVar27;
        pbVar27 = pbVar27 + 1;
        if (uVar26 != 0x2a) {
          uVar13 = 0;
          while (uVar22 = uVar12, uVar11 = uVar13, uVar26 - 0x30 < 10) {
            if (0xccccccc < (int)uVar13) {
              return -1;
            }
            if ((int)(-0x7fffffd1 - uVar26) < (int)(uVar13 * 10)) {
              return -1;
            }
            uVar13 = uVar13 * 10 + (uVar26 - 0x30);
            uVar26 = (uint)*pbVar27;
            pbVar27 = pbVar27 + 1;
          }
joined_r0x0102b4b4:
          if (uVar26 == 0x24) goto LAB_0102b51c;
          goto LAB_0102b108;
        }
        uVar39 = 0;
        pbVar28 = pbVar27;
        while( true ) {
          bVar2 = *pbVar28;
          uVar23 = bVar2 - 0x30;
          if (9 < uVar23) break;
          if (0xccccccc < (int)uVar39) {
            return -1;
          }
          iVar35 = (int)uVar39 * 10;
          if ((int)(-0x7fffffd1 - (uint)bVar2) < iVar35) {
            return -1;
          }
          uVar39 = (ulong)(iVar35 + uVar23);
          pbVar28 = pbVar28 + 1;
        }
        if (bVar2 == 0x24) {
          if (puVar37 == (undefined1 *)0x0) {
            puVar37 = local_a8;
            local_f0 = local_c8;
            uStack_e8 = uStack_c0;
            local_e0 = local_b8;
            uStack_d8 = uStack_b0;
            FUN_0102aad0(param_3,&local_f0);
          }
          pbVar27 = pbVar28 + 1;
          uVar23 = *(uint *)(puVar37 + (-(uVar39 >> 0x1f) & 0xfffffff800000000 | uVar39 << 3));
        }
        else if (puVar37 == (undefined1 *)0x0) {
          uVar40 = (ulong)(iVar36 + 1U);
          if (iVar34 < 0) {
            iVar36 = iVar34 + 8;
            puVar19 = (uint *)(lVar33 + iVar34);
            iVar34 = iVar36;
            if (0 < iVar36) goto LAB_0102b454;
          }
          else {
LAB_0102b454:
            puVar19 = puVar29;
            puVar29 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
          }
          uVar23 = *puVar19;
        }
        else {
          uVar39 = uVar40 << 3;
          uVar21 = uVar40 >> 0x1f;
          uVar40 = (ulong)(iVar36 + 1U);
          uVar23 = *(uint *)(puVar37 + (-uVar21 & 0xfffffff800000000 | uVar39));
        }
        if ((int)uVar23 < 0) {
          uVar23 = 0xffffffff;
        }
        goto LAB_0102b0fc;
      }
      if (0x2e < uVar26) {
        if (uVar26 == 0x44) {
          bVar3 = true;
          goto LAB_0102b60c;
        }
        if (uVar26 < 0x45) {
          if (uVar26 == 0x30) {
            uVar14 = uVar14 | 0x80;
            goto LAB_0102b0fc;
          }
          if ((0x2f < uVar26) && (uVar26 < 0x3a)) {
            uVar22 = 0;
            while( true ) {
              if ((int)(-0x7fffffd1 - uVar26) < (int)(uVar22 * 10)) {
                return -1;
              }
              uVar22 = uVar22 * 10 + (uVar26 - 0x30);
              pbVar27 = pbVar28 + 1;
              uVar26 = (uint)*pbVar28;
              uVar13 = uVar22;
              uVar11 = uVar23;
              if (9 < uVar26 - 0x30) break;
              pbVar28 = pbVar27;
              if (0xccccccc < (int)uVar22) {
                return -1;
              }
            }
            goto joined_r0x0102b4b4;
          }
        }
        else {
          if (uVar26 == 0x55) {
            bVar3 = true;
LAB_0102ba50:
            iVar35 = iVar34;
            if (bVar4) {
LAB_0102bb1c:
              if (puVar37 == (undefined1 *)0x0) {
                if (-1 < iVar34) {
LAB_0102bb50:
                  uVar40 = (long)puVar29 + 0xb;
                  goto LAB_0102bb54;
                }
                iVar35 = iVar34 + 8;
                puVar30 = (uint *)(lVar33 + iVar34);
                iVar34 = iVar35;
                if (0 < iVar35) goto LAB_0102bb50;
                goto LAB_0102bb5c;
              }
              uVar39 = (ulong)*(uint *)(puVar37 +
                                       (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3));
            }
            else {
              if ((((!bVar5) && (!bVar3)) && (!bVar8)) && (!bVar9)) {
                if (!bVar6) {
                  if (!bVar7) goto LAB_0102bb1c;
                  if (puVar37 != (undefined1 *)0x0) {
                    uVar39 = (ulong)(byte)puVar37[-(uVar40 >> 0x1f) & 0xfffffff800000000 |
                                                  uVar40 << 3];
                    goto LAB_0102bb2c;
                  }
                  if (iVar34 < 0) {
                    iVar35 = iVar34 + 8;
                    puVar30 = (uint *)(lVar33 + iVar34);
                    iVar34 = iVar35;
                    if (0 < iVar35) goto LAB_0102bb08;
                  }
                  else {
LAB_0102bb08:
                    puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                    puVar30 = puVar29;
                  }
                  uVar39 = (ulong)(byte)*puVar30;
                  puVar29 = puVar19;
                  goto LAB_0102bb2c;
                }
                if (puVar37 != (undefined1 *)0x0) {
                  uVar39 = (ulong)*(ushort *)
                                   (puVar37 + (-(uVar40 >> 0x1f) & 0xfffffff800000000 | uVar40 << 3)
                                   );
                  goto LAB_0102bb2c;
                }
                if (iVar34 < 0) {
                  iVar35 = iVar34 + 8;
                  puVar30 = (uint *)(lVar33 + iVar34);
                  iVar34 = iVar35;
                  if (0 < iVar35) goto LAB_0102bac8;
                }
                else {
LAB_0102bac8:
                  puVar19 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
                  puVar30 = puVar29;
                }
                uVar39 = (ulong)(ushort)*puVar30;
                puVar29 = puVar19;
                goto LAB_0102bb2c;
              }
              if (puVar37 != (undefined1 *)0x0) {
                uVar39 = *(ulong *)(puVar37 + (long)iVar36 * 8);
                goto LAB_0102bb2c;
              }
              if (iVar34 < 0) {
                iVar35 = iVar34 + 8;
                puVar30 = (uint *)(lVar33 + iVar34);
                iVar34 = iVar35;
                if (iVar35 < 1) goto LAB_0102bb5c;
              }
              uVar40 = (long)puVar29 + 0xf;
LAB_0102bb54:
              puVar19 = (uint *)(uVar40 & 0xfffffffffffffff8);
              puVar30 = puVar29;
              iVar34 = iVar35;
LAB_0102bb5c:
              uVar39 = (ulong)*puVar30;
              puVar29 = puVar19;
            }
LAB_0102bb2c:
            uVar40 = (ulong)(iVar36 + 1);
            uVar26 = 0;
            iVar36 = 1;
            goto LAB_0102bca4;
          }
          if (uVar26 == 0x58) {
            local_130 = 0x102e428;
            goto LAB_0102bb88;
          }
          if (uVar26 == 0x4f) {
            bVar3 = true;
            goto LAB_0102b840;
          }
        }
LAB_0102bda0:
        if (uVar26 == 0) {
          return local_f4;
        }
        local_68[0] = (char)uVar26;
        goto LAB_0102bdac;
      }
      if (uVar26 == 0x27) goto LAB_0102b0fc;
      if (uVar26 < 0x28) {
        if (uVar26 == 0x20) {
          if (uVar24 == 0) {
            uVar38 = uVar26;
          }
        }
        else {
          if (uVar26 != 0x23) goto LAB_0102bda0;
          bVar10 = true;
        }
        goto LAB_0102b0fc;
      }
      uVar38 = uVar26;
      if (uVar26 == 0x2b) goto LAB_0102b0fc;
      if (uVar26 == 0x2d) goto LAB_0102b34c;
      if (uVar26 != 0x2a) goto LAB_0102bda0;
      uVar39 = 0;
      while( true ) {
        uVar12 = (uint)*pbVar28;
        if (9 < uVar12 - 0x30) break;
        if (0xccccccc < (int)uVar39) {
          return -1;
        }
        iVar35 = (int)uVar39 * 10;
        if ((int)(-0x7fffffd1 - uVar12) < iVar35) {
          return -1;
        }
        uVar39 = (ulong)(iVar35 + (uVar12 - 0x30));
        pbVar28 = pbVar28 + 1;
      }
      if (uVar12 == 0x24) {
        if (puVar37 == (undefined1 *)0x0) {
          puVar37 = local_a8;
          local_f0 = local_c8;
          uStack_e8 = uStack_c0;
          local_e0 = local_b8;
          uStack_d8 = uStack_b0;
          FUN_0102aad0(param_3,&local_f0);
        }
        pbVar27 = pbVar28 + 1;
        uVar12 = *(uint *)(puVar37 + (-(uVar39 >> 0x1f) & 0xfffffff800000000 | uVar39 << 3));
      }
      else if (puVar37 == (undefined1 *)0x0) {
        uVar40 = (ulong)(iVar36 + 1U);
        if (iVar34 < 0) {
          iVar36 = iVar34 + 8;
          puVar19 = (uint *)(lVar33 + iVar34);
          iVar34 = iVar36;
          if (0 < iVar36) goto LAB_0102b328;
        }
        else {
LAB_0102b328:
          puVar19 = puVar29;
          puVar29 = (uint *)((long)puVar29 + 0xbU & 0xfffffffffffffff8);
        }
        uVar12 = *puVar19;
      }
      else {
        uVar39 = uVar40 << 3;
        uVar21 = uVar40 >> 0x1f;
        uVar40 = (ulong)(iVar36 + 1U);
        uVar12 = *(uint *)(puVar37 + (-uVar21 & 0xfffffff800000000 | uVar39));
      }
      uVar38 = uVar24;
      if ((int)uVar12 < 0) {
        if (uVar12 == 0x80000000) {
          return -1;
        }
        uVar12 = -uVar12;
LAB_0102b34c:
        uVar14 = uVar14 | 4;
        uVar38 = uVar24;
      }
    } while( true );
  }
  goto LAB_0102b05c;
LAB_0102b51c:
  uVar40 = (ulong)uVar13;
  if (puVar37 == (undefined1 *)0x0) {
    puVar37 = local_a8;
    local_f0 = local_c8;
    uStack_e8 = uStack_c0;
    local_e0 = local_b8;
    uStack_d8 = uStack_b0;
    FUN_0102aad0(param_3,&local_f0);
  }
  goto LAB_0102b0fc;
}



/* @ 0x102c1f4  FUN_0102c1f4 */

long FUN_0102c1f4(char *param_1)

{
  char *pcVar1;
  
  for (pcVar1 = param_1; *pcVar1 != '\0'; pcVar1 = pcVar1 + 1) {
  }
  return (long)pcVar1 - (long)param_1;
}



/* @ 0x102c210  FUN_0102c210 */

void FUN_0102c210(long param_1,undefined1 param_2,long param_3)

{
  long lVar1;
  
  lVar1 = 0;
  if (param_3 != 0) {
    do {
      *(undefined1 *)(param_1 + lVar1) = param_2;
      lVar1 = lVar1 + 1;
    } while (lVar1 != param_3);
  }
  return;
}



/* @ 0x102c22c  FUN_0102c22c */

char * FUN_0102c22c(char *param_1,char param_2,char *param_3)

{
  char *pcVar1;
  char *pcVar2;
  
  pcVar2 = param_1;
  pcVar1 = param_3;
  while( true ) {
    if (pcVar1 == (char *)0x0) {
      return (char *)0x0;
    }
    if (*pcVar2 == param_2) break;
    pcVar1 = param_1 + ((long)param_3 - (long)(pcVar2 + 1));
    pcVar2 = pcVar2 + 1;
  }
  return pcVar2;
}



/* @ 0x102c270  FUN_0102c270 */

void FUN_0102c270(long param_1)

{
  *(undefined2 *)(param_1 + 0xf4) = 0xf;
  return;
}



/* @ 0x102c27c  FUN_0102c27c */

undefined8 FUN_0102c27c(long param_1,undefined4 *param_2)

{
  uint uVar1;
  uint uVar2;
  undefined1 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined8 uVar6;
  bool bVar7;
  
  if (param_1 == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e516,1,0x102e4a0,0x102e540,0x2bf);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e529,1,0x102e4a0,0x102e540,0x2c0);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = *(uint *)(param_1 + 0x110);
  uVar2 = (uVar1 & 0xf) - 1;
  uVar5 = 100000000;
  if ((uVar1 >> 0x13 & 1) != 1) {
    uVar5 = 25000000;
  }
  param_2[5] = uVar5;
  if (uVar2 < 0xf) {
                    /* WARNING: Could not emulate address calculation at 0x0102c360 */
                    /* WARNING: Treating indirect jump as call */
    uVar6 = (*(code *)((long)*(char *)((ulong)uVar2 + 0x102e460) * 4 + 0x102c370))();
    return uVar6;
  }
  *param_2 = uVar5;
  uVar2 = (uVar1 >> 4 & 7) - 1;
  if (uVar2 < 7) {
                    /* WARNING: Could not emulate address calculation at 0x0102c440 */
                    /* WARNING: Treating indirect jump as call */
    uVar6 = (*(code *)((long)*(char *)((ulong)uVar2 + 0x102e470) * 4 + 0x102c450))();
    return uVar6;
  }
  param_2[1] = uVar5;
  uVar2 = uVar1 >> 7 & 3;
  if (uVar2 == 0) {
    bVar7 = true;
    uVar4 = uVar5;
  }
  else {
    bVar7 = false;
    uVar4 = 3000000000;
    if (uVar2 != 1) {
      uVar4 = 1500000000;
    }
  }
  param_2[2] = uVar4;
  uVar4 = uVar5;
  if (!bVar7) {
    uVar2 = uVar1 >> 9 & 3;
    if (uVar2 == 2) {
      uVar5 = 375000000;
      uVar4 = 428000000;
    }
    else if (uVar2 == 3) {
      uVar5 = 375000000;
      uVar4 = 500000000;
    }
    else if (uVar2 == 1) {
      uVar5 = 375000000;
      uVar4 = uVar5;
    }
    else {
      uVar5 = 375000000;
      uVar4 = 250000000;
    }
  }
  param_2[3] = uVar4;
  param_2[6] = uVar1 >> 0xb & 1;
  uVar2 = uVar1 >> 0xc & 3;
  param_2[4] = uVar5;
  uVar5 = 1;
  param_2[0x10] = 1;
  if (uVar2 == 2) {
    uVar4 = 0;
  }
  else if ((uVar2 == 3) || (uVar2 == 1)) {
    uVar4 = 1;
    uVar5 = 0;
  }
  else {
    uVar4 = 1;
  }
  param_2[7] = uVar5;
  param_2[9] = uVar1 >> 0xe & 1;
  uVar2 = (uVar1 >> 0xf & 7) - 2;
  param_2[8] = uVar4;
  if (5 < uVar2) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e536,1,0x102e4a0,0x102e480,0x20b);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
    uVar2 = uVar1 >> 0x14 & 3;
    param_2[10] = 3;
    param_2[0xb] = uVar1 >> 0x12 & 1;
    uVar5 = 1;
    if (((uVar2 != 1) && (uVar5 = 0, uVar2 != 0)) && (uVar5 = 2, uVar2 != 3)) {
      FUN_0102a684(0);
      FUN_0102a9c8(&DAT_0102e0c1,0x102e536,1,0x102e4a0,0x102e558,600);
      FUN_0102a6a0(0);
      FUN_010294c8(0);
      uVar5 = 0;
    }
    param_2[0xc] = uVar5;
    param_2[0xd] = uVar1 >> 0x16 & 1;
    param_2[0xf] = 0;
    uVar3 = 0x57;
    if ((uVar1 >> 0x17 & 1) != 1) {
      uVar3 = 0x50;
    }
    *(undefined1 *)(param_2 + 0xe) = uVar3;
    return 0;
  }
                    /* WARNING: Could not emulate address calculation at 0x0102c5b8 */
                    /* WARNING: Treating indirect jump as call */
  uVar6 = (*(code *)((long)*(char *)((ulong)uVar2 + 0x102e478) * 4 + 0x102c5c8))();
  return uVar6;
}



/* @ 0x102c6d8  FUN_0102c6d8 */

void FUN_0102c6d8(long *param_1,long param_2)

{
  if (param_2 == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e611,1,0x102e5a0,0x102e588,0x37);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  *param_1 = param_2;
  return;
}



/* @ 0x102c744  FUN_0102c744 */

undefined8 FUN_0102c744(undefined8 *param_1,int param_2,undefined1 *param_3,int param_4)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e623,1,0x102e5a0,0x102e660,0x7e);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_3 == (undefined1 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e638,1,0x102e5a0,0x102e660,0x7f);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_2 == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e642,1,0x102e5a0,0x102e660,0x80);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  puVar4 = (undefined4 *)*param_1;
  iVar3 = 0;
  uVar1 = puVar4[2];
  do {
    if (param_2 == 0) {
      return 0;
    }
    if ((uVar1 & 0xc0) == 0) {
      if ((puVar4[5] & 1) == 0) goto LAB_0102c8ac;
      iVar2 = 1;
LAB_0102c87c:
      iVar2 = iVar2 - param_2;
      for (; (iVar2 + param_2 != 0 && (param_2 != 0)); param_2 = param_2 + -1) {
        *param_3 = (char)*puVar4;
        param_3 = param_3 + 1;
      }
      iVar3 = 0;
    }
    else {
      iVar2 = puVar4[0x21];
      if (iVar2 != 0) goto LAB_0102c87c;
LAB_0102c8ac:
      if (param_4 != -1) {
        iVar3 = iVar3 + 1;
        FUN_0102a54c(1);
      }
    }
    if ((param_4 == -1) < (param_4 < iVar3)) {
      FUN_0102a684(0);
      FUN_0102a9c8(0x102e64d,0x102e660);
      FUN_0102a6a0(0);
      return 0xffffffc2;
    }
  } while( true );
}



/* @ 0x102c914  FUN_0102c914 */

undefined8 FUN_0102c914(undefined8 *param_1,int param_2,byte *param_3,int param_4)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e623,1,0x102e5a0,0x102e578,0xb8);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_3 == (byte *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e638,1,0x102e5a0,0x102e578,0xb9);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_2 == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e642,1,0x102e5a0,0x102e578,0xba);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  puVar4 = (uint *)*param_1;
  iVar3 = 0;
  uVar1 = puVar4[2];
  do {
    if (param_2 == 0) {
      return 0;
    }
    if ((uVar1 & 0xc0) == 0) {
      if ((puVar4[5] >> 5 & 1) == 0) goto LAB_0102ca84;
      iVar2 = 1;
LAB_0102ca54:
      iVar2 = iVar2 - param_2;
      for (; (iVar2 + param_2 != 0 && (param_2 != 0)); param_2 = param_2 + -1) {
        *puVar4 = (uint)*param_3;
        param_3 = param_3 + 1;
      }
      iVar3 = 0;
    }
    else {
      iVar2 = 0x40 - puVar4[0x20];
      if (iVar2 != 0) goto LAB_0102ca54;
LAB_0102ca84:
      if (param_4 != -1) {
        iVar3 = iVar3 + 1;
        FUN_0102a54c(1);
      }
    }
    if ((param_4 == -1) < (param_4 < iVar3)) {
      FUN_0102a684(0);
      FUN_0102a9c8(0x102e64d,0x102e578);
      FUN_0102a6a0(0);
      return 0xffffffc2;
    }
  } while( true );
}



/* @ 0x102caec  FUN_0102caec */

uint FUN_0102caec(long *param_1)

{
  return *(uint *)(*param_1 + 0x14) & 1;
}



/* @ 0x102cafc  FUN_0102cafc */

undefined8 FUN_0102cafc(long *param_1,long param_2,undefined1 param_3,undefined1 param_4)

{
  if (param_1 == (long *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e7a0,0x62);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_2 == 0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e74f,1,0x102e6b8,0x102e7a0,99);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  *(undefined1 *)(param_1 + 1) = param_3;
  *(undefined1 *)((long)param_1 + 9) = param_4;
  *param_1 = param_2;
  return 0;
}



/* @ 0x102cbcc  FUN_0102cbcc */

void FUN_0102cbcc(long *param_1,int param_2,int param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  if (param_1 == (long *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e670,0x77);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_3 == 1) {
    uVar2 = 0x40;
  }
  else {
    if (param_3 != 0) {
      if (param_3 == 2) {
        uVar2 = 1;
        uVar3 = 1;
      }
      else {
        uVar2 = 0;
        uVar3 = 0;
      }
      goto LAB_0102cc78;
    }
    uVar2 = 0;
  }
  uVar3 = 0x41;
LAB_0102cc78:
  uVar1 = uVar2 | 2;
  if (param_2 != 1) {
    uVar1 = uVar2;
  }
  uVar2 = uVar1;
  if (param_4 != 0) {
    uVar2 = uVar1 | 8;
    if (param_4 == 1) {
      uVar2 = uVar1 | 4;
    }
  }
  *(uint *)(*param_1 + 8) = uVar2 | *(uint *)(*param_1 + 8) & ((uVar3 | 0xe) ^ 0xffffffff);
  return;
}



/* @ 0x102ccb8  FUN_0102ccb8 */

void FUN_0102ccb8(undefined8 *param_1,undefined4 param_2)

{
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e6a0,0x8c);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  *(undefined4 *)*param_1 = param_2;
  return;
}



/* @ 0x102cd28  FUN_0102cd28 */

void FUN_0102cd28(long *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (long *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e688,0xa9);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = 0x20;
  if (param_2 != 1) {
    uVar1 = 0;
  }
  *(uint *)(*param_1 + 8) = uVar1 | *(uint *)(*param_1 + 8) & 0xffffffdf;
  return;
}



/* @ 0x102cdb0  FUN_0102cdb0 */

void FUN_0102cdb0(long *param_1,int param_2)

{
  uint uVar1;
  
  if (param_1 == (long *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e778,0xbf);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = 0x80;
  if (param_2 != 1) {
    uVar1 = 0;
  }
  *(uint *)(*param_1 + 8) = uVar1 | *(uint *)(*param_1 + 8) & 0xffffff7f;
  return;
}



/* @ 0x102ce38  FUN_0102ce38 */

undefined4 FUN_0102ce38(long *param_1)

{
  if (param_1 == (long *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e733,1,0x102e6b8,0x102e788,0xf8);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  return *(undefined4 *)(*param_1 + 4);
}



/* @ 0x102cea4  FUN_0102cea4 */

void FUN_0102cea4(uint *param_1,long param_2)

{
  uint uVar1;
  
  if (param_1 == (uint *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e8cc,1,0x102e848,0x102e9f0,0x10);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  uVar1 = *(uint *)(param_2 + 0x4400);
  *(long *)(param_1 + 2) = param_2;
  *param_1 = uVar1 >> 8 & 0xff;
  return;
}



/* @ 0x102cf20  FUN_0102cf20 */

void FUN_0102cf20(undefined8 *param_1,uint *param_2,long param_3)

{
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e8cc,1,0x102e848,0x102e7d8,0x20);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (param_2 == (uint *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e8d6,1,0x102e848,0x102e7d8,0x21);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (*param_2 < 4) {
    if (param_3 == 0) goto LAB_0102d028;
  }
  else if (param_3 != 0) goto LAB_0102d028;
  FUN_0102a684(0);
  FUN_0102a9c8(&DAT_0102e0c1,0x102e8e7,1,0x102e848,0x102e7d8,0x26);
  FUN_0102a6a0(0);
  FUN_010294c8(0);
LAB_0102d028:
  *param_1 = param_2;
  param_1[1] = param_3;
  return;
}



/* @ 0x102d040  FUN_0102d040 */

undefined4 FUN_0102d040(long param_1)

{
  return *(undefined4 *)(*(long *)(param_1 + 8) + 0x1004);
}



/* @ 0x102d04c  FUN_0102d04c */

void FUN_0102d04c(undefined8 *param_1,undefined4 param_2,undefined4 param_3)

{
  long lVar1;
  
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(0x102f0c1,0x102f8cc,1,0x102f848,0x102f800,0xf3);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (*(uint *)*param_1 < 4) {
    lVar1 = *(long *)((uint *)*param_1 + 2);
    *(undefined4 *)(lVar1 + 0x100) = param_2;
    *(undefined4 *)(lVar1 + 0x104) = param_3;
    return;
  }
  FUN_0102a684(0);
  FUN_0102a9c8(0x102f965,0x102f800,*(undefined4 *)*param_1);
  FUN_0102a6a0(0);
  FUN_0102a684(0);
  FUN_0102a9c8(0x102f0c1,0x102f536,1,0x102f848,0x102f800,0xfd);
  FUN_0102a6a0(0);
  FUN_010294c8(0);
  return;
}



/* @ 0x102d158  FUN_0102d158 */

void FUN_0102d158(undefined8 *param_1,uint param_2,undefined4 param_3)

{
  if (param_1 == (undefined8 *)0x0) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e8cc,1,0x102e848,0x102e828,0x11b);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (3 < param_2) {
    FUN_0102a684(0);
    FUN_0102a9c8(&DAT_0102e0c1,0x102e959,1,0x102e848,0x102e828,0x11c);
    FUN_0102a6a0(0);
    FUN_010294c8(0);
  }
  if (*(uint *)*param_1 < 4) {
    *(undefined4 *)(*(long *)((uint *)*param_1 + 2) + (ulong)param_2 * 0x100 + 0x2020) = param_3;
    return;
  }
  FUN_0102a684(0);
  FUN_0102a9c8(0x102e965,0x102e828,*(undefined4 *)*param_1);
  FUN_0102a6a0(0);
  FUN_0102a684(0);
  FUN_0102a9c8(&DAT_0102e0c1,0x102e536,1,0x102e848,0x102e828,0x125);
  FUN_0102a6a0(0);
  FUN_010294c8(0);
  return;
}



/* @ 0x102d5c0  FUN_0102d5c0 */

void FUN_0102d5c0(int param_1)

{
  long lVar1;
  int iVar2;
  uint *puVar3;
  long lVar4;
  int iVar5;
  
  puVar3 = (uint *)&DAT_f0200004;
  iVar5 = 0x20;
  if (param_1 != 0) {
    puVar3 = (uint *)&DAT_f0009004;
  }
  iVar2 = ((*puVar3 & 0x1f) + 1) * 0x20;
  for (; iVar5 < iVar2; iVar5 = iVar5 + 0x20) {
    lVar4 = 0x200000;
    if (param_1 != 0) {
      lVar4 = 0x9000;
    }
    *(undefined4 *)((long)((iVar5 >> 3) + 0x80) + 0xf0000000 + lVar4) = 0xffffffff;
  }
  for (lVar4 = 0xf0000420; (int)lVar4 + 0xffffc00 < iVar2; lVar4 = lVar4 + 4) {
    lVar1 = 0x200000;
    if (param_1 != 0) {
      lVar1 = 0x9000;
    }
    *(undefined4 *)(lVar1 + lVar4) = 0x80808080;
  }
  puVar3 = (uint *)&DAT_f0200000;
  if (param_1 != 0) {
    puVar3 = (uint *)&DAT_f0009000;
  }
  *puVar3 = *puVar3 | 0x11;
  return;
}



/* @ 0x102d684  FUN_0102d684 */

void FUN_0102d684(void)

{
  int iVar1;
  long lVar2;
  
  iVar1 = FUN_01029248();
  lVar2 = (long)((iVar1 + 0x14) * 0x20000);
  *(undefined4 *)(lVar2 + 0xf0000014) = 2;
  do {
  } while ((*(uint *)(lVar2 + 0xf0000014) >> 2 & 1) == 0);
  return;
}



/* @ 0x102d6c0  FUN_0102d6c0 */

void FUN_0102d6c0(long param_1)

{
  *(undefined4 *)(param_1 + 0x90008) = 1;
  FUN_0102d5c0(0);
  FUN_0102d5c0(1);
  return;
}



/* @ 0x102d6f0  FUN_0102d6f0 */

void FUN_0102d6f0(void)

{
  return;
}



/* @ 0x102d6f4  FUN_0102d6f4 */


void FUN_0102d6f4(void)

{
  int iVar1;
  long lVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  iVar1 = FUN_01029248();
  lVar2 = (long)((iVar1 + 0x14) * 0x20000);
  *(undefined4 *)(lVar2 + 0xf0010080) = 0xffffffff;
  puVar3 = (undefined4 *)(lVar2 + 0xf0010400);
  do {
    puVar4 = puVar3 + 1;
    *puVar3 = 0x80808080;
    puVar3 = puVar4;
  } while (puVar4 != (undefined4 *)(lVar2 + 0xf0010420));
  _DAT_f0100004 = 0xff;
  _DAT_f0100000 = _DAT_f0100000 & 0xfffffd82 | 0x79;
  _DAT_f0009080 = 0xffffffff;
  _DAT_f0009400 = 0x80808080;
  _DAT_f0009404 = 0x80808080;
  _DAT_f0009408 = 0x80808080;
  _DAT_f000940c = 0x80808080;
  _DAT_f0009410 = 0x80808080;
  _DAT_f0009414 = 0x80808080;
  _DAT_f0009418 = 0x80808080;
  _DAT_f000941c = 0x80808080;
  _DAT_f000a004 = 0xff;
  _DAT_f000a000 = _DAT_f000a000 & 0xfffffd82 | 0x78;
  return;
}



/* @ 0x102d7d0  FUN_0102d7d0 */


void FUN_0102d7d0(void)

{
  long lVar1;
  int iVar2;
  
  iVar2 = FUN_01029248();
  lVar1 = (long)((iVar2 + 0x14) * 0x20000);
  *(undefined4 *)(lVar1 + 0xf0000014) = 0;
  do {
  } while ((*(uint *)(lVar1 + 0xf0000014) >> 2 & 1) != 0);
  _DAT_f0100004 = 0xff;
  return;
}



/* @ 0x102d810  FUN_0102d810 */

void FUN_0102d810(long param_1)

{
  ulong uVar1;
  ulong uVar2;
  
  *DAT_0102d870 = 1;
  do {
  } while (*DAT_0102d878 != 2);
  uVar1 = UnkSytemRegRead(3,1,0xf,0,0);
  UnkSytemRegWrite(3,1,0xf,0,0,uVar1 & 0xffffffffffffffef | 0x108);
  uVar1 = UnkSytemRegRead(3,1,0xb,0,2);
  uVar2 = uVar1 & 0xfffffffffffffe38 | 0x82;
  if (param_1 != 0) {
    uVar2 = uVar1 & 0xfffffffffffffe38 | 0x200082;
  }
  UnkSytemRegWrite(3,1,0xb,0,2,uVar2);
  InstructionSynchronizationBarrier();
  return;
}



