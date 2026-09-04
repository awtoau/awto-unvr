## @file
#  Platform description for the Ubiquiti UNVR (Annapurna Labs Alpine V2,
#  4x Cortex-A57, ARMv8-A, sysid 0xea16).
#
#  P0 (docs/uefi.md): UART console, ARM generic timer, GIC-v3, DXE core +
#  boot services, UEFI Shell over serial. P1: internal PCIe only (bus
#  enumeration + AXI-snoop/APP_CONTROL fixup) - external PCIe0/USB/SATA/
#  network/DT-install are still P1.5-P4. Adapted from imbushuo/
#  ccr2004-uefi (same SoC family, PeilessSec PEI-less boot flow, same
#  internal-PCIe layout) with everything past current scope stripped out.
#
#  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = UNVR
  PLATFORM_GUID                  = 5a2e8c1d-3b7f-4a94-9e6d-1c8f5a2b3e70
  PLATFORM_VERSION                = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/UNVR
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Ubiquiti/UNVR/Unvr.fdf

################################################################################
#
# Library Class section
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  # Base libraries
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf

  # Debug
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf

  # Serial (NS16550A over MMIO - docs/uefi.md §3: NOT PL011)
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

  # PciLib: BaseSerialPortLib16550 has an (unused - we set
  # PcdSerialUseMmio=TRUE) PCI-config-space code path that still needs
  # this to resolve statically; segment-0-only (BasePciLibPciExpress),
  # harmless unlinked-in. Real PCI access (both segments) goes through
  # PciSegmentLib below, not this.
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf

  # PciSegmentLib: multi-segment (P1.5 - internal + external PCIe0, two
  # separate ECAM windows, see Library/PciSegmentInfoLib). Our own fork
  # of the stock MdePkg library (Library/PciSegmentLib/) - external
  # PCIe0 aliases its one real device across every devfn (confirmed
  # live 2026-09-03), so devfn!=0 on that segment is redirected to a
  # dummy all-0xFF buffer instead of real MMIO, mirroring the devfn!=0
  # guard every mainline pcie-designware-host.c Linux driver already
  # has built in. See PciSegmentLibCommon.c's file header.
  PciSegmentInfoLib|Platform/Ubiquiti/UNVR/Library/PciSegmentInfoLib/PciSegmentInfoLib.inf
  PciSegmentLib|Platform/Ubiquiti/UNVR/Library/PciSegmentLib/PciSegmentLib.inf

  # ARM libraries (ArmLib moved from ArmPkg to MdePkg upstream since the
  # CCR2004 reference/docs/uefi.md were written - confirmed by cross-
  # checking ArmVirtPkg's own DSC, edk2-stable202608)
  ArmLib|MdePkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmMonitorLib|ArmPkg/Library/ArmMonitorLib/ArmMonitorLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf

  # Platform
  ArmPlatformLib|Platform/Ubiquiti/UNVR/Library/PlatformLib/PlatformLib.inf

  # PCI host bridge (P1 - internal PCIe only, docs/uefi.md)
  PciHostBridgeLib|Platform/Ubiquiti/UNVR/Library/PciHostBridgeLib/PciHostBridgeLib.inf

  # Annapurna Labs HAL (eth + UDMA + IOFIC), shared by the ethernet SNP driver
  AlpineHalLib|Platform/Ubiquiti/UNVR/Library/AlpineHalLib/AlpineHalLib.inf

  # Null stubs
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf

  # PCD (default for DXE; SEC overrides to BasePcdLibNull)
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf

  # Entry points
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  # UEFI services
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

  # DXE HOB and memory
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

  # HII (TerminalDxe/ConSplitterDxe need this for language selection)
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

  # BDS / Boot Manager - generic ArmPkg one, not board-specific (P0 just
  # needs "no configured boot option -> fall back to the internal shell",
  # which is what this implements; no reason to write our own for P0)
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  PlatformBootManagerLib|ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf

  # Exception handling - unified UefiCpuPkg lib upstream now, no separate
  # DefaultExceptionHandlerLib (also confirmed against ArmVirtPkg's DSC)
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf

  # Reset (PSCI - docs/uefi.md §6: untested from an EDK2 context, P0 runs
  # single-core so this path isn't exercised by MpInitLib at boot)
  ResetSystemLib|ArmPkg/Library/ArmPsciResetSystemLib/ArmPsciResetSystemLib.inf

  # Security / Variable support (RAM-backed, no flash var store at P0)
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
  VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf

  # Capsule
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

  # RTC
  RealTimeClockLib|EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf
  TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf

  # Image properties
  ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf

  # Sort
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  # Block storage (P2 - SATA/USB mass storage, docs/uefi.md)
  # GptLib     <- PartitionDxe (GPT header/entry parsing)
  # UefiScsiLib <- ScsiBusDxe/ScsiDiskDxe (SCSI command helpers)
  # Both are the same instances ArmVirtPkg/ArmVirt.dsc.inc and
  # OvmfPkg/RiscVVirt use; no platform-specific choice exists.
  GptLib|MdeModulePkg/Library/GptLib/GptLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

  # Shell
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  ShellCommandLib|ShellPkg/Library/UefiShellCommandLib/UefiShellCommandLib.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  HandleParsingLib|ShellPkg/Library/UefiHandleParsingLib/UefiHandleParsingLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  BcfgCommandLib|ShellPkg/Library/UefiShellBcfgCommandLib/UefiShellBcfgCommandLib.inf

  # RNG (Cortex-A57 has no hardware TRNG at this SoC generation - matches
  # the CCR2004 reference's own note on its A72; timer-based is fine here,
  # nothing security-sensitive depends on it at P0)
  RngLib|MdeModulePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

  # Security (needed by SecurityStubDxe regardless of network/TLS scope)
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf

