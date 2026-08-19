// CoverageReport.java -- headless post-script: byte-level code/data/undefined
// accounting over the loaded image block(s), for verifying 100% decompile coverage.
//
// The BinaryLoader image is the only *initialized* memory block (SetupAlpineMemory
// creates its RAM/MMIO regions uninitialized), so we classify every code unit in
// each initialized block:
//   - Instruction            -> CODE
//   - defined Data           -> DATA
//   - undefined Data         -> UNDEF (a gap: undiscovered code or unclassified data)
// Emits per-block totals, the full undefined-run list (addr,len), and the largest
// gaps first, to <out>/coverage.txt. Also counts functions.
//
// Usage (headless): -postScript CoverageReport.java <out_dir>
// @category awto
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import java.io.*;
import java.util.*;

public class CoverageReport extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : "/tmp";
        PrintWriter w = new PrintWriter(new FileWriter(out + "/coverage.txt"));
        Listing listing = currentProgram.getListing();
        Memory mem = currentProgram.getMemory();

        long tCode = 0, tData = 0, tUndef = 0;
        List<long[]> gaps = new ArrayList<>();   // {start, len}

        for (MemoryBlock blk : mem.getBlocks()) {
            if (!blk.isInitialized()) continue;   // skip MMIO/RAM shells
            long bCode = 0, bData = 0, bUndef = 0;
            AddressSet set = new AddressSet(blk.getStart(), blk.getEnd());
            CodeUnitIterator it = listing.getCodeUnits(set, true);
            long gapStart = -1, gapLen = 0;
            while (it.hasNext()) {
                CodeUnit cu = it.next();
                int len = cu.getLength();
                boolean undef = false;
                if (cu instanceof Instruction) { bCode += len; }
                else if (cu instanceof Data) {
                    Data d = (Data) cu;
                    if (d.isDefined()) bData += len; else { bUndef += len; undef = true; }
                } else { bUndef += len; undef = true; }
                long addr = cu.getMinAddress().getOffset();
                if (undef) {
                    if (gapStart < 0) { gapStart = addr; gapLen = len; }
                    else if (addr == gapStart + gapLen) { gapLen += len; }
                    else { gaps.add(new long[]{gapStart, gapLen}); gapStart = addr; gapLen = len; }
                } else if (gapStart >= 0) { gaps.add(new long[]{gapStart, gapLen}); gapStart = -1; }
            }
            if (gapStart >= 0) gaps.add(new long[]{gapStart, gapLen});
            long tot = bCode + bData + bUndef;
            w.printf("BLOCK %-12s 0x%08x..0x%08x  total=%d  code=%d (%.2f%%)  data=%d (%.2f%%)  undef=%d (%.2f%%)%n",
                blk.getName(), blk.getStart().getOffset(), blk.getEnd().getOffset(), tot,
                bCode, pct(bCode, tot), bData, pct(bData, tot), bUndef, pct(bUndef, tot));
            tCode += bCode; tData += bData; tUndef += bUndef;
        }
        long grand = tCode + tData + tUndef;
        w.printf("%nTOTAL total=%d  code=%d (%.2f%%)  data=%d (%.2f%%)  undef=%d (%.2f%%)%n",
            grand, tCode, pct(tCode, grand), tData, pct(tData, grand), tUndef, pct(tUndef, grand));

        int nf = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) nf++;
        w.printf("FUNCTIONS %d%n", nf);

        // Gaps, largest first
        gaps.sort((a, b) -> Long.compare(b[1], a[1]));
        w.printf("%nUNDEFINED RUNS: %d (largest first)%n", gaps.size());
        for (long[] g : gaps)
            w.printf("  0x%08x  len=%d%n", g[0], g[1]);
        w.close();
        println("CoverageReport: code=" + tCode + " data=" + tData + " undef=" + tUndef + " gaps=" + gaps.size());
    }

    private static double pct(long n, long d) { return d == 0 ? 0.0 : 100.0 * n / d; }
}
