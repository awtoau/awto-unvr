/** @file
  Alpine al_eth 1G RGMII SimpleNetworkProtocol driver for the Ubiquiti UNVR.

  Adapted from imbushuo/ccr2004-uefi's Drivers/AlEthNextDxe (BSD-2-Clause-Patent,
  same SoC family AL-324/Alpine V2, same delroth-vintage HAL 2.9).

  Board deltas vs that reference:
  - PHY address is fixed at 4 (AR8031/AR8033), not MDIO-scanned. Hardware-of-
    record; a scan can latch onto a stale/aliasing address.
  - No AR8035 RGMII TX-delay poke. The UNVR's delay is strapped/board-set, and
    our working U-Boot driver does not touch it (al_eth_dm.c).
  - No MikroTik BoardInfo protocol and no EmbeddedGpio PHY reset (neither
    exists on this platform). MAC comes from the EC filter register, which
    our U-Boot writes at probe from NOR 0x1f0000 (al_eth_hwaddr.c).
  - No al_eth_mac_link_config(). Stock skips it for external-PHY RGMII; the
    AR8033 drives the link and the MAC follows via RGMII in-band (al_eth_dm.c).

  Copyright (c) 2024, MikroTik. All rights reserved.
  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AlEth1gSnpDxe.h"

STATIC EFI_CPU_ARCH_PROTOCOL  *mCpu = NULL;

STATIC EFI_STATUS EFIAPI AlEth1gSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

STATIC EFI_STATUS EFIAPI AlEth1gStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

STATIC EFI_STATUS EFIAPI AlEth1gStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

STATIC EFI_DRIVER_BINDING_PROTOCOL  mDriverBinding = {
  AlEth1gSupported,
  AlEth1gStart,
  AlEth1gStop,
  0x10,
  NULL,
  NULL
};

#define MacRead32(Ctx, Off)        MmioRead32 ((Ctx)->MacBase + (Off))
#define MacWrite32(Ctx, Off, Val)  MmioWrite32 ((Ctx)->MacBase + (Off), (Val))

/* ---------- PHY ---------- */

/**
  Bring up the AR8031/AR8033 at the board's fixed MDIO address.

  Reset, then advertise 10/100/1000-full and restart autoneg. Link state is
  read from the MAC's own RGMII status register afterwards, not from the PHY.
**/
STATIC
EFI_STATUS
AlEth1gPhyInit (
  IN AL_ETH_1G_CONTEXT  *Ctx
  )
{
  UINT16  PhyId1;
  UINT16  PhyId2;
  UINT16  Bmcr;
  UINT32  Polls;
  int     Err;

  Ctx->PhyAddr    = AL_ETH_UNVR_PHY_ADDR;
  Ctx->PhyPresent = FALSE;

  Err = al_eth_mdio_read (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 2, &PhyId1);
  if (Err == 0) {
    Err = al_eth_mdio_read (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 3, &PhyId2);
  }

  if (Err != 0 || PhyId1 == 0xFFFF || PhyId1 == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "AlEth1g: no PHY at MDIO addr %u (err %d, id1 0x%04x)\n",
      Ctx->PhyAddr, Err, PhyId1
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "AlEth1g: PHY at MDIO addr %u, ID %04x:%04x\n",
    Ctx->PhyAddr, PhyId1, PhyId2
    ));

  /* Software reset; BMCR bit 15 self-clears when done. */
  al_eth_mdio_write (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 0, BIT15);

  for (Polls = 0; Polls < AL_ETH_PHY_RESET_MAX_POLLS; Polls++) {
    MicroSecondDelay (AL_ETH_PHY_RESET_POLL_MS * 1000);
    if (al_eth_mdio_read (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 0, &Bmcr) != 0) {
      continue;
    }
    if ((Bmcr & BIT15) == 0) {
      break;
    }
  }

  if (Polls >= AL_ETH_PHY_RESET_MAX_POLLS) {
    DEBUG ((
      DEBUG_ERROR,
      "AlEth1g: PHY reset did not clear BMCR.RESET after %u ms\n",
      AL_ETH_PHY_RESET_MAX_POLLS * AL_ETH_PHY_RESET_POLL_MS
      ));
    return EFI_TIMEOUT;
  }

  /* Advertise 10/100 (ANAR) and 1000BASE-T full (GBCR), then restart AN. */
  al_eth_mdio_write (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 4, 0x01E1);
  al_eth_mdio_write (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 9, 0x0200);
  al_eth_mdio_write (&Ctx->HalAdapter, Ctx->PhyAddr, 0, 0, 0x1200);

  Ctx->PhyPresent = TRUE;
  return EFI_SUCCESS;
}

/* ---------- Hardware init / shutdown ---------- */

