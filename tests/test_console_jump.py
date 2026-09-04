"""_console stage-handoff detection: prove a failed jump aborts, and that a
healthy one never does.

The failure it exists for (real transcript, 2026-09-05): `go`/`bootedk2` did
not take, EDK2 never started, and the BdsWait 's' hotkey spam landed on the
still-live U-Boot prompt:

    Hit any key to stop autoboot:  0
    awto-nas# sssssssssssssssssssssssssssssssss

That is provable in well under a second. The run used to sit out the whole
180s SHELL_TIMEOUT_S afterwards.

Run: pytest tests/test_console_jump.py -q
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _console  # noqa: E402

PROMPT = "awto-nas#"


# --- the failure this was written for ------------------------------------


def test_hotkey_echo_at_uboot_prompt_is_proof():
    text = "Hit any key to stop autoboot:  0 \r\nawto-nas# " + "s" * 33
    assert _console.jump_failed(text, "s", PROMPT)


def test_prompt_reappearing_after_jump_is_proof():
    text = "## Starting application at 0x20000000 ...\r\nawto-nas# "
    assert _console.jump_failed(text, "s", PROMPT)


def test_unknown_command_is_proof():
    # `bootedk2` on a U-Boot built without it: usage text, then the prompt.
    text = "Unknown command 'bootedk2' - try 'help'\r\nawto-nas# "
    assert _console.jump_failed(text, "s", PROMPT)


# --- the success path must never trip it ---------------------------------


def test_edk2_trace_is_not_proof():
    text = (
        "## bootedk2: cleanup_before_linux() then jump to 0x20000000\r\n"
        "UEFI firmware (version 1.0 built at 10:00:00 on Sep  5 2026)\r\n"
        "[Bds]BdsWait(3)\r\n[Bds]BdsWait(2)\r\n"
    )
    assert _console.jump_failed(text, "s", PROMPT) is None


def test_lone_s_characters_in_trace_are_not_proof():
    # Ordinary trace is full of single and double 's'. Only a run of three
    # counts, which trace prose does not produce for our exact hotkey byte.
    text = "InstallProtocolInterface: address success status\r\n"
    assert _console.jump_failed(text, "s", PROMPT) is None


def test_silence_is_not_proof():
    # A slow-but-progressing jump must never be aborted by this check.
    assert _console.jump_failed("", "s", PROMPT) is None


def test_prompt_scrolled_out_of_the_tail_is_not_proof():
    # The prompt from BEFORE the jump must not count once real output has
    # pushed it past the tail window.
    text = "awto-nas# " + "EDK2 dispatch trace line\r\n" * 200
    assert _console.jump_failed(text, "s", PROMPT) is None


def test_no_hotkey_disables_the_echo_check():
    # --no-hotkey passes "": the prompt check still applies, the echo one
    # cannot, since nothing of ours is being typed.
    text = "some output with sss inside it\r\n"
    assert _console.jump_failed(text, "", PROMPT) is None


# --- banner detection ----------------------------------------------------


def test_banner_missing_on_empty():
    assert _console.jump_banner_missing("")


def test_go_banner_recognised():
    assert not _console.jump_banner_missing("## Starting application at 0x20000000 ...")


def test_bootedk2_banner_recognised():
    assert not _console.jump_banner_missing(
        "## bootedk2: cleanup_before_linux() then jump to 0x20000000"
    )


# --- ssh-woomera MAC matching under ARP flux -----------------------------
#
# All four host NICs answer ARP for any local IP (arp_ignore=0) and both of
# the box's ports sit on one subnet (#170), so `ip neigh` routinely names the
# 10G port's MAC for an IP the 1G port also answers. Requiring the 1G MAC
# specifically reported the box as absent while it was up.


def _ssh_woomera():
    import importlib.util

    spec = importlib.util.spec_from_file_location(
        "_ssh_woomera", REPO / "scripts/ssh-woomera.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_matches_the_1g_mac(monkeypatch):
    m = _ssh_woomera()
    monkeypatch.setattr(m, "macs_of", lambda ip: ["74:ac:b9:41:a8:11"])
    assert m.is_woomera("192.168.25.140")


def test_matches_the_10g_mac(monkeypatch):
    # The regression: this used to return False and the box read as absent.
    m = _ssh_woomera()
    monkeypatch.setattr(m, "macs_of", lambda ip: ["74:ac:b9:41:a8:12"])
    assert m.is_woomera("192.168.25.140")


def test_matches_when_a_stale_row_comes_first(monkeypatch):
    m = _ssh_woomera()
    monkeypatch.setattr(
        m, "macs_of", lambda ip: ["00:11:22:33:44:55", "74:ac:b9:41:a8:11"]
    )
    assert m.is_woomera("192.168.25.140")


def test_other_ubnt_gear_sharing_the_oui_is_not_woomera(monkeypatch):
    # Still exact MACs, never the OUI - an OUI match once picked a neighbour.
    m = _ssh_woomera()
    monkeypatch.setattr(m, "macs_of", lambda ip: ["74:ac:b9:99:99:99"])
    assert not m.is_woomera("192.168.25.140")


def test_no_neighbour_entry_is_not_woomera(monkeypatch):
    m = _ssh_woomera()
    monkeypatch.setattr(m, "macs_of", lambda ip: [])
    assert not m.is_woomera("192.168.25.140")
