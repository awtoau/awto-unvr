# sources

Downloaded artifacts. Binaries are gitignored; this manifest is the tracked part.

## UNVR-5.1.25.bin

| | |
|---|---|
| URL | <https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-4d75bc90-b895-4d8f-baff-382f24fd36cf.bin> |
| Version | `v5.1.25+84c48e7` |
| ubnt_version | `UNVR4.al324.v5.1.25.84c48e7.260710.1602` |
| Released | 2026-07-27 |
| Retrieved | 2026-08-15T14:05:38+10:00 |
| Size | 786253430 bytes |
| sha256 | `74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc` |
| md5 | `3f661a852b6fb70946e2a94712df6923` |

Discovered via `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr`
(platform `UNVR`, product `unifi-nvr`).

### Telling a good copy from a bad one

- Exactly 786253430 bytes. Any other size is wrong.
- sha256 must match above. Verify: `./scripts/fetch-unvr-firmware.py verify`
- Not an HTML login/error page: `file` reports data, not HTML, and the first
  bytes are not `<!DOCTYPE`.

Fetched with curl by explicit approval - playwrong has no binary-download tool.
The vendor checksum is what makes that safe; do not skip verification.
