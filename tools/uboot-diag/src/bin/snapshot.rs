//! Rust twin of scripts/snapshot.py - full hardware-state snapshot: CPU,
//! GIC, CCU, PLL ring, bootstrap strap, DDR-ready/SRAM-agent block, PCIe
//! ext0 + GPIO, NOR/MTD partition checksums, full I2C bus/mux scan +
//! known-device state. Every address cited against docs/hardware.md.
//! Sections degrade gracefully rather than abort the whole snapshot.
//! Root only.

use std::fs;
use std::path::Path;
use std::process::Command;
use uboot_diag::{dump_gpio, dump_regs, ltssm_state, MemReader};

fn run(cmd: &str, args: &[&str]) -> Option<String> {
    Command::new(cmd)
        .args(args)
        .output()
        .ok()
        .filter(|o| o.status.success() || !o.stdout.is_empty())
        .map(|o| String::from_utf8_lossy(&o.stdout).into_owned())
}

fn dump_cpu() {
    println!("--- CPU ---");
    if let Some(u) = run("uname", &["-a"]) {
        println!("uname: {}", u.trim());
    }
    if let Ok(cpuinfo) = fs::read_to_string("/proc/cpuinfo") {
        let impl_ = cpuinfo
            .lines()
            .find(|l| l.starts_with("CPU implementer"))
            .and_then(|l| l.split(':').nth(1))
            .map(str::trim)
            .unwrap_or("?");
        let part = cpuinfo
            .lines()
            .find(|l| l.starts_with("CPU part"))
            .and_then(|l| l.split(':').nth(1))
            .map(str::trim)
            .unwrap_or("?");
        let cores = cpuinfo.matches("processor\t:").count();
        println!("implementer={impl_} part={part} cores={cores}");
    }
}

fn dump_gic(mem: &MemReader) {
    let base = 0xF0200000u64;
    dump_regs(
        mem,
        "GIC-v3 distributor",
        &[
            ("GICD_CTLR", base),
            ("GICD_TYPER", base + 0x4),
            ("GICD_ISENABLER0", base + 0x100),
            ("GICD_ISENABLER1", base + 0x104),
            ("GICD_ISENABLER2", base + 0x108),
        ],
    );
}

fn dump_ccu(mem: &MemReader) {
    let base = 0xF0090000u64;
    dump_regs(
        mem,
        "CCU (coherency)",
        &[
            ("speculation_ctrl", base + 0x4),
            ("slaves[3].snoop_ctrl (cluster0)", base + 0x4000),
            ("slaves[4].snoop_ctrl (cluster1)", base + 0x5000),
        ],
    );
}

fn dump_pll(mem: &MemReader) {
    let base = 0xFD860000u64;
    dump_regs(
        mem,
        "PLL ring",
        &[
            ("SB_PLL setup_0", base + 0xB00),
            ("SB_PLL status_7", base + 0xB00 + 0x1C),
            ("NB_PLL setup_0", base + 0xC00),
            ("NB_PLL status_7", base + 0xC00 + 0x1C),
            ("CPU_PLL setup_0", base + 0xD00),
            ("CPU_PLL status_7", base + 0xD00 + 0x1C),
            ("thermal", base + 0xA00),
        ],
    );
}

fn dump_bootstrap(mem: &MemReader) {
    dump_regs(
        mem,
        "PBS bootstrap (raw - see read-ddr-bootstrap.py for decode)",
        &[("boot_strap", 0xFD8A8110)],
    );
}

fn dump_ddr_sram(mem: &MemReader) {
    let base = 0xFBFF4000u64;
    dump_regs(
        mem,
        "DDR-ready / SRAM-agent block",
        &[
            ("SRAM_DEV_INFO_ADDRESS", base + 0x100),
            ("SRAM_CPU_RESUME_ADDRESS (cold/warm magic)", base + 0x120),
            ("magic_num/ddr_size", base + 0x150),
            ("SRAM_AGENT_ADDRESS", base + 0x200),
        ],
    );
}

