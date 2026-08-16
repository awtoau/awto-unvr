# UNVR (Alpine V2 / AL-324, ea16, "woomera") — improvements audit

Scope: audit of the running 7.1.8 kernel config
(`build-out-71/unvr-ea16-7.1.config`), netboot bootargs, and the Fedora-on-SSD
plan (`docs/fedora-on-ssd.md`). Every finding is issue-ready: title, body
bullets, priority, exact symbol/setting. **[Fedora-milestone]** tags findings
that block or materially help stock Fedora-on-SSD boot.

Bootargs audited:
`console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 reboot=warm rw iommu.passthrough=1 pci=pcie_bus_perf panic=15`

Key hardware facts driving this: no cpufreq driver for AL-324 in mainline (cores
fixed at 1.7 GHz via PLL strap); no SMMU/IOMMU enabled; al_ssm gives async
AES-XTS/CBC hardware crypto; al_dma exposes 4 XOR/PQ channels to async_tx; SoC
`al_thermal` custom driver is NOT built (only al_eth/al_dma/al_ssm/al_sgpo are).

---

## Dimension 1 — Kernel config gaps vs the hardware

### 1.1 Enable ARM64 CE SHA/CRC crypto acceleration — High
- Only `AES_ARM64_CE_BLK` + `GHASH_ARM64_CE` are on. **No hardware SHA**: `CRYPTO_SHA256`/`SHA512` are the *software* generic versions; the A57 CE SHA extensions are unused.
- NAS hits SHA constantly: btrfs/dm-verity checksums, TLS, SMB3 signing, ssh, dnf. CE gives multi-x speedup.
- Set: `CONFIG_CRYPTO_SHA1_ARM64_CE=y`, `CONFIG_CRYPTO_SHA2_ARM64_CE=y`, `CONFIG_CRYPTO_SHA512_ARM64_CE=y`, `CONFIG_CRYPTO_CRCT10DIF_ARM64_CE=y`, `CONFIG_CRYPTO_POLYVAL_ARM64_CE=y`.

### 1.2 No zram/zswap on a 4 GiB box — Med
- `CONFIG_ZRAM` and `CONFIG_ZSWAP` both unset. 4 GiB is tight once Samba/containers/md run; compressed swap avoids early OOM without touching the SSD.
- zstd/lz4 compressors already built (`CRYPTO_ZSTD=y`, `LZ4_*`). zram is a cheap module.
- Set: `CONFIG_ZRAM=m` (+ `ZRAM_DEF_COMP_ZSTD`), optionally `CONFIG_ZSWAP=y`. Ship `zram-generator` in Fedora rootfs.

### 1.3 XFS not built; BTRFS lacks POSIX ACL — Med [Fedora-milestone-adjacent]
- `CONFIG_XFS_FS` unset. `BTRFS_FS=m` but `BTRFS_FS_POSIX_ACL` unset — ACLs are standard for a file server.
- Data array on 2×8 TB wants a real NAS fs. Root is ext4 (boots fine), but the array needs xfs or acl-enabled btrfs.
- Set: `CONFIG_XFS_FS=m`, `CONFIG_BTRFS_FS_POSIX_ACL=y`. (exfat/ntfs3 for USB interop = Low: `CONFIG_EXFAT_FS=m`, `CONFIG_NTFS3_FS=m`.)

### 1.4 al_thermal (SoC junction sensor) not ported → no CPU-die temperature — Med
- Only al_eth/al_dma/al_ssm/al_sgpo are built. The custom `al_thermal` (`annapurna-labs,al-thermal` @ `0xfd860a00`) is absent, so there's no SoC/CPU die temp — only the ADT7475 board/fan-adjacent sensors over I2C.
- Matters if OC is ever applied (1.6): no die-temp feedback. `al_thermal-standalone` exists for modern kernels.
- Action: port `al_thermal` as an out-of-tree module alongside the al_* set (issue).

### 1.5 CPU idle limited to plain WFI — Med
- `CONFIG_CPU_IDLE=y` but `CONFIG_ARM_PSCI_CPUIDLE` unset → cores only get arch-default WFI, no deeper PSCI idle states → higher idle power on an always-on box.
- Set: `CONFIG_ARM_PSCI_CPUIDLE=y` (verify AL-324 PSCI exposes idle states in DT; if none, no-op — harmless).

