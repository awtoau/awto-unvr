// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! The `serdes_tx_*` sysfs knobs, the link bounce that commits them, and the
//! shared driver-unit budget.
//!
//! Two facts the API enforces so callers cannot get them wrong:
//! - store parses HEX (`kstrtoul(buf, 16)`), show prints DECIMAL - writing "10"
//!   sets 16 (al_eth_sysfs.c).
//! - a write only sets tx_param_dirty; al_eth_serdes_static_tx_params_set()
//!   runs from phylink pcs_config, so the link must bounce to apply it.

use crate::exec::Runner;
use std::collections::BTreeMap;
use std::time::{Duration, Instant};

pub const TX_PARAMS: [&str; 6] = [
    "amp",
    "total_driver_units",
    "c_minus_1",
    "c_plus_1",
    "c_plus_2",
    "slew_rate",
];

/// Taps that draw from the total_driver_units pool.
pub const TAPS: [&str; 3] = ["c_minus_1", "c_plus_1", "c_plus_2"];

/// Time for a 10G link to re-establish after a bounce. Observed ~6 s
/// (scripts/serdes-tx-sweep.py LINK_SETTLE_S); 8 s ~= 1.3x. On expiry: the
/// point is recorded as LINK DOWN and scored as invalid, never as "clean".
pub const LINK_UP_LIMIT_S: u64 = 8;

/// amp is a 3-bit swing select; 0 is not a legal setting.
pub fn amp_mv(v: u32) -> Option<&'static str> {
    match v {
        1 => Some("952 mV diff pk-pk (lowest)"),
        7 => Some("1331 mV diff pk-pk (highest)"),
        2..=6 => Some("between 952 and 1331 mV diff pk-pk"),
        _ => None,
    }
}

pub fn slew_ps(v: u32) -> Option<&'static str> {
    match v {
        0 => Some("31 ps"),
        1 => Some("33 ps"),
        2 => Some("68 ps"),
        3 => Some("170 ps"),
        _ => None,
    }
}

pub struct Port {
    pub iface: String,
    pub sysfs: String,
}

impl Port {
    /// Resolve the PCI device directory from the interface name - never
    /// hardcode 0000:00:02.0, the same driver runs on the 1G ports.
    pub fn resolve(r: &Runner, iface: &str) -> Result<Port, String> {
        let dir = r.short(
            &format!("readlink -f /sys/class/net/{iface}/device"),
            "resolve sysfs device dir",
        )?;
        let dir = dir.trim().to_string();
        if dir.is_empty() {
            return Err(format!(
                "/sys/class/net/{iface}/device does not resolve - no such interface?"
            ));
        }
        let probe = r.short(
            &format!("test -e {dir}/serdes_tx_c_plus_1 && echo yes || echo no"),
            "probe serdes_tx_* attributes",
        )?;
        if probe.trim() != "yes" {
            return Err(format!(
                "{dir}/serdes_tx_c_plus_1 absent - al_eth without the SerDes sysfs \
                 knobs, or the wrong interface ({iface})"
            ));
        }
        Ok(Port {
            iface: iface.to_string(),
            sysfs: dir,
        })
    }

    pub fn path(&self, param: &str) -> String {
        format!("{}/serdes_tx_{param}", self.sysfs)
    }

    pub fn read_params(&self, r: &Runner) -> Result<BTreeMap<String, u32>, String> {
        let cmd = TX_PARAMS
            .iter()
            .map(|p| format!("cat {}", self.path(p)))
            .collect::<Vec<_>>()
            .join("; ");
        let out = r.short(&cmd, "read serdes_tx_* params")?;
        let vals: Vec<&str> = out.split_whitespace().collect();
        if vals.len() != TX_PARAMS.len() {
            return Err(format!(
                "read serdes_tx_*: expected {} values, got {:?}",
                TX_PARAMS.len(),
                vals
            ));
        }
        let mut m = BTreeMap::new();
        for (p, v) in TX_PARAMS.iter().zip(vals) {
            let n: u32 = v
                .parse()
                .map_err(|_| format!("serdes_tx_{p}: unparseable value {v:?}"))?;
            m.insert(p.to_string(), n);
        }
        Ok(m)
    }

    /// Write `value` (decimal here, hex on the wire) and verify the readback.
    pub fn write_param(&self, r: &Runner, param: &str, value: u32) -> Result<u32, String> {
        r.short(
            &format!("echo {value:x} > {}", self.path(param)),
            &format!("write serdes_tx_{param}={value}"),
        )?;
        let back = r.short(
            &format!("cat {}", self.path(param)),
            &format!("read back serdes_tx_{param}"),
        )?;
        back.trim()
            .parse::<u32>()
            .map_err(|_| format!("serdes_tx_{param}: unparseable readback {back:?}"))
    }

    pub fn carrier(&self, r: &Runner) -> Result<bool, String> {
        let out = r.short(
            &format!("cat /sys/class/net/{}/carrier 2>/dev/null || echo 0", self.iface),
            "read carrier",
        )?;
        Ok(out.trim() == "1")
    }

