# RPS interface specification — ea16 (UNVR)

Wire-level spec for talking to a **USP-RPS** from this board, derived by static RE of the
stock 5.1.25 `/sbin/rpsd`. Sufficient to drive the link without a logic analyser.

Companion: [rps-subsystem.md](rps-subsystem.md) (physical/board view, JB4, U122, power
path). This doc is the *protocol and software* half; it does not restate the board work.

## Provenance

- Binary: `sbin/rpsd` from `tmp/sections/03-rootfs.bin` (squashfs, 5.1.25, built 2026-07-10),
  AArch64 PIE, **stripped**, BuildID `211132b3843733e0d56d7d97428ffaeb655e511d`.
- Decompile: `scripts/rps-decompile.py` → `tmp/ghidra-out-rpsd/` (Ghidra 12.2_DEV, ELF loader,
  452 functions). Ghidra image base **0x100000**, so `vaddr = file_off + 0x100000`.
- Symbol recovery: `scripts/rps-func-names.py` — rpsd logs via `libubnt_log(lvl, tag, __func__, fmt…)`,
  so arg 3 of every log call is the enclosing function's real name. 64 functions named,
  44 unambiguous. Renamed source: `tmp/rps-spec/rpsd-named.c`; ledger `tmp/rps-spec/rpsd-names.tsv`.
- Pin/board tables: `scripts/rps-pin-table.py` → `tmp/logs/rps-pin-table.log` (210 descriptors).
- **Citations below are file offsets** unless prefixed `vaddr`. Line numbers refer to
  `tmp/rps-spec/rpsd-named.c` (regenerable from the two scripts above).
- **Decompiler artefact to expect:** several functions are exported twice at adjacent
  addresses 4 bytes apart (e.g. `FUN_0011875c`/`FUN_00118760`,
  `FUN_00107c7c`/`FUN_00107c80`, `FUN_00107fac`/`FUN_00107fb0`) — Ghidra split them at
  the stack-guard prologue. The bodies are identical; treat each pair as one function.

Inference is flagged **[inferred]** where it is not directly readable from the binary.

---

## 1. Identity and role

- Board matched by **sysid**, from `board_identify()` (`libubnt.so.1`).
- ea16's board descriptor: **file 0x377c0** — name `"UNVR/UNAS"`, desc
  `"Unifi NVR/NAS series RPS drivers"`, magic `'UNVR'`, sysid list @rodata 0x20010.
- **Sysid list is 5 entries: `ea16, ea1a, ea20, ea51, ea67`** then `0xffff` terminator
  (rodata 0x20010). All five share this one profile.
- Port config pointer → **file 0x28990**; port count **1**.
- **Role = PD (1)**, not PSE. Role enum table at file 0x2c960: `{0,"PSE"}, {1,"PD"}`.
  `rps_board_check` (line 14280) branches `role==0` → PSE checks, `role==1` → PD checks.
  ea16 is a power **consumer**: it asks the USP-RPS for power, it does not supply it.
- Role is reported to the peer as the string `"PSE"` / `"PD"`; the peer's role is parsed
  with `strcasecmp(role,"PD")` (line 2338).

### Cross-check against `libubnt.so.1.0.0`'s board table

`rpsd` gets the board struct from `board_identify()` in `libubnt.so.1`. The ea16 entry is
at **file 0x5fdf0** in `usr/lib/aarch64-linux-gnu/libubnt.so.1.0.0` (located by the unique
occurrence of `0xea16` at file 0x64398). Reading the offsets `rps_board_check` uses:

| board off | file | value | meaning |
|---|---|---|---|
| +0x45a8 | 0x64398 | `0xea16` | sysid |
| +0x46b8 | 0x644a8 | **1** | `rps.role` → **PD** |
| +0x46bc | 0x644ac | **0** | `rps.pd_power_req_max_12v` — **unset** |
| +0x46c4 | 0x644b4 | **1** | RPS port count |
| +0x46c8 | 0x644b8 | `{name:"", +0x20:1, +0x24:1, +0x28:0}` | port[0] defaults |

Two independent confirmations and one consequence:
- role = PD and port count = 1 match the `rpsd` board table exactly.
- `pd_power_req_max_12v` is **0 in the board table**, which is precisely why
  `rps_board_check` logs `"RPS PD required 12v power … not set"` and why
  `/sbin/rpsd_power_budget` must write `rps.pd.power_required_max_12v=150` into
  `/var/run/rpsd.conf` before rpsd is useful. On ea16/ea1a that script hardcodes
  `max_power=150`, `base_power=40`, `disk_power=15`.

### Port config struct (0xc0 bytes, file 0x28990)

| off | ea16 value | meaning |
|---|---|---|
| +0x00 | `1` | phy_id |
| +0x08 | `"phy-1"` | port name |
| +0x10 | `0x38a50` (file 0x28a50) | → pin descriptor table |
| +0x18 | `4` | npins |
| +0x20 | **all zero** | hwmon12v {compat, sysfs path, i2c addr, …} |
| +0x68 | **all zero** | hwmon54v {compat, sysfs path, i2c addr, …} |
| +0xb0 | `"uart-common"` | UART backend |
| +0xb8 | `"/dev/ttyS2"` | UART device |

Layout proven by diffing against a PSE profile at file 0x28b50, where +0x20 holds
`"hwmon-isl28022"` / addr `0x44` and +0x68 holds `"hwmon-ina230"` / addr `0x40`.
**On ea16 both hwmon slots are NULL** — see §7.

---

## 2. Pin table (file 0x28a50, 4 entries × 0x40 B)

Descriptor layout, confirmed against `rps_drv_pin_init` (vaddr 0x10d070, line 6418;
stride 0x40, bomrev gate reads `+0x30`/`+0x34`, backend name `+0x10`, location `+0x18`):

| off | field |
|---|---|
| +0x00 | pin id (`RPS_PIN_*` enum) |
| +0x08 | name string |
| +0x10 | backend "compatible" |
| +0x18 | lo32 = offset within chip, **hi32 = absolute gpio** |
| +0x20 | flags |
| +0x28 | backend-private (i2c bus/addr for expanders) |
| +0x30 | bomrev_min (u32) |
| +0x34 | bomrev_max (u32) |

### ea16 entries

| file | id | name | backend | gpio | flags | bomrev range |
|---|---|---|---|---|---|---|
| 0x28a50 | 3 | `RPS_PIN_RPS_PRNT` | gpiolib-sysfs | **33** | **0x5** | `[0x000b1011 .. 0xffffffff]` |
| 0x28a90 | 8 | `RPS_PIN_12V_LP` | gpiolib-sysfs | **34** | 0x1 | `[0x000b1011 .. 0xffffffff]` |
| 0x28ad0 | 3 | `RPS_PIN_RPS_PRNT` | gpiolib-sysfs | **33** | 0x1 | `[0x00000000 .. 0x000b1010]` |
| 0x28b10 | 8 | `RPS_PIN_12V_LP` | gpiolib-sysfs | **34** | 0x1 | `[0x00000000 .. 0x000b1010]` |