### 1.6 SATA drive-temp hwmon not built — Low
- `CONFIG_SENSORS_DRIVETEMP` unset → no `hwmon` temp for the WD/SSD (smartctl still works, but drivetemp integrates with `sensors`/monitoring/thermal).
- Set: `CONFIG_SENSORS_DRIVETEMP=m`.

### 1.7 NVMe genuinely N/A — Low (document, don't enable)
- `BLK_DEV_NVME` unset. Board has no M.2/NVMe; the one external PCIe Gen2 x1 link carries the ASMedia xHCI. No free slot. Leave off; note in docs so it isn't re-raised.

### 1.8 Strengths to keep (no action)
- md RAID + offload already correct: `MD_RAID456=m`, `ASYNC_TX_DMA=y`, `ASYNC_XOR/PQ/RAID6_RECOV=m`, `RAID6_PQ=m` → md5/6 parity can offload to al_dma's 4 channels.
- `DM_CRYPT=m` + al_ssm async AES-XTS → full-disk encryption offloads to the SSM engine (a real strength; wire LUKS to it).
- Watchdog `ARM_SP805_WATCHDOG=y` + `WATCHDOG_HANDLE_BOOT_ENABLED=y`; ADT7475 `SENSORS_ADT7475=y`; NFS/NFSD/CIFS/SQUASHFS/OVERLAY_FS/FUSE all present; `SATA_MOBILE_LPM_POLICY=3` (ALPM on).

---

## Dimension 2 — Fedora 44 / systemd requirements [Fedora-milestone]

Cgroup base is correct: `CGROUPS=y`, `MEMCG=y`, `MEMCG_V1` **off** (pure v2
unified), `BLK_CGROUP`, `CGROUP_SCHED/PIDS/FREEZER/DEVICE/CPUACCT`, `CPUSETS`,
all namespaces incl `USER_NS`, `SECCOMP_FILTER`, `FANOTIFY`, `INOTIFY_USER`,
`SIGNALFD/TIMERFD/EPOLL/FHANDLE`, `TMPFS_POSIX_ACL/XATTR`, `DEVTMPFS_MOUNT`,
`AUTOFS`… — boot will come up. Gaps that degrade/break specific Fedora defaults:

### 2.1 systemd-oomd will not start — no PSI — Med [Fedora-milestone]
- `CONFIG_PSI` unset. Fedora enables `systemd-oomd` by default; it hard-requires PSI and fails to start without it. Also loses pressure metrics for NAS monitoring.
- Set: `CONFIG_PSI=y` (optionally `PSI_DEFAULT_DISABLED=y` to keep overhead off until asked).

### 2.2 cgroup device control + BPF degraded — Med [Fedora-milestone]
- `CONFIG_CGROUP_BPF` unset and `CONFIG_BPF_JIT` unset. On cgroup v2 there is no devices controller — systemd enforces `DeviceAllow=` via a BPF `cgroup/device` program. Without `CGROUP_BPF`, device sandboxing silently no-ops; without JIT, all eBPF is interpreted (slow).
- Set: `CONFIG_CGROUP_BPF=y`, `CONFIG_BPF_JIT=y`.

### 2.3 AUTOFS present but BINFMT_MISC missing — Low [Fedora-milestone]
- `CONFIG_BINFMT_MISC` unset → `proc-sys-fs-binfmt_misc.mount`/`.automount` and any qemu-user/.NET registration fail (non-fatal, logs errors).
- Set: `CONFIG_BINFMT_MISC=m`.

### 2.4 No BTF → bpftrace/BCC/libbpf CO-RE unavailable — Low
- `PAHOLE_VERSION=0`, no `DEBUG_INFO_BTF`. Fedora's eBPF tooling (bpftrace, bcc) needs kernel BTF. Not a boot issue; a diagnosability gap on the self-hosting dev box.
- Set: `CONFIG_DEBUG_INFO_BTF=y` (needs `pahole`/dwarves in the build env).

### 2.5 HUGETLBFS off — Low
- `CONFIG_HUGETLBFS` unset → `dev-hugepages.mount` fails (non-fatal). Enable if any DB/qemu workload is planned: `CONFIG_HUGETLBFS=y`.

---

## Dimension 3 — Boot/perf tuning

