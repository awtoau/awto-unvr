// DisasmGaps.java -- headless PRE-script: disassemble a supplied list of addresses
// (one hex VA per line) and turn each into a function, so auto-analysis then follows
// their call graph. Used to recover code that is reachable only through pointer
// tables / vtables the analyzer will not chase -- verified as code out-of-band
// (capstone control-flow test in scripts/measure-preboot-coverage.py) before feeding
// here, so this never forces data into code blindly.
//
// arg[0] = path to address list file. arg[1] = "arm" | "thumb" (default arm).
// Runs BEFORE auto-analysis. Skips an address that is already an instruction.
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import java.io.*;
import java.math.BigInteger;
import java.util.*;

public class DisasmGaps extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        if (a.length < 1) { println("DisasmGaps: need address-list file arg"); return; }
        boolean thumb = a.length > 1 && a[1].equalsIgnoreCase("thumb");
        Register tmode = currentProgram.getProgramContext().getRegister("TMode");
        int n = 0;
        for (String line : readLines(a[0])) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            // per-line "<addr> [arm|thumb]" overrides the default mode
            String[] tok = line.split("\\s+");
            boolean t = thumb;
            if (tok.length > 1) t = tok[1].equalsIgnoreCase("thumb");
            Address at = toAddr(tok[0]);
            if (getInstructionAt(at) != null) continue;
            if (tmode != null)
                currentProgram.getProgramContext().setValue(tmode, at, at,
                    t ? BigInteger.ONE : BigInteger.ZERO);
            clearListing(at);
            disassemble(at);
            createFunction(at, null);
            n++;
        }
        println("DisasmGaps: disassembled " + n + " gap addresses");
    }

    private List<String> readLines(String path) throws IOException {
        List<String> out = new ArrayList<>();
        try (BufferedReader r = new BufferedReader(new FileReader(path))) {
            for (String s; (s = r.readLine()) != null; ) out.add(s);
        }
        return out;
    }
}
