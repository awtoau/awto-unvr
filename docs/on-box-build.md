# Native on-box builds (woomera)

Builds `modules/al_eth` natively on woomera itself instead of cross-compiling
on the dev host + rsyncing modules over. Faster iteration once network is
stable (1G confirmed working), avoids toolchain drift between cross/native
compilers.

## One-time setup (redo whenever the box's running kernel changes, or after a reflash)

`build-on-box.py` auto-detects the staged tree (globs `/root/src/linux-v*`,
excluding the `linux-alpine-v2` community reference clone) rather than
hardcoding a version - it previously pointed at a hardcoded `linux-v7.1.8`
that went stale and silently broke at the next kernel migration. The
detection itself doesn't remove the need for the setup below: it still
fails loudly (missing `.config`/`Module.symvers`, or a vermagic mismatch
against the running kernel) if this hasn't been done for the *current*
kernel.

- Build tools: `dnf install -y gcc make git rsync bc flex bison openssl-devel
  elfutils-libelf-devel ncurses-devel perl dwarves`
- Kernel source tree at `/root/src/linux-v<ver>` (rsynced from the dev host's
  matching `/mnt/2tb/unvr-port-refs/linux-v<ver>-*` tree, ~28GB - must include
  `.git`, `.config`, `Module.symvers` from a build that matches the box's
  currently running kernel exactly).
- **`git config --global --add safe.directory /root/src/linux-v<ver>`** -
  required. Without it, git's ownership-safety check silently blocks
  `scripts/setlocalversion`'s dirty-tree detection when building as root over
  files owned by a different UID, and the built module's vermagic comes out
  missing the `-dirty` suffix the running kernel actually has - `insmod`
  then correctly refuses to load it.
- Repo at `/root/awto-unvr` (rsync of `git ls-files`, kept in sync by
  `scripts/build-on-box.py`).

## Usage

```
./scripts/build-on-box.py            # rsync repo + native build + modules_install + depmod
./scripts/build-on-box.py --no-sync  # skip the rsync step (repo already current on box)
./scripts/build-on-box.py --reload   # ...then live rmmod+insmod (see warning below)
```

Default mode installs to `/lib/modules/$(uname -r)/updates/al_eth.ko` and runs
`depmod` - takes effect on the next `modprobe`/reboot, does NOT touch the
currently loaded module. Vermagic is checked and the run aborts if it doesn't
match the running kernel, rather than risking a mismatched module.

**`--reload` bounces enp0s1/enp0s2 live and is genuinely risky** - #115: a
plain `rmmod al_eth` leaks a `mdio_bus` sysfs kobject that blocks every
subsequent reload until a full reset. Only use `--reload` for a deliberate
hardware test with the console available for recovery, never for routine
iteration.

## No persistent clock (#114)

The box boots to epoch (1970-01-01) every time - no RTC (intentionally
disabled, #98/rtc-s35390a-fault.md) and `systemd-timesyncd` refuses to start
(`ConditionVirtualization=!container` on this Fedora Container-Image rootfs).
`build-on-box.py` sets the clock from the dev host's own clock before every
build to avoid spurious "Clock skew detected" warnings and TLS cert failures
(`dnf` fails outright on an epoch clock). This is a per-boot workaround, not a
fix - see #114 for the durable fix (NTP override or similar).