- **New finding: the ea16 pin table is BOM-rev gated at `0x000b1011`.** Two entries apply,
  not four; `is_bomrev_in_range` picks by `unifi_get_bomrev()`. Newer BOMs give `RPS_PRNT`
  flags **0x5** instead of 0x1 — bit 0x4 differs. Elsewhere 0x104 (pca953x `RPS_PRNT`) and
  0x100/0x2 appear, so the flags are a bitfield of {direction, invert, …}; the exact
  meaning of bit 0x4 is **[inferred]** as active-low/invert and is not proven here.
- Only pins 3 and 8 exist on ea16. `rps_update_port_hw_info`'s PD branch (line 4944)
  additionally queries ids 8, 9, 12, 13 — ids 9/12/13 are absent from the table, so
  `rps_drv_get_pin_value` returns `-0x5f` (not-found) and the fields read `false`.

### Recovered `RPS_PIN_*` enum (from all 210 descriptors, `tmp/logs/rps-pin-table.log`)

```
 0 RPS_PIN_12V_EN            8 RPS_PIN_12V_LP          14 RPS_PIN_12V_GUARD
 1 RPS_PIN_54V_EN            9 RPS_PIN_54V_LP          15 RPS_PIN_54V_GUARD
 2 RPS_PIN_PD_PRNT          12 RPS_PIN_12V_PSU_PG      16 RPS_PIN_12V_OUT_OC_ORING
 3 RPS_PIN_RPS_PRNT         13 RPS_PIN_54V_PSU_PG      17 RPS_PIN_54V_OUT_OC_ORING
 4 RPS_PIN_12V_SW                                      20 RPS_PIN_12V_BATT_GUARD
 5 RPS_PIN_54V_SW                                      21 RPS_PIN_54V_BATT_GUARD
 6 RPS_PIN_12V_OC
 7 RPS_PIN_54V_OC
```
(ids 10, 11, 18, 19 are unused by every board profile in this binary.)

---

## 3. Transport — `/dev/ttyS2`

`rps_drv_uart_common_configure` — **file 0x12790** (vaddr 0x112790, decompiled line 9204;
matches the offset already recorded in `rps-subsystem.md`). termios is built from
`tcgetattr` then masked. Exact masks from the binary:

```
c_iflag &= 0xffffe2ff      clears ICRNL IXON IXANY IXOFF
c_oflag &= 0xfffffffe      clears OPOST                        (raw output)
c_cflag &= 0x7ffffe8f      clears CSIZE CSTOPB PARENB CRTSCTS
c_cflag |= 0x000008b0      sets   CS8 | CREAD | CLOCAL
c_lflag &= 0xffffffe4      clears ISIG ICANON ECHO ECHOE       (raw input)
c_cc[VTIME] = 10           1.0 s inter-character timer
c_cc[VMIN]  = 0
cfsetispeed/cfsetospeed    B115200
tcflush(fd, TCIOFLUSH)     before tcsetattr
```

**Summary: 115200 8N1, raw, no flow control, VMIN=0 VTIME=10.**

> Corrects `rps-subsystem.md`, which lists "VTIME=1.0 s" without VMIN. VMIN=0 matters:
> a `read()` returns as soon as any byte arrives, or empty after 1.0 s of silence.

### Framing

- **TX** (`rps_port_do_command`, file 0x82b0 / vaddr 0x1082b0, line 3010):
  `json_dumps(obj, 0x20)` → **`JSON_COMPACT`** → the trailing NUL is overwritten with
  `'\n'` → single `write()`. No checksum, no length prefix, no framing byte.
- **RX** (`rps_command_read`, file 0x7fb0, line 2760): `select()` with **`tv_sec = 3`**,
  then `read()` in a loop into a 4096-byte buffer until the last byte read is `'\n'`
  or 4096 bytes are consumed; then NUL-terminated.
- **Line splitting**: the received blob is split with `strtok_r` on the 2-char delimiter
  set **`"\r\n"`** (bytes verified at file 0x1dad8). So a peer may terminate with CR, LF
  or CRLF and multiple JSON objects may arrive in one read.
- **Correlation**: of the split tokens, the last one containing the substring `"status"`
  is kept; it is `json_loads`'d and accepted only if `json_object_get(root,"status")` is
  a **`JSON_STRING`** (jansson type 2) equal to **`"ok"`**.
- **Retries: 5.** `local_1134 = 5`, decremented once per failed receive iteration
  (line ~3000). Combined with the 3 s select, a fully dead peer costs **up to ~15 s**
  per `rps_port_do_command` call.

`rps_port_do_command(port, obj, expect_response, correlate)` — the last two are flags.
`expect_response = 0` fire-and-forget (used by `diag put`); `= 1` runs the retry loop.

---

## 4. JSON-RPC command set

Two distinct schemas share the same daemon; do not conflate them.

### 4a. The UART protocol (this is the USP-RPS link)

Only **one** `op` exists: **`update`**. There is no other verb. `process_request`
(line 3097) does `strcmp("update", op)` and logs `"%s: op not found"` for anything else.

**Request** — built by `FUN_0011875c` (file 0x1875c, line 12969). Ghidra also emits the
same body at `FUN_00118760`; the two are one function (the exporter split it at the
stack-guard prologue, as it did for several others — see `rpsd-names.tsv`).

```json
{"resource":"device","op":"update","data":{ …device fields…,
  "rps_port_table":[ { …port fields (levels 0+1+2)… } ]}}
```

**Response** — built by `rps_json_response_ok_set_new` (file 0x18a70, line 13130):

```json
{"status":"ok","result":{"data":{"peer":{ …device fields…,
  "rps_port_table":[ { …port fields (level 0)… } ]}}}}
```

Note the asymmetry: the request's payload is at `data`, the response's at
`result.data.peer`. Both carry the same device schema. `FUN_00107bb4` (line 2462) walks
`status → result → data → peer` and hands the peer object to `update_pair_device_info`.

#### Device fields — always present
Built by `FUN_00118610` (line 12913) + `FUN_00117970` (line 12579).

| key | type | source |
|---|---|---|
| `mac` | string `%02x:…` | dev+0x9d6, 6 bytes |
| `hostname` | string | `freadline("/proc/sys/kernel/hostname")` |
| `model` | string | dev+0x955, from `rps.model` config |
| `version` | string | dev+0x910 |
| `role` | `"PSE"` \| `"PD"` | dev+0x9dc via the role enum |
| `notify_mode` | int | dev+0x9e4 (only when flag bit 8 clear) |

