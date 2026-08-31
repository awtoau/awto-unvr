# Multi-connection manager — design plan (serial + 1G + 10G)

Design only — no `dev.py`/tooling code written yet, no hardware touched. Scope:
bring awto-l8's tio lifecycle discipline to awto-unvr's three simultaneous
connections to woomera: serial console, SSH over `enp0s1` (1G), SSH over
`enp0s2` (10G).

## 1. What awto-l8 actually does (reference architecture)

Package: `awto-l8/scripts/tio/` (11 modules, ~3800 lines), driven by
`awto-l8/scripts/dev.py`.

**One terminal WINDOW per board, supervised, not daemonized.**
`tio/mon.py:1-100` docstring: `tio -S` only multiplexes onto its socket when it
owns a real tty — detached with stdin on a pipe it logs "Non-interactive mode
enabled" and the socket carries zero bytes. So the console lives inside a
terminal-emulator window (`ctl.py:82-87` tries ptyxis/gnome-terminal/konsole/
xfce4-terminal in order), and `mon.py` runs *inside* that window as a
**supervisor loop**, not a one-shot wrapper (`mon.py:570-683`):
- spawns `tio` as a child, arms `PR_SET_PDEATHSIG(SIGTERM)` on it so the kernel
  kills it the instant the wrapper dies — even on SIGKILL/OOM (`mon.py:43-59,
  135-163`). This is what stops an orphaned `tio` outliving its manager (one
  ran 22 hours before this fix).
- listens on a control FIFO (`tmp/tio_ctl.<key>.fifo`) for `release` / `resume`
  / `quit`. `release` SIGTERMs the tio child and parks (port free, window still
  open); `resume` spawns a fresh tio on the *same* tty. Scrollback, window
  position and focus survive because the wrapper process never exits
  (`mon.py:62-90`). Used when a board is flashed over the same USB device its
  console uses (ESP32-S3): `dev.py:646-667` `release_esp_console()`/
  `reacquire_esp_console()` call `ctl.release()`/`ctl.reacquire()`, which talk
  the control FIFO (`ctl.py:463-517`).
- re-claims `TIOCEXCL` on the port every 5 s while running (`mon.py:401-404,
  662-666`), via `port_guard.py:71-104` `claim_exclusive()` — a Linux tty is
  not exclusive unless a holder asks, so a second `open()` from some other
  script would otherwise silently steal bytes and rewrite termios with no
  error anywhere (`port_guard.py:1-21`). `port_guard.py:125-166`
  `guarded_open()`/`refuse_if_held()` is the one place any script is meant to
  open a monitored port — it raises `PortHeld` instead.

**State machine — `tio/health.py:338-417` `classify()`, pure function over
`/proc` FD evidence, wrapped by `ctl.py:173-202` `state_of()`:**

| state | meaning |
|---|---|
| `running` | healthy: tio writing, wrapper draining into the clean log |
| `parked` | window open, port released, waiting for `resume` |
| `orphan` | tio streaming but nothing is recording it (wrapper gone) |
| `wedged` | reader alive but not draining — bytes stuck in the FIFO |
| `stale-socket` | socket file left behind by a dead monitor |
| `stale-device` / `device-gone` | tio still holds a device node udev replaced after a reset/re-enumeration (`health.py` `classify_device()`, ~580-615) |
| `stopped` | nothing running — the rest state |

