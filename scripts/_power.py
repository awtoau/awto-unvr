"""Remote power control for the UNVR via the Sonoff TH smart outlet.

Shared by scripts/power-cycle.py (the ./dev.py power-cycle CLI entry) and
scripts/ram-boot-deploy.py (which needs a fresh boot before every RAM-boot
attempt) - ONE copy of the cut/restore/verify logic, not two that can drift
apart. See docs (memory: unvr-power-cycle-via-hass) for the outlet's role.
"""

from __future__ import annotations

import subprocess

HASS_HOST = "so-th-1.local"

# aioesphomeapi lives in awto-terminal's own venv, not this project's - shell
# out to that interpreter rather than pulling a cross-project dependency in
# here. (Discovered the hard way: a version of this used to import it directly
# and crashed immediately with ModuleNotFoundError on first real use.)
AWTO_TERMINAL_PY = "/mnt/2tb/git/awto-terminal/.venv/bin/python3"
AWTO_TERMINAL_REPO = "/mnt/2tb/git/awto-terminal"

_POWER_CYCLE_SCRIPT = """
import asyncio
from aioesphomeapi import APIClient

async def set_state(state):
    cli = APIClient('so-th-1.local', 6053, None)
    await cli.connect(login=True)
    ents, _ = await cli.list_entities_services()
    relay = next(e for e in ents if type(e).__name__ == 'SwitchInfo')
    cli.switch_command(relay.key, state)
    await asyncio.sleep(1)
    await cli.disconnect()

async def get_state():
    cli = APIClient('so-th-1.local', 6053, None)
    await cli.connect(login=True)
    ents, _ = await cli.list_entities_services()
    relay = next(e for e in ents if type(e).__name__ == 'SwitchInfo')
    states = []
    cli.subscribe_states(states.append)
    await asyncio.sleep(2)
    await cli.disconnect()
    return next((s.state for s in states if s.key == relay.key), None)

async def cycle():
    await set_state(False)
    await asyncio.sleep(5)
    await set_state(True)
    await asyncio.sleep(2)
    st = await get_state()
    print('FINAL_STATE:', st)

asyncio.run(cycle())
"""

_POWER_SET_SCRIPT_TEMPLATE = """
import asyncio
from aioesphomeapi import APIClient

async def set_state(state):
    cli = APIClient('so-th-1.local', 6053, None)
    await cli.connect(login=True)
    ents, _ = await cli.list_entities_services()
    relay = next(e for e in ents if type(e).__name__ == 'SwitchInfo')
    cli.switch_command(relay.key, state)
    await asyncio.sleep(1)
    await cli.disconnect()

async def get_state():
    cli = APIClient('so-th-1.local', 6053, None)
    await cli.connect(login=True)
    ents, _ = await cli.list_entities_services()
    relay = next(e for e in ents if type(e).__name__ == 'SwitchInfo')
    states = []
    cli.subscribe_states(states.append)
    await asyncio.sleep(2)
    await cli.disconnect()
    return next((s.state for s in states if s.key == relay.key), None)

async def set_and_verify():
    await set_state({state!r})
    st = await get_state()
    print('FINAL_STATE:', st)

asyncio.run(set_and_verify())
"""


def power_set_verified(state: bool, log=print) -> None:
    """Turn the outlet on or off, VERIFYING the resulting state - same
    verify-don't-trust discipline as power_cycle_verified(). For a plain
    on/off (not a full cycle), e.g. holding the box off while someone
    reseats a cable."""
    log(f"power: setting {'ON' if state else 'OFF'} via so-th-1")
    r = subprocess.run(
        [AWTO_TERMINAL_PY, "-c", _POWER_SET_SCRIPT_TEMPLATE.format(state=state)],
        cwd=AWTO_TERMINAL_REPO,
        capture_output=True,
        text=True,
        timeout=30,
    )
    out = r.stdout + r.stderr
    if f"FINAL_STATE: {state}" not in out:
        raise RuntimeError(f"power set to {state} did not take, output:\n{out}")
    log(f"power: verified {'ON' if state else 'OFF'}")


def power_cycle_verified(log=print) -> None:
    """Cut and restore power via the Sonoff TH outlet, VERIFYING the final
    state - this project was bitten once by trusting the ON command without
    checking, and the box sat dark until the user noticed."""
    log("power: cycling via so-th-1")
    r = subprocess.run(
        [AWTO_TERMINAL_PY, "-c", _POWER_CYCLE_SCRIPT],
        cwd=AWTO_TERMINAL_REPO,
        capture_output=True,
        text=True,
        timeout=30,
    )
    out = r.stdout + r.stderr
    if "FINAL_STATE: True" not in out:
        raise RuntimeError(f"power restore did not take, output:\n{out}")
    log("power: restored, verified ON")


if __name__ == "__main__":
    # Not a CLI entry point - see scripts/power-cycle.py / ./dev.py power-cycle.
    raise SystemExit(
        f"{__file__}: import-only module, run ./dev.py power-cycle instead"
    )
