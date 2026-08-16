// SetupAlpineMemory.java -- create the AL-324 (Alpine V2) MMIO + RAM memory blocks
// BEFORE auto-analysis so the analyzer never wanders into unmapped space and MMIO
// is correctly marked volatile / non-executable.
//
// Map derived from docs/hw-reference/.../iomem.txt + live.dts (see docs/ghidra.md
// §3). MMIO blocks: uninitialized, R/W, volatile, NON-exec. RAM blocks: R/W/exec.
// Idempotent: a block already present is left alone.
//
// The image's own code/data block (from BinaryLoader) is untouched; this only ADDS
// the peripheral + RAM regions around it. Register labels are a separate step
// (ApplyAlRegs.java). For the 32-bit preboot, the al_boot payload loads at
// 0x01000000 and additionally touches SoC-fabric regs (0xf00xxxxx, 0xfbffxxxx) and
// S2 SRAM 0xf2200000 -- pass --preboot-fabric via arg[0]=preboot to add those too.
//
// Usage (headless):
//   analyzeHeadless <proj> <name> -process <bin> -preScript SetupAlpineMemory.java [preboot]
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;

public class SetupAlpineMemory extends GhidraScript {
    // {name, base, size, isRam}. MMIO -> volatile,non-exec; RAM -> exec.
    // Keep in sync with the table in docs/ghidra.md §3.
    static final Object[][] MAP = {
        // --- DRAM (two banks; iomem: 0-0xbfffffff and 0x2_00000000-0x2_3fffffff) ---
        {"DRAM0", 0x00000000L, 0xc0000000L, true},   // bank0, 3 GiB
        {"DRAM1", 0x200000000L, 0x40000000L, true},  // bank1, 1 GiB above 4 GiB
        // --- PBS peripherals (fd88xxxx / fd89xxxx / fd8axxxx / fd8bxxxx) ---
        {"i2c0",  0xfd880000L, 0x1000L, false},      // snps,designware-i2c (i2c-pld)
        {"spi0",  0xfd882000L, 0x1000L, false},      // amazon,alpine-dw-apb-ssi
        {"uart0", 0xfd883000L, 0x1000L, false},      // ns16550a
        {"uart1", 0xfd884000L, 0x1000L, false},
        {"uart2", 0xfd885000L, 0x1000L, false},
        {"uart3", 0xfd886000L, 0x1000L, false},      // in DT; not mapped in Linux iomem
        {"gpio0", 0xfd887000L, 0x1000L, false},      // arm,pl061
        {"gpio1", 0xfd888000L, 0x1000L, false},
        {"gpio2", 0xfd889000L, 0x1000L, false},
        {"gpio3", 0xfd88a000L, 0x1000L, false},
        {"gpio4", 0xfd88b000L, 0x1000L, false},
        {"wdt0",  0xfd88c000L, 0x4000L, false},      // arm,sp805 x4 (fd88c-fd88f)
        {"timer", 0xfd890000L, 0x4000L, false},      // arm,sp804 x4 (fd890-fd893)
        {"i2c1",  0xfd894000L, 0x1000L, false},      // snps,designware-i2c
        {"gpio5", 0xfd897000L, 0x1000L, false},      // arm,pl061
        {"otp_efuse", 0xfd896000L, 0x1000L, false},  // eFuse/OTP; modulus hash @0xfd89608c
        {"pbs",   0xfd8a8000L, 0x1000L, false},      // annapurna-labs,al-pbs
        {"sgpo",  0xfd8b4000L, 0x5000L, false},      // annapurna-labs,alpine-sgpo
        // --- PCIe controllers / ECAM ---
        {"pcie_ext0_ctl", 0xfd800000L, 0x20000L, false},
        {"pcie_ext1_ctl", 0xfd820000L, 0x20000L, false},
        {"pcie_ext2_ctl", 0xfd840000L, 0x20000L, false},
        {"pcie_int_ecam", 0xfbc00000L, 0x100000L, false},
        {"pcie_ext0_win", 0xfb600000L, 0x100000L, false},
        {"pcie_ext0_mem", 0xc0010000L, 0x07ff0000L, false}, // BAR window (xhci behind)
        // --- IO fabric: eth / dma / sata (fe0xxxxx / fe1xxxxx) ---
        {"eth0",  0xfe000000L, 0x20000L, false},     // al_eth
        {"eth1",  0xfe020000L, 0x20000L, false},
        {"dma0",  0xfe0e0000L, 0x20000L, false},     // al_dma (udma)
        {"dma1",  0xfe100000L, 0x20000L, false},
        {"eth2",  0xfe120000L, 0x10000L, false},
        {"dma2",  0xfe140000L, 0x10000L, false},
        {"eth3",  0xfe150000L, 0x4000L, false},
        {"ahci0", 0xfe154000L, 0x4000L, false},      // SATA
        {"ahci1", 0xfe158000L, 0x4000L, false},
        {"eth4",  0xfe15c000L, 0x1000L, false},
        {"eth5",  0xfe15d000L, 0x1000L, false},
    };
    // Preboot-only SoC-fabric regions (not in Linux DT; from preboot-decompile.md).
    static final Object[][] PREBOOT = {
        {"s2_sram",    0xf2200000L, 0x40000L, true},  // S2 first-stage link base (SRAM)
        {"agent_mb0",  0xf0070000L, 0x1000L, false},  // CVOS agent mailbox
        {"agent_mb1",  0xf0090000L, 0x1000L, false},
        {"ddr_ready",  0xfbff4000L, 0x1000L, false},  // _DAT_fbff4150 = 0x31415926 poll
    };

    @Override public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        int made = 0;
        for (Object[] r : MAP) made += add(mem, r);
        String[] a = getScriptArgs();
        if (a.length > 0 && a[0].equalsIgnoreCase("preboot"))
            for (Object[] r : PREBOOT) made += add(mem, r);
        println("SetupAlpineMemory: added " + made + " blocks");
    }

    private int add(Memory mem, Object[] r) throws Exception {
        String name = (String) r[0];
        long base = ((Number) r[1]).longValue(), size = ((Number) r[2]).longValue();
        boolean ram = (Boolean) r[3];
        Address at = toAddr(String.format("0x%x", base));
        // Skip if any part of the range already belongs to a block (e.g. loaded image).
        if (mem.getBlock(at) != null) { println(name + ": overlaps existing, skip"); return 0; }
        MemoryBlock b = mem.createUninitializedBlock(name, at, size, false);
        b.setRead(true); b.setWrite(true);
        b.setExecute(ram);          // RAM executable; MMIO never
        b.setVolatile(!ram);        // MMIO volatile; RAM not
        println(String.format("%s: 0x%x +0x%x%s", name, base, size, ram ? " RAM" : " MMIO(vol)"));
        return 1;
    }
}
