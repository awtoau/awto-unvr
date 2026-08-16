// ExportAll.java -- headless post-script: decompile every function to files.
// Adapted (trimmed, project-neutral) from awto-2000/tools/ghidra/ExportAll.java.
//
// arg[0] = output directory (default /tmp)
// Writes <out>/decompiled.c and <out>/disassembly.asm. Each function is preceded
// by its entry VA; the two no-signal Ghidra boilerplate warnings are stripped.
//
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class ExportAll extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : "/tmp";
        FunctionManager fm = currentProgram.getFunctionManager();
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter dec = new PrintWriter(new FileWriter(out + "/decompiled.c"));
        PrintWriter lst = new PrintWriter(new FileWriter(out + "/disassembly.asm"));
        Listing listing = currentProgram.getListing();
        int nf = 0;
        for (Function f : fm.getFunctions(true)) {
            nf++;
            String vh = Long.toHexString(f.getEntryPoint().getOffset());
            lst.println("\n; ==== " + f.getName() + " @ " + f.getEntryPoint() + " ====");
            InstructionIterator it = listing.getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                lst.println(ins.getAddress() + ":  " + ins);
            }
            DecompileResults r = di.decompileFunction(f, 30, monitor);
            if (r != null && r.decompileCompleted() && r.getDecompiledFunction() != null) {
                String c = r.getDecompiledFunction().getC();
                if (c != null) {
                    dec.println("/* @ 0x" + vh + "  " + f.getName() + " */");
                    dec.println(denoise(c));
                    continue;
                }
            }
            println("SKIP decompile: " + f.getName() + " @ " + f.getEntryPoint());
        }
        dec.close();
        lst.close();
        println("Exported " + nf + " functions to " + out);
    }

    // Drop the two most common no-information Ghidra boilerplate warnings.
    private static String denoise(String c) {
        StringBuilder sb = new StringBuilder();
        for (String ln : c.split("\n", -1)) {
            String t = ln.trim();
            if (t.equals("/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */")
                || t.equals("/* WARNING: Unknown calling convention -- yet parameter storage is locked */"))
                continue;
            sb.append(ln).append("\n");
        }
        return sb.toString();
    }
}