/**
  Allocate one uncached region and hand its pages to the caller.

  Zeroed while still cacheable, flushed, then remapped UC - the descriptor
  rings and RX buffers are read by the UDMA, which is not coherent with the
  CPU caches at this stage.
**/
STATIC
EFI_STATUS
AlEth1gAllocUc (
  IN  UINTN                 Size,
  OUT VOID                  **Virt,
  OUT EFI_PHYSICAL_ADDRESS  *Phys
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Pages;
  UINTN                 NumPages;

  NumPages = EFI_SIZE_TO_PAGES (Size);
  Pages    = 0xFFFFFFFF;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  NumPages,
                  &Pages
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Virt = (VOID *)(UINTN)Pages;
  *Phys = Pages;

  ZeroMem (*Virt, Size);
  WriteBackInvalidateDataCacheRange (*Virt, Size);

  if (mCpu != NULL) {
    mCpu->SetMemoryAttributes (
            mCpu,
            Pages,
            EFI_PAGES_TO_SIZE (NumPages),
            EFI_MEMORY_UC
            );
  }

  return EFI_SUCCESS;
}

/**
  Clear whatever the previous boot stage left in the UDMA.

  U-Boot runs its own al_eth on this port and does not always halt it before
  handing over. Without this the fresh adapter_init inherits a running queue.
  Sequence matches U-Boot's al_eth_halt().
**/
STATIC
VOID
AlEth1gQuiesceStaleState (
  IN AL_ETH_1G_CONTEXT  *Ctx
  )
{
  struct al_hal_eth_adapter     TmpAdapter;
  struct al_eth_adapter_params  TmpParams;
  struct al_udma_q              *TmpQ;

  ZeroMem (&TmpAdapter, sizeof (TmpAdapter));
  ZeroMem (&TmpParams, sizeof (TmpParams));

  TmpParams.rev_id           = AL_ETH_REV_ID_2;
  TmpParams.dev_id           = AL_ETH_DEV_ID_STANDARD;
  TmpParams.udma_id          = 0;
  TmpParams.enable_rx_parser = 0;
  TmpParams.udma_regs_base   = (void *)(UINTN)Ctx->UdmaBase;
  TmpParams.ec_regs_base     = (void *)(UINTN)Ctx->EcBase;
  TmpParams.mac_regs_base    = (void *)(UINTN)Ctx->MacBase;

  if (al_eth_adapter_init (&TmpAdapter, &TmpParams) != 0) {
    return;
  }

  al_eth_mac_stop (&TmpAdapter);
  MicroSecondDelay (10);

  if (al_udma_q_handle_get (&TmpAdapter.tx_udma, 0, &TmpQ) == 0) {
    al_udma_q_reset (TmpQ);
  }
  if (al_udma_q_handle_get (&TmpAdapter.rx_udma, 0, &TmpQ) == 0) {
    al_udma_q_reset (TmpQ);
  }

  al_eth_adapter_stop (&TmpAdapter);
}

