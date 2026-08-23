#!/usr/bin/env python3
"""Build the S2 stage2_loader function-name ledger (confirmed + speculative).

Method (iterative, evidence-first):
  1. CONFIRMED via string: for each FUN_, resolve every literal-pool word to a
     string; if it is a bare C identifier (a __func__ the code logs), that is the
     function's name. This recovers al_ddr_*, al_pll_*, al_i2c_*, al_flash_* names
     the loader passes to its al_err/printf helper.
  2. CONFIRMED via curated: merge docs/nor-reference/s2-names.tsv (hand-RE + the DDR
     call graph verified in ddr-s2-parser-analysis.md), which wins over (1) when they
     disagree (e.g. a wrapper that logs a callee's __func__).
  3. SPECULATIVE via context: a still-bare FUN_ in the DDR-init subgraph is named
     from its role/caller, prefixed `s_` (never presented as confirmed).

Emits:
  docs/nor-reference/preboot-s2-names.sym  (name<TAB>0xADDR ; ApplyAlRegs input)
  docs/nor-reference/preboot-s2-names.md   (addr, name, confidence, rationale)
and reports DDR-init subgraph naming coverage.
Static only.
"""

from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path("/mnt/2tb/git/awto-unvr")
BLOB = REPO / "docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin"
ASM = REPO / "tmp/ghidra-out/s2-final/disassembly.asm"
CURATED = REPO / "docs/nor-reference/s2-names.tsv"
SYM_OUT = REPO / "docs/nor-reference/preboot-s2-names.sym"
MD_OUT = REPO / "docs/nor-reference/preboot-s2-names.md"
BASE = 0xF2200000
HDR = re.compile(r"^; ==== FUN_[0-9a-f]+ @ ([0-9a-f]+) ====")
CALL = re.compile(r"\b(?:bl|blx)\s+0x([0-9a-f]{6,8})\b")
POOL = re.compile(r"\[0x([0-9a-f]{6,8})\]")
IDENT = re.compile(r"^[a-z_][a-z0-9_]{3,}$")

# Speculative names for DDR-subgraph functions with no __func__ (role from caller).
SPEC = {
    "f2201716": ("s_al_err_printf", "error/log printer, called by al_ddr_*/spd_parse"),
    "f2201618": ("s_vprintf_core", "112-ins formatter under s_al_err_printf"),
    "f220150a": ("s_printf_fmt_num", "under s_vprintf_core"),
    "f220159c": ("s_printf_fmt_str", "under s_vprintf_core"),
    "f22015e4": ("s_printf_pad", "under s_vprintf_core"),
    "f22014dc": ("s_printf_emit", "under formatter chain"),
    "f22014f8": ("s_printf_putc", "under formatter chain"),
    "f220178c": ("s_printf_helper", "under s_printf_emit"),
    "f2204518": ("s_udelay", "busy-wait, called by al_ddr_init/pll"),
    "f2200098": ("s_memcpy_words", "32-byte unrolled block copy"),
    "f22000b4": ("s_memset_words", "unrolled memset"),
    "f22017a4": ("s_spd4_timing_get", "helper of al_ddr4_spd_parse"),
    "f220180e": ("s_ddr_train_step", "callee of al_ddr_init"),
    "f2201948": ("s_ddr_train_step2", "callee of al_ddr_init"),
    "f22019f4": ("s_ddr_train_step3", "callee of al_ddr_init"),
    "f220186a": ("s_ddr_delay_step", "callee of al_ddr_init (calls s_udelay)"),
    "f2203d7e": ("s_pll_reg_a", "under al_pll_channel_div_set"),
    "f2203d90": ("s_pll_reg_b", "under al_pll_channel_div_set"),
    "f2204694": ("s_i2c_xfer_a", "callee of i2c_eeprom_read"),
    "f22046fc": ("s_i2c_xfer_b", "callee of i2c_eeprom_read"),
    "f22044c8": ("s_shared_params_write_size", "orchestrator, after magic write"),
    "f2200814": ("s_cfg_get_helper", "orchestrator helper"),
    "f220061c": ("s_orch_helper", "orchestrator helper"),
    "f22003d0": ("s_rec_present_check", "shared by rec readers/impedance/voltage"),
    "f2200100": ("s_freq_helper_a", "under ddr_freq_change_according_to_spd"),
    "f2200114": ("s_freq_helper_b", "under s_freq_helper chain"),
    "f2200340": ("s_freq_helper_c", "under ddr_freq_change_according_to_spd"),
}