fn dump_pcie_ext0_gpio(mem: &MemReader) {
    dump_regs(mem, "PCIe ext0 (#140)", &uboot_diag::pcie_ext0_regs());
    if let Some(v) = mem.read32(uboot_diag::PCIE_EXT0_BASE + 0x2080) {
        println!("  -> LTSSM state 0x{:x}", ltssm_state(v));
    }
    dump_gpio(mem);
}

fn dump_mtd() {
    println!("--- MTD / NOR partitions ---");
    let Some(out) = run("cat", &["/proc/mtd"]) else {
        println!("(no /proc/mtd on this boot)");
        return;
    };
    print!("{out}");
    let small = [
        "u-boot",
        "u-boot-env",
        "u-boot-env-redundant",
        "factory",
        "eeprom",
        "cksum",
    ];
    for line in out.lines().skip(1) {
        let Some(dev) = line.split(':').next() else { continue };
        let Some(label_start) = line.find('"') else { continue };
        let label = line[label_start + 1..].trim_end_matches('"');
        if !small.contains(&label) {
            continue;
        }
        if let Some(sha) = run("sha256sum", &[&format!("/dev/{dev}")]) {
            if let Some(hash) = sha.split_whitespace().next() {
                println!("  {label:<24} {dev:<8} {hash}");
            }
        }
    }
}

fn dump_i2c_scan() {
    println!("--- I2C bus scan (all buses incl. pca9546 mux channels) ---");
    let Some(buses) = run("i2cdetect", &["-l"]) else {
        return;
    };
    print!("{buses}");
    for line in buses.lines() {
        let Some(rest) = line.strip_prefix("i2c-") else { continue };
        let Some(n) = rest.split_whitespace().next() else { continue };
        if let Some(out) = run("i2cdetect", &["-y", n]) {
            println!("i2c-{n}:\n{}", out.trim());
        }
    }
}

fn dump_i2c_devices() {
    println!("--- Known I2C device state (docs/hardware.md:373-379) ---");

    if let Some(out) = run("hwclock", &["--rtc=/dev/rtc0", "-r"]) {
        println!("RTC (s35390a): {}", out.trim());
    }

    if let Ok(entries) = fs::read_dir("/sys/class/hwmon") {
        for e in entries.flatten() {
            let name_f = e.path().join("name");
            if fs::read_to_string(&name_f).map_or(false, |n| n.contains("adt7475")) {
                println!("ADT7475 hwmon: {}", e.path().display());
                if let Ok(files) = fs::read_dir(e.path()) {
                    let mut names: Vec<_> = files.flatten().map(|f| f.path()).collect();
                    names.sort();
                    for f in names {
                        let fname = f.file_name().unwrap().to_string_lossy();
                        if (fname.starts_with("temp") || fname.starts_with("fan"))
                            && fname.ends_with("_input")
                        {
                            if let Ok(v) = fs::read_to_string(&f) {
                                println!("  {:<16} {}", fname, v.trim());
                            }
                        }
                    }
                }
            }
        }
    }

    if Path::new("/sys/bus/i2c/devices/0-0057").exists() {
        println!("EEPROM (at24c64 @0x57) sysfs node: /sys/bus/i2c/devices/0-0057");
    }
}

fn main() {
    let Some(mem) = MemReader::open() else {
        std::process::exit(2);
    };

    println!("=== awto-unvr full hardware snapshot (Rust) ===");
    dump_cpu();
    dump_gic(&mem);
    dump_ccu(&mem);
    dump_pll(&mem);
    dump_bootstrap(&mem);
    dump_ddr_sram(&mem);
    dump_pcie_ext0_gpio(&mem);
    dump_mtd();
    dump_i2c_scan();
    dump_i2c_devices();
    println!("=== snapshot done ===");
}
