---
name: never-kill-shared-tio
description: "NEVER pkill/kill/stop a tio you didn't start — it's the user's shared serial console; use the Ignition MCP console tasks + attach via the socket"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T01:13:42.866Z
---

**NEVER `pkill tio`, `kill <tio pid>`, or `dev.py console-stop` a tio session unless you are CERTAIN it is one you started this turn.** The tio owning the CP2102 serial port is very likely the **user's** shared console session that they are watching.

**Why:** during the baud demo I pkill'd tio to restart it at 921600 and killed the user's session (pid 1808171). They were furious — "do not fucking open tio session unless you are sure they are yours." This is the shared-console rule ([[box-testing-over-ssh]], [[own-the-box-reset-no-approval]]) — owning the box does NOT mean stomping the user's terminal.

**How to apply:**
- Console setup goes through the **Ignition MCP tasks** the user set up: `mcp__ignition-mcp__task_woomera_serial_console_tio` (owns the port) + `task_woomera_serial_console_attach` (attach). Use those, not raw tio/pkill.
- Interact by **attaching to the existing socket** (`dev.py console-send`/`console-tcl` talk to `/run/user/1000/tio-unvr.sock`) — a client, never restarting the owner.
- **Baud changes need a tio restart** → that inherently disrupts the shared session. Do NOT do it unilaterally: ask the user to restart their console at the new baud, or use the task. A `reset` reverts baud to 115200 default (env is ENV_IS_NOWHERE) so recovery is a reboot, not a kill.
- If a stale/socketless tio blocks you, surface it to the user — don't kill it.