def cstr(b, va, mx=80):
    o = va - BASE
    if o < 0 or o >= len(b):
        return ""
    c = b[o : o + mx]
    e = c.find(b"\0")
    if e != -1:
        c = c[:e]
    try:
        s = c.decode("ascii")
    except UnicodeDecodeError:
        return ""
    return s if s and all(32 <= ord(x) < 127 for x in s) else ""


def main() -> int:
    b = BLOB.read_bytes()
    cur = None
    body = defaultdict(list)
    calls = defaultdict(set)
    for ln in ASM.read_text(errors="replace").splitlines():
        m = HDR.match(ln)
        if m:
            cur = m.group(1)
            continue
        if cur:
            body[cur].append(ln)
            for t in CALL.findall(ln):
                calls[cur].add(t.zfill(8))
    funcs = sorted(body)

    curated = {}
    for l in CURATED.read_text().splitlines():
        l = l.strip()
        if l and not l.startswith("#"):
            va, nm = re.split(r"\s+", l, maxsplit=1)
            curated[va.lower().replace("0x", "").zfill(8)] = nm

    ledger = {}  # va -> (name, confidence, rationale)
    for f in funcs:
        # string-confirmed
        found = None
        for ph in POOL.findall("\n".join(body[f])):
            w = (
                int.from_bytes(b[int(ph, 16) - BASE : int(ph, 16) - BASE + 4], "little")
                if 0 <= int(ph, 16) - BASE <= len(b) - 4
                else 0
            )
            s = cstr(b, w)
            if s and IDENT.match(s):
                found = s
                break
        if f in curated:
            ledger[f] = (curated[f], "confirmed", "curated (ddr call graph / hand-RE)")
        elif found:
            ledger[f] = (found, "confirmed", f'__func__ string "{found}"')
        elif f in SPEC:
            nm, why = SPEC[f]
            ledger[f] = (nm, "speculative", why)
    # DDR subgraph coverage
    seen = set()
    stack = ["f2201a90", "f22003d8"]
    while stack:
        x = stack.pop().zfill(8)
        if x in seen:
            continue
        seen.add(x)
        stack += list(calls.get(x, ()))
    sub_named = sum(1 for f in seen if f in ledger)
    sub_conf = sum(1 for f in seen if f in ledger and ledger[f][1] == "confirmed")

    with SYM_OUT.open("w") as f:
        f.write(
            "# S2 stage2_loader function names -> ApplyAlRegs .sym (name<TAB>0xADDR)\n"
        )
        for va in sorted(ledger):
            f.write(f"{ledger[va][0]}\t0x{va}\n")
    with MD_OUT.open("w") as f:
        f.write("# S2 stage2_loader — function-name ledger\n\n")
        f.write(
            "Machine form: `preboot-s2-names.sym` (applied via ApplyAlRegs / annotate-preboot.py).\n"
        )
        f.write(
            f"Auto-built by `scripts/build-s2-name-ledger.py`. {len(ledger)} names "
            f"({sum(1 for v in ledger.values() if v[1] == 'confirmed')} confirmed, "
            f"{sum(1 for v in ledger.values() if v[1] == 'speculative')} speculative).\n\n"
        )
        f.write(
            f"DDR-init subgraph (from al_ddr_init + orchestrator): {len(seen)} functions, "
            f"{sub_named} named ({sub_conf} confirmed, {sub_named - sub_conf} speculative), "
            f"{len(seen) - sub_named} bare.\n\n"
        )
        f.write("| addr | name | confidence | rationale |\n|---|---|---|---|\n")
        for va in sorted(ledger):
            nm, conf, why = ledger[va]
            f.write(f"| 0x{va} | `{nm}` | {conf} | {why} |\n")
    print(f"ledger: {len(ledger)} names -> {SYM_OUT.name}, {MD_OUT.name}")
    print(
        f"DDR-init subgraph: {len(seen)} fns, {sub_named} named "
        f"({sub_conf} confirmed, {sub_named - sub_conf} speculative), {len(seen) - sub_named} bare"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
