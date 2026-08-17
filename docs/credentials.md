# Console credentials

Stock factory defaults — publicly documented, **not** device secrets. Same for
every unit of the given firmware generation; verified against each firmware's
baked-in `/etc/shadow` ($5$/$6$ crypt, cracked).

| Firmware generation | Versions | User | Password |
|---|---|---|---|
| arm64 | 1.3.35, 1.4.9 | root | `ubnt` |
| al324 | 2.3.14 … 5.1.25 | root | `ui` |

- Serial console: **115200 8N1**.
- Works while USERDEV has no configured accounts (fresh/wiped stick) — the image's
  built-in root account is in force until the setup wizard runs.
- Recovery mode (serial/telnet): `root` / `ubnt`.

Device-specific secrets are **not published** — RSA keys, TLS/SSH private keys,
password hashes, JWT secret, cloud identity. Their inventory and locations are in
`secrets.yaml` (gitignored); impact is tracked in the security-analysis issue.