#### Device fields — PD role only (what ea16 sends)
`FUN_00117970` line 12882, branch `role == 1`:

| key | type |
|---|---|
| `power_required_12v` | int (mW) |
| `power_required_54v` | int (mW) |
| `power_required_max_12v` | int (mW) |
| `power_required_max_54v` | int (mW) |
| `required_power_12v` | int — **legacy alias**, same value as `power_required_12v` |
| `required_power_54v` | int — legacy alias |
| `graceful_shutdown` | int |

**Version gating** (`update_pair_device_info`, line 2404): a peer reporting `version <= 0x100`
is parsed with the *legacy* keys `required_power_12v/54v` only; `> 0x100` uses the
`power_required_*` set plus `graceful_shutdown`. rpsd always *emits* both.

#### Device fields — PSE role only (what a USP-RPS sends back)
`FUN_00117970` lines 12629–12855. Present-if noted where gated by a board flag.

`power_management_mode`, `power_supply_12v`, `power_supply_54v`,
`power_delivering_12v`, `power_delivering_54v`, `power_remaining_12v`,
`power_remaining_54v`, `temperature`, `temperature_max`, `fan_management_mode`,
`fan_ctrl` (`"auto"`|`"manual"`), `fan_speed`, `fan_duty`, `psu_12v_overload` (bool),
`psu_54v_overload` (bool), `anomalies` (int bitmask), `anomalies_details` (array of
`{anomaly, title, ports[]}`), `fsm_timer_tick` (int, ms), `flags` (string `0x%08x`),
`high_priority_ports` (array of `{port_idx, peer}`),
`stat` (object: `oring_12v_power_max`, `oring_54v_power_max`).

Battery block, gated on device flag bit 0xc:
`pse_batt_mode`, `pse_batt_qty`, `pse_batt_power_supply`,
`pse_batt_protecting_remaining`, `pse_batt_power_remaining`.

ORing block, gated on device flag2 bit 0:
`protecting_remaining_12v`, `protecting_remaining_54v`,
`oring_12v_oc` (bool), `oring_12v_power`, `oring_12v_power_crit`,
`oring_12v_power_crit_enable` (bool), `oring_12v_voltage`, `oring_12v_current`,
and the identical `oring_54v_*` set, plus `oring_poe_warning_flag`,
`oring_poe_warning_level`, `oring_poe_warning_percentlabel`,
`power_delivering_outlet` (gated further on a board-table byte).

> All `oring_*` / `power_*v_voltage|current|power` values are **strings** formatted
> `"%u.%03u"` from a milli-unit integer — e.g. 12345 mV renders `"12.345"`. They are
> not JSON numbers.

#### Port object (`rps_port_table[]`)
`FUN_00116c10(obj, port, level)` (file 0x16c10, line 12202) fills three levels; a request
sends levels 0+1+2, a response sends level 0 only.

**Level 0 — always:**
`port_idx`, `phy_id`, `name`, `port_mode`, `port_state`, `port_error_disabled` (bool),
`anomalies` (int), `overload_anomalies` (int), `flags` (string `0x%08x`),
`priority_raw` (string `0x%04x`), `up` (bool), `power_12v_low_power` (bool),
`power_54v_low_power` (bool). Plus, when the port is PD:
`power_12v_psu_power` (bool), `power_54v_psu_power` (bool).

**Level 1 — PSE ports only** (`port_mode` field `[0x25] == 0`):
`power_12v_enable`, `power_12v_active`, `power_12v_oc`, `power_12v_psu_power_good`,
`power_12v_guard`, `power_12v_batt_guard`, `power_12v_voltage`, `power_12v_current`,
`power_12v_power`, `power_12v_power_crit`, `power_12v_power_crit_enable`,
the identical `power_54v_*` set, `power_active`, `power_delivering`, `high_priority`,
and `stat` = `{power_12v_power_max, power_54v_power_max}`.

**Level 2 — nested peer:** `peer` = `{version, mac, model, hostname, role, …}` plus,
by the peer's role, either `rps_port_table` (PSE peer) or the five
`power_required_*` + `graceful_shutdown` keys (PD peer).

### 4b. The diag commands (`diag` namespace on the same UART)

Built with `json_pack("{s:{s:s}}", "diag", …)`:

| wire | direction | response |
|---|---|---|
| `{"diag":{"put":"<string>"}}` | TX only, `expect_response = 0` | none — pure write test |
| `{"diag":{"loopback":"<string>"}}` | TX, `expect_response = 1` | peer must echo it back |

Loopback pass criteria (line 13775): either the reply's `status` is the string `"ok"`,
**or** `json_equal(request["diag"], reply["diag"])` — i.e. a peer that echoes the object
verbatim also passes. Failure strings: `"port:%d no resp"`,
`"port:%d loopback status error"`, `"port:%d loopback string is not equal"`.

**Incoming diag handling** (`do_diag_loopback_resp`, line 13820): if a received line
contains *both* substrings `"diag"` and `"loopback"` and has **no** `status` key, rpsd
sets `status:"ok"` on the received object and echoes it straight back. **This is the
loopback responder — rpsd will answer a `{"diag":{"loopback":"x"}}` sent *to* it.**

---

## 5. State machine and cadence

### Timers (all from `rpsd_init`, lines 2113–2131)

| timer | period | callback |
|---|---|---|
| FSM tick | first fire **3 s**, then `fsm_timer_tick` ms, self-rescheduling | `ev_fsm_cb` (file 0x19d0c, line 13446) |
| status-file write | **7 s** | `LAB_00119c40` → `rps_update_status_file` |
| UART read | none — level-triggered | `event_assign(port+0x28, base, fd, EV_READ, …)` line 2033 |
| UDS control | none — level-triggered | `/var/run/rps_uds_server` |

`fsm_timer_tick` is computed by `rps_update_fsm_timer_tick` (file 0xb6e0, line 5221):
per port, **300 ms base**, `+100` if `power_12v_active`, `+300` if `power_54v_active`;
summed over ports and **capped at 3000 ms**.

> **For ea16 (1 port, PD, no 12V/54V enable pins → both inactive) the tick is 300 ms.**
> That is the poll rate for gpio 33/34 and the transmit rate for the `update` request.

### What happens on each tick (PD path, `rps_fsm_pd` file 0x130ec, line 9667)

1. `rps_update_port_hw_info` (file 0xaaa0, line 4771) — **skipped when `port_mode` is 3
   or 4** (guard `1 < mode - 3` at line 9689). PD branch at line 4944 reads pin ids
   **3 (RPS_PRNT), 8 (12V_LP), 9 (54V_LP), 12 (12V_PSU_PG), 13 (54V_PSU_PG)** via
   `/sys/class/gpio/gpioN/value`. Only 3 and 8 exist on ea16.
