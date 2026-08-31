# Reference DTS — not built

Community/upstream device trees for Alpine V2 boards we do not build. Kept for
reading, not compiling: nothing in `scripts/` stages or registers them.

| file | board | source |
|---|---|---|
| `alpine-v2-ubnt-udmpro.dts` | UniFi Dream Machine Pro | live device analysis (4.19.152-ui-alpine) + mainline `amazon/alpine-v2.dtsi` |
| `alpine-v2-ubnt-unvr.dts` | UNVR ea1a (the eMMC variant) | community Alpine V2 port |

Both are COMMUNITY content — see the header in each file.

The board we actually build is `../alpine-v2-ubnt-unvr-ea16.dts` (ea16, the
NAND-boot variant), staged by `scripts/build-linux-ea16.py`.

Extracted from the retired `kernel-patches/` series, which nothing applied and
which had drifted 391 lines from the built DTS.
