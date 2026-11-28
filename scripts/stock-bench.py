#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Stock-kernel benchmark that runs ON woomera under Ubiquiti's 4.19.152 kernel (#381). Stdlib only, py3.9.

  Installed into the stock rootfs by `./dev.py stock-boot-prep --hdd`; runs from a systemd unit at boot.

- No ssh/ARP needed: the box is the iperf3 CLIENT toward the dev host (iperf3 -s -p PORT started by the host).
- Read-only fio on whole devices found by serial, 15 s each; 10G rx/tx with 1 and 4 streams; combined.
- Per run: throughput, busy CPU (of 400), interrupts/s, from /proc/stat and /proc/interrupts diffs.
- Writes /var/log/awto-stock-bench.json on its own root (the stock-boot Seagate), then
  `echo b > /proc/sysrq-trigger`: immediate restart WITHOUT device shutdown hooks (the stock bay-power
  module cuts drive power on an orderly shutdown and our U-Boot then cannot read the SSD).
"""

import json
import os
import subprocess
import time

LOG_HOST = os.environ.get("AWTO_LOG_HOST", "")  # dev host 10G address, set in the unit
LOG_PORT = os.environ.get("AWTO_IPERF_PORT", "5702")
OUT = "/var/log/awto-stock-bench.json"
RUN_S = 15  # steady state for these devices is reached in ~5 s; 15 s averages out noise
SERIALS = {
    "sata_ssd": "S21BNXAG501763W",  # Samsung 850 EVO on SATA
    "hdd_wd": "VGG8B0LG",  # WD82PURZ 8 TB mirror member, read-only
    "usb_860evo": "S3YBNB0M621787E",
    "usb_m2_a": "S5GENJ0NB03022H",
    "usb_m2_b": "S5GENJ0NB02962Z",
}
RESULT = {
    "started": time.strftime("%FT%T%z"),
    "kernel": os.uname().release,
    "runs": {},
    "notes": [],
}


def sh(cmd, timeout=120):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
    return (r.stdout + r.stderr).strip()


def note(msg):
    RESULT["notes"].append("%s %s" % (time.strftime("%T"), msg))
    save()


def save():
    with open(OUT, "w") as f:
        json.dump(RESULT, f, indent=1)
        f.flush()
        os.fsync(f.fileno())


def cpu_snapshot():
    with open("/proc/stat") as f:
        v = [int(x) for x in f.readline().split()[1:]]
    idle = v[3] + v[4]
    total = sum(v)
    irq = 0
    with open("/proc/interrupts") as f:
        for line in f.readlines()[1:]:
            parts = line.split()
            for p in parts[1:5]:
                if p.isdigit():
                    irq += int(p)
    return total, idle, irq, time.time()


def measured(name, fn):
    a = cpu_snapshot()
    out = fn()
    b = cpu_snapshot()
    dt = b[3] - a[3]
    busy = 400.0 * (1 - (b[1] - a[1]) / max(1, (b[0] - a[0])))
    RESULT["runs"][name] = {
        "result": out,
        "busy_of_400": round(busy, 1),
        "irq_per_s": round((b[2] - a[2]) / dt),
        "secs": round(dt, 1),
    }
    save()


def dev_by_serial(serial):
    for d in sorted(os.listdir("/sys/block")):
        if not d.startswith("sd"):
            continue
        try:
            s = sh(
                "udevadm info --query=property --name=/dev/%s | grep -E '^ID_SERIAL_SHORT=' | cut -d= -f2"
                % d
            )
        except Exception:
            s = ""
        if s == serial:
            return "/dev/" + d
    return None


def fio(devs):
    jobs = " ".join("--name=j%d --filename=%s" % (i, d) for i, d in enumerate(devs))
    out = sh(
        "fio --readonly --direct=1 --ioengine=libaio --rw=read --bs=1M --iodepth=32 --runtime=%d --time_based --output-format=json %s"
        % (RUN_S, jobs),
        RUN_S + 60,
    )
    try:
        j = json.JSONDecoder().raw_decode(out[out.find("{") :])[0]
        return {jb["jobname"]: round(jb["read"]["bw"] / 1024, 1) for jb in j["jobs"]}
    except Exception:
        return {"error": out[-300:]}


def iperf(streams, reverse):
    cmd = "iperf3 -c %s -p %s -t %d -P %d %s -J" % (
        LOG_HOST,
        LOG_PORT,
        RUN_S,
        streams,
        "-R" if reverse else "",
    )
    out = sh(cmd, RUN_S + 30)
    try:
        j = json.loads(out)
        e = j["end"]
        return {
            "Mbit_s": round(
                (e["sum_received"] if reverse else e["sum_sent"])["bits_per_second"]
                / 1e6
            ),
            "retransmits": e["sum_sent"].get("retransmits"),
        }
    except Exception:
        return {"error": out[-300:]}


def wait_net():
    for _ in range(60):
        if LOG_HOST and "1 received" in sh("ping -c1 -W1 %s" % LOG_HOST):
            return True
        time.sleep(1)
    return False


def main():
    save()
    note("start; kernel %s" % RESULT["kernel"])
    RESULT["disks"] = sh("lsblk -d -o NAME,SIZE,MODEL,SERIAL,TRAN")
    RESULT["usb"] = sh("lsusb -t 2>/dev/null | head -30")
    RESULT["links"] = sh("ethtool eth1 2>/dev/null | grep -E 'Speed|Link detected'")
    RESULT["addr"] = sh("ip -br a")
    devs = {k: dev_by_serial(v) for k, v in SERIALS.items()}
    RESULT["devs"] = devs
    save()
    for k in ("sata_ssd", "hdd_wd", "usb_860evo", "usb_m2_a", "usb_m2_b"):
        if devs.get(k):
            measured("fio_" + k, lambda d=devs[k]: fio([d]))
    usb = [devs[k] for k in ("usb_860evo", "usb_m2_a", "usb_m2_b") if devs.get(k)]
    if len(usb) > 1:
        measured("fio_usb_together", lambda: fio(usb))
    if wait_net():
        for streams in (1, 4):
            measured("net_rx_%d" % streams, lambda s=streams: iperf(s, True))
            measured("net_tx_%d" % streams, lambda s=streams: iperf(s, False))
        if devs.get("sata_ssd") and usb:
            import threading

            res = {}
            t = threading.Thread(target=lambda: res.update(rx=iperf(4, True)))

            def combo():
                t.start()
                res["fio"] = fio([devs["sata_ssd"]] + usb)
                t.join()
                return res

            measured("combined_net_rx4_sata_usb", combo)
    else:
        note("network to %s not reachable after 60 s; skipped iperf" % LOG_HOST)
    RESULT["finished"] = time.strftime("%FT%T%z")
    save()
    sh("sync")
    note("done; restarting via sysrq-b")
    sh("sync")
    with open("/proc/sysrq-trigger", "w") as f:
        f.write("b")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:  # always leave a trace and always leave the box
        RESULT["notes"].append("EXCEPTION %r" % (e,))
        save()
        sh("sync")
        with open("/proc/sysrq-trigger", "w") as f:
            f.write("b")