2. On failure: log `"port[%d]: fail to update info, skip"` and bump an error counter.
3. Dispatch on `port_mode`, then on `port_state`.
4. In the steady states, `send_pd_inform` builds the `update` request
   (`FUN_00118760`) and calls `rps_port_do_command(…, expect_response = 1)`.
5. On a good reply → reset the no-response counter, then `sync_with_peer_port_state`:
   if the peer's port state differs from ours **and** is in `[5..9]`
   (`standby … blocked`), adopt it via `rps_fsm_port_state_change`.
6. On no reply → increment the counter; when `count * fsm_timer_tick >= 300000` ms,
   log `"RPS heartbeat no response than %d seconds", 300`. **The heartbeat timeout is
   300 s**; at a 300 ms tick that is ~1000 consecutive failures.

### Enum tables — `{u32 id; u32 pad; char *name}`, stride 16, NULL-name terminated

Struct layout proven by the lookup helpers `FUN_00118d40` (id→name) and `FUN_00118d70`
(name→id, line 13261): both step `param_1 + 4` u32s and read the name at byte offset 8.

**`port_state`** (file 0x2c830):
```
0 none            4 bound             8 power_overload
1 initializing    5 standby           9 blocked
2 disconnected    6 power_delivering 10 disabled
3 searching       7 out_of_power_budget
```

**`port_mode`** (file 0x2c8f0): `0 none, 1 auto, 2 FTU, 3 manual, 4 disabled, 4 off`
(`disabled` and `off` are aliases of 4).

**`role`** (file 0x2c960): `0 PSE, 1 PD`.

### GPIO-driven transitions

`rps_fsm_pd` (line 9667) runs its main body only when `port_mode == 1` (**auto**); any
other mode logs `"port[%d]: Unknown mode %d"` except mode 4 (`disabled`/`off`), which
forces the port to state 10 (`disabled`) via `rps_port_reset_to_state` (line 9861).

At startup `rpsd_init` (line 2010) sets `port_state = port_prev_state = 1`
(**`initializing`**). `port_mode` comes from the config struct field at `+0x14c`, set
only by `rps.port.N.mode` (`rps_load_config_from_file`, line 5507). The per-port
defaults copied out of `board_identify()`'s board struct (`rps_conf_load`, line 5591)
populate `+0xbc`, `+0xc0` and `+0x148` — **not `+0x14c`**.

**So `port_mode` defaults to 0 (`none`) and the PD FSM will idle logging
`"port[%d]: Unknown mode %d"` unless `rps.port.1.mode=auto` is in the config file.**
Verified against the ea16 entry in `libubnt.so.1.0.0` (below): its port-default block
is `{name: "", +0x20: 1, +0x24: 1, +0x28: 0}` — no mode value.
This matters for bench work: setting the mode is a prerequisite, not a given.

Within auto mode the dispatch splits on the RPS-present pin (id 3, gpio 33), cached at
`port[0x69]`:

| `rps_prnt` | from state | → to state | note |
|---|---|---|---|
| 0 | 1 `initializing` | 9 `blocked` | only if port flag bit 0 set, else the loop breaks |
| 0 | anything else | 2 `disconnected` | |
| 1 | 2 `disconnected` | 3 `searching` | |
| 1 | 3 `searching` | 4 `bound` | **only on a successful `rps_port_do_command`** |
| 1 | 1 `initializing` | 9 `blocked` | same flag-bit-0 gate |
| 1 | ≥ 4 | (stays) | runs `send_pd_inform` + `sync_with_peer_port_state` each tick |
| 1 | 9 `blocked` | 1 `initializing` | recovery, when mode is still 1 |

- **`searching` is where transmission starts.** `FUN_00118760` builds the request and
  `rps_port_do_command(port, obj, expect_response = 1)` sends it; a non-NULL reply
  promotes the port to `bound`.
- **12V_LP (id 8, gpio 34) is not a transition trigger.** It is sampled every tick and
  published as the port field `power_12v_low_power`; on ea16 it is reported only.
  **[inferred]** from its absence from every state-change predicate in `rps_fsm_pd`.
- Every state change calls `rps_fsm_port_state_change` (file 0x12b10, line 9613), which
  logs `loop(%llu): port:%d state [%s] -> [%s]`, dumps the full port object at log
  level ≥ 4, and — when device flag bit 8 is set — emits a UDS notification (§6).

There is **no interrupt or poll on the gpio** — no `poll()` on the `value` file, no edge
trigger. Both pins are sampled synchronously inside the FSM tick.

---

## 6. Local control surfaces (no USP-RPS involved)

- **`/sbin/rps-ctrl` is a symlink to `/sbin/rpsd`** — one multi-call binary dispatched on
  `basename(argv[0])` (rodata 0x1d730 `"rpsd"`, 0x1d738 `"rps-ctrl"`, error
  `"What are you trying to run? It looks like %s"`).
- Daemon getopt string: **`c:Dvh`** (rodata 0x22360). Note the usage text advertises `-d`
  ("Run on foreground and log to stdout") but getopt accepts **`-D`** — a real mismatch
  in the shipped binary, so `-d` is silently rejected.
- Multi-call table at file 0x26df0: `{"rpsd", 0x1c4a0}, {"rps-ctrl", 0xc6a4}`.

### `rps-ctrl` options (getopt `F:t:i:o:p:m:vh`, rodata 0x1eb00; `rps_control_main` line 5989)

Option state lives in one global struct at vaddr 0x13c9b8 (file 0x3c9b8).

| opt | parsed as | default | struct off | goes to |
|---|---|---|---|---|
| `-t` | raw `optarg` | *required* | +0 | the operation name |
| `-p` | raw `optarg` | — | +8 | request key `port_id` |
| `-o` | raw `optarg` | — | +16 | request key `value` |
| `-m` | `strtol(optarg,0,10)` | — | +24 | **nothing — dead** |
| `-F` | `strtol(optarg,0,16)` — **hex** | **0x84** | +32 | request key `resp_fmt` |
| `-i` | `strtol(optarg,0,10)` | **5** | — | `sk_msg_send`/`sk_msg_recv` timeout |
| `-v` | — | — | — | doubles the log level (`level <<= 1`) |
| `-h` | — | — | — | usage to stdout, exit |

