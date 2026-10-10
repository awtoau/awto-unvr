#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""READ-ONLY survey of the NAND through al_nand (#208). Never opens an mtd node
for writing, never erases. Findings: docs/nand-study.md.

  bbm   bad-block markers on every NAND partition: OOB[0..3] of pages 0, 1 and
        63 of each block (raw OOB, no ECC engine), the in-RAM BBT verdict
        (MEMGETBADBLOCK), and any on-flash BBT signature. Runs ON the box
        (this file is piped to `python3 -` over ssh); ~12k single-page reads.

  ecc   raw page+OOB dump of a partition range via `nanddump --noecc --oob`,
        checked against scripts/nand_bch.py - an ECC model independent of the
        controller. Use after any write: a sha256 match plus 0 corrected_bits
        only shows the controller agrees with itself; this shows the parity
        bytes are the ones stock wrote. Classifies each page: erased /
        parity ok / 1-bit flip (HW would correct) / other mismatch.

  eccflags  the controller's own verdict per page (uncorrectable / corrected
        flags as the driver reads them), via kretprobes on
        al_nand_uncorr_err_get/al_nand_corr_err_get; reads pages through the
        normal ECC path. Compare with `ecc` to see whether the hardware and
        the independent model agree. Probes are removed on exit.

    ./scripts/nand-survey.py bbm
    ./scripts/nand-survey.py ecc --label linux_kernel --pages all
    ./scripts/nand-survey.py ecc --label device_tree --pages 64
    ./scripts/nand-survey.py eccflags --label rootfs --start-page 0 --pages 8
