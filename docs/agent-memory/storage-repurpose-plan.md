---
name: storage-repurpose-plan
description: "Plan for the UNVR's two 8TB SATA disks - analyse, wipe, repurpose for the Linux port"
metadata: 
  node_type: memory
  type: project
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-16T01:10:53.495Z
---

Two 8 TB WD `WDC_WD82PURZ` in the UNVR, `md3` RAID1 mirror (~6.9 TB recordings + Protect DB). Both members are identical (mirror).

Decision (owner, 2026-08-16):
- **Recordings not needed → discard.**
- **Analyse both read-only first** for anything useful, THEN wipe.
- **Wipe one → Linux test-boot / root drive** for the AL-324 mainline port.
- **Other → work store**: copy all useful artifacts onto it — rescued USB image, NAND + NOR dumps, analysis docs, and the gathered upstream refs staged at `/mnt/2tb/unvr-port-refs` (Linux 6.18/7.1 source, prior-art repos, al_eth). "Same for USB/NAND/NOR contents."

Constraints / gotchas:
- SATA showed **link-down + ata5/ata7 PHY errors** at capture — confirm drives present + SMART-healthy before relying on them. Ties to the SFP/ata concerns parked earlier.
- `/mnt/2tb` host disk is 96% full (~87 GB) — big gathers must land on the work-store drive, not here.
- Non-binary artifacts already on host + GitHub `awtoau/awto-unvr`.

Feeds the [[docs-terse-no-history]] roadmap (Phase 1 needs a test root target = the wiped drive).
