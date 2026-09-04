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