STATIC
EFI_STATUS
AlEth1gHwInitialize (
  IN AL_ETH_1G_CONTEXT  *Ctx
  )
{
  EFI_STATUS                    Status;
  int                           Err;
  UINT32                        Idx;
  struct al_eth_adapter_params  AdapterParams;
  struct al_udma_q_params       TxQParams;
  struct al_udma_q_params       RxQParams;

  Status = AlEth1gAllocUc (
             TOTAL_DESC_SIZE,
             &Ctx->DescRingBase,
             &Ctx->DescRingPhys
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: desc ring alloc failed: %r\n", Status));
    return Status;
  }

  for (Idx = 0; Idx < AL_ETH_NUM_RX_DESC; Idx++) {
    Status = AlEth1gAllocUc (
               AL_ETH_RX_BUF_SIZE,
               &Ctx->RxBuffers[Idx],
               &Ctx->RxBuffersPhys[Idx]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AlEth1g: RX buf %u alloc failed: %r\n", Idx, Status));
      return Status;
    }
  }

  AlEth1gQuiesceStaleState (Ctx);

  ZeroMem (&Ctx->HalAdapter, sizeof (Ctx->HalAdapter));
  ZeroMem (&AdapterParams, sizeof (AdapterParams));

  AdapterParams.rev_id           = AL_ETH_REV_ID_2;
  AdapterParams.dev_id           = AL_ETH_DEV_ID_STANDARD;
  AdapterParams.udma_id          = 0;
  AdapterParams.enable_rx_parser = 0;
  AdapterParams.udma_regs_base   = (void *)(UINTN)Ctx->UdmaBase;
  AdapterParams.ec_regs_base     = (void *)(UINTN)Ctx->EcBase;
  AdapterParams.mac_regs_base    = (void *)(UINTN)Ctx->MacBase;

  Err = al_eth_adapter_init (&Ctx->HalAdapter, &AdapterParams);
  if (Err != 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: al_eth_adapter_init failed: %d\n", Err));
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (&TxQParams, sizeof (TxQParams));
  TxQParams.size           = AL_ETH_DESCS_PER_Q;
  TxQParams.desc_base      = (union al_udma_desc *)((UINTN)Ctx->DescRingBase + TX_SDESC_OFFSET);
  TxQParams.desc_phy_base  = Ctx->DescRingPhys + TX_SDESC_OFFSET;
  TxQParams.cdesc_base     = (uint8_t *)((UINTN)Ctx->DescRingBase + TX_CDESC_OFFSET);
  TxQParams.cdesc_phy_base = Ctx->DescRingPhys + TX_CDESC_OFFSET;
  TxQParams.cdesc_size     = AL_ETH_CDESC_SIZE;

  Err = al_eth_queue_config (&Ctx->HalAdapter, UDMA_TX, 0, &TxQParams);
  if (Err != 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: TX queue_config failed: %d\n", Err));
    return EFI_DEVICE_ERROR;
  }

  /* Stub returning -EPERM in this HAL vintage; U-Boot ignores it too. */
  al_eth_queue_enable (&Ctx->HalAdapter, UDMA_TX, 0);

  ZeroMem (&RxQParams, sizeof (RxQParams));
  RxQParams.size           = AL_ETH_DESCS_PER_Q;
  RxQParams.desc_base      = (union al_udma_desc *)((UINTN)Ctx->DescRingBase + RX_SDESC_OFFSET);
  RxQParams.desc_phy_base  = Ctx->DescRingPhys + RX_SDESC_OFFSET;
  RxQParams.cdesc_base     = (uint8_t *)((UINTN)Ctx->DescRingBase + RX_CDESC_OFFSET);
  RxQParams.cdesc_phy_base = Ctx->DescRingPhys + RX_CDESC_OFFSET;
  RxQParams.cdesc_size     = AL_ETH_CDESC_SIZE;

  Err = al_eth_queue_config (&Ctx->HalAdapter, UDMA_RX, 0, &RxQParams);
  if (Err != 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: RX queue_config failed: %d\n", Err));
    return EFI_DEVICE_ERROR;
  }

  al_eth_queue_enable (&Ctx->HalAdapter, UDMA_RX, 0);

  al_udma_q_handle_get (&Ctx->HalAdapter.tx_udma, 0, &Ctx->TxDmaQ);
  al_udma_q_handle_get (&Ctx->HalAdapter.rx_udma, 0, &Ctx->RxDmaQ);

  /* RGMII. No al_eth_mac_link_config() - see file header. */
  Err = al_eth_mac_config (&Ctx->HalAdapter, AL_ETH_MAC_MODE_RGMII);
  if (Err != 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: mac_config(RGMII) failed: %d\n", Err));
    return EFI_DEVICE_ERROR;
  }

  al_eth_rx_pkt_limit_config (&Ctx->HalAdapter, 30, AL_ETH_MAX_PKT_SIZE);

  Err = al_eth_mdio_config (
          &Ctx->HalAdapter,
          AL_ETH_MDIO_TYPE_CLAUSE_22,
          AL_TRUE,
          AL_ETH_UNVR_REF_CLK,
          AL_ETH_UNVR_MDIO_CLK_KHZ
          );
  if (Err != 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: mdio_config failed: %d\n", Err));
    return EFI_DEVICE_ERROR;
  }

  /* A dead PHY is not fatal: the MAC is up and the link register still reads.
   * Reported, then carry on. */
  Status = AlEth1gPhyInit (Ctx);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "AlEth1g: PHY init failed (%r), continuing\n", Status));
  }

  al_eth_mac_addr_store (
    (void *)(UINTN)Ctx->EcBase,
    0,
    (uint8_t *)&Ctx->SnpMode.CurrentAddress
    );

  for (Idx = 0; Idx < AL_ETH_NUM_RX_DESC; Idx++) {
    struct al_buf  Buf;

    Buf.addr = (al_phys_addr_t)Ctx->RxBuffersPhys[Idx];
    Buf.len  = AL_ETH_RX_BUF_SIZE;

    Err = al_eth_rx_buffer_add (Ctx->RxDmaQ, &Buf, AL_ETH_RX_FLAGS_INT, NULL);
    if (Err != 0) {
      DEBUG ((DEBUG_ERROR, "AlEth1g: rx_buffer_add[%u] failed: %d\n", Idx, Err));
      return EFI_DEVICE_ERROR;
    }
  }

  Ctx->RxBufTailIdx = 0;
  al_eth_rx_buffer_action (Ctx->RxDmaQ, AL_ETH_NUM_RX_DESC);

  al_eth_mac_start (&Ctx->HalAdapter);

  Ctx->SnpMode.MediaPresent =
    ((MacRead32 (Ctx, MAC_GEN_RGMII_STAT) & RGMII_STAT_LINK) != 0);
  Ctx->TxBufInFlight = NULL;

  DEBUG ((
    DEBUG_INFO,
    "AlEth1g: init complete, link %a\n",
    Ctx->SnpMode.MediaPresent ? "UP" : "DOWN"
    ));

  return EFI_SUCCESS;
}

STATIC
VOID
AlEth1gHwShutdown (
  IN AL_ETH_1G_CONTEXT  *Ctx
  )
{
  UINT32  Idx;

  al_eth_mac_stop (&Ctx->HalAdapter);
  MicroSecondDelay (10);

  if (Ctx->TxDmaQ != NULL) {
    al_udma_q_reset (Ctx->TxDmaQ);
  }
  if (Ctx->RxDmaQ != NULL) {
    al_udma_q_reset (Ctx->RxDmaQ);
  }

  al_eth_adapter_stop (&Ctx->HalAdapter);
  MicroSecondDelay (100);

  for (Idx = 0; Idx < AL_ETH_NUM_RX_DESC; Idx++) {
    if (Ctx->RxBuffers[Idx] != NULL) {
      gBS->FreePages (
             Ctx->RxBuffersPhys[Idx],
             EFI_SIZE_TO_PAGES (AL_ETH_RX_BUF_SIZE)
             );
      Ctx->RxBuffers[Idx] = NULL;
    }
  }

  if (Ctx->DescRingBase != NULL) {
    gBS->FreePages (Ctx->DescRingPhys, EFI_SIZE_TO_PAGES (TOTAL_DESC_SIZE));
    Ctx->DescRingBase = NULL;
  }

  Ctx->TxDmaQ = NULL;
  Ctx->RxDmaQ = NULL;
}