    /// Bounce the link so a pending tx-param override is consumed at re-link.
    /// Returns Ok(false) if the link did not come back inside the limit.
    pub fn bounce(&self, r: &Runner) -> Result<bool, String> {
        r.short(
            &format!("ip link set {} down", self.iface),
            "link down",
        )?;
        r.short(&format!("ip link set {} up", self.iface), "link up")?;
        let limit = Duration::from_secs(LINK_UP_LIMIT_S);
        let started = Instant::now();
        loop {
            if self.carrier(r)? {
                return Ok(true);
            }
            if started.elapsed() >= limit {
                eprintln!(
                    "WARNING: {} did not carrier-up within {:.1}s of a bounce \
                     (elapsed {:.1}s) - treating this point as LINK DOWN",
                    self.iface,
                    limit.as_secs_f64(),
                    started.elapsed().as_secs_f64()
                );
                return Ok(false);
            }
            std::thread::sleep(Duration::from_millis(250));
        }
    }

    pub fn ipv4(&self, r: &Runner) -> Result<Option<String>, String> {
        let out = r.short(
            &format!("ip -o -4 addr show dev {} 2>/dev/null || true", self.iface),
            "read interface address",
        )?;
        for tok in out.split_whitespace().collect::<Vec<_>>().windows(2) {
            if tok[0] == "inet" {
                return Ok(Some(tok[1].split('/').next().unwrap_or("").to_string()));
            }
        }
        Ok(None)
    }

    /// Refuse to bounce the link the SSH session itself rides on.
    pub fn check_not_control_path(&self, r: &Runner) -> Result<(), String> {
        let client = r.short(
            "printf '%s' \"${SSH_CLIENT%% *}\"",
            "read SSH_CLIENT",
        )?;
        let client = client.trim();
        if client.is_empty() {
            return Ok(()); // local run, or no SSH_CLIENT: nothing to protect
        }
        let route = r.short(
            &format!("ip route get {client} 2>/dev/null || true"),
            "route to ssh client",
        )?;
        let toks: Vec<&str> = route.split_whitespace().collect();
        let dev = toks
            .iter()
            .position(|t| *t == "dev")
            .and_then(|i| toks.get(i + 1))
            .copied()
            .unwrap_or("");
        if dev == self.iface {
            return Err(format!(
                "REFUSING to sweep: the control SSH session to {client} routes over \
                 {}, the very interface this would bounce. Drive the box from a \
                 different NIC (or run with --local on the box).",
                self.iface
            ));
        }
        Ok(())
    }
}

/// main_cursor = total_driver_units - (c_minus_1 + c_plus_1 + c_plus_2).
/// Raising a tap steals from the main cursor; the lane dies when it gets small.
pub fn main_cursor(p: &BTreeMap<String, u32>) -> i64 {
    let get = |k: &str| p.get(k).copied().unwrap_or(0) as i64;
    get("total_driver_units") - TAPS.iter().map(|t| get(t)).sum::<i64>()
}

pub fn describe(p: &BTreeMap<String, u32>) -> String {
    let mut o = String::new();
    for k in TX_PARAMS {
        let v = p.get(k).copied().unwrap_or(0);
        let note = match k {
            "amp" => amp_mv(v).unwrap_or("ILLEGAL (amp is a 3-bit select, 1..7)"),
            "slew_rate" => slew_ps(v).unwrap_or("ILLEGAL (slew_rate is 0..3)"),
            _ => "",
        };
        o.push_str(&format!("  serdes_tx_{k:<20} {v:>3}  (0x{v:x})  {note}\n"));
    }
    let mc = main_cursor(p);
    o.push_str(&format!(
        "  {:<31} {mc:>3}  = total_driver_units - (c_minus_1 + c_plus_1 + c_plus_2)\n",
        "main cursor (derived)"
    ));
    if mc <= 0 {
        o.push_str("  MAIN CURSOR <= 0: the taps have consumed the whole budget; the lane cannot drive.\n");
    }
    o
}

#[cfg(test)]
mod tests {
    use super::*;

    fn params(tdu: u32, cm1: u32, cp1: u32, cp2: u32) -> BTreeMap<String, u32> {
        let mut m = BTreeMap::new();
        m.insert("amp".into(), 1);
        m.insert("total_driver_units".into(), tdu);
        m.insert("c_minus_1".into(), cm1);
        m.insert("c_plus_1".into(), cp1);
        m.insert("c_plus_2".into(), cp2);
        m.insert("slew_rate".into(), 0);
        m
    }

    #[test]
    fn main_cursor_is_the_leftover_budget() {
        assert_eq!(main_cursor(&params(19, 2, 5, 0)), 12);
        assert_eq!(main_cursor(&params(19, 2, 10, 0)), 7);
        assert_eq!(main_cursor(&params(19, 9, 10, 0)), 0);
        assert_eq!(main_cursor(&params(19, 9, 10, 4)), -4);
    }

    #[test]
    fn describe_flags_illegal_and_exhausted_settings() {
        let mut p = params(19, 9, 10, 4);
        p.insert("amp".into(), 0);
        p.insert("slew_rate".into(), 9);
        let d = describe(&p);
        assert!(d.contains("MAIN CURSOR <= 0"), "{d}");
        assert!(d.contains("ILLEGAL (amp is a 3-bit select, 1..7)"), "{d}");
        assert!(d.contains("ILLEGAL (slew_rate is 0..3)"), "{d}");
    }

    #[test]
    fn amp_and_slew_tables_reject_out_of_range() {
        assert!(amp_mv(0).is_none());
        assert!(amp_mv(8).is_none());
        assert!(amp_mv(1).unwrap().contains("952"));
        assert_eq!(slew_ps(3), Some("170 ps"));
        assert!(slew_ps(4).is_none());
    }
}
