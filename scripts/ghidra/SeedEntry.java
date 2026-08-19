// SeedEntry.java -- headless PRE-script: seed the reset entry point so auto-analysis
// follows the call graph from it. A raw BinaryLoader import sets no entry, so code
// only reachable from reset (the exception-vector table, early init) is never
// disassembled and shows up as an "undefined" gap. Seeding the true entry fixes that
// far more safely than force-disassembling arbitrary gaps (which risks eating data).
//
// arg[0] = entry VA (hex, e.g. 0x1000000)
// arg[1] = "arm" (A32) or "thumb" (T32); sets TMode on the entry for Thumb blobs.
// Runs BEFORE auto-analysis. Only disassembles the single seed; the analyzer walks
// the rest. Idempotent: skips if already an instruction there.
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class SeedEntry extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        if (a.length < 1) { println("SeedEntry: need entry VA arg"); return; }
        Address e = toAddr(a[0]);
        boolean thumb = a.length > 1 && a[1].equalsIgnoreCase("thumb");
        if (thumb) {
            Register tmode = currentProgram.getProgramContext().getRegister("TMode");
            if (tmode != null)
                currentProgram.getProgramContext().setValue(tmode, e, e, java.math.BigInteger.ONE);
        }
        currentProgram.getSymbolTable().addExternalEntryPoint(e);
        addEntryPoint(e);
        if (getInstructionAt(e) == null) disassemble(e);
        createFunction(e, null);
        println("SeedEntry: seeded " + e + (thumb ? " (Thumb)" : " (ARM)"));
    }
}