/**
  Stop the DMA before the OS takes over.

  Without this the UDMA keeps writing into memory the OS is about to reuse.
**/
STATIC
VOID
EFIAPI
AlEth1gExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = (AL_ETH_1G_CONTEXT *)Context;

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return;
  }

  al_eth_mac_stop (&Ctx->HalAdapter);
  al_eth_adapter_stop (&Ctx->HalAdapter);
}

/* ---------- SNP methods ---------- */

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State == EfiSimpleNetworkStarted ||
      Ctx->SnpMode.State == EfiSimpleNetworkInitialized)
  {
    return EFI_ALREADY_STARTED;
  }

  if (Ctx->SnpMode.State != EfiSimpleNetworkStopped) {
    return EFI_DEVICE_ERROR;
  }

  Ctx->SnpMode.State = EfiSimpleNetworkStarted;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State == EfiSimpleNetworkStopped) {
    return EFI_NOT_STARTED;
  }

  Ctx->SnpMode.State = EfiSimpleNetworkStopped;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN UINTN                        ExtraRxBufSize  OPTIONAL,
  IN UINTN                        ExtraTxBufSize  OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);
  EFI_STATUS         Status;

  if (Ctx->SnpMode.State == EfiSimpleNetworkStopped) {
    return EFI_NOT_STARTED;
  }

  if (Ctx->SnpMode.State == EfiSimpleNetworkInitialized) {
    return EFI_SUCCESS;
  }

  Status = AlEth1gHwInitialize (Ctx);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ctx->SnpMode.State = EfiSimpleNetworkInitialized;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN BOOLEAN                      ExtendedVerification
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);
  EFI_STATUS         Status;

  if (Ctx->SnpMode.State == EfiSimpleNetworkStopped) {
    return EFI_NOT_STARTED;
  }

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return EFI_DEVICE_ERROR;
  }

  AlEth1gHwShutdown (Ctx);

  Status = AlEth1gHwInitialize (Ctx);
  if (EFI_ERROR (Status)) {
    Ctx->SnpMode.State = EfiSimpleNetworkStarted;
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State == EfiSimpleNetworkStopped) {
    return EFI_NOT_STARTED;
  }

  if (Ctx->SnpMode.State == EfiSimpleNetworkInitialized) {
    AlEth1gHwShutdown (Ctx);
  }

  Ctx->SnpMode.State = EfiSimpleNetworkStarted;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN UINT32                       Enable,
  IN UINT32                       Disable,
  IN BOOLEAN                      ResetMCastFilter,
  IN UINTN                        MCastFilterCnt  OPTIONAL,
  IN EFI_MAC_ADDRESS              *MCastFilter    OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);
  UINT32             CmdCfg;

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  Ctx->SnpMode.ReceiveFilterSetting |= Enable;
  Ctx->SnpMode.ReceiveFilterSetting &= ~Disable;

  CmdCfg = MacRead32 (Ctx, MAC_1G_CMD_CFG);
  if ((Ctx->SnpMode.ReceiveFilterSetting &
       EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) != 0)
  {
    CmdCfg |= MAC_1G_CMD_PROMIS_EN;
  } else {
    CmdCfg &= ~(UINT32)MAC_1G_CMD_PROMIS_EN;
  }

  MacWrite32 (Ctx, MAC_1G_CMD_CFG, CmdCfg);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN BOOLEAN                      Reset,
  IN EFI_MAC_ADDRESS              *New  OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  if (Reset) {
    CopyMem (
      &Ctx->SnpMode.CurrentAddress,
      &Ctx->SnpMode.PermanentAddress,
      sizeof (EFI_MAC_ADDRESS)
      );
  } else if (New != NULL) {
    CopyMem (&Ctx->SnpMode.CurrentAddress, New, sizeof (EFI_MAC_ADDRESS));
  } else {
    return EFI_INVALID_PARAMETER;
  }

  al_eth_mac_addr_store (
    (void *)(UINTN)Ctx->EcBase,
    0,
    (uint8_t *)&Ctx->SnpMode.CurrentAddress
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpStatistics (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                      Reset,
  IN OUT UINTN                        *StatisticsSize   OPTIONAL,
  OUT    EFI_NETWORK_STATISTICS       *StatisticsTable  OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  if (Reset) {
    ZeroMem (&Ctx->Stats, sizeof (Ctx->Stats));
  }

  if (StatisticsSize != NULL) {
    if (*StatisticsSize < sizeof (EFI_NETWORK_STATISTICS) ||
        StatisticsTable == NULL)
    {
      *StatisticsSize = sizeof (EFI_NETWORK_STATISTICS);
      return EFI_BUFFER_TOO_SMALL;
    }

    *StatisticsSize = sizeof (EFI_NETWORK_STATISTICS);
    CopyMem (StatisticsTable, &Ctx->Stats, sizeof (EFI_NETWORK_STATISTICS));
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpMCastIpToMac (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  BOOLEAN                      IPv6,
  IN  EFI_IP_ADDRESS               *IP,
  OUT EFI_MAC_ADDRESS              *MAC
  )
{
  if (IP == NULL || MAC == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IPv6) {
    MAC->Addr[0] = 0x33;
    MAC->Addr[1] = 0x33;
    MAC->Addr[2] = IP->v6.Addr[12];
    MAC->Addr[3] = IP->v6.Addr[13];
    MAC->Addr[4] = IP->v6.Addr[14];
    MAC->Addr[5] = IP->v6.Addr[15];
  } else {
    MAC->Addr[0] = 0x01;
    MAC->Addr[1] = 0x00;
    MAC->Addr[2] = 0x5E;
    MAC->Addr[3] = IP->v4.Addr[1] & 0x7F;
    MAC->Addr[4] = IP->v4.Addr[2];
    MAC->Addr[5] = IP->v4.Addr[3];
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpNvData (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                      ReadWrite,
  IN     UINTN                        Offset,
  IN     UINTN                        BufferSize,
  IN OUT VOID                         *Buffer
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpGetStatus (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  OUT UINT32                       *InterruptStatus  OPTIONAL,
  OUT VOID                         **TxBuf           OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  if (InterruptStatus != NULL) {
    *InterruptStatus = 0;
  }

  if (TxBuf != NULL) {
    *TxBuf = NULL;
    if (Ctx->TxBufInFlight != NULL && al_eth_comp_tx_get (Ctx->TxDmaQ) > 0) {
      *TxBuf             = Ctx->TxBufInFlight;
      Ctx->TxBufInFlight = NULL;
    }
  }

  Ctx->SnpMode.MediaPresent =
    ((MacRead32 (Ctx, MAC_GEN_RGMII_STAT) & RGMII_STAT_LINK) != 0);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN UINTN                        HeaderSize,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer,
  IN EFI_MAC_ADDRESS              *SrcAddr   OPTIONAL,
  IN EFI_MAC_ADDRESS              *DestAddr  OPTIONAL,
  IN UINT16                       *Protocol  OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);
  UINT8              *Pkt;
  int                NumDescs;
  struct al_eth_pkt  TxPkt;

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize < Ctx->SnpMode.MediaHeaderSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  /* One TX in flight (MultipleTxSupported = FALSE). Reclaim before reusing. */
  if (Ctx->TxBufInFlight != NULL) {
    if (al_eth_comp_tx_get (Ctx->TxDmaQ) > 0) {
      Ctx->TxBufInFlight = NULL;
    } else {
      return EFI_NOT_READY;
    }
  }

  if (HeaderSize != 0) {
    if (HeaderSize != Ctx->SnpMode.MediaHeaderSize ||
        DestAddr == NULL || Protocol == NULL)
    {
      return EFI_INVALID_PARAMETER;
    }

    Pkt = (UINT8 *)Buffer;
    CopyMem (Pkt, DestAddr, NET_ETHER_ADDR_LEN);
    CopyMem (
      Pkt + NET_ETHER_ADDR_LEN,
      (SrcAddr != NULL) ? SrcAddr : &Ctx->SnpMode.CurrentAddress,
      NET_ETHER_ADDR_LEN
      );
    Pkt[12] = (UINT8)(*Protocol >> 8);
    Pkt[13] = (UINT8)(*Protocol & 0xFF);
  }

  /* Caller's buffer is normal cacheable memory; the UDMA reads DRAM. */
  WriteBackInvalidateDataCacheRange (Buffer, BufferSize);

  ZeroMem (&TxPkt, sizeof (TxPkt));
  TxPkt.num_of_bufs  = 1;
  TxPkt.bufs[0].addr = (al_phys_addr_t)(UINTN)Buffer;
  TxPkt.bufs[0].len  = (uint32_t)BufferSize;

  NumDescs = al_eth_tx_pkt_prepare (Ctx->TxDmaQ, &TxPkt);
  if (NumDescs == 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: tx_pkt_prepare returned 0\n"));
    return EFI_DEVICE_ERROR;
  }

  al_eth_tx_dma_action (Ctx->TxDmaQ, (uint32_t)NumDescs);

  Ctx->TxBufInFlight = Buffer;
  Ctx->Stats.TxGoodFrames++;

  return EFI_SUCCESS;
}

/**
  Hand back the RX buffer at the tail and advance.
**/
STATIC
VOID
AlEth1gRxResubmit (
  IN AL_ETH_1G_CONTEXT  *Ctx
  )
{
  struct al_buf  Buf;

  Buf.addr = (al_phys_addr_t)Ctx->RxBuffersPhys[Ctx->RxBufTailIdx];
  Buf.len  = AL_ETH_RX_BUF_SIZE;

  if (al_eth_rx_buffer_add (Ctx->RxDmaQ, &Buf, AL_ETH_RX_FLAGS_INT, NULL) == 0) {
    al_eth_rx_buffer_action (Ctx->RxDmaQ, 1);
  }

  Ctx->RxBufTailIdx = (Ctx->RxBufTailIdx + 1) % AL_ETH_NUM_RX_DESC;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gSnpReceive (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  OUT    UINTN                        *HeaderSize  OPTIONAL,
  IN OUT UINTN                        *BufferSize,
  OUT    VOID                         *Buffer,
  OUT    EFI_MAC_ADDRESS              *SrcAddr     OPTIONAL,
  OUT    EFI_MAC_ADDRESS              *DestAddr    OPTIONAL,
  OUT    UINT16                       *Protocol    OPTIONAL
  )
{
  AL_ETH_1G_CONTEXT  *Ctx = AL_ETH_1G_FROM_SNP (Snp);
  struct al_eth_pkt  RxPkt;
  uint32_t           NumDescs;
  UINT32             PktLen;
  UINT8              *PktData;

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized) {
    return (Ctx->SnpMode.State == EfiSimpleNetworkStopped)
           ? EFI_NOT_STARTED : EFI_DEVICE_ERROR;
  }

  if (Buffer == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&RxPkt, sizeof (RxPkt));
  NumDescs = al_eth_pkt_rx (Ctx->RxDmaQ, &RxPkt);
  if (NumDescs == 0) {
    return EFI_NOT_READY;
  }

  if ((RxPkt.flags & (AL_ETH_RX_ERROR | AL_UDMA_CDESC_ERROR)) != 0) {
    DEBUG ((DEBUG_VERBOSE, "AlEth1g: RX error flags 0x%x\n", RxPkt.flags));
    Ctx->Stats.RxDroppedFrames++;
    AlEth1gRxResubmit (Ctx);
    return EFI_NOT_READY;
  }

  PktLen = RxPkt.bufs[0].len;
  if (PktLen < Ctx->SnpMode.MediaHeaderSize || PktLen > AL_ETH_RX_BUF_SIZE) {
    DEBUG ((DEBUG_VERBOSE, "AlEth1g: RX bad len %u\n", PktLen));
    Ctx->Stats.RxDroppedFrames++;
    AlEth1gRxResubmit (Ctx);
    return EFI_NOT_READY;
  }

  /* Too small: report the needed size and DROP. The HAL has already consumed
   * the completion descriptor, so the packet cannot be presented again - a
   * "caller will retry" path would hand back the next packet's length for
   * this one's data. */
  if (*BufferSize < PktLen) {
    *BufferSize = PktLen;
    Ctx->Stats.RxDroppedFrames++;
    AlEth1gRxResubmit (Ctx);
    return EFI_BUFFER_TOO_SMALL;
  }

  PktData = (UINT8 *)Ctx->RxBuffers[Ctx->RxBufTailIdx];
  InvalidateDataCacheRange (PktData, PktLen);
  CopyMem (Buffer, PktData, PktLen);
  *BufferSize = PktLen;

  if (HeaderSize != NULL) {
    *HeaderSize = Ctx->SnpMode.MediaHeaderSize;
  }
  if (DestAddr != NULL) {
    CopyMem (DestAddr, PktData, NET_ETHER_ADDR_LEN);
  }
  if (SrcAddr != NULL) {
    CopyMem (SrcAddr, PktData + NET_ETHER_ADDR_LEN, NET_ETHER_ADDR_LEN);
  }
  if (Protocol != NULL) {
    *Protocol = (UINT16)((PktData[12] << 8) | PktData[13]);
  }

  Ctx->Stats.RxGoodFrames++;
  AlEth1gRxResubmit (Ctx);

  return EFI_SUCCESS;
}

/**
  WaitForPacket: signal when the RX completion ring has an unconsumed entry.

  Peeks the completion descriptor rather than calling al_eth_pkt_rx(), which
  would consume it.
**/
STATIC
VOID
EFIAPI
AlEth1gWaitForPacket (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  AL_ETH_1G_CONTEXT             *Ctx = (AL_ETH_1G_CONTEXT *)Context;
  volatile union al_udma_cdesc  *CDesc;
  uint32_t                      Flags;

  if (Ctx->SnpMode.State != EfiSimpleNetworkInitialized ||
      Ctx->RxDmaQ == NULL)
  {
    return;
  }

  CDesc = al_udma_cdesc_idx_to_ptr (Ctx->RxDmaQ, Ctx->RxDmaQ->next_cdesc_idx);
  Flags = MmioRead32 ((UINTN)CDesc);

  if (al_udma_new_cdesc (Ctx->RxDmaQ, Flags)) {
    gBS->SignalEvent (Event);
  }
}

/* ---------- Driver binding ---------- */

STATIC
EFI_STATUS
EFIAPI
AlEth1gSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               VendorId;
  UINT16               DeviceId;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &VendorId
                        );
  if (!EFI_ERROR (Status)) {
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint16,
                          PCI_DEVICE_ID_OFFSET,
                          1,
                          &DeviceId
                          );
  }

  if (EFI_ERROR (Status) ||
      VendorId != AL_ETH_VENDOR_ID ||
      DeviceId != AL_ETH_DEVICE_ID)
  {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Read the port MAC.

  Source is the EC filter register, which our U-Boot populates at probe from
  the NOR identity blob (al_eth_hwaddr.c: base at 0x1f0000, port 2 = base+1).
  EDK2 has no SPI-NOR driver, so there is no second source; a zero reading
  means U-Boot did not run its al_eth, and a locally-administered address is
  synthesised so the SNP still installs.
**/
STATIC
VOID
AlEth1gReadMacAddress (
  IN  AL_ETH_1G_CONTEXT  *Ctx,
  OUT EFI_MAC_ADDRESS    *MacAddr
  )
{
  UINT32   Idx;
  BOOLEAN  IsZero;
  UINT64   Counter;

  ZeroMem (MacAddr, sizeof (EFI_MAC_ADDRESS));
  al_eth_mac_addr_read ((void *)(UINTN)Ctx->EcBase, 0, (uint8_t *)MacAddr);

  IsZero = TRUE;
  for (Idx = 0; Idx < NET_ETHER_ADDR_LEN; Idx++) {
    if (MacAddr->Addr[Idx] != 0) {
      IsZero = FALSE;
      break;
    }
  }

  if (!IsZero) {
    return;
  }

  Counter          = GetPerformanceCounter ();
  MacAddr->Addr[0] = 0x02;
  MacAddr->Addr[1] = (UINT8)(Counter >> 8);
  MacAddr->Addr[2] = (UINT8)(Counter >> 16);
  MacAddr->Addr[3] = (UINT8)(Counter >> 24);
  MacAddr->Addr[4] = (UINT8)(Counter >> 32);
  MacAddr->Addr[5] = (UINT8)(Counter >> 40);

  DEBUG ((
    DEBUG_WARN,
    "AlEth1g: EC filter holds no MAC - U-Boot's al_eth did not run. "
    "Using a locally-administered address.\n"
    ));
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  AL_ETH_1G_CONTEXT         *Ctx;
  EFI_MAC_ADDRESS           MacAddr;
  UINT32                    BarLo;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FullPath;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ctx = AllocateZeroPool (sizeof (AL_ETH_1G_CONTEXT));
  if (Ctx == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseProtocol;
  }

  Ctx->Signature        = AL_ETH_1G_SNP_SIGNATURE;
  Ctx->ControllerHandle = Controller;
  Ctx->PciIo            = PciIo;

  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationGet,
           0,
           &Ctx->OriginalPciAttributes
           );
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationEnable,
           EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER,
           NULL
           );

  /* BARs read straight from config space: the internal PCIe window is flat
   * ECAM and identity-mapped, so the BAR value is the CPU address. */
  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               PCI_BASE_ADDRESSREG_OFFSET + AL_ETH_BAR_UDMA * 4,
               1,
               &BarLo
               );
  Ctx->UdmaBase = (UINTN)(BarLo & 0xFFFFFFF0);

  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               PCI_BASE_ADDRESSREG_OFFSET + AL_ETH_BAR_MAC * 4,
               1,
               &BarLo
               );
  Ctx->MacBase = (UINTN)(BarLo & 0xFFFFFFF0);

  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               PCI_BASE_ADDRESSREG_OFFSET + AL_ETH_BAR_EC * 4,
               1,
               &BarLo
               );
  Ctx->EcBase = (UINTN)(BarLo & 0xFFFFFFF0);

  DEBUG ((
    DEBUG_INFO,
    "AlEth1g: BARs UDMA 0x%lx MAC 0x%lx EC 0x%lx\n",
    (UINT64)Ctx->UdmaBase, (UINT64)Ctx->MacBase, (UINT64)Ctx->EcBase
    ));

  if (Ctx->UdmaBase == 0 || Ctx->MacBase == 0 || Ctx->EcBase == 0) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: a required BAR is unassigned\n"));
    Status = EFI_DEVICE_ERROR;
    goto FreeCtx;
  }

  AlEth1gReadMacAddress (Ctx, &MacAddr);

  DEBUG ((
    DEBUG_INFO,
    "AlEth1g: MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
    MacAddr.Addr[0], MacAddr.Addr[1], MacAddr.Addr[2],
    MacAddr.Addr[3], MacAddr.Addr[4], MacAddr.Addr[5]
    ));

  Ctx->SnpMode.State                = EfiSimpleNetworkStopped;
  Ctx->SnpMode.HwAddressSize        = NET_ETHER_ADDR_LEN;
  Ctx->SnpMode.MediaHeaderSize      = 14;
  Ctx->SnpMode.MaxPacketSize        = 1500;
  Ctx->SnpMode.NvRamSize            = 0;
  Ctx->SnpMode.NvRamAccessSize      = 0;
  Ctx->SnpMode.ReceiveFilterMask    = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                                      EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
                                      EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  Ctx->SnpMode.ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
                                      EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  Ctx->SnpMode.IfType               = NET_IFTYPE_ETHERNET;
  Ctx->SnpMode.MacAddressChangeable = TRUE;
  Ctx->SnpMode.MultipleTxSupported  = FALSE;
  Ctx->SnpMode.MediaPresentSupported = TRUE;
  Ctx->SnpMode.MediaPresent          = FALSE;

  CopyMem (&Ctx->SnpMode.CurrentAddress, &MacAddr, sizeof (EFI_MAC_ADDRESS));
  CopyMem (&Ctx->SnpMode.PermanentAddress, &MacAddr, sizeof (EFI_MAC_ADDRESS));
  SetMem (&Ctx->SnpMode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xFF);

  Ctx->Snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  Ctx->Snp.Start          = AlEth1gSnpStart;
  Ctx->Snp.Stop           = AlEth1gSnpStop;
  Ctx->Snp.Initialize     = AlEth1gSnpInitialize;
  Ctx->Snp.Reset          = AlEth1gSnpReset;
  Ctx->Snp.Shutdown       = AlEth1gSnpShutdown;
  Ctx->Snp.ReceiveFilters = AlEth1gSnpReceiveFilters;
  Ctx->Snp.StationAddress = AlEth1gSnpStationAddress;
  Ctx->Snp.Statistics     = AlEth1gSnpStatistics;
  Ctx->Snp.MCastIpToMac   = AlEth1gSnpMCastIpToMac;
  Ctx->Snp.NvData         = AlEth1gSnpNvData;
  Ctx->Snp.GetStatus      = AlEth1gSnpGetStatus;
  Ctx->Snp.Transmit       = AlEth1gSnpTransmit;
  Ctx->Snp.Receive        = AlEth1gSnpReceive;
  Ctx->Snp.Mode           = &Ctx->SnpMode;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  AlEth1gWaitForPacket,
                  Ctx,
                  &Ctx->Snp.WaitForPacket
                  );
  if (EFI_ERROR (Status)) {
    goto FreeCtx;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  AlEth1gExitBootServices,
                  Ctx,
                  &Ctx->ExitBootServicesEvent
                  );
  if (EFI_ERROR (Status)) {
    goto CloseWfpEvent;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto CloseExitEvent;
  }

  Ctx->DevicePath = AllocateZeroPool (sizeof (AL_ETH_1G_DEVICE_PATH));
  if (Ctx->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseExitEvent;
  }

  Ctx->DevicePath->MacAddrNode.Header.Type    = MESSAGING_DEVICE_PATH;
  Ctx->DevicePath->MacAddrNode.Header.SubType = MSG_MAC_ADDR_DP;
  SetDevicePathNodeLength (
    &Ctx->DevicePath->MacAddrNode.Header,
    sizeof (MAC_ADDR_DEVICE_PATH)
    );
  CopyMem (
    &Ctx->DevicePath->MacAddrNode.MacAddress,
    &MacAddr,
    sizeof (EFI_MAC_ADDRESS)
    );
  Ctx->DevicePath->MacAddrNode.IfType = NET_IFTYPE_ETHERNET;
  SetDevicePathEndNode (&Ctx->DevicePath->End);

  FullPath = AppendDevicePathNode (
               ParentDevicePath,
               (EFI_DEVICE_PATH_PROTOCOL *)&Ctx->DevicePath->MacAddrNode
               );
  FreePool (Ctx->DevicePath);
  Ctx->DevicePath = (AL_ETH_1G_DEVICE_PATH *)FullPath;
  if (Ctx->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CloseExitEvent;
  }

  Ctx->ChildHandle = NULL;
  Status           = gBS->InstallMultipleProtocolInterfaces (
                            &Ctx->ChildHandle,
                            &gEfiSimpleNetworkProtocolGuid,
                            &Ctx->Snp,
                            &gEfiDevicePathProtocolGuid,
                            Ctx->DevicePath,
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    goto FreeDevicePath;
  }

  gBS->OpenProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         (VOID **)&PciIo,
         This->DriverBindingHandle,
         Ctx->ChildHandle,
         EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
         );

  DEBUG ((DEBUG_INFO, "AlEth1g: SNP installed on 1c36:0001\n"));
  return EFI_SUCCESS;