> **`-m` is parsed and then never read.** The request builder at file 0xc4a0 loads only
> `[x19+0]`, `[x19+8]`, `[x19+16]` and `[x19+32]` — `[x19+24]` is never touched
> (verified in the disassembly, `objdump -d --start-address=0xc4a0`). `-m` is also absent
> from the usage text despite being in the getopt string. **To set a mode use
> `-t mode -o <mode>`.**
>
> The usage text advertises only `JSON_COMPACT 0x20` for `-F`, but the default is `0x84`.
> Missing `-t` prints usage to stderr and exits non-zero. An unknown `-t` value prints
> `"%s: command not found"` and **exits 0 without sending anything**.

### CLI → daemon wire format

`FUN_0010c4a0` (file 0xc4a0, line 5909) builds one flat object; every operation shares
this single builder (all six entries of the CLI table point at it):

```json
{"op":"<operation>", "port_id":"<-p>", "value":"<-o>", "resp_fmt":<-F as int>}
```

`port_id` and `value` are JSON **strings** (not ints); `resp_fmt` is an integer. Each is
omitted when unset — but `resp_fmt` defaults to 0x84, so it is normally always present.
The request is always dumped with `json_dumps(obj, 0x20)` (`JSON_COMPACT`) **regardless
of `-F`** — `-F` only controls the *reply* format, because the daemon feeds `resp_fmt`
back into its own `json_dumps`.

**Transport**: `AF_UNIX` + `SOCK_DGRAM`. The daemon binds `/var/run/rps_uds_server`
(rodata 0x1eb10); the client binds its own auto-generated `/var/run/uds_client-<pid>-<rand>`
and `connect()`s, which is why the CLI can use address-less `sk_msg_send`/`sk_msg_recv`
while the daemon uses `sk_msg_sendto`/`sk_msg_recvfrom`.

**Framing — the header is its own datagram:**

```c
struct sk_msg { int32_t type; int32_t len; buffer_t *buf; };  /* only the first 8 B go on the wire */
```

`sk_msg_send` (libubnt 0x26220) sends exactly **8 bytes** first, then the payload in
chunks of **≤ 0x4000 (16384)** bytes until `len` is exhausted. `sk_msg_recv` mirrors it.
The libubnt log string `"Send type:%d, len:%d, data:%s"` confirms the layout. The payload
is bare JSON — no NUL, no newline, no length prefix beyond the header.

| direction | `type` | payload |
|---|---|---|
| request | **2** | the JSON object above |
| reply — RPS not ready | 1 | literal `"error"` |
| reply — handler produced no output | 1 | literal `"ok"` |
| reply — handler produced output | 2 | the handler's JSON string |

CLI interpretation: `(type,len) == (1,2)` with body `"ok"` → exit 0, print nothing;
`type == 2` → `puts(payload)`, exit 0; anything else → log `"Error: %s"`, print the
payload, exit −1.

> `"header, reply buffer destroyed"` (rodata 0x21fe0) is **not a protocol field** — it is
> a diagnostic logged when `buffer_create()` for the *reply* buffer fails after the header
> buffer was already made.

CLI replies use the key **`"rc"`**, *not* `"status"` (that key belongs to the UART
protocol): `json_pack("{s:s, s:s}", "rc","error", "error_reason", msg)` at line 13480,
plus `"payload"` for the result object.

### Operations

**CLI-side table** (file 0x26b88, `{name, builder}`, NULL-terminated) — six names:
`reset`, `mode`, `state`, `dump`, `list`, `diag`.

**Daemon-side table** (file 0x26d00, `{name, handler}`) — **five**:

| op | handler | note |
|---|---|---|
| `reset` | 0x1c0b4 | `cmd_op_reset` |
| `mode` | 0x1adc0 | sets `port_mode`; accepts the `port_mode` enum names |
| `state` | 0x1bd44 | sets/queries `port_state` by enum name |
| `dump` | 0x1b374 | `cmd_op_dump` — returns the full device object |
| `diag` | 0x1b150 | `cmd_op_diag`, dispatches the sub-table below |

**`list` is in the CLI table but has no daemon handler** — the CLI will send it and the
daemon will reject it. Conversely the usage text names only `dump`, `diag`, `mode`.

**`diag` sub-table** (file 0x26d60, `{name, handler}`) — eight sub-commands:

| sub-command | handler | recovered name |
|---|---|---|
| `power` | 0x1a970 | `diag_power_set` |
| `pin` | 0x1a650 | `diag_pin_set` |
| `uart` | 0x1b690 | `diag_uart` — `put` / `loopback` (§4b) |
| `hwmon` | 0x19550 | `diag_hwmon_set` |
| `flag` | 0x19ea0 | `-o "flag [set\|unset] [value]"` |
| `fan` | 0x19364 | `diag_fan` — `duty` |
| `log` | 0x191c0 | `diag_log` — `level` |
| `shutdown` | 0x190d0 | `diag_graceful_shutdown` |

`cmd_op_diag` splits `value` on **spaces** (`strtok_r`, delimiter `" "` at rodata 0x21c70,
max 32 tokens), so `-o` must be quoted. `argv[0]` selects the sub-command; the handler is
called as `fn(ctx, port_bitmap, argc-1, &argv[1], reply_buf)`.

| sub-command | syntax | notes |
|---|---|---|
| `power` | `power {on\|off} [12v] [54v]` | flags in any order; `on`==`off` → error; neither rail named → **both**; **refused on a PD port** (`"PD cannot set power pins"`) |
| `pin` | `pin <name> {on\|off\|1\|0}` | `0`/`1` take a *raw* write path, `on`/`off` a logical one |
| `uart` | `uart l[oopback] [text]` / `uart w[rite] [text]` | **matched on the first character only**; defaults `"RPS UART loopback test"` / `"RPS UART write test"` |
| `hwmon` | `hwmon {12v_\|54v_\|oring12v_\|oring54v_}{power_crit\|power_crit_enable\|clear} <val>`, or `hwmon fan_ctrl {auto\|manual}`, or `hwmon fan_duty <n>` | |
| `flag` | `flag [set\|unset] [value]` | `strtol` base **0** (so `0x…` works); bare `flag` just reports; acts on `ctx+0x900` |
| `fan` | `fan mode {auto\|manual}` / `fan duty <n>` | `duty` forces mode to manual first |
| `log` | `log level <n>` | writes the libubnt log level directly |
| `shutdown` | `shutdown <int>` | → `rps_set_graceful_shutdown` |

`diag uart` and `diag flag` hardcode `0x84` for their replies and ignore `resp_fmt`.

On ea16 the useful ones are `dump`, `state`, `mode`, `diag uart`, `diag log`, `diag flag`
and `diag shutdown`. `diag hwmon` and `diag power` act on hardware this board does not
have (§8) — and `diag power` is refused outright anyway because ea16's role is PD.
`diag pin` can only name pins 3 and 8, both inputs, so `__drv_pin_set` rejects them with
`"%s: unsupported operation %s"`.

