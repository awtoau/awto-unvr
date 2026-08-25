# Ghidra dev build — install info (for docs/ghidra.md)

Built 2026-08-16 from current master. Replaces the stale 2026-05-17 12.2_DEV snapshot.

## What was built
- **Version:** Ghidra 12.2 DEV
- **Git commit:** `d5f144c24d6bc53c9cbf4448c6d11143e7696206` (NationalSecurityAgency/ghidra master, authored 2026-08-13)
- **Build date string:** `2026-Aug-16 1201 AEST` (`application.build.date.short=20260816`)
- **Install path:** `/home/dan/tools/ghidra_12.2_DEV_20260816`
- **Source tree:** `/home/dan/tools/ghidra-src`
- **Dist zip:** `/home/dan/tools/ghidra-src/build/dist/ghidra_12.2_DEV_20260816.zip` (540 MB)
- Old stale install still present at `/home/dan/tools/ghidra_12.2_DEV` (May snapshot, untouched).

## JDK — CHANGED vs the old build
- Master now hard-requires **JDK 25** to build AND run:
  `application.java.min=25`, `application.java.compiler=25` (bytecode targets 25, so it
  will NOT run on JDK 21 — the launcher rejects 21). The old May build was java.min=21.
- README line 30 ("Install JDK 21") is the STALE run-time note; the build prereq (line 50)
  is JDK 25. compiler=25 is authoritative → JDK 25 required at runtime.
- Runtime JDK used: **Temurin/OpenJDK 25.0.4** at `/usr/lib/jvm/java-25-openjdk`, set via
  `JAVA_HOME_OVERRIDE=/usr/lib/jvm/java-25-openjdk` in `support/launch.properties`.
- **"JDK 25 hangs the decompiler" is STALE / does not reproduce** on this master build.
  Decompiler completed in seconds — see verification below.

## Build recipe (reused standard Ghidra dev flow)
- gradle: cached **9.4.1** wrapper dist (master needs 9.1+):
  `/home/dan/.gradle/wrapper/dists/gradle-9.4.1-bin/.../gradle-9.4.1/bin/gradle`
- `JAVA_HOME=/usr/lib/jvm/java-25-openjdk gradle -I gradle/support/fetchDependencies.gradle`
- `JAVA_HOME=/usr/lib/jvm/java-25-openjdk gradle buildGhidra` (BUILD SUCCESSFUL ~4 min)
- extract dist zip → rename inner `ghidra_12.2_DEV` to dated `ghidra_12.2_DEV_20260816`.
- Full build log: `tmp/logs/ghidra-build.log`.

## Verification (headless import + analyze + decompile, JDK 25)
- **ELF `/bin/true` (x86-64, complete binary):** 103 functions exported, **0 decompile
  failures**, 3264 lines of C, whole run ~30 s. Log: `tmp/logs/ghidra-verify-elf.log`.
- **ARM32 `s2-blob.bin` (25 KB truncated blob @0x1000000):** 69 functions, 68 clean +
  1 SKIP (`FUN_01001a90` — branch into non-existing memory, a blob-truncation artifact,
  not a decompiler fault), 3180 lines C. Log: `tmp/logs/ghidra-verify.log`.
- `scripts/ghidra-decompile.py` updated to point at the new install; re-ran end-to-end
  clean (exit 0).
