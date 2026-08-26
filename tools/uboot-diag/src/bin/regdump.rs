//! Rust twin of scripts/regdump.py - #140 PCIe ext0 + GPIO register dump
//! via /dev/mem, from Linux userspace. Same register list, same output
//! shape. Root only.

use uboot_diag::{dump_gpio, dump_regs, ltssm_name, ltssm_state, MemReader, PCIE_EXT0_BASE};

fn main() {
    let Some(mem) = MemReader::open() else {
        std::process::exit(2);
    };

    dump_regs(&mem, "PCIe ext0 (#140)", &uboot_diag::pcie_ext0_regs());

    if let Some(v) = mem.read32(PCIE_EXT0_BASE + 0x2080) {
        let state = ltssm_state(v);
        println!("  -> LTSSM state 0x{state:x} ({})", ltssm_name(state));
    }

    dump_gpio(&mem);
}