### `-p` is a port **bitmap**, not an index

`port_id` is parsed daemon-side by `FUN_0011d450` (file 0x1d450, line 14692):
`strtok_r` on `","` (rodata 0x20958), then `sscanf(tok, "%d-%d")` (rodata 0x227e0) —
two conversions give an inclusive **range**, one gives a single bit. The result is a
**64-bit mask, valid ports 0–63**; every consumer iterates it with find-first/find-next-set.

- `-p 0`, `-p "0,2"`, `-p "0-3"`, `-p "0,4-7"` are all valid.
- **Omitting `-p` yields mask 0, which every handler treats as "all ports".**
- **On ea16 there is one port, so `-p 0` and omitting `-p` are identical.**

### `-t mode` values

`cmd_op_mode` resolves `value` case-insensitively (`strcasecmp` via `FUN_00118d70`)
against the `port_mode` table (§5): `none`(0), `auto`(1), `FTU`(2), `manual`(3),
`disabled`(4), `off`(4). Unknown → `"unknown mode:[%s]"`; missing → `"missing value parameter"`.

> `"dynamic"` (rodata 0x1e6d0) and `"static"` (rodata 0x21bd0) are **not** port modes —
> `dynamic` is the default for the config key `rps.power_management_mode`, and `static`
> has zero code cross-references in the binary.

### `-t state` values

`cmd_op_state` (file 0x1bd44) resolves `value` against the `port_state` table (§5),
case-insensitively, then calls `rps_port_set_state` per selected port. **`none` is
rejected**: it maps to 0, which is the lookup-failure sentinel, so `-o none` yields
`"unknown state:[none]"`. Everything from `initializing`(1) through `disabled`(10) works.

### `-t reset`

`cmd_op_reset` (file 0x1c0b4, line 14056) takes **no `value`** — it reads only
`resp_fmt` and `port_id`. Two behaviours:

- **`-p` absent (mask 0) → full device reset**: `rps_dev_reset` on the whole RPS device,
  then each port. Failure logs `"RSP device reset failed"` (sic — a typo in the vendor's
  source) and sets `rc":"error"`.
- **`-p <mask>` → per-port reset** only. Each port's result is appended to
  `rps_port_table[]` as `{"reset":"ok"|"error","port_idx":N}` and logged as
  `"port:%d reset by command %s"` with `successfully`/`failed`. Any failure adds
  `"error_reason":"fail to reset"`.
### `-F` response format

Parsed as **hex** and forwarded verbatim to jansson's `json_dumps` — there is no
validation table in the binary. Literal values the binary actually uses:

| value | where |
|---|---|
| **0x84** | CLI default; daemon substitutes it whenever `resp_fmt == 0`; hardcoded in `diag flag`, `diag uart` and the generic error builder |
| **0x20** | request serialization; every UART peer message (`JSON_COMPACT`) |
| **0xa0** | the status/notify dump (line 9620) |
| **0** | debug dumps (lines 3173, 9451) |

Against the jansson ABI, `0x84` = `JSON_INDENT(4) | JSON_PRESERVE_ORDER` and
`0xa0` = `JSON_COMPACT | JSON_PRESERVE_ORDER`. **[inferred]** — the binary proves only
that these integers reach `json_dumps`; no flag *names* appear in it. Practical values:
`-F 20` compact, `-F 84` pretty (default), `-F 0` → server rewrites to 0x84.

### Quick reference

```
rps-ctrl [-F <hex>] [-i <sec>] [-p <ports>] -t <op> [-o "<value>"] [-v] [-h]

ops:  dump | state | mode | reset | diag        (list is sent but unhandled)
-p:   "0" | "0,2" | "0-3" | "0,4-7"  bitmap, ports 0-63; omit = all
                                      (ea16 has one port: -p 0 == omitting it)
-F:   hex json_dumps flags; default 84; 0 -> 84; 20 = compact
-i:   timeout seconds, default 5
-m:   PARSED BUT IGNORED -- use -t mode -o <mode>

-t mode  -o {none|auto|FTU|manual|disabled|off}         case-insensitive
-t state -o {initializing|disconnected|searching|bound|standby|
             power_delivering|out_of_power_budget|power_overload|
             blocked|disabled}                          "none" is rejected
-t reset                                                no -p = full device reset

-t diag -o "power {on|off} [12v] [54v]"        (refused on ea16: PD role)
-t diag -o "pin <name> {on|off|1|0}"
-t diag -o "uart l [text]"  |  "uart w [text]"  (first char only)
-t diag -o "hwmon {12v_|54v_|oring12v_|oring54v_}{power_crit|power_crit_enable|clear} <v>"
-t diag -o "hwmon fan_ctrl {auto|manual}"  |  "hwmon fan_duty <n>"
-t diag -o "flag [set|unset] [value]"           (strtol base 0)
-t diag -o "fan mode {auto|manual}"  |  "fan duty <n>"
-t diag -o "log level <n>"
-t diag -o "shutdown <n>"
```

- **Notification socket**: `/var/run/rps_uds_notify_r` / `_s` (in `librps_uds.so`).
  `rps_fsm_send_notify` (line 9431) posts, on every state change when device flag bit 8
  is set: `{"port_idx":N,"port_state":"…","port_prev_state":"…"}`.
- **Status file**: `/var/run/rps.status`, rewritten every 7 s via mkstemp
  `/var/run/.rps.status-XXXXXX` + `rename()`. Contains the full device object.
  **This is the read-only observability surface — no socket needed.**
- **Config**: `/tmp/default.cfg` is loaded first if it exists, then the `-c` path
  (default `/var/run/rpsd.conf`) overrides (`rps_conf_load`, line 5609).
  `SIGHUP` reloads (wired at line 2119); `/sbin/rpsd_power_budget` (a udev
  `999-rpsd.rules` PROGRAM on block add/change/remove) rewrites the conf and sends it.
- **Config keys that take effect on ea16 (PD)** — `rps_load_config_from_file` line 5436:
  `rps.model`, `rps.notify_mode`, `rps.pd.power_required_max_12v`,
  `rps.pd.power_required_max_54v`, `rps.pd.power_required_12v`,
  `rps.pd.power_required_54v`, `rps.port.N.name`, `rps.port.N.mode`.
  Every `rps.pse.*` key is in the `role == 0` branch and is **dead on this board**.
  `rpsd_power_budget` writes `power_required_12v = 40 + 15 × ndisks`, max 150 W for ea16.
- **Refuses to start** if `/var/run/rpsd.pid` names a live process
  ("Another instance is running, aborting…").

---

## 7. Is a USP-RPS required to respond? — testing without the PSU

**No. Most of the interface can be exercised with nothing attached to JB4.**

