# Native on-box builds (woomera)

Builds `modules/al_eth` natively on woomera itself instead of cross-compiling
on the dev host + rsyncing modules over. Faster iteration once network is
stable (1G confirmed working), avoids toolchain drift between cross/native
compilers.

## One-time setup (already done as of 2026-08-20, redo if the box is reflashed)

- Build tools: `dnf install -y gcc make git rsync bc flex bison openssl-devel
  elfutils-libelf-devel ncurses-devel perl dwarves`
- Kernel source tree at `/root/src/linux-v7.1.8` (rsynced from the dev host's
  `/mnt/2tb/unvr-port-refs/linux-v7.1.8`, 28GB - includes `.git`, `.config`,
  `Module.symvers`).
- **`git config --global --add safe.directory /root/src/linux-v7.1.8`** -
  required. Without it, git's ownership-safety check silently blocks
  `scripts/setlocalversion`'s dirty-tree detection when building as root over
  files owned by a different UID, and the built module's vermagic comes out
  as `7.1.8` instead of `7.1.8-dirty` (matching the running kernel) - `insmod`
  then correctly refuses to load it.
- Repo at `/root/awto-unvr` (rsync of `git ls-files`, kept in sync by
  `scripts/build-on-box.py`).

## Usage

```
./scripts/build-on-box.py            # rsync repo + native build + modules_install + depmod
./scripts/build-on-box.py --no-sync  # skip the rsync step (repo already current on box)
./scripts/build-on-box.py --reload   # ...then live rmmod+insmod (see warning below)
```

Default mode installs to `/lib/modules/7.1.8-dirty/updates/al_eth.ko` and runs
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