################################################################################
#
# PCD needed for generic ArmPkg PlatformBootManagerLib to compile at all
#
################################################################################

[PcdsFixedAtBuild.common]
  # ArmPkg/Library/PlatformBootManagerLib/PlatformBm.c has a build-time
  # STATIC_ASSERT requiring this be TTYTERM (=4) - it hardcodes the serial
  # console as a ConIn/ConOut/ErrOut device path. PcdUartDefault{Parity,
  # StopBits} already default to non-zero (MdePkg.dec), satisfying that
  # lib's other two static asserts without an override.
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  # UefiBootManagerLib's BmRepairAllControllers() looks up
  # gEfiFormBrowser2ProtocolGuid unconditionally unless this is
  # ZeroGuid (BmDriverHealth.c's own documented escape hatch) - we have
  # no SetupBrowserDxe/FormBrowser2 in this P0 component list at all
  # (no Setup UI), so the lookup fails "Not Found" and asserts every
  # boot attempt. Confirmed live 2026-09-02, see docs/uefi.md.
  gEfiMdeModulePkgTokenSpaceGuid.PcdDriverHealthConfigureForm|{0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}

################################################################################
#
# Phase-specific library overrides
#
################################################################################

[LibraryClasses.common.SEC]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
  PrePiHobListPointerLib|ArmPlatformPkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
  MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  MemoryInitPeiLib|ArmPlatformPkg/MemoryInitPei/MemoryInitPeiLib.inf
  PlatformPeiLib|ArmPlatformPkg/PlatformPei/PlatformPeiLib.inf
  PeilessSecMeasureLib|SecurityPkg/Library/PeilessSecMeasureLib/PeilessSecMeasureLibNull.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  # Runtime drivers must not touch MMIO-backed serial after
  # SetVirtualAddressMap - null debug lib avoids a data abort when EFI
  # Runtime Services are called post-ExitBootServices.
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf

################################################################################
#
# PCD section - values from docs/uefi.md §3, confirmed on bench §5
#
################################################################################

