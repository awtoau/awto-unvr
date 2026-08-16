# kernel-patches — Alpine V2 (AL-324, sysid ea16) mainline port

- git-format-patch series bringing up the Ubiquiti UNVR (Annapurna Labs Alpine V2
  / AL-324, aarch64) on mainline Linux. Netboot only; no kernel fork.
- Primary base: **v6.18.44** (patches generated against it). Also applies to
  **v7.1.8** and **v6.12.103** — see Rebase notes.
- Hardware-verified: 6.18.44 and 7.1.8 netbooted on the unit (internal PCIe, SATA,
  al_eth 1G+10G, al_ssm, al_dma, xHCI all up).

## Patches (apply in order)

| # | file | target | ours / upstream / vendor |
|---|------|--------|--------------------------|
| 0001 | `0001-PCI-al-add-Alpine-V2-internal-PCIe-host-controller-d.patch` | new `drivers/pci/controller/pcie-al-internal.c` + Kconfig + Makefile | driver = COMMUNITY; Kconfig/Makefile glue = OURS |
| 0002 | `0002-PCI-al-pre-set-DBI-base-for-external-DWC-PCIe-xHCI-U.patch` | `drivers/pci/controller/dwc/pcie-al.c` (in-place DBI-fix) | OURS |
| 0003 | `0003-arm64-dts-amazon-add-Ubiquiti-UNVR-ea16-NAND-boot-bo.patch` | new ea16 DTS + gpio-hog + amazon Makefile reg | OURS |
| 0004 | `0004-arm64-dts-amazon-add-community-UNVR-ea1a-UDM-Pro-ref.patch` | new ea1a UNVR + UDM-Pro reference DTS (not built) | COMMUNITY |
| 0005 | `0005-arm64-configs-add-unvr_defconfig.patch` | new `arch/arm64/configs/unvr_defconfig` | COMMUNITY (build asserts PCIE_AL* + toggles) |
| 0006 | `0006-al_sgpo-return-int-from-gpio_chip-.set-kernels-6.15.patch` | OOT `modules/al_sgpo/al_sgpo.c` (NOT the kernel tree) | OURS (1-line API fix); driver = COMMUNITY |

- 0001–0005 apply to the kernel tree (`git am` from its root).
- 0006 applies to the out-of-tree al_sgpo module source, not the kernel tree.

## What the port needs (why these patches)

- Every on-SoC block (both AHCI, al_eth, al_dma, al_ssm, xHCI) is a PCI endpoint
  on the **internal** PCIe bus. Mainline has no driver for
  `annapurna-labs,alpine-internal-pcie` → bus never enumerates → no SATA, no net.
  0001 fixes this (+ SMCC snoop / APP_CONTROL coherent-DMA glue).
- 0002 pre-sets DBI base so the external DWC PCIe (USB) probes without a resource
  conflict.
- 0003 = this unit's board (NAND boot, no eMMC) + gpio-hog to power SATA bays.
- Enclosure drivers (AHCI, i2c-dw, PCA954x mux, PCA953x gpio, ADT7475, S35390A)
  are all `=y` in 0005 → bind automatically once the internal bus enumerates.

## Out-of-tree drivers (vendored, built via `make -C <src> M=…`, NOT in kernel tree)

- `al_eth`, `al_dma`, `al_ssm`, `al_sgpo` — from the community port
  `github.com/bcyangkmluohmars/linux-alpine-v2` (6.12-era, vendor GPL HAL +
  community glue). Ancestry: al_eth ← delroth/al_eth-standalone; HAL ← Annapurna
  vendor GPL.
- `al_eth` / `al_dma` / `al_ssm`: built UNCHANGED against 6.12 / 6.18 / 7.1 — no
  patch. `al_sgpo`: one API fix (0006) for kernels ≥ 6.15.

## ours vs upstream (one line)

- OURS: pcie-al.c DBI-fix, ea16 DTS + gpio-hog, Kconfig/Makefile glue for
  PCIE_AL_INTERNAL, al_sgpo int-return fix. Everything else = pristine upstream
  kernel + community/vendor OOT drivers integrated unmodified.

## Rebase notes (base per series)

- **v6.18.44** — base. Apply 0001–0006 as-is.
- **v7.1.8** — apply 0001–0006 unchanged (byte-for-byte). Zero functional deltas
  vs 6.18. (pcie-al-internal.c is identical 6.18↔7.1; the pcie-al.c comment string
  says "6.18" — cosmetic only.)
- **v6.12.103** — apply 0002–0005 unchanged; **0001 applies but needs two API
  lines reverted to compile** (6.12 predates the 6.15 renames):
  - in `pcie-al-internal.c`: drop `#include "pci-host-common.h"` (6.12 declares
    probe/remove in `<linux/pci-ecam.h>`), and `.remove = pci_host_common_remove`
    → `.remove_new = …`.
  - **skip 0006** — 6.12 builds with gcc-12, which tolerates the void `.set`;
    al_sgpo needs no patch there.

## Apply

```
# kernel tree (from its root), base checked out at the matching tag:
git am /path/to/kernel-patches/000{1,2,3,4,5}-*.patch
# OOT al_sgpo source (>= 6.15 only):
cd <al_sgpo module src> && git am /path/to/kernel-patches/0006-*.patch
```

- Automated applier (does the copy-in + per-version adaptations + build):
  `scripts/build-linux-612-ea16.py`, `scripts/build-linux-618-ea16.py`,
  `scripts/build-linux-71-ea16.py`. These reproduce the same net result as this
  series and are the tested path; the patches are the reviewable/portable form.
- Build/porting detail: `docs/linux-6.12-build.md`, `docs/linux-6.18-build.md`,
  `docs/linux-71-build.md`.
</content>