"""

from __future__ import annotations

import argparse
import fcntl
import json
import os
import struct
import subprocess
import sys
from pathlib import Path

PAGE = 4096
OOB = 224
PAGES_PER_BLOCK = 64
BLOCK = PAGE * PAGES_PER_BLOCK
# ioctl numbers from include/uapi/mtd/mtd-abi.h, 64-bit layout.
MEMGETBADBLOCK = 0x40084D0B  # _IOW('M', 11, loff_t)
MEMREADOOB = 0xC0104D04  # _IOWR('M', 4, struct mtd_oob_buf {u32,u32,ptr})
BBT_SIGNATURES = (b"Bbt0", b"1tbB")

# Measured 2026-10-10, rsync running: a full 1.1 GB raw rootfs pass took 135 s
# end to end (8.2 MB/s, host-side Python parity check included). The ecc limit
# is 1.25x that rate's time + 10 s of ssh start. On expiry: aborts and says so.
READ_MB_S = 8.0
BBM_TIMEOUT_S = 60  # 12288 page reads at ~0.3 ms each = ~4 s; ~15x margin


# ----------------------------------------------------------------- on the box


def _nand_partitions() -> list[tuple[str, int, int, int]]:
    """(label, mtd index, chip offset, size) for every 256 KiB-erase partition."""
    out = []
    for line in Path("/proc/mtd").read_text().splitlines()[1:]:
        dev, size, erase, rest = line.split(None, 3)
        if int(erase, 16) != BLOCK:
            continue
        idx = int(dev.removeprefix("mtd").rstrip(":"))
        sysfs = Path(f"/sys/class/mtd/mtd{idx}")
        out.append(
            (
                rest.strip().strip('"'),
                idx,
                int((sysfs / "offset").read_text()),
                int(size, 16),
            )
        )
    return out


def _read_oob(fd: int, offset: int) -> bytes:
    import ctypes

    buf = bytearray(OOB)
    addr = ctypes.addressof((ctypes.c_char * OOB).from_buffer(buf))
    fcntl.ioctl(fd, MEMREADOOB, struct.pack("IIQ", offset, OOB, addr))
    return bytes(buf)


def on_box_bbm() -> int:
    result = []
    for label, idx, base, size in _nand_partitions():
        fd = os.open(f"/dev/mtd{idx}", os.O_RDONLY)
        flagged, bbt_bad, sigs = [], [], []
        nblocks = size // BLOCK
        for b in range(nblocks):
            start = b * BLOCK
            oobs = {}
            for pg in (0, 1, PAGES_PER_BLOCK - 1):
                oobs[pg] = _read_oob(fd, start + pg * PAGE)
            marks = {pg: o[:4].hex() for pg, o in oobs.items() if o[:4] != b"\xff" * 4}
            if marks:
                flagged.append({"block": base // BLOCK + b, "marks": marks})
            for pg, o in oobs.items():
                if any(o[8:12] == s for s in BBT_SIGNATURES):
                    sigs.append({"block": base // BLOCK + b, "page": pg})
            # mutable buffer -> returns the ioctl's int result (1 = bad)
            if fcntl.ioctl(
                fd, MEMGETBADBLOCK, bytearray(struct.pack("Q", start)), True
            ):
                bbt_bad.append(base // BLOCK + b)
        os.close(fd)
        result.append(
            {
                "label": label,
                "mtd": idx,
                "first_block": base // BLOCK,
                "blocks": nblocks,
                "marker_flagged": flagged,
                "bbt_bad": bbt_bad,
                "bbt_signatures": sigs,
            }
        )
    print(json.dumps(result))
    return 0


TRACEFS = Path("/sys/kernel/tracing")
KPROBES = (
    ("alnand_unc", "al_nand_uncorr_err_get"),
    ("alnand_corr", "al_nand_corr_err_get"),
)


def _tf(path: Path, text: str) -> None:
    """tracefs rejects O_CREAT (pathlib's open) with EINVAL."""
    fd = os.open(path, os.O_WRONLY | os.O_APPEND)
    try:
        os.write(fd, text.encode())
    finally:
        os.close(fd)


def on_box_eccflags(label: str, start: int, count: int) -> int:
    part = next((p for p in _nand_partitions() if p[0] == label), None)
    if part is None:
        print(json.dumps({"error": f"no partition {label!r}"}))
        return 1
    _, idx, _, size = part
    if (start + count) * PAGE > size:
        print(json.dumps({"error": "range exceeds partition"}))
        return 1
    events = TRACEFS / "kprobe_events"
    out = []
    try:
        for name, sym in KPROBES:
            _tf(events, f"r:{name} {sym} ret=$retval:s32\n")
            _tf(TRACEFS / f"events/kprobes/{name}/enable", "1")
        fd = os.open(f"/dev/mtd{idx}ro", os.O_RDONLY)
        for pg in range(start, start + count):
            fd_t = os.open(TRACEFS / "trace", os.O_WRONLY | os.O_TRUNC)
            os.close(fd_t)
            try:
                os.pread(fd, PAGE, pg * PAGE)
                err = ""
            except OSError as e:
                err = e.strerror or str(e)
            seen = {}
            for line in (TRACEFS / "trace").read_text().splitlines():
                for name, _ in KPROBES:
                    if f"{name}:" in line and "ret=" in line:
                        seen[name] = int(line.rsplit("ret=", 1)[1].split()[0])
            out.append(
                {
                    "page": pg,
                    "unc": seen.get("alnand_unc"),
                    "corr": seen.get("alnand_corr"),
                    "read_error": err,
                }
            )
        os.close(fd)
    finally:
        for name, _ in KPROBES:
            try:
                _tf(TRACEFS / f"events/kprobes/{name}/enable", "0")
            except OSError:
                pass
        for name, _ in KPROBES:
            try:
                _tf(events, f"-:{name}\n")
            except OSError as e:
                print(f"probe {name} not removed: {e}", file=sys.stderr)
    print(json.dumps(out))
    return 0


# ----------------------------------------------------------------- on the host


def host_main(args: argparse.Namespace) -> int:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
    from _box import require, ssh_argv  # noqa: E402
    from _repo import make_log  # noqa: E402

    log = make_log("nand-survey")
    host = require(hint="is woomera up?")

    if args.cmd == "bbm":
        return host_bbm(host, ssh_argv, log)
    if args.cmd == "eccflags":
        return host_eccflags(host, ssh_argv, log, args)
    return host_ecc(host, ssh_argv, log, args)


def host_bbm(host, ssh_argv, log) -> int:
    log(f"bbm survey on {host} (read-only, ~12k OOB reads)")
    try:
        r = subprocess.run(
            ssh_argv(host, cmd=["python3", "-", "--on-box"], batch=True),
            input=Path(__file__).read_bytes(),
            capture_output=True,
            timeout=BBM_TIMEOUT_S,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log(f"TIMEOUT after {BBM_TIMEOUT_S}s (expected ~4 s): bbm survey aborted")
        return 124
    if r.returncode != 0:
        log(f"on-box survey failed rc={r.returncode}: {r.stderr.decode()[-400:]}")
        return 1
    parts = json.loads(r.stdout)
    total = sum(p["blocks"] for p in parts)
    marked = sum(len(p["marker_flagged"]) for p in parts)
    bad = sum(len(p["bbt_bad"]) for p in parts)
    sigs = sum(len(p["bbt_signatures"]) for p in parts)
    for p in parts:
        log(
            f"  {p['label']:13s} mtd{p['mtd']} blocks {p['first_block']}+{p['blocks']}: "
            f"marker!=ff {len(p['marker_flagged'])}, in-RAM BBT bad {len(p['bbt_bad'])}, "
            f"BBT signature {len(p['bbt_signatures'])}"
        )
        for f in p["marker_flagged"][:20]:
            log(f"      block {f['block']}: OOB[0:4] by page {f['marks']}")
    log(
        f"TOTAL {total} blocks; marker!=ff {marked}; in-RAM BBT bad {bad}; "
        f"on-flash BBT signatures {sigs}"
    )
    flagged = {f["block"] for p in parts for f in p["marker_flagged"]}
    bbt = {b for p in parts for b in p["bbt_bad"]}
    if flagged != bbt:
        log(
            f"MISMATCH marker vs BBT: only-marker {sorted(flagged - bbt)}, "
            f"only-BBT {sorted(bbt - flagged)}"
        )
        return 1
    return 0


def host_eccflags(host, ssh_argv, log, args) -> int:
    # 8 reads + 2 probes; a 4 KiB ECC read is ~0.3 ms, so ~5 s covers the ssh
    # round trip and tracefs setup; scales with page count.
    limit = 15 + args.pages // 100
    try:
        r = subprocess.run(
            ssh_argv(
                host,
                cmd=[
                    "python3",
                    "-",
                    "--on-box-eccflags",
                    args.label,
                    str(args.start_page),
                    str(args.pages),
                ],
                batch=True,
            ),
            input=Path(__file__).read_bytes(),
            capture_output=True,
            timeout=limit,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log(
            f"TIMEOUT after {limit}s: eccflags aborted (probes may remain: "
            "check /sys/kernel/tracing/kprobe_events)"
        )
        return 124
    if r.returncode != 0:
        log(
            f"on-box failed rc={r.returncode}: {r.stdout.decode()[-200:]} {r.stderr.decode()[-300:]}"
        )
        return 1
    for e in json.loads(r.stdout):
        log(
            f"  {args.label} page {e['page']}: uncorrectable={e['unc']} "
            f"corrected={e['corr']} read_error={e['read_error'] or '-'}"
        )
    return 0


def _box_sh(host, ssh_argv, cmd: str, timeout: int) -> str:
    r = subprocess.run(
        ssh_argv(host, cmd=[cmd], batch=True),
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )
    if r.returncode != 0:
        raise RuntimeError(f"{cmd!r} rc={r.returncode}: {r.stderr[-200:]}")
    return r.stdout


def host_ecc(host, ssh_argv, log, args) -> int:
    import time

    import nand_bch as nb

    from _repo import log_path, rel

    mtd = _box_sh(host, ssh_argv, "cat /proc/mtd", 20)
    idx = None
    for line in mtd.splitlines()[1:]:
        if f'"{args.label}"' in line:
            idx = int(line.split(":")[0].removeprefix("mtd"))
    if idx is None:
        log(f"no partition labelled {args.label!r} in /proc/mtd")
        return 1
    off = int(_box_sh(host, ssh_argv, f"cat /sys/class/mtd/mtd{idx}/offset", 20))
    size = int(_box_sh(host, ssh_argv, f"cat /sys/class/mtd/mtd{idx}/size", 20))
    total_pages = size // PAGE
    pages = total_pages - args.start_page if args.pages == "all" else int(args.pages)
    if args.start_page + pages > total_pages:
        log(f"range exceeds {args.label} ({total_pages} pages)")
        return 1
    raw_bytes = pages * (PAGE + OOB)
    limit = int(1.25 * raw_bytes / (READ_MB_S * 1e6)) + 10
    log(
        f"ecc check {args.label} mtd{idx} pages {args.start_page}+{pages} "
        f"(chip page {(off // PAGE) + args.start_page}), ssh limit {limit}s"
    )
    # nice: the box is shared; --bb=dumpbad: raw bytes of a bad block are data too.
    cmd = (
        "nice -n 19 /usr/sbin/nanddump -q -n -o --bb=dumpbad "
        f"-s {args.start_page * PAGE} -l {pages * PAGE} /dev/mtd{idx}"
    )
    p = subprocess.Popen(
        ssh_argv(host, cmd=[cmd], batch=True),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    tally = {"erased": 0, "ok": 0, "flip1": 0, "other": 0, "head": 0, "tail": 0}
    found: dict[str, list[dict]] = {"flip1": [], "other": [], "head": [], "tail": []}
    tails: set[bytes] = set()
    t0 = time.monotonic()
    n = 0
    while n < pages:
        if time.monotonic() - t0 > limit:
            p.kill()
            log(f"TIMEOUT after {limit}s at page {n}/{pages}: aborted")
            return 124
        raw = p.stdout.read(PAGE + OOB)
        if len(raw) < PAGE + OOB:
            break
        pg = args.start_page + n
        n += 1
        if raw == b"\xff" * (PAGE + OOB):
            tally["erased"] += 1
            continue
        res = nb.check_page(raw)
        bad = [c for c in res.cws if not c.ok]
        note = "; ".join(f"cw{c.cw} {c.note}" for c in bad)
        rec = {"page": pg, "chip_page": off // PAGE + pg, "note": note}
        if not res.oob_head_ff:
            tally["head"] += 1
            found["head"].append(rec)
        if not res.oob_tail_ff:
            tally["tail"] += 1
            found["tail"].append(rec)
            tails.add(raw[PAGE + 132 :])
        if not bad:
            tally["ok"] += 1
        elif all("1 data bit" in c.note or "1 parity bit" in c.note for c in bad):
            tally["flip1"] += 1
            found["flip1"].append(rec)
        else:
            tally["other"] += 1
            found["other"].append(rec)
    p.kill()
    p.wait()
    if n < pages:
        log(f"short read: {n}/{pages} pages; ssh stderr: {p.stderr.read()[-200:]!r}")
        return 1
    detail = log_path(f"nand-survey-{args.label}").with_suffix(".json")
    detail.write_text(json.dumps(found))
    for kind, recs in found.items():
        for r in recs[:15]:
            log(f"  {kind}: page {r['page']} (chip page {r['chip_page']}) {r['note']}")
        if len(recs) > 15:
            log(f"  {kind}: ... {len(recs) - 15} more, all in {rel(detail)}")
    log(
        f"RESULT {args.label}: {n} pages: erased {tally['erased']}, parity ok "
        f"{tally['ok']}, 1-bit flip {tally['flip1']}, other mismatch "
        f"{tally['other']}, OOB[0:4] not 0xff {tally['head']}; "
        f"OOB[132:224) not 0xff {tally['tail']} pages, {len(tails)} distinct pattern(s)"
    )
    return 0 if tally["other"] == 0 and tally["head"] == 0 else 1


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] == "--on-box":
        return on_box_bbm()
    if len(sys.argv) > 1 and sys.argv[1] == "--on-box-eccflags":
        return on_box_eccflags(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]))
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("bbm", help="bad-block marker survey")
    e = sub.add_parser("ecc", help="independent ECC check of raw pages")
    e.add_argument("--label", required=True)
    e.add_argument("--start-page", type=int, default=0)
    e.add_argument("--pages", default="64", help="count, or 'all'")
    f = sub.add_parser("eccflags", help="controller ECC verdict per page")
    f.add_argument("--label", required=True)
    f.add_argument("--start-page", type=int, default=0)
    f.add_argument("--pages", type=int, default=8)
    return host_main(ap.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
