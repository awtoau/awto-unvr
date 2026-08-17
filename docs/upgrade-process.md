# Upgrade process — the serial runbook WE USED

Reproducible steps for walking a stuck UNVR (Alpine V2 / AL-324, sysid 0xea16)
up the firmware ladder **1.3.35 → 5.1.25** over the serial console. This is what
was actually done on 2026-08-15, reconstructed from `tmp/logs/`.

Plan + release list: [upgrade-path.md](upgrade-path.md). fdt-rm / U-Boot detail:
[uboot-update-path.md](uboot-update-path.md), [nor-boot-chain.md](nor-boot-chain.md).
Logins: [credentials.md](credentials.md). Partition map: [nand-1.3.35.md](nand-1.3.35.md).

Scripts: `scripts/upgrade-ladder.py` (driver), `scripts/dump-unvr-mtd.py`
(backups), `scripts/tftpd.py` (transport).

## Prereqs

- **Serial console up**: 3-pin UART header behind the SFP+ cage (GND/TX/RX, **do
  not wire 3V3**), 115200 8N1. Driven through a `tio` unix socket
  (`/run/user/1000/tio-unvr.sock`); the ladder talks to that socket, not a raw tty.
- **TFTP transport**: `scripts/tftpd.py --root images/tftp --port 6969`. Device
  pulls with `tftp -b <blksize> -g` (not netcat — see nand-1.3.35.md).
- **USERDEV present** and root login working. Password **changes at the al324
  boundary** — `root:ubnt` on 1.3.35/1.4.9 (arm64), `root:ui` on 2.3.14…5.1.25
  (al324). The ladder tries `["ui","ubnt"]` in order. Cite [credentials.md](credentials.md).
- **Pre-captured 1.3.35 MTD dumps** exist and are off-box
  (`images/mtd/…-164103/`, `-164222/`) — cannot be remade once overwritten.

## Per-hop mechanism (`upgrade-ladder.py --go`)

Each hop, in order:

1. **Pre-hop MTD backup** — `dump-unvr-mtd.py --preset state --tag pre-<ver>` →
   `images/mtd/UNVR-<serial>-sysidea16-<ts>-pre-<ver>/`. Backup FAIL aborts the hop.
2. **Fetch + sha256** — pull the `.bin` from the firmware API's `_links.data.href`
   to `sources/`; verify against `sha256_checksum`; a cached copy is re-hashed, a
   bad one deleted and refetched.
3. **Stage over TFTP** — `mkdir -p /mnt/.rwfs/upgrade && rm -f …/fw-image.bin`,
   then device-side `tftp -g` into `/mnt/.rwfs/upgrade/fw-image.bin`.
4. **md5 on device** — `md5sum` the staged file, compare to the host's md5. Mismatch aborts.
5. **Quiet the console** — `dmesg -n 1` (emergency-only) so kernel spam doesn't
   swamp the flash output. Reverted implicitly by the reboot.
6. **Signature check** — see the per-generation gotcha below.
7. **Hand off to the stock path** — `sync; sync; reboot`. `mount_premount` finds
   `${MNT_RWFS}/upgrade/fw-image.bin` before UniFi OS starts, runs this
   generation's `upgrade_firmware` (fwsplit → `nandwrite`, incl. the 4-byte LE
   length prefix on the kernel partition), deletes the image, reboots. We do **not**
   drive `nandwrite` ourselves. Only kernel+rootfs are written — U-Boot, its env,
   and the DT are untouched.
8. **Confirm version** — after login returns, read `/usr/lib/version` (fallback
   `/etc/version`) and match the target.
9. **Post-hop backup** — `--tag post-<ver>`.

## Load-bearing gotchas (facts, keep these)

- **The spurious 5th NAND `device_tree` partition boot-loops everything > 1.4.9.**
  With 5 NAND partitions present, `mtd8=al_boot mtd9=device_tree mtd10=kernel
  mtd11=rootfs`. 1.4.9's initramfs (`KERNELDEV=/dev/mtd10 BOOTDEV=/dev/mtd11`)
  matches → boots. 2.3.14+ hardcode `mtd9/mtd10` → they mount the **kernel**
  partition as rootfs and panic:
  `Begin: Mounting /dev/mtd10 to /dev/ram0 … Kernel panic - not syncing: Attempted
  to kill init! … Rebooting in 5 seconds` — a ~5 s reboot loop (observed
  19:06–19:15, ~30 restarts after the 2.3.14 flash).
  - **Fix**: add `fdt rm /soc/nand-flash/partition@1` to `bootcmd` (drops the DT
    node → NAND has 4 partitions → `mtd9=kernel mtd10=rootfs`), then `saveenv`.
  - Applied volatile at the U-Boot prompt for the first 2.3.14 boot, made
    **persistent at 20:35–20:36**: saved `bootcmd_orig` first, set the new
    `bootcmd`, `saveenv` → *"Saving Environment to SPI Flash… Erasing… Writing… done"*.
    (saveenv writes mtd1 even though env CRC was previously bad/zeroed.)
  - Detail + the permanent U-Boot-reflash alternative: [uboot-update-path.md](uboot-update-path.md).