FreeDevicePath:
  FreePool (Ctx->DevicePath);
CloseExitEvent:
  gBS->CloseEvent (Ctx->ExitBootServicesEvent);
CloseWfpEvent:
  gBS->CloseEvent (Ctx->Snp.WaitForPacket);
FreeCtx:
  PciIo->Attributes (
           PciIo,
           EfiPciIoAttributeOperationSet,
           Ctx->OriginalPciAttributes,
           NULL
           );
  FreePool (Ctx);
CloseProtocol:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AlEth1gStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  AL_ETH_1G_CONTEXT            *Ctx;

  if (NumberOfChildren == 0) {
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **)&Snp,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ctx = AL_ETH_1G_FROM_SNP (Snp);

  if (Ctx->SnpMode.State == EfiSimpleNetworkInitialized) {
    AlEth1gHwShutdown (Ctx);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Ctx->ChildHandle
         );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Ctx->ChildHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  &Ctx->Snp,
                  &gEfiDevicePathProtocolGuid,
                  Ctx->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseEvent (Ctx->ExitBootServicesEvent);
  gBS->CloseEvent (Ctx->Snp.WaitForPacket);

  Ctx->PciIo->Attributes (
                Ctx->PciIo,
                EfiPciIoAttributeOperationSet,
                Ctx->OriginalPciAttributes,
                NULL
                );

  FreePool (Ctx->DevicePath);
  FreePool (Ctx);

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AlEth1gSnpDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID **)&mCpu
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AlEth1g: no CPU Arch Protocol: %r\n", Status));
    return Status;
  }

  return EfiLibInstallDriverBinding (
           ImageHandle,
           SystemTable,
           &mDriverBinding,
           ImageHandle
           );
}
