// ApplyAlRegs.java -- apply AL register labels (+ optional bitfield equates)
// produced by scripts/gen-al-reg-symbols.py.
//
// Each arg is an input file:
//   *.sym       lines "<name>\t0x<addr>"  -> create a label at <addr>
//   *.equ.tsv   lines "<name>\t<decimal>" -> named equate over matching operands
//
// Label rule (from awto-2000 SetupMemory): only replace a null / DAT_ / DEFAULT
// symbol; never clobber a curated name. Addresses outside any block are skipped
// (run SetupAlpineMemory.java first so peripheral blocks exist).
//
// Equates are HEURISTIC: a named equate is attached to every scalar operand whose
// value == the define AND value >= MIN_EQU (default 8) to avoid painting ubiquitous
// 0/1/2 shift amounts everywhere. Treat as a readability aid, verify at call sites.
//
// Usage (headless, after auto-analysis):
//   analyzeHeadless <proj> <name> -process <bin> -noanalysis \
//     -postScript ApplyAlRegs.java tmp/ghidra-in/i2c0.sym tmp/ghidra-in/AL_I2C.equ.tsv
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.lang.OperandType;
import java.io.*;

public class ApplyAlRegs extends GhidraScript {
    static final long MIN_EQU = 8;

    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        int labels = 0, equates = 0;
        for (String path : args) {
            if (path.endsWith(".equ.tsv")) equates += applyEquates(path);
            else labels += applyLabels(path);
        }
        println("ApplyAlRegs: " + labels + " labels, " + equates + " equate refs");
    }

    private int applyLabels(String path) throws Exception {
        SymbolTable syms = currentProgram.getSymbolTable();
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
        int n = 0;
        BufferedReader br = new BufferedReader(new FileReader(path));
        String line;
        while ((line = br.readLine()) != null) {
            String[] p = line.split("\t");
            if (p.length < 2) continue;
            long va;
            try { va = Long.decode(p[1].trim()); } catch (Exception e) { continue; }
            Address a = sp.getAddress(va);
            if (currentProgram.getMemory().getBlock(a) == null) continue; // unmapped
            Symbol s = syms.getPrimarySymbol(a);
            if (s == null || s.getName().startsWith("DAT_") || s.getSource() == SourceType.DEFAULT) {
                createLabel(a, p[0].trim(), true, SourceType.IMPORTED);
                n++;
            }
        }
        br.close();
        println("  " + path + ": " + n + " labels");
        return n;
    }

    private int applyEquates(String path) throws Exception {
        EquateTable et = currentProgram.getEquateTable();
        Listing lst = currentProgram.getListing();
        int refs = 0;
        BufferedReader br = new BufferedReader(new FileReader(path));
        String line;
        java.util.Map<Long, String> want = new java.util.HashMap<>();
        while ((line = br.readLine()) != null) {
            String[] p = line.split("\t");
            if (p.length < 2) continue;
            try {
                long v = Long.parseLong(p[1].trim());
                if (v >= MIN_EQU) want.put(v, p[0].trim());
            } catch (Exception e) { /* skip */ }
        }
        br.close();
        if (want.isEmpty()) return 0;
        InstructionIterator it = lst.getInstructions(true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            for (int op = 0; op < ins.getNumOperands(); op++) {
                if ((ins.getOperandType(op) & OperandType.SCALAR) == 0) continue;
                Scalar sc = ins.getScalar(op);
                if (sc == null) continue;
                String nm = want.get(sc.getUnsignedValue());
                if (nm == null) continue;
                Equate e = et.getEquate(nm);
                if (e == null) e = et.createEquate(nm, sc.getUnsignedValue());
                e.addReference(ins.getAddress(), op);
                refs++;
            }
        }
        println("  " + path + ": " + refs + " equate refs");
        return refs;
    }
}