Each state maps to exactly one remedy string (`ctl.py:208-225` `REMEDY` dict) —
`repair`, `start`, or "report it" for `wedged` (the window holds the only copy
of what it captured, so it's never touched). There is **no `restart` verb** —
`ctl.py:20-28`: a live monitor is never restarted; tests enforce this (#854).

**Lease model — `tio/send.py:147-176, 184-230`.** `tmp/console_down.<key>`
holds `when` / `reason` / `owner=<pid or none>` / `expires=<iso or never>`.
`Lease.state` derives `held` / `leaked` / `expired` from that data alone: owner
pid gone → `leaked`; past `expires` → `expired`; neither → `held`
(`send.py:159-166`). Leases this process took are released via `atexit`
(`send.py:179-253`), and `install_lease_release()` turns SIGTERM/SIGHUP into a
`SystemExit` so atexit still fires — a bare SIGTERM otherwise skips it, which
is exactly how a leaked lease happened before (#854). `dev.py:817-830` shows
the pattern in use: `ESP_DEPLOY_LEASE_S = 1550.0` bounds the whole flash
window, 1.25x its own sub-timeouts summed, and the lease is cleared in a
`finally` after the console is put back (`dev.py:823-830`).

A missing socket is diagnosed from this evidence, never guessed —
`send.py:298-345` `diagnose_missing_socket()` reads the lease first (held /
leaked / expired have different remedies), then falls back to `health.probe()`
for orphan/broken, then "no monitor — start one". Raised as `TioUnavailable`
(`send.py:278-296`) with `.kind` (`flashing | stale-lease | orphan |
no-monitor | stale-socket`) so a caller can tell "wait, it's coming back" from
"nothing is running" (distinct exit codes in `send.py:609-625`, `main()`).

**The one programmatic channel — `tio/send.py` `TioSession`
(`send.py:350-419`).** Connects to the tio `-S` unix socket only — never taps
the tty, never reads the log file programmatically. `send.py:10-26` explains
why: a split transport (write to tty, read from log) lost hours when an
orphaned tio kept streaming into a FIFO nobody drained — the send succeeded,
the read silently returned nothing, and a healthy board looked dead (#491).

**Multi-target — `dev.py:696-718` `start_tio_parallel(targets)`.** One window
per target (`sam`, `erin`, `con` optionally), each independently in whatever
state `ctl.state_of()` reports; a healthy one is left alone, anything else is
reaped then restarted. `ctl.py:576-585` `ensure(targets)` is the "make it so"
call `dev.py` runs on every invocation — nobody has to type a tio command by
hand for the normal path. This is the closest existing analogue to awto-unvr
needing 3 simultaneous connections instead of 1 board's console.

**Background health sweep — `tio/watch.py:1-97`.** Runs every target through
`health.probe()` + `health.probe_device()` + the lease read every `POLL_S =
5.0` s (cheap, no console traffic). Silence is deliberately **not** the alarm
signal (`watch.py:14-27`): a proof-of-life CR is only sent once a board's last
proof is older than `PROOF_INTERVAL_S = 60.0`, and a state must survive
`CONFIRM_CYCLES = 2` sweeps before it alarms (transient conditions self-heal
inside one sweep; a real fault — #672 — persisted 25 minutes unnoticed before
this existed). `ALARM_STATES` deliberately excludes `no-monitor` and a live
lease: **nobody watching a console is a decision, not a fault**
(`watch.py:89-93`). The watcher never remediates — it says what broke and the
fix command, because the bench is shared and a watcher that restarts monitors
fights whoever is using them (`watch.py:40-42`).

## 2. What awto-unvr has today, and the gaps

Everything lives directly in `dev.py` (no package) — `dev.py:16-22` states the
model: "ONE tio owns the serial port and exposes a unix socket + a plain log."

- `cmd_console` (`dev.py:248-304`): `setsid script -qec '<tio invocation>'
  /dev/null`, backgrounded. **`tio` is the top-level process** — there is no
  Python supervisor wrapping it, so none of `mon.py`'s guarantees apply: no
  `PR_SET_PDEATHSIG`, no control FIFO, no release/reacquire, no re-claimed
  `TIOCEXCL`.
- Liveness is `_console_pid()` (`dev.py:197-216`): `pgrep -x tio` filtered by
  matching the socket name in `/proc/<pid>/cmdline`. **Binary** — running or
  not. No orphan/wedged/stale-device classification: if the CP2102 by-id node
  got replaced by a re-enumeration while tio holds the old fd, nothing
  detects it (the #672 failure class, unguarded here).
- `cmd_console_attach` (`dev.py:390-415`): spawns `socat
  -,raw,echo=0,escape=0x1d UNIX-CONNECT:$SOCK` via **blocking**
  `subprocess.call` in the foreground — correct for a human sitting at a
  terminal (Ctrl-] detaches, tio keeps running). **Nothing tracks the attach
  client's PID.** `cmd_console_status` (`dev.py:359-374`) reports the `tio`
  pid/socket/log only — it has no visibility into attach clients at all. This
  is the concrete gap this session hit: an earlier `console-attach` call's
  `socat` was still running hours later, found by hand. There is no
  `conn status`-equivalent that would have shown it, and no `stop`-equivalent
  to reap it short of `pkill socat` by hand.
- No lease concept anywhere in this repo — `grep -rl "console_down\|
  mark_console_down"` over `scripts/` and `dev.py` returns nothing. Not
  currently load-bearing (nothing today contends with tio for the UART), but
  becomes load-bearing the moment any script wants the port for something
  else (a future firmware flash over the same USB-serial adapter, or a
  deliberate "hold serial down during a power-cycle" window).
- The one existing partial cross-reference to a "down for a reason, tell
  callers why" pattern is `scripts/_power.py`'s `power_cycle_verified()` —
  it *waits and verifies* a state transition but does not publish a lease file
  another script could read; a script calling `_console.sh()` mid-power-cycle
  just gets a socket-read timeout with no explanation.
- `scripts/_console.py` (`_console.py:1-99`) is the one existing "programmatic
  channel" analogue to `TioSession` — `connect()`/`login()`/`sh()` against the
  same tio socket, RC-fenced (`sh()` appends `; echo @@RC=$?@@`). Sound
  design, same "one channel" principle as `send.py`, just serial-only and with
  no lease-aware diagnosis on failure (a missing socket raises a bare
  `FileNotFoundError`, not a `TioUnavailable`-style explained failure).
- **No SSH connection management exists at all** — every network touch is
  ad hoc:
  - `scripts/ssh-woomera.py` (`ssh-woomera.py:1-126`): MAC-OUI LAN discovery +
    `tmp/woomera-addr` cache (single address, no per-NIC distinction), then a
    fresh `os.execvp("ssh", …)` per invocation — no connection reuse.
  - `scripts/test-eth.py` (`test-eth.py:55-90, 253-300`): `sshpass` + plain
    `ssh` per remote command, no ControlMaster, no persistent session.
  - `scripts/eth-watch.py` (`eth-watch.py:1-40`): the closest thing to a
    *persistent* SSH session in this repo — one long-lived `ssh` process
    piping a remote poll loop back to stdout — but it's a single ad hoc
    script with no PID file, no state file, dies on Ctrl-C only, and is not
    reusable by any other script.
- **The wrong-NIC trap is already a documented, real bug class**
  (`docs/network-throughput-testing.md:1-26`, incident #121): the box has
  **two separate DHCP leases, one per NIC** (`enp0s1`/1G, `enp0s2`/10G).
  "The box's IP" from host discovery answers whichever NIC responded to ARP
  first — not necessarily the one you meant to test — and even once you have
  the *right* box IP, Linux route selection on the dev host is
  destination-driven, so the wrong local NIC can still carry the traffic
  silently. `test-eth.py:253-300` already codes around this for one-shot
  iperf3 runs (`enp0s2_ip()` queries the live IP over SSH, `check_local_route()`
  reports which local NIC `ip route get` would actually use). **This
  correctness problem has no formal state today** — it is re-solved ad hoc,
  per script, with no shared "is this SSH connection actually on the NIC it
  claims" check.

## 3. Design: three connection kinds under one lifecycle model

Package: `scripts/conn/` (mirrors `awto-l8/scripts/tio/`), covering targets
`serial`, `1g` (enp0s1), `10g` (enp0s2).

### 3.1 Don't port the sanitizer — port the supervisor shape

`tio/mon.py`'s FIFO + `StreamSanitizer` pipeline exists to fix a specific
awto-l8 bug: fragmented-echo artifacts on the STM32/ESP consoles tripping
Anthropic's content classifier (#386, `mon.py:11-15`). awto-unvr's serial
console does not have that problem — `_console.py`'s direct `tio --log-file`
(no FIFO stage) is adequate and should stay. **Port the supervisor
(PR_SET_PDEATHSIG + control-FIFO release/resume + periodic TIOCEXCL
re-claim), not the sanitize stage.**

### 3.2 State machine — shared vocabulary, kind-specific detail

| state | serial (tio) | ssh (1g / 10g) |
|---|---|---|
| `running` | tio writing, socket live | ControlMaster socket answers `ssh -O check` |
| `parked` | port released, window/wrapper alive, port free | *(not used initially — no competing exclusive use of the NIC; see 3.5)* |
| `orphan` | tio streaming, wrapper gone | ControlMaster process gone but socket file remains |
| `stale-socket` / `stale-master` | socket file, dead monitor | socket file, `ssh -O check` refuses |
| `wedged` | FIFO backlogged, reader stuck | *(no direct analogue — a genuinely hung mux is rare; treat as `unreachable` after the confirm window instead of inventing a state nothing can observe)* |
| `stale-device` / `device-gone` | tio holds a udev-replaced by-id node | *(no analogue — SSH has no local device node)* |
| **`wrong-nic`** | — | **novel**: cached IP for this link no longer routes out the expected local NIC (formalizes the #121 trap — see 3.4) |
| `unreachable` | — | no route / refused / timeout to the cached IP — box off, this link's cable/SFP down, or DHCP lease drifted |
| `stopped` | nothing running | nothing running |

Every state still maps to exactly one remedy string, same `REMEDY`-dict
pattern as `ctl.py:208-225`. No `restart` verb here either, for the same
reason: a live console/session is the user's surface, and blanket-restart is
how a `stopped`-vs-`something's-actually-wrong` distinction disappears.

### 3.3 Lease model — ported near-verbatim

`tmp/conn_down.<key>` (`key` = `serial` | `1g` | `10g`), same fields as
`send.py:184-230` (`when` / `reason` / `owner` / `expires`), same
`held`/`leaked`/`expired` derivation, same atexit-release +
`install_lease_release()` SIGTERM/SIGHUP handling. Concrete uses in this repo:
- **Coordinated recovery windows.** `power_cycle_verified()`
  (`_power.py`) currently gives a caller no signal that a mid-cycle SSH/serial
  failure is *expected*. Wrapping it to `mark_console_down("serial", …,
  hold_s=…)` / same for `1g`/`10g` around the cut-restore-verify window means
  any script hitting a dead connection mid-cycle gets "recovering since
  <when>, expect it back" instead of a bare connection-refused that reads as
  a dead box.
- **A future firmware flash over the same UART** the console uses (no such
  path exists yet, but the awto-l8 ESP32-S3 case is exactly this shape) —
  `release()`/`reacquire()` plus a lease around the flash window is the
  direct port target for that day.

### 3.4 Novel: formalizing the wrong-NIC trap as a health state

No awto-l8 analogue — its boards each have one physical link. `conn/health.py`
for `1g`/`10g` should actively check, not just remember, which NIC an address
resolves through:
1. Read the link's own cached IP (`tmp/conn-addr.<1g|10g>`, populated the same
   way `test-eth.py:253-261` `enp0s2_ip()` does — query
   `ip -4 -o addr show enp0sN` over whichever connection is currently up,
   bootstrapping from `ssh-woomera.py`'s MAC-OUI discovery on first contact).
2. Before trusting `running`, run `ip route get <cached-IP>` **on the dev
   host** and confirm the selected local device matches the NIC this target
   is supposed to test through (mirrors `test-eth.py:262-295`
   `check_local_route()`, but as a first-class recurring health check instead
   of a one-shot script gate).
3. Mismatch → `wrong-nic`, with the remedy already documented in
   `docs/network-throughput-testing.md:19-21`: a temporary host route
   (`ip route add <IP>/32 dev <nic>`).

This makes "is the 10G session actually testing the 10G path" a status-line
answer instead of something re-derived by reading a doc every time.

### 3.5 SSH connection kind: ControlMaster, not a supervised window

SSH needs none of the window/PDEATHSIG machinery serial needs — there is no
human "watching" an SSH session the way they watch a serial pane, and no
device-exclusivity concern (multiple SSH sessions to the same host are normal,
unlike a second `open()` on a tty). Mechanism:

```
ssh -M -N -f -S tmp/ssh.<1g|10g>.sock \
    -o ControlPersist=<lease-driven, not a guess> \
    root@<link's cached IP>
```

- `-M -N -f`: establish a backgrounded ControlMaster, no remote command.
  Password auth (`sshpass`, per `test-eth.py:65-90` — no SSH key is set up for
  this box today) happens **once**, at master creation; every subsequent
  `ssh -S tmp/ssh.<key>.sock root@<ip> <cmd>` reuses the multiplexed
  connection with no further auth cost. This is the direct SSH analogue of
  tio's `-S unix:…` socket.
- Liveness: `ssh -O check -S tmp/ssh.<key>.sock root@<ip>` — the ControlMaster
  equivalent of `tio_health.probe()`'s FIFO-topology check.
- `conn/send.py`'s `SshSession` (parallel to `TioSession`) wraps
  `ssh -S <sock> root@<ip> <cmd>`, folding in `_console.py`'s login/PS1/RC-fence
  pattern is unnecessary here — a plain SSH command already returns a real
  exit code, so the RC-fence hack (needed only because tio has no concept of
  command boundaries) does not apply. This is strictly simpler than the
  serial channel.
- `release`/`reacquire` are **not needed initially** for 1g/10g — nothing
  competes with SSH for the NIC the way esptool competes with tio for the
  USB-JTAG port. Design the lease file format to support it (3.3) in case a
  future need appears (e.g. a link-flap test that wants to own "known-down"
  state), but don't build release/reacquire plumbing before there's a caller.

### 3.6 `dev.py` surface

Keep every existing serial command (`console`, `console-own`,
`console-status`, `console-stop`, `console-attach`, `console-send`, `check`)
working unchanged — they become the `serial`-target special case, not
replaced. Add a new `conn` verb family for the cross-cutting operations that
don't fit the serial-specific names:

```
./dev.py conn status [serial|1g|10g|all]     # one line per target + remedy
./dev.py conn start  [serial|1g|10g|all]     # ensure() — idempotent
./dev.py conn stop   [serial|1g|10g|all]
./dev.py conn send   <target> "<cmd>" [timeout]   # dispatches to _console.sh()
                                                    # or SshSession per target
./dev.py conn watch                           # background sweep, Phase 3
```

`conn status all` is the direct answer to this session's actual need
tonight — "is 1G up, is 10G up, is serial up, right now, in one call" instead
of three different ad hoc probes.

### 3.7 Fixing the leftover-socat gap specifically

This is a small, independent fix — do it whether or not the rest of this plan
proceeds. `cmd_console_attach` should write `tmp/conn-attach.serial.<pid>.pid`
(one file per attach client, since `tio -S` multiplexes many clients
concurrently — plural by design, not a single slot) on spawn and unlink it on
exit; `conn status serial` (or `console-status`, extended) lists any that are
still present with their age, and a new `conn attach-stop [pid]` SIGTERMs
them. This alone would have surfaced tonight's stray `socat` in the very next
`console-status` call instead of requiring a manual `ps` sweep hours later.

## 4. What's ported vs genuinely novel

**Ported near-verbatim:**
- Lease file format + `held`/`leaked`/`expired` derivation (`send.py:147-176,
  184-230`).
- State-to-single-remedy dict pattern, no blanket `restart` verb
  (`ctl.py:208-225`, `ctl.py:20-28`).
- Watch-loop shape: silence-is-not-signal, proof-of-life probing, N-cycle
  confirm before alarm, never remediates (`watch.py:14-42`).
- Supervisor shape for serial: `PR_SET_PDEATHSIG` + control-FIFO
  release/resume + periodic `TIOCEXCL` re-claim (`mon.py`, `port_guard.py`) —
  **not** the FIFO+sanitizer log pipeline, which is awto-l8-specific (3.1).
- One-channel principle: never split "write here, read there" — `send.py`'s
  `TioSession` and the new `SshSession` both connect once and do both
  directions on the same socket.

**Genuinely novel (no awto-l8 equivalent):**
- SSH ControlMaster as a connection kind — simpler than serial (no window, no
  device exclusivity, no release/reacquire needed yet).
- `wrong-nic` as a first-class, actively-checked health state (3.4) — awto-l8
  has one physical link per board, so this problem class does not exist there.
- Per-NIC address caching (`tmp/conn-addr.<1g|10g>`) as a health-relevant
  artifact, not just a discovery convenience (`ssh-woomera.py`'s single
  `tmp/woomera-addr` cache has no per-NIC concept today).

## 5. Rollout order

1. **Leftover-socat fix (3.7)** — standalone, no dependency on anything else.
2. **Serial supervisor** — port `mon.py`'s shape onto `cmd_console`, gaining
   orphan/stale-device detection and release/reacquire plumbing (even with no
   caller yet) for free.
3. **SSH ControlMaster kind** for `1g`/`10g`, with `wrong-nic` detection from
   day one — that's the part actually motivated by tonight's session.
4. **Lease model wired into `power_cycle_verified()`** — recovery windows
   stop reading as ambiguous failures on whichever connection is mid-cycle.
5. **`conn watch`** background sweep, once there's more than one caller
   wanting "tell me when something breaks" rather than polling `conn status`.

Phases 3-5 are the actual "1G and 10G vconnections" ask; phases 1-2 are
prerequisite hygiene that phase 3 would otherwise have to duplicate per link.