### 3.1 `iommu.passthrough=1` is a dead bootarg — Low (cleanup)
- No IOMMU is built (`ARM_SMMU`/`ARM_SMMU_V3` both unset), so `iommu.passthrough=1` does nothing. Harmless but misleading. Drop it, or enable the SMMU only if device isolation is ever wanted (costs DMA perf — not for a NAS).

### 3.2 CPU governor question is moot — document — Med
- No `CONFIG_CPU_FREQ`, no mainline AL-324 cpufreq driver → cores are **fixed at 1.7 GHz** by PLL strap. "performance vs schedutil" does not apply. The only compute lever is the overclock (Dimension 6). State this in docs so it isn't chased.

### 3.3 10G TCP: no BBR, no fq qdisc — Med
- `CONFIG_TCP_CONG_ADVANCED` unset → only CUBIC. `NET_SCH_FQ` unset (only fq_codel). BBR+fq materially improves single-flow 10 G throughput/latency.
- Set: `CONFIG_TCP_CONG_ADVANCED=y`, `CONFIG_TCP_CONG_BBR=y`, `CONFIG_NET_SCH_FQ=y`; then Fedora sysctl `net.core.default_qdisc=fq`, `net.ipv4.tcp_congestion_control=bbr`.
- Pair with 10 G socket buffers (userspace, Fedora `sysctl.d`): `net.core.rmem_max=134217728`, `net.core.wmem_max=134217728`, `net.ipv4.tcp_rmem/wmem` tails to 128 MiB. BQL is already on (`CONFIG_BQL=y`).

### 3.4 I/O scheduler per device (SSD vs HDD) — Med
- `mq-deadline`, `kyber`, `bfq` all built. Set an explicit policy via Fedora udev rule: SSD (`sda`, `queue/rotational=0`) → `none` or `mq-deadline`; 8 TB HDDs → `mq-deadline` (or `bfq` for desktop-like fairness).
- Ship `/etc/udev/rules.d/60-ioschedulers.rules`.

### 3.5 Spinning-disk readahead + dirty tuning — Med
- 8 TB CMR platters want larger readahead and a byte-based dirty cap (not the default 20% of 4 GiB = ~800 MB burst). Userspace, Fedora `sysctl.d` + udev:
  - `blockdev --setra 4096` (2 MiB) on the WD drives via udev.
  - `vm.dirty_background_bytes=67108864`, `vm.dirty_bytes=268435456` to smooth write bursts.

### 3.6 `pci=pcie_bus_perf` — keep, verify — Low
- Sets MPS to max per link; reasonable for the fixed internal fabric. Confirmed working on hardware. Leave as-is; if any PCIe AER/MPS mismatch shows up, fall back to `pcie_bus_safe`.

### 3.7 Optional: `mitigations=off` for the trusted NAS — Med (perf vs security)
- `CONFIG_CPU_MITIGATIONS=y` + full A57 Spectre workarounds are active. On a single-tenant LAN NAS with no untrusted local code, `mitigations=off` recovers measurable syscall/context-switch cost. Explicit tradeoff — record the decision if taken.

### 3.8 THP is `madvise` — Low
- `TRANSPARENT_HUGEPAGE_MADVISE=y`. Fine default. Consider bootarg `transparent_hugepage=always` only if a large-anon workload (VM/DB) lands; not for pure file serving.

---

## Dimension 4 — Storage / RAID strategy

### 4.1 Recommend a concrete layout — Med
- Boot: SSD `sda2` ext4 (as planned) — keep. Root stays simple, fsck-able, no initramfs needed.
- Data: 2×8 TB as **mdadm RAID1 + XFS** (needs 1.3 `XFS_FS=m`) OR **btrfs raid1** (needs 1.3 ACL). Prefer btrfs-raid1 if you want checksums/scrub/snapshots; prefer md+xfs for raw throughput and simplicity. md RAID1 has no parity so al_dma offload is irrelevant here; al_dma XOR/PQ only pays off at RAID5/6 (3+ drives) — not applicable to 2 disks.
- Encryption: LUKS2 on the data disks with the al_ssm async AES-XTS engine (see 1.8) — near-free full-disk crypto.

### 4.2 SSD as cache is unavailable — Med (decide)
- `BCACHE`, `DM_CACHE`, `DM_WRITECACHE` all unset. If the SSD should also cache the HDD array (not just boot), enable one: `CONFIG_BCACHE=m` or `CONFIG_DM_CACHE=m` (lvmcache). For a 2-disk mirror the win is marginal; revisit only if the array grows to RAID5/6. Leave off for now, document.

