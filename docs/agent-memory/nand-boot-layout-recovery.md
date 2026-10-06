---
name: nand-boot-layout-recovery
description: "woomera NAND layout + boot chain since #216 (awto-uboot@0x1300000, kernel on SSD); `run bootnand` is NOT a recovery path (#166)"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:22:29.550Z
---

Boot chain since #216 (2026-09-04): `stock U-Boot → NAND 0x1300000 (awto-uboot, raw, go) → ext4load /boot/uImage from SSD → Linux`.

NAND layout (erase block 256 KiB `0x40000`, page 4096 B; Linux does not see NAND, only U-Boot reads it):
- mtd8 al_boot 2 MB — erased
- mtd9 linux_kernel `0x300000` 16 MB — the "factory recovery" image. **NOT genuine stock, does not boot** (#166). See [[genuine-stock-firmware-wont-boot]].
- `0x1300000` 1 MiB — **awto-uboot**. Was the Fedora kernel slot before #216; a kernel flashed here now destroys the bootloader.
- Kernel + DTB live on the SSD (`/boot/uImage`, `/boot/unvr.dtb`), not in NAND.
- rootfs = SSD `root=PARTUUID=dcdc291e-9956-48cd-9d7c-48219877881a` (ext4).

**Deploy** = `./dev.py publish-fedora` then `./dev.py deploy-ssd [--reboot]` — two separate commands, never chained (an scp once raced the file being written → Bad Data CRC). `deploy-ssd --reboot` verifies the kernel banner on return (#258).

**Recovery** if it will not boot: `<Esc><Esc>` at stock, then netboot from stock. **`run bootnand` is NOT a recovery path.** `./dev.py flash` (NAND) is recovery-only and overwrites awto-uboot; it refuses without an explicit flag. Pristine MTD backups: `git_debris/woomera-mtd/...-164356/`.

**Why:** three memories and CLAUDE.md all carried the pre-#216 layout and the bootnand claim for weeks after both changed; a recovery path you only discover is wrong when you need it is worse than none.
**How to apply:** before flashing or recovering, check the chain above, not the address a script comment names. See [[test-every-change-never-assert]], [[kernel-module-mismatch-recurring]].
