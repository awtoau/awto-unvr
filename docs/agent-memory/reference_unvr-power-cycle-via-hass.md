---
name: unvr-power-cycle-via-hass
description: "How to remotely power-cycle the UNVR box via Home Assistant / Sonoff TH smart outlet, without needing the user physically present"
metadata: 
  node_type: memory
  type: reference
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-24T01:01:33.838Z
---

The UNVR box (woomera) is plugged into a Sonoff TH smart outlet (`so-th-1`, entity `switch.so_th_1_relay`) that can be controlled remotely - full doc: `/mnt/2tb/git/awto-terminal/hass/th-power-control.md`.

**This matters a lot for this project**: many recovery/debugging cycles this session needed a genuine physical power cycle (cold boot) rather than a software reset - e.g. the al_eth ABORT-state-survives-warm-reset issue (docs/eth-tx-abort.md in awto-unvr, #138), or a wedged U-Boot with no OS running to trigger the SP805 watchdog. Previously this meant asking the user to walk over and physically cycle it. This capability removes that dependency.

**Fastest method - direct device API (no Home Assistant needed in the path):**

```python
import asyncio
from aioesphomeapi import APIClient   # awto-terminal repo .venv has it

async def power(state: bool):
    cli = APIClient("so-th-1.local", 6053, None)
    await cli.connect(login=True)
    ents, _ = await cli.list_entities_services()
    relay = next(e for e in ents if type(e).__name__ == "SwitchInfo")
    cli.switch_command(relay.key, state)
    await asyncio.sleep(1)
    await cli.disconnect()

asyncio.run(power(False))   # cut power
# wait a few seconds for full discharge (a few seconds is enough for most ESP-class boards; UNVR itself likely needs longer - verify)
asyncio.run(power(True))    # restore
```

**Via Home Assistant API instead** (from `/mnt/2tb/git/awto-terminal`, credentials auto-loaded from `~/.config/hass-cli/env`):

```bash
python3 scripts/hass.py service call switch.turn_off --arguments entity_id=switch.so_th_1_relay
python3 scripts/hass.py service call switch.turn_on  --arguments entity_id=switch.so_th_1_relay
python3 scripts/hass.py -o json state get switch.so_th_1_relay   # verify - the call itself returns [] either way
```

**Why:** User pointed this out directly (2026-08-24) after a long stretch of needing physical power-cycles for cold-boot recovery during al_eth/#131/#90 debugging.

**How to apply:** Before asking the user to physically power-cycle the UNVR box, check whether this remote path works first - it's faster and doesn't interrupt them. Still verify state after the call (`state get`) since Home Assistant service calls can return success even when nothing changed.

**Confirmed bitten by this (2026-08-24):** ran the off->sleep->on cycle via the direct ESPHome API, did not verify the final ON state, moved on. Box went dark minutes later - relay was actually still OFF (the restore command silently hadn't taken). User had to flag "i think the box powered off" before it was caught. Always subscribe to state and confirm `state=True` after the ON command, not just after issuing it - don't trust that the call completing means the switch actually flipped.