### 4.3 ZFS — out of scope in-kernel — Low
- Not in mainline; would need OpenZFS DKMS against `7.1.8-dirty` (fragile against a non-release kernel). Not recommended over btrfs/xfs for this box.

---

## Dimension 5 — Thermal / fans / power

### 5.1 Thermal zone has no cooling device — fans not auto-managed by kernel — Med/High
- `THERMAL=y`, `GOV_STEP_WISE`, `THERMAL_OF=y` — but `CPU_THERMAL` unset, `PWM` subsystem unset, and al_thermal isn't built. Net: the thermal governor has **no cooling device** and no working zone → zero automatic thermal action from the kernel.
- Fan safety therefore depends entirely on the ADT7475: put it in **hardware automatic mode** (`pwmN_enable=2`) or run userspace `fancontrol` (lm_sensors, already installed). Without one of these, fans may sit at a fixed duty regardless of temp.
- Actions: (a) ship an lm_sensors `fancontrol` config, or set ADT7475 auto mode at boot; (b) longer-term, register the ADT7475 PWMs as thermal cooling devices bound to a real sensor. `THERMAL_EMERGENCY_POWEROFF_DELAY_MS=0` means no emergency poweroff — rely on ADT7475 THERM limits.

### 5.2 Wire the SP805 watchdog to systemd — Med (robustness)
- Driver is built and handles a boot-enabled timer, but nothing pets it in the Fedora userland yet. Enable auto-recovery.
- Fedora `/etc/systemd/system.conf`: `RuntimeWatchdogSec=30s`, `RebootWatchdogSec=2min`. Verify `/dev/watchdog` binds to sp805.

### 5.3 Idle power — bays can't be runtime-gated; CPU idle shallow — Low/Med
- DTB `gpio-hog` holds PCA9575 pwren lines output-high → bay power isn't runtime-controllable; `hdd-manage.py` can only `hdparm -Y` spin-down. To power-gate unused bays, drop the hog and drive the line via gpiod (tracked; risk = mis-gate a live bay). ALPM is already on (LPM policy 3).
- Combine with 1.5 (`ARM_PSCI_CPUIDLE`) for lower SoC idle draw.

---

## Dimension 6 — Overclock (#29, not applied)

### 6.1 Wire a conservative CPU overclock to ~2.0 GHz — Med
- Cores are at 1.7 GHz; vendor strap table goes to 2.7 GHz. With **no cpufreq**, this is the only compute lever — a flat ~18% uplift at 2.0 GHz for md parity, al_ssm-independent crypto, Samba, dnf, builds.
- Runtime path (recipe in `overclock-and-caps.md`): MMIO writes to CPU PLL `setup_0` @ `0xfd860d40` (target 2.0 GHz = `0x81030114`), poll lock @ `0xfd860d1c`. **Must do the bypass dance** (`setup_6 |= BYPASS` → change → clear+RELOCK → poll) because all 4 cores run from this PLL; a naive single write can glitch/hang.
- Risk: moderate, and **recoverable** — not persisted in OTP; a bad OC just needs a reboot. Gate it on die-temp feedback → depends on 1.4 (al_thermal). Prefer a small kernel/early-boot step over a loose devmem sequence.
- Recommendation: land 1.8–2.0 GHz as a togglable boot service; leave 2.4 GHz+ and any DRAM OC (boot-strap only, needs PHY retrain, can brick the boot) out of the milestone.

### 6.2 DRAM overclock — defer — Low
- DDR4-1866 → strap allows 2400/2600, but only at boot before training, via strap-pin/OTP (irreversible) and full CVOS retrain. High effort, low headroom on 4 GB parts binned for 1866. Not worth it for the milestone.

---

## Dimension 7 — Security / robustness

### 7.1 Default root password `unvr` + PermitRootLogin yes + password auth — High [Fedora-milestone]
- `build-fedora-rootfs.py` sets `root:unvr` and `PermitRootLogin yes`. A guessable root password reachable over SSH on the LAN is the single worst residual risk.
- Actions: force-change on first boot (or bake a strong one); install an SSH authorized_key and set `PasswordAuthentication no` + `PermitRootLogin prohibit-password`; consider a non-root admin user.