[PcdsFixedAtBuild.common]
  # System memory (primary/DRAM0 only - the full 4-bank map, including
  # non-contiguous DRAM1 @ 0x200000000, is in PlatformLibMem.c)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x0000000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x00C0000000

  # Firmware device - loaded by U-Boot's `go`/`bootm`, not flash-resident
  # (docs/uefi.md §1: chainload never touches flash)
  gArmTokenSpaceGuid.PcdFdBaseAddress|0x0020000000
  gArmTokenSpaceGuid.PcdFdSize|0x0000800000
  gArmTokenSpaceGuid.PcdFvBaseAddress|0x0020000000
  gArmTokenSpaceGuid.PcdFvSize|0x0000800000

  # Stack and UEFI region
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x10000
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  # Core count - P0 runs single-core; PSCI cpu_on from an EDK2 context is
  # untested (docs/uefi.md §6), secondaries stay parked
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1

  # Serial port (NS16550A @ 0xFD883000, live.dts: 500MHz clock, stride 4)
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0xFD883000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|500000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate|115200
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterAccessWidth|32
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|0x03
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x27

  # GICv3
  gArmTokenSpaceGuid.PcdGicDistributorBase|0xF0200000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0xF0280000

  # ARM generic timer: no PCD override needed/available in this edk2 -
  # ArmArchTimerLib's ArmGenericTimerGetTimerFreq() reads CNTFRQ_EL0
  # directly at runtime, which will be the live DT's true 58333312 Hz
  # (U-Boot leaves it correctly programmed), NOT U-Boot's own unrelated
  # hardcoded COUNTER_FREQUENCY=50000000 build-time constant (docs/uefi.md
  # §6 - that's a different, source-build-only value, not what's in the
  # register EDK2 actually reads).

  # PSCI: no PCD needed - live.dts says method=smc, and ArmMonitorLib's
  # gArmTokenSpaceGuid.PcdMonitorConduitHvc defaults to FALSE (=SMC
  # conduit), which is what we want; only HVC boards need to set it.
  # See PcdCoreCount note above for why PSCI cpu_on isn't exercised yet.

  # Debug
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x07
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x800000CF

  # Firmware version string
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"UNVR EDK2 P1"

  # Emulated variable store (RAM-backed, no flash at P0)
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVolatileVariableSize|0x20000

  # Boot timeout
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3

  # RNG: see RngLib note above
  gEfiMdePkgTokenSpaceGuid.PcdEnforceSecureRngAlgorithms|FALSE

  # PCI Express ECAM base (Alpine V2 internal PCIe, bus 0 only, flat
  # ECAM - no link/PHY/LTSSM concept, see docs/uefi.md P1). External
  # PCIe0 (0xfd800000/0xfb600000, ASM1042A xHCI) is out of scope for P1.
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xFBC00000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseSize|0x100000

################################################################################
#
# Build options
#
################################################################################

[BuildOptions.common.EDKII.DXE_CORE,BuildOptions.common.EDKII.DXE_DRIVER,BuildOptions.common.EDKII.UEFI_DRIVER,BuildOptions.common.EDKII.UEFI_APPLICATION]
  *_*_*_DLINK_FLAGS = -z common-page-size=0x1000

[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER]
  # AArch64 with 64KB pages requires 0x10000 section alignment for DXE
  # memory protection (W^X). GCC defaults to -z common-page-size=0x1000.
  GCC:*_*_AARCH64_DLINK_FLAGS = -z common-page-size=0x10000

################################################################################
#
# Components section - P0 scope only
#
################################################################################

