// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Fetching the SFP+ EEPROM off the box.
//!
//! Preferred path is `ethtool -m`: phylink publishes netdev->sfp_bus, so the
//! core ethtool code serves A0h+A2h without al_eth implementing anything
//! (al_eth_phylink.c). i2c is the fallback for when the module is not bound to
//! an sfp bus - it reads the same two pages directly off the mux channel.

use crate::exec::Runner;
use crate::sff8472::Eeprom;
use std::time::Duration;

/// The SFP cage sits on mux ch1 = Linux i2c-2 (docs/i2c-map.md).
pub const DEFAULT_I2C_BUS: u32 = 2;

fn parse_hex_stream(text: &str) -> Vec<u8> {
    let mut out = Vec::new();
    for tok in text.split_whitespace() {
        if tok.len() == 2 {
            if let Ok(b) = u8::from_str_radix(tok, 16) {
                out.push(b);
            }
        }
    }
    out
}

/// i2cdump -y <bus> <addr> b prints "00: xx xx ..." rows, 16 bytes each.
fn parse_i2cdump(text: &str) -> Vec<u8> {
    let mut bytes = vec![0u8; 256];
    let mut seen = false;
    for line in text.lines() {
        let line = line.trim();
        let Some((off, rest)) = line.split_once(':') else {
            continue;
        };
        let Ok(row) = usize::from_str_radix(off.trim(), 16) else {
            continue;
        };
        if row > 0xf0 || row % 16 != 0 {
            continue;
        }
        for (i, tok) in rest.split_whitespace().take(16).enumerate() {
            if let Ok(v) = u8::from_str_radix(tok, 16) {
                if row + i < 256 {
                    bytes[row + i] = v;
                    seen = true;
                }
            }
        }
    }
    if seen {
        bytes
    } else {
        Vec::new()
    }
}

fn via_ethtool(r: &Runner, iface: &str) -> Result<Eeprom, String> {
    let out = r.run(
        &format!("ethtool -m {iface} raw on | od -An -tx1 -v"),
        Duration::from_secs(crate::exec::SHORT_CMD_S),
        "ethtool -m raw",
    )?;
    if !out.ok() {
        return Err(format!(
            "ethtool -m {iface}: exit {} ({})",
            out.code,
            out.stderr.trim()
        ));
    }
    let bytes = parse_hex_stream(&out.stdout);
    if bytes.len() < 96 {
        return Err(format!(
            "ethtool -m {iface}: only {} bytes returned, need at least 96",
            bytes.len()
        ));
    }
    let a0 = bytes[..bytes.len().min(256)].to_vec();
    let a2 = if bytes.len() >= 512 {
        Some(bytes[256..512].to_vec())
    } else {
        None
    };
    Ok(Eeprom {
        a0,
        a2,
        source: format!("ethtool -m {iface} raw on ({} bytes)", bytes.len()),
    })
}

fn via_i2c(r: &Runner, bus: u32) -> Result<Eeprom, String> {
    r.short("modprobe i2c-dev 2>/dev/null || true", "modprobe i2c-dev")?;
    let read = |addr: u32| -> Result<Vec<u8>, String> {
        let o = r.run(
            &format!("i2cdump -y {bus} 0x{addr:02x} b"),
            Duration::from_secs(crate::exec::SHORT_CMD_S),
            &format!("i2cdump 0x{addr:02x}"),
        )?;
        if !o.ok() {
            return Err(format!(
                "i2cdump -y {bus} 0x{addr:02x}: exit {} ({})",
                o.code,
                o.stderr.trim()
            ));
        }
        let b = parse_i2cdump(&o.stdout);
        if b.is_empty() {
            return Err(format!("i2cdump -y {bus} 0x{addr:02x}: no rows parsed"));
        }
        Ok(b)
    };
    let a0 = read(0x50)?;
    let a2 = read(0x51).ok();
    Ok(Eeprom {
        a0,
        a2,
        source: format!("i2c bus {bus}, 0x50 + 0x51 (i2cdump)"),
    })
}

/// `how` is "auto" (ethtool then i2c), "ethtool" or "i2c".
pub fn fetch(r: &Runner, iface: &str, how: &str, bus: u32) -> Result<Eeprom, String> {
    match how {
        "ethtool" => via_ethtool(r, iface),
        "i2c" => via_i2c(r, bus),
        "auto" => match via_ethtool(r, iface) {
            Ok(e) => Ok(e),
            Err(e1) => {
                eprintln!("# ethtool -m path unavailable ({e1}); falling back to i2c");
                via_i2c(r, bus).map_err(|e2| format!("both EEPROM paths failed:\n  {e1}\n  {e2}"))
            }
        },
        other => Err(format!("--via {other}: expected auto, ethtool or i2c")),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_od_hex_stream() {
        let b = parse_hex_stream(" 03 04 07 10 00 00\n 06 67 00 00\n");
        assert_eq!(b, vec![0x03, 0x04, 0x07, 0x10, 0x00, 0x00, 0x06, 0x67, 0x00, 0x00]);
    }

    #[test]
    fn parses_i2cdump_rows() {
        let text = "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n\
                    00: 03 04 07 10 00 00 00 00 00 00 00 06 67 00 00 00\n\
                    10: 00 50 00 1e 41 57 54 4f 20 20 20 20 20 20 20 20\n";
        let b = parse_i2cdump(text);
        assert_eq!(b.len(), 256);
        assert_eq!(b[0], 0x03);
        assert_eq!(b[12], 0x67);
        assert_eq!(b[0x11], 0x50);
        assert_eq!(b[255], 0x00);
    }

    #[test]
    fn empty_dump_is_an_error_not_zeroes() {
        assert!(parse_i2cdump("i2cdump: nothing here\n").is_empty());
    }
}