### 7.2 SELinux disabled — residual risk, note the re-enable path — Med
- Documented tradeoff (no dracut → no relabel path). Residual: no MAC confinement of sshd/Samba/containers. To re-enable later: boot once with a relabel initramfs or `touch /.autorelabel` + `enforcing=0`, then flip to enforcing. Track as a follow-up, not a milestone blocker.

### 7.3 Unsigned boot / physical console — accept, document — Med
- Boot chain is unsigned (#2) and `serial-getty@ttyS0` + open U-Boot mean physical UART access can `init=/bin/bash` past any login. Vendor U-Boot is frozen, so this is accepted; document that physical security is the boundary. Optionally set a U-Boot env password (frozen U-Boot may not support it).

### 7.4 Cheap hardening toggles are off — Low
- `SLAB_FREELIST_HARDENED`, `SLAB_FREELIST_RANDOM`, `BUG_ON_DATA_CORRUPTION`, `INIT_ON_FREE` all unset. `LSM=` lists `yama` but `SECURITY_YAMA` isn't built (silently ignored).
- Set (near-free): `CONFIG_SLAB_FREELIST_HARDENED=y`, `CONFIG_SECURITY_YAMA=y` (ptrace_scope). KASLR/stackprotector-strong/DMESG_RESTRICT are already on — good.

### 7.5 Milestone-reliability items (not classic security, but robustness)

#### 7.5a Root device naming is fragile — use PARTUUID — Med/High [Fedora-milestone]
- Plan uses `root=/dev/sda2`, which only holds while the USB stick is unplugged; any enumeration change misboots. With no initramfs, the kernel resolves `root=PARTUUID=` natively but **not** `LABEL=`/`UUID=` (those need udev). fstab already uses `LABEL=unvr-root`, so root= and fstab disagree in robustness.
- Action: set bootargs `root=PARTUUID=<sda2-partuuid> rootfstype=ext4 rootwait`.

#### 7.5b al_* modules must autoload without dracut — verify — Med/High [Fedora-milestone]
- No initramfs → module autoload relies on udev matching `modules.alias`. al_eth binds by PCI ID; if `al_eth/al_dma/al_ssm.ko` don't export `MODULE_DEVICE_TABLE(pci,…)` into `modules.alias`, **eth never comes up** (no NAS). al_sgpo/al_thermal are platform/DT.
- Action: after `depmod`, confirm the PCI aliases (`1c36:0001/0002/0022`) are in `modules.alias`; if absent, ship `/etc/modules-load.d/al.conf` listing all al_* modules explicitly.

---

## TOP 10 (ranked, across all dimensions)

1. **[High, Fedora]** Change default root creds / SSH policy — `root:unvr` + `PermitRootLogin yes` + password auth (7.1).
2. **[High]** Enable ARM64 CE SHA/CRC crypto accel — `CRYPTO_SHA2/SHA512/SHA1_ARM64_CE`, `CRCT10DIF_ARM64_CE` (1.1).
3. **[Med/High, Fedora]** Root by `PARTUUID=` + `rootwait`, not `/dev/sda2` (7.5a).
4. **[Med/High, Fedora]** Verify al_* PCI modaliases autoload without dracut; else `modules-load.d` (7.5b).
5. **[Med/High]** Fans: put ADT7475 in auto mode / ship fancontrol — kernel thermal zone has no cooling device (5.1).
6. **[Med, Fedora]** `CONFIG_PSI=y` so systemd-oomd starts (2.1).
7. **[Med, Fedora]** `CONFIG_CGROUP_BPF=y` + `CONFIG_BPF_JIT=y` for systemd device sandboxing + eBPF (2.2).
8. **[Med]** 10 G networking: `TCP_CONG_ADVANCED`+`BBR`+`NET_SCH_FQ` and matching sysctls (3.3).
9. **[Med]** `CONFIG_ZRAM=m` + zram-generator on the 4 GiB box (1.2).
10. **[Med]** Conservative CPU overclock to ~2.0 GHz as a togglable boot step (6.1), gated on porting al_thermal (1.4).

Runners-up: XFS/btrfs-ACL for the data array (1.3); SP805→systemd watchdog (5.2); per-device I/O scheduler + HDD readahead/dirty tuning (3.4/3.5); drop dead `iommu.passthrough=1` (3.1); cheap slab/YAMA hardening (7.4).