[Components.common]
  #
  # SEC (PeilessSec: SEC -> DXE directly, no PEI phase) - stock EDK2
  # module, no board-specific SEC code needed (see PlatformLib.c header)
  #
  ArmPlatformPkg/PeilessSec/PeilessSec.inf

  #
  # Alpine HAL - listed as a component so it is compiled even before the
  # ethernet SNP driver that consumes it exists.
  #
  Platform/Ubiquiti/UNVR/Library/AlpineHalLib/AlpineHalLib.inf

  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

  #
  # Architectural protocol drivers
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  ArmPkg/Drivers/ArmGicDxe/ArmGicV3Dxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf

  #
  # Infrastructure
  #
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  Platform/Ubiquiti/UNVR/Drivers/MinimalHiiDxe/MinimalHiiDxe.inf

  #
  # Serial console
  #
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf

  #
  # Unicode Collation (required by Shell)
  #
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # PCI (internal P1 + external PCIe0 P1.5, docs/uefi.md). ArmPciCpuIo2Dxe
  # supplies CpuIo2, a PciHostBridgeDxe dependency even with no I/O space
  # routed.
  #
  ArmPkg/Drivers/ArmPciCpuIo2Dxe/ArmPciCpuIo2Dxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/AlPcieSnoopFixDxe.inf

  #
  # USB (P1.5 - external PCIe0 xHCI, ASM1042A). Independent differential
  # test of issue #140's still-open U-Boot xHCI SLOT_ID bug: EDK2's XHCI
  # driver is a completely separate codebase from U-Boot's - if it hits
  # the same failure on the same hardware, that's strong evidence the
  # bug is silicon/coherency-level, not specific to U-Boot's driver.
  #
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  #
  # Block storage (P2, docs/uefi.md) - all stock EDK2, the exact set
  # OvmfPkg/RiscVVirt/RiscVVirtQemu.dsc uses for AHCI, plus the disk
  # layers from ArmVirtPkg/ArmVirt.dsc.inc. No board-specific driver:
  # the 2x AHCI EPs (1c36:0031, abar 0xfe154000/0xfe158000, 4 ports
  # each) report PCI class 01/06/01 = generic AHCI 1.0, which is what
  # SataControllerDxe's IS_PCI_SATADPA match and AtaAtapiPassThru's
  # AHCI path expect (class bytes read off the live box, 2026-09-04).
  #
  # Stack: PciBusDxe -> SataControllerDxe (IdeControllerInit, a
  # TO_START dependency of AtaAtapiPassThru) -> AtaAtapiPassThru
  # (AtaPassThru/ExtScsiPassThru) -> AtaBusDxe (BlockIo) ->
  # PartitionDxe (GPT/MBR) -> DiskIoDxe -> Fat.inf (the ESP).
  # ScsiBus/ScsiDisk serve ATAPI and the USB mass-storage path.
  #
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf

  # Boot Manager Menu. NOTE (2026-09-02, see docs/uefi.md): this app's
  # own interactive menu auto-enumerates generic "non-block boot
  # devices" pointing at the raw memory-mapped FV region with no
  # specific file - "Expand ... -> <null string>", can never resolve to
  # anything bootable. The REAL, correctly-configured boot options are
  # Boot0000/Boot0001 in NVRAM (this app + Shell.inf, each with a proper
  # FvFile(GUID) device path) - reaching Shell needs those, not this
  # app's own menu. Tried dropping this component entirely so BDS's
  # automatic phase would fall through to Boot0001 (Shell) - the box
  # HUNG instead (PlatformRecovery0000 fails, "BootManagerMenu FFS
  # section can not be found, skip its boot option registration", then
  # nothing - no crash, no further output, unresponsive). Reverted.
  # Kept in, still non-ideal (P0 status: reaches a clean menu, not the
  # shell) - next session needs a proper NVRAM BootNext/BootOrder set to
  # Boot0001, not a component-list change.
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf

  #
  # FV as filesystem (lets the shell see FV contents as FSn:)
  #
  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf

  #
  # UEFI Shell (no network command libs - no network stack at P0)
  #
  ShellPkg/Application/Shell/Shell.inf {
    <PcdsFixedAtBuild>
      gEfiShellPkgTokenSpaceGuid.PcdShellLibAutoInitialize|FALSE
    <LibraryClasses>
      NULL|ShellPkg/Library/UefiShellLevel2CommandsLib/UefiShellLevel2CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel1CommandsLib/UefiShellLevel1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellLevel3CommandsLib/UefiShellLevel3CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDriver1CommandsLib/UefiShellDriver1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellInstall1CommandsLib/UefiShellInstall1CommandsLib.inf
      NULL|ShellPkg/Library/UefiShellDebug1CommandsLib/UefiShellDebug1CommandsLib.inf
  }
