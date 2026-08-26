//! Shared /dev/mem MMIO access + register lists for regdump/snapshot.
//!
//! Deliberately no external crates - raw libc FFI only (see Cargo.toml),
//! so this cross-builds offline. Register addresses mirror
//! `al_diag_dump()` (uboot-port/board/annapurna/alpine/alpine.c) and
//! scripts/regdump.py / scripts/snapshot.py - keep all four in sync.

use std::ffi::c_void;
use std::os::raw::{c_char, c_int, c_long};

const O_RDONLY: c_int = 0;
const PROT_READ: c_int = 1;
const MAP_SHARED: c_int = 1;
const PAGE_SIZE: usize = 4096;

extern "C" {
    fn open(path: *const c_char, flags: c_int, ...) -> c_int;
    fn close(fd: c_int) -> c_int;
    fn mmap(
        addr: *mut c_void,
        len: usize,
        prot: c_int,
        flags: c_int,
        fd: c_int,
        offset: c_long,
    ) -> *mut c_void;
    fn munmap(addr: *mut c_void, len: usize) -> c_int;
    fn __errno_location() -> *mut c_int;
}

fn errno() -> i32 {
    unsafe { *__errno_location() }
}

/// A held-open /dev/mem handle for repeated register reads.
pub struct MemReader {
    fd: c_int,
}

impl MemReader {
    /// Opens /dev/mem read-only. Fails (None) if not running as root.
    pub fn open() -> Option<Self> {
        let path = b"/dev/mem\0".as_ptr() as *const c_char;
        let fd = unsafe { open(path, O_RDONLY) };
        if fd < 0 {
            eprintln!("/dev/mem open failed (errno {}) -- run as root", errno());
            return None;
        }
        Some(MemReader { fd })
    }

    /// Reads one little-endian u32 from physical address `addr`.
    /// Maps just the containing 4KiB page, per read - simple and safe,
    /// this is a diagnostic tool not a hot loop.
    pub fn read32(&self, addr: u64) -> Option<u32> {
        let page = addr & !(PAGE_SIZE as u64 - 1);
        let off = (addr & (PAGE_SIZE as u64 - 1)) as usize;
        unsafe {
            let map = mmap(
                std::ptr::null_mut(),
                PAGE_SIZE,
                PROT_READ,
                MAP_SHARED,
                self.fd,
                page as c_long,
            );
            if map as isize == -1 {
                eprintln!("mmap(0x{addr:08x}) failed (errno {})", errno());
                return None;
            }
            let byte_ptr = (map as *const u8).add(off);
            let mut bytes = [0u8; 4];
            std::ptr::copy_nonoverlapping(byte_ptr, bytes.as_mut_ptr(), 4);
            munmap(map, PAGE_SIZE);
            Some(u32::from_le_bytes(bytes))
        }
    }
}

impl Drop for MemReader {
    fn drop(&mut self) {
        unsafe {
            close(self.fd);
        }
    }
}

/// Prints "name @ 0xADDR = 0xVALUE" for each register, skipping ones that
/// fail to read (matches the Python scripts' graceful-degradation style).
pub fn dump_regs(mem: &MemReader, section: &str, regs: &[(&str, u64)]) {
    println!("--- {section} ---");
    for (name, addr) in regs {
        if let Some(val) = mem.read32(*addr) {
            println!("{name:<20} @ 0x{addr:08x} = 0x{val:08x}");
        }
    }
}

pub const PCIE_EXT0_BASE: u64 = 0xFD800000;

/// al_diag_dump()'s register list, alpine.c - keep addresses in sync.
pub fn pcie_ext0_regs() -> Vec<(&'static str, u64)> {
    let base = PCIE_EXT0_BASE;
    vec![
        ("DEV_REV_ID", base + 0x16C),
        ("GLOBAL_CONF", base + 0xC8),
        ("PF_SEL", base + 0x1B0),
        ("MASTER_ARCTL", base + 0x14),
        ("MASTER_AWCTL", base + 0x18),
        ("PBS_PCIE_CONF_1", 0xFD8A80E4),
        ("CFG_TARGET_BUS", base + 0x30),
        ("PORT_INIT", base + 0x2000),
        ("DEBUG_INFO_0", base + 0x2080),
        ("CFGHDR_VID_DID", base + 0x10000),
        ("CFGHDR_CMD_STAT", base + 0x10004),
        ("PORT_LINK_CTRL", base + 0x10700 + 0x10),
        ("GEN2_CTRL", base + 0x10700 + 0x10C),
        ("VC0_POSTED_Q", base + 0x10700 + 0x48),
        ("VC0_NONPOST_Q", base + 0x10700 + 0x4C),
        ("AXI_SLAVE_ERR", base + 0x10700 + 0x1D0),
    ]
}

/// PL061 GPIODATA, address-mask trick: offset 0x3FC = all 8 mask bits set,
/// i.e. the read returns the real port value, not a masked subset.
pub const GPIO_BANK_BASE: [u64; 6] = [
    0xFD887000, 0xFD888000, 0xFD889000, 0xFD88A000, 0xFD88B000, 0xFD897000,
];
pub const GPIO_GPIODATA_ALL_OFF: u64 = 0x3FC;

pub fn dump_gpio(mem: &MemReader) {
    println!("--- GPIO banks (docs/hardware.md:242-251) ---");
    for (i, base) in GPIO_BANK_BASE.iter().enumerate() {
        if let Some(val) = mem.read32(base + GPIO_GPIODATA_ALL_OFF) {
            println!("GPIO bank {i} @ 0x{base:08x} = 0x{:02x}", val & 0xFF);
        }
    }
}

/// LTSSM state names, al_hal_pcie.h - only the ones this issue has hit.
pub fn ltssm_name(state: u32) -> &'static str {
    match state {
        0x0 => "DETECT_QUIET",
        0x1 => "DETECT_ACT",
        0x2 => "POLL_ACTIVE",
        0x3 => "POLL_COMPLIANCE",
        0x4 => "POLL_CONFIG",
        0x5 => "PRE_DETECT_QUIET",
        0x6 => "DETECT_WAIT",
        0x11 => "L0",
        0x12 => "L0S",
        _ => "?",
    }
}

pub fn ltssm_state(debug_info_0: u32) -> u32 {
    (debug_info_0 & 0x1F8) >> 3
}
