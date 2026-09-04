/** @file
  Alpine al_eth 1G RGMII SimpleNetworkProtocol driver for the Ubiquiti UNVR.

  Binds PCI 1c36:0001 (the always-connected RJ45), RGMII to an AR8031/AR8033
  at MDIO address 4. HAL-based: AlpineHalLib does the register work.

  Adapted from imbushuo/ccr2004-uefi's Drivers/AlEthNextDxe (same SoC family,
  same HAL vintage). Board deltas vs that reference are listed in the .c file
  header.

  Copyright (c) 2024, MikroTik. All rights reserved.
  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AL_ETH_1G_SNP_DXE_H_
#define AL_ETH_1G_SNP_DXE_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/Pci.h>
#include <Protocol/Cpu.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleNetwork.h>

/* Alpine HAL */
#include "al_hal_eth.h"
#include "al_hal_udma.h"

/* Defined locally rather than via NetworkPkg's NetLib.h. DxeNetLib carries
 * [Depex] gEfiRngProtocolGuid, which nothing in this build installs - the
 * driver was then never dispatched at all (confirmed live 2026-09-04: every
 * other driver loaded, this one silently did not). Only these two macros were
 * ever used from that header; no NetLib function is called. */
#define NET_ETHER_ADDR_LEN   6
#define NET_IFTYPE_ETHERNET  0x01

/* PCI identification - the 1G RJ45 port. The 10G SFP+ is 0x0002 and is NOT
 * bound here: it needs SerDes bring-up this driver deliberately omits. */
#define AL_ETH_VENDOR_ID  0x1C36
#define AL_ETH_DEVICE_ID  0x0001

/* BAR indices (al_eth_port.c) */
#define AL_ETH_BAR_UDMA  0
#define AL_ETH_BAR_MAC   2
#define AL_ETH_BAR_EC    4

/* Ring configuration */
#define AL_ETH_NUM_RX_DESC   32
#define AL_ETH_RX_BUF_SIZE   2048
#define AL_ETH_MAX_PKT_SIZE  1518
#define AL_ETH_CDESC_SIZE    16
#define AL_ETH_DESC_SIZE     16

/* HAL reserves AL_UDMA_MAX_NUM_CDESC_PER_CACHE_LINE (16) slots for completion
 * padding, so usable = DESCS_PER_Q - 16. 64 gives 48 >= NUM_RX_DESC. */
#define AL_ETH_DESCS_PER_Q    64
#define AL_ETH_Q_DESCS_SIZE   (AL_ETH_DESCS_PER_Q * AL_ETH_DESC_SIZE)

#define TX_SDESC_OFFSET  (0 * AL_ETH_Q_DESCS_SIZE)
#define TX_CDESC_OFFSET  (1 * AL_ETH_Q_DESCS_SIZE)
#define RX_SDESC_OFFSET  (2 * AL_ETH_Q_DESCS_SIZE)
#define RX_CDESC_OFFSET  (3 * AL_ETH_Q_DESCS_SIZE)
#define TOTAL_DESC_SIZE  (4 * AL_ETH_Q_DESCS_SIZE)

/* MAC gen block at 0x900 in the MAC BAR; RGMII status at gen + 0x1C. */
#define MAC_GEN_RGMII_STAT  0x91C
#define RGMII_STAT_LINK     BIT4

/* MAC 1G cmd_cfg, MAC BAR + 0x008 */
#define MAC_1G_CMD_CFG        0x008
#define MAC_1G_CMD_PROMIS_EN  BIT4

/* Board facts, hardware-of-record (uboot-port/drivers/net/al_eth/al_eth_dm.c).
 * Set explicitly, NOT read from the MAC scratchpad board params: the scratchpad
 * is the U-Boot->Linux handoff and may be unwritten when EDK2 runs. */
#define AL_ETH_UNVR_PHY_ADDR      4
#define AL_ETH_UNVR_MDIO_CLK_KHZ  1000
#define AL_ETH_UNVR_REF_CLK       AL_ETH_REF_FREQ_500_MHZ

/* PHY reset poll: BMCR self-clears within 0.5 s on an AR8031 (datasheet
 * "software reset" <= 500 ms). 1.25x that = 625 ms, polled at 1 ms.
 * On expiry: log the elapsed time and continue without the PHY. */
#define AL_ETH_PHY_RESET_POLL_MS   1
#define AL_ETH_PHY_RESET_MAX_POLLS 625

#define AL_ETH_1G_SNP_SIGNATURE  SIGNATURE_32('A','L','1','G')

typedef struct {
  MAC_ADDR_DEVICE_PATH        MacAddrNode;
  EFI_DEVICE_PATH_PROTOCOL    End;
} AL_ETH_1G_DEVICE_PATH;

typedef struct {
  UINT32                         Signature;
  EFI_HANDLE                     ControllerHandle;
  EFI_HANDLE                     ChildHandle;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  UINT64                         OriginalPciAttributes;

  /* BAR bases, read straight from config space (flat ECAM, virt == phys) */
  UINTN                          UdmaBase;
  UINTN                          MacBase;
  UINTN                          EcBase;

  struct al_hal_eth_adapter      HalAdapter;
  struct al_udma_q               *TxDmaQ;
  struct al_udma_q               *RxDmaQ;

  VOID                           *DescRingBase;
  EFI_PHYSICAL_ADDRESS           DescRingPhys;

  VOID                           *RxBuffers[AL_ETH_NUM_RX_DESC];
  EFI_PHYSICAL_ADDRESS           RxBuffersPhys[AL_ETH_NUM_RX_DESC];
  UINT32                         RxBufTailIdx;

  VOID                           *TxBufInFlight;

  UINT32                         PhyAddr;
  BOOLEAN                        PhyPresent;

  EFI_SIMPLE_NETWORK_PROTOCOL    Snp;
  EFI_SIMPLE_NETWORK_MODE        SnpMode;
  EFI_EVENT                      ExitBootServicesEvent;
  EFI_NETWORK_STATISTICS         Stats;

  AL_ETH_1G_DEVICE_PATH          *DevicePath;
} AL_ETH_1G_CONTEXT;

#define AL_ETH_1G_FROM_SNP(a) \
  CR (a, AL_ETH_1G_CONTEXT, Snp, AL_ETH_1G_SNP_SIGNATURE)

#endif /* AL_ETH_1G_SNP_DXE_H_ */