| behaviour | needs a real USP-RPS? | evidence |
|---|---|---|
| rpsd starts, inits pins, opens ttyS2 | no | `rpsd_init` only needs the board table + config |
| gpio 33/34 sampled every 300 ms | no | `rps_update_port_hw_info` reads sysfs |
| `/var/run/rps.status` written every 7 s | no | timer is unconditional |
| FSM reaches `disconnected` / `blocked` | no | driven by `rps_prnt` = 0 |
| FSM runs at all | no — but needs `port_mode == 1` | otherwise "Unknown mode 0" every tick |
| FSM reaches `searching`, TX begins | **no PSU needed — mode 1 + gpio 33 high** | transition is on the pin, not on a reply |
| FSM reaches `bound` | yes, or any peer that answers | needs a `status:"ok"` reply |
| `diag loopback` passes | needs *a* responder, not a USP-RPS | echo or `status:"ok"` both pass |

Practical consequences for bench testing:

- **rpsd is not passive.** Once `rps_prnt` (gpio 33) reads high *and* `port_mode == 1`,
  it transmits an `update` request every FSM tick regardless of whether anything answers.
- **Two preconditions to make the box talk**, not one: `rps.port.1.mode=auto` in the
  config file (§5 — the board table leaves mode at 0/`none`), **and** gpio 33 high.
  Under our Fedora build the pin is unclaimed, so exporting gpio 33 and forcing it high
  with the mode set should start traffic on ttyS2 with no PSU present.
  **[inferred]** — not tested on hardware; the box was mid-recovery when this was written.
- **A loopback plug on the JB4 RS-232 TX/RX pair should satisfy `diag loopback`**, because
  rpsd answers its own `{"diag":{"loopback":…}}` (§4b) and the pass test accepts an exact
  echo. **[inferred]** — depends on the JB4 pinout, which is still unprobed.
- **Any Python script speaking 115200 8N1 raw can impersonate the USP-RPS**: read a line,
  and reply with one line of compact JSON ending in `\n`:
  `{"status":"ok","result":{"data":{"peer":{"role":"PSE","mac":"…","model":"…","version":"…"}}}}`
  Minimum to reach `bound` is `status:"ok"` plus a parseable `result.data.peer` carrying
  `role` and a well-formed `mac` — `update_pair_device_info` (line 2331) rejects the
  message if `role` is missing or `sscanf` of `mac` does not yield 6 bytes.
- A dead peer is cheap to detect but slow to give up on: **~15 s per attempt**
  (5 retries × 3 s select), **300 s** before the heartbeat warning.

---

## 8. Is there a SECOND interface? — **No. Closed, with evidence.**

This was the highest-value question. The answer is negative and the evidence is
exhaustive rather than a sample.

**Every device path in the entire binary** (all path-like strings in `.rodata`,
0x1d728–0x227e6):

```
/dev/ttyS2  /dev/ttyHS1  /dev/ttyAMA2  /dev/ttyRPS1..8      (UARTs, other boards)
/dev/i2c-                                                    (i2c, PSE profiles only)
/sys/class/gpio/  /sys/class/gpio/gpio%d                     (gpiolib-sysfs)
/sys/bus/i2c/devices/%d-00%02x/gpio                          (pca953x base lookup)
/sys/bus/i2c/devices/%d-00%02hx/hwmon                        (hwmon lookup)
/proc/sys/kernel/hostname
/var/run  /var/run/rps.status  /var/run/.rps.status-XXXXXX
/var/run/rpsd.conf  /var/run/rpsd.pid  /var/run/rps_uds_server
/var/run/rps_uds_notify_s  /var/run/rps_uds_notify_r  /tmp/default.cfg
```

There is **no SPI path, no `/dev/spidev*`, no `/sys/bus/spi`, no 1-Wire (`/sys/bus/w1`),
no `/dev/gpiochip*`, no `/sys/class/hwmon` outside the i2c lookup, and no MDIO, NVMEM or
mailbox path anywhere in the binary.**

Corroborating, independent lines of evidence:

1. **The ea16 port config's hwmon slots are NULL.** `+0x20` (hwmon12v) and `+0x68`
   (hwmon54v) are all-zero at file 0x289b0 / 0x289f8, where the PSE profile at file
   0x28b50 holds `"hwmon-isl28022"`/0x44 and `"hwmon-ina230"`/0x40. No hwmon device is
   configured for this board at all, so `rps_drv_hwmon_common_probe` is never reached.
2. **No i2c backend appears in any ea16 pin descriptor.** Both ea16 pins use
   `gpiolib-sysfs`. Across **all 210 pin descriptors in the whole binary**, only two
   backends are ever referenced: `gpiolib-sysfs` (31) and `gpio-pca953x` (179)
   — `tmp/logs/rps-pin-table.log`.
3. **`libubnt-pindrv` is a dead stub.** rpsd registers a backend named `libubnt-pindrv`
   (rodata 0x1fe38) bound to `libubnt.so.1`'s `ubnt_rps_pin_request` /
   `ubnt_rps_pin_get_value` / `ubnt_rps_pin_free`. All three are **WEAK, 8 bytes, and
   unconditionally `mov w0, #-1; ret`** (`libubnt.so.1.0.0` @0x44d50/0x44d60/0x44d70).
   No board's pin table references it anyway.
4. **`gpio-custompath` and every `hwmon-*` backend are registered but unreferenced** by
   any board's pin table.
5. **The only `ioctl`s in the binary are i2c**: `0x706` (`I2C_SLAVE`) and `0x720`
   (`I2C_RDWR`), at lines 9012/9062/9114/9119/9164/9169 — all inside `rps_drv_i2c_*`,
   reachable only from the pca953x and hwmon backends, i.e. only from PSE profiles.
6. **The dynamic import list** contains no SPI, w1, smbus, libgpiod or hwmon symbol; the
   only hardware-facing imports are `open`/`read`/`write`/`ioctl`/`select` and the
   `tc*attr` termios calls.

Taken with #64's earlier finding that `i2c_gen` @0xfd894000 has its MUIO pins physically
muxed to ETH-LED and ulogo_blue, **the ea16 RPS interface is exactly: one UART plus two
input GPIOs. There is no second bus.**

### Independent corroboration from the GPL / kernel trees

Searched the 1.3.35 GPL drop and the shipping 4.19.152 kernel source:

- **The complete released i2c device map for the UNVR family is four parts, none in
  0x40–0x4f**: PCA9548 mux @0x70 (`u-boot/include/configs/alpine_db_common.h:376`),
  PCA9575 @0x20 and @0x21 (`alpine_v2_64_ubnt_nas.h:46-51`), ADT7475 @0x2E
  (`alpine_v2_64_ubnt_nas.h:57-61` + `u-boot/drivers/hwmon/adt7475.c:12`).