- **Forward-only. Never downgrade past 1.4.9 once the fdt-rm layout is in force** —
  1.4.9 reads rootfs from `mtd11`, which no longer exists under the 4-partition
  layout. The script refuses any downgrade.
- **Three false "failures" — all broken checks, not the box:**
  - `fwupdate -dc` on 1.3.35 exits **254 with no output** — v1.1 has no `-c` flag;
    reads exactly like a signature failure (stopped the 1.4.9 hop once). That is
    the 5.1.25 initramfs invocation; do not use it on old firmware.
  - `fwsplit -s /etc/ssl/unas.pub` on **al324** exits **255 + usage** because the
    key is not in the rootfs (it ships in the initramfs) — indistinguishable from a
    bad signature (stopped the 2.3.14 hop on a good image).
  - **Version-read after a good flash returns nothing** when the al324 password
    changed and login silently failed → looks like "did not come back within 600s"
    when the flash actually applied (cost a re-run on 1.4.9).
- **Console reads need a sentinel + login handling, not read-until-quiet.** Kernel
  log spam never goes quiet. Every command is `… ; echo __UL<n>__$?`; read until the
  sentinel; parse `$?` from it. Never send the sentinel form at a getty prompt — it
  gets typed as the username.
- **Verify-before-flash, per generation:**
  - 1.3.35 (has `/etc/ssl/unas.pub`): pre-verify with `fwsplit -s /etc/ssl/unas.pub
    -o /tmp/.fwchk <img>` (extract-to-tmpfs = verify; clean exit proves the sig;
    delete after — rootfs section ~300 MB).
  - al324 (no key in rootfs): **skip** the pre-verify; the initramfs verifies the
    signature with its own key before writing a block. Safety still holds:
    sha256 on download + md5 on device + initramfs signature check.

## Hop sequence actually taken (2026-08-15, AEST)

| Hop | Flashed | Booted OK | Pre-hop backup dir | Notes |
|---|---|---|---|---|
| 1.3.35 → **1.4.9** | 18:34 | 18:36 | `…-183100-pre-1.4.9/` | crosses arm64→al324; password now `ui`. First run mis-read version (login gotcha). |
| 1.4.9 → **2.3.14** | 19:03 | ~19:34 | `…-190009-pre-2.3.14/` | **boot-loop** → applied `fdt rm` at U-Boot to boot once. |
| 2.3.14 → **3.1.16** | 20:44 | 20:47 | `…-204005-pre-3.1.16/` | `fdt rm` made persistent (saveenv) at 20:35 before this hop. |
| 3.1.16 → **4.1.22** | 21:09 | 21:15 | `…-210511-pre-4.1.22/` | clean; `post-4.1.22/` at 21:17. |
| 4.1.22 → **5.1.25** | 21:20 | 21:26 | `…-211742-pre-5.1.25/` | clean. Final backup `…-post-5.1.25-final/` at 21:31. |

Extra U-Boot-env backup before touching env:
`…-203030-on-2.3.14-pre-uboot-env/` (holds the pristine `mtd05-u-boot-1835008B` and
zeroed env copies).

## Recovery levers

- **Catch U-Boot**: `scripts/catch-uboot.py` streams `<ESC><ESC>` to the tio socket;
  power-cycle the unit and it stops autoboot at the `ALPINE_UBNT_NAS_ALL>` prompt
  (used at 19:15, 19:23, 19:43, 20:34).
- **Initramfs shell**: `setenv bootargsextra reboot=warm break=premount` at the
  U-Boot prompt drops into the premount shell — inspect
  `/proc/device-tree/soc/nand-flash/partition@1`, mtd numbering, etc.
- **`bootcmd_orig`**: the original `bootcmd` is stored in that env var before the
  fdt-rm edit — `setenv bootcmd "$bootcmd_orig"; saveenv` reverts.
- **Bricked mtd0 (U-Boot)**: no software fallback — external SPI-NOR programmer
  restoring the saved `mtd05-u-boot-1835008B-*.img`. See uboot-update-path.md §3.