- **The shipping 4.1.37 kernel has HWMON compiled out entirely** —
  `# CONFIG_HWMON is not set` in all three configs
  (`linux-arm64-unvr-4.1.37-ubnt/arch/arm64/configs/{alpine,alpine_na,}defconfig:114/119`).
  Every ina2xx / ltc / pmbus / max / adm driver is therefore unbuildable on the firmware
  this board actually runs.
- In the 4.19.152 rebase, `CONFIG_SENSORS_ADT7475=y` is the only sensor UBNT added
  (`alpine_ubnt_nas_defconfig:527`); `CONFIG_SENSORS_INA2XX=m` sits in the generic
  upstream block at line 304 alongside LM75/LM90, i.e. inherited boilerplate.
  **`PMBUS`, `CONFIG_W1`, `ISL28022`, `LTC4*`, `ADM12*`, `MAX5970`, `TPS2*`, `LM5066`,
  `UCD9*`, `ZL6100` are absent from every config in both trees.**
- `u-boot/board/annapurna-labs/alpine_ubnt/board.c:588,593` accepts **only**
  `"nxp,pca9575"` as an i2c-pld GPIO child and errors on anything else.
- The only UBNT power driver in the tree,
  `linux-arm64-unvr-4.1.37-ubnt/drivers/power/ubnt-hdd-pwrctl.c`, is **purely GPIO**
  (`present_gpio`/`pwren_gpio`, polled 2 s) and reads no sensor of any kind.
- **No `U48`, no `oring`/`hot-swap`/`ideal diode` in a power sense, no `RPS`, no
  `usp-rps`, no `MAX3221`, no `ttyS2` reference anywhere in either tree.** Both UARTs
  U-Boot wires are accounted for: `ttyS0` console and UART1 driving the front-panel
  character LCD (`u-boot/board/annapurna-labs/alpine_ubnt/lcd.c:38-67`).
- Structural caveat: **the ea16 device tree is not in either GPL tree.**
  `arch/arm64/boot/dts/al/` holds only the generic upstream `alpine-v2.dtsi` /
  `alpine-v2-evp.dts` (no i2c, gpio or hwmon node at all); the real DTB is copied from
  flash at boot (`alpine_v2_64_ubnt_nas.h:20,23`). So the GPL trees cannot *enumerate*
  the live i2c children — but they do prove no driver capable of binding a 0x40–0x4f
  power monitor is built, and our own live DTB work under #64 already covered the rest.

### What the GPIO isolators on the PCB are, then

The isolators are consistent with galvanically isolating exactly these three signals
across the RPS boundary — RS-232 TX/RX to the MAX3221 (`U122`) and the two sense lines
to gpio 33/34 — which is what an inter-chassis connector needs regardless of bus count.
**[inferred]** — the isolator count and their net assignment have not been read off the
board; this is a consistency argument, not a measurement. A continuity probe of JB4
would settle it and is already an open item in `rps-subsystem.md`.

---

## 9. `U48` — not identified. Negative result.

`U48` (~10-pin QFN in the ORing path, marking illegible) could not be identified from any
binary or source we hold.

Searched and **not found**:

- **GPL / kernel trees** (`UNVR-1.3.35-GPL/`, `urnvr-kernel-4.19.152/`): zero hits for
  `U48`; every `oring`/`hot-swap` hit is English prose about bitwise-OR or PCI CompactPCI
  HotSwap; `ideal diode` returns nothing; no ORing/hot-swap/ideal-diode controller driver
  is present *or* enabled in any defconfig. Full detail in §8.
- **`rpsd`**: no string `U48`, no `oring` reference tied to ea16, no i2c address for this
  board, no hwmon descriptor for this board (§8, points 1 and 2). The only ORing-monitor
  code in the binary (`rps_set_oring12v_power_crit`, `hwmon-ina230`/`ina237`/`isl28022`)
  is reached only through the PSE port-config hwmon slots, which are **NULL on ea16**.
- **Board tables**: the ea16 descriptor (file 0x377c0) has no monitor field of any kind.
- The `oring_*` JSON keys exist in the *device* schema, but on ea16 they are emitted only
  when device flag2 bit 0 is set, which is populated from a per-port config bit
  (`rps_conf_apply`, line 5875) that no ea16 port sets.

**Conclusion:** if `U48` exists in the ORing path, **no firmware on this board talks to
it**. That is consistent with it being a purely analogue part — an ideal-diode / ORing
FET controller or a hot-swap current-limit controller with no digital interface — which
is also what #64 concluded independently. Identifying it needs a clearer macro photograph
or an in-circuit probe; it is not recoverable from software.

---

## 10. Corrections to `rps-subsystem.md`

- **VTIME**: the termios sets `VTIME = 10` (1.0 s) **and `VMIN = 0`**. VMIN was not stated.
- **The receive timeout that actually bounds a transaction is the `select()` at 3 s**, not
  VTIME. Worst case per command = 5 × 3 s ≈ 15 s.
- **`\r\n` is a `strtok_r` delimiter *set*, not a required terminator** — CR, LF or CRLF
  all split correctly.
- The ea16 profile is shared by **five** sysids (`ea16 ea1a ea20 ea51 ea67`), not ea16 alone.
- The ea16 pin table has **4 descriptors covering 2 pins**, BOM-rev gated at `0x000b1011`;
  `RPS_PIN_RPS_PRNT` flags differ across that boundary (0x5 vs 0x1).
- ea16's role is **PD**, and the `rps.pse.*` half of the config file is dead code on it.
- `json_dumps` flag is `0x20` = `JSON_COMPACT` (the doc said "json_dumps()" without the flag).

## Scripts added by this doc

- `scripts/rps-decompile.py` — headless Ghidra over the rpsd ELF (ELF loader, not BinaryLoader).
- `scripts/rps-func-names.py` — recovers function names from `__func__` log arguments;
  emits `.sym`/`.tsv` and a renamed copy of the decompiled C. `--show NAME` prints a body.
- `scripts/rps-pin-table.py` — enumerates every pin descriptor in `.data` and recovers the
  `RPS_PIN_*` enum.

(`scripts/rps_walk.py` from the earlier pass is still the tool for ad-hoc pointer walks.)

## Still open

- JB4 per-blade rail/signal map — continuity probe (carried over from `rps-subsystem.md`).
- `U48` part number — needs a photograph or probe; software is exhausted (§9).
- Meaning of pin-descriptor flag bit `0x4` (§2) — **[inferred]** as invert/active-low.
- Bench proof of §7: export gpio 33 high on Fedora and watch ttyS2. Not attempted; the
  box was at a bootloader prompt during this work.
