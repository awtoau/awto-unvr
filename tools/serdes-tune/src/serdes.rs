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

/// Kernel store keeps the value in a u8 (`*(uint8_t *)&tx_params + off = param`).
pub const KERNEL_MAX: u32 = 255;

/// Reject a value the hardware or the kernel store cannot represent, before
/// anything is written. Out-of-range amp/slew are not clamped by the driver.
pub fn validate_value(param: &str, v: u32) -> Result<(), String> {
    if !TX_PARAMS.contains(&param) {
        return Err(format!("{param}: expected one of {TX_PARAMS:?}"));
    }
    if v > KERNEL_MAX {
        return Err(format!(
            "serdes_tx_{param}={v}: the kernel store truncates to u8 (max {KERNEL_MAX}); \
             {v} would land as {}",
            v & 0xff
        ));
    }
    if param == "amp" && amp_mv(v).is_none() {
        return Err(format!(
            "serdes_tx_amp={v}: amp is a 3-bit select, legal 1..7"
        ));
    }
    if param == "slew_rate" && slew_ps(v).is_none() {
        return Err(format!("serdes_tx_slew_rate={v}: legal 0..3"));
    }
    Ok(())
}

/// The string put on the wire: the store parses HEX.
pub fn wire_hex(v: u32) -> String {
    format!("{v:x}")
}

#[cfg(test)]
/// Model of `kstrtoul(buf, 16)` into a u8 field, for tests of the encoding.
pub fn kernel_store(wire: &str) -> Option<u8> {
    u64::from_str_radix(wire.trim(), 16).ok().map(|n| n as u8)
}

/// Parse the show output (DECIMAL).
pub fn parse_readback(param: &str, back: &str) -> Result<u32, String> {
    back.trim()
        .parse::<u32>()
        .map_err(|_| format!("serdes_tx_{param}: unparseable readback {back:?}"))
}

/// Compare intended vs actual; never report success on a mismatch.
pub fn verify_readback(param: &str, intended: u32, actual: u32) -> Result<(), String> {
    if intended == actual {
        return Ok(());
    }
    Err(format!(
        "READBACK MISMATCH serdes_tx_{param}: intended {intended} (wrote 0x{intended:x}), \
         actual {actual} (0x{actual:x})"
    ))
}

/// What a bounce established, from carrier before and after.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum BounceOutcome {
    /// Up before and after: the override was applied and the link survived.
    LinkUp,
    /// Up before, down after: the override is suspected of breaking the link.
    LinkLost,
    /// Down before and after: no peer or cable; the override cannot be judged.
    NoPeer,
    /// Down before, up after: link raised by the bounce itself.
    LinkRaised,
}

impl BounceOutcome {
    pub fn classify(before: bool, after: bool) -> Self {
        match (before, after) {
            (true, true) => Self::LinkUp,
            (true, false) => Self::LinkLost,
            (false, false) => Self::NoPeer,
            (false, true) => Self::LinkRaised,
        }
    }

    /// 0 = applied and link up; 5 = link lost; 6 = unverifiable (no peer).
    pub fn exit_code(self) -> i32 {
        match self {
            Self::LinkUp | Self::LinkRaised => 0,
            Self::LinkLost => 5,
            Self::NoPeer => 6,
        }
    }

    pub fn message(self) -> &'static str {
        match self {
            Self::LinkUp => "link bounced, carrier up",
            Self::LinkRaised => "link bounced, carrier was down before and is up now",
            Self::LinkLost => {
                "LINK LOST: carrier was up before the bounce and is DOWN after - suspect \
                 the new value; restore the previous one"
            }
            Self::NoPeer => {
                "NO LINK before or after the bounce (no peer/cable?): the override is \
                 staged and applies at the next link-up, but its effect is UNVERIFIED"
            }
        }
    }
}

/// Exactly what `params --set` would do; printed by `--dry-run`.
pub fn plan_text(
    iface: &str,
    sysfs: &str,
    param: &str,
    cur: &BTreeMap<String, u32>,
    v: u32,
    bounce: bool,
) -> String {
    let old = cur.get(param).copied().unwrap_or(0);
    let mut proj = cur.clone();
    proj.insert(param.to_string(), v);
    let mut o = String::new();
    o.push_str(&format!(
        "--dry-run: nothing is written. Plan for {iface}:\n"
    ));
    o.push_str(&format!(
        "  write   echo {} > {sysfs}/serdes_tx_{param}   (value {v}, current {old})\n",
        wire_hex(v)
    ));
    o.push_str("  scope   per adapter (one PCI function); the sysfs knob has no lane index\n");
    o.push_str(&format!(
        "  verify  cat serdes_tx_{param} must print {v}; mismatch => no bounce, restore {old}, exit 1\n"
    ));
    o.push_str(&format!(
        "  main cursor {} -> {}\n",
        main_cursor(cur),
        main_cursor(&proj)
    ));
    if bounce {
        o.push_str(&format!(
            "  bounce  ip link set {iface} down; up; poll carrier up to {LINK_UP_LIMIT_S}s\n"
        ));
        o.push_str("  exit    0 link up, 5 link lost after bounce, 6 no link before or after\n");
    } else {
        o.push_str("  bounce  none (--no-bounce): staged until the next link event\n");
    }
    o
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

    /// Write `value` (decimal here, hex on the wire); returns the raw readback.
    pub fn write_param(&self, r: &Runner, param: &str, value: u32) -> Result<u32, String> {
        validate_value(param, value)?;
        r.short(
            &format!("echo {} > {}", wire_hex(value), self.path(param)),
            &format!("write serdes_tx_{param}={value}"),
        )?;
        let back = r.short(
            &format!("cat {}", self.path(param)),
            &format!("read back serdes_tx_{param}"),
        )?;
        parse_readback(param, &back)
    }

    /// write_param, then require readback == value. On mismatch, best-effort
    /// restore `previous` and return the mismatch as an error.
    pub fn write_verified(
        &self,
        r: &Runner,
        param: &str,
        value: u32,
        previous: u32,
    ) -> Result<(), String> {
        let back = self.write_param(r, param, value)?;
        if let Err(e) = verify_readback(param, value, back) {
            let restored = match self.write_param(r, param, previous) {
                Ok(b) if b == previous => format!("restored previous value {previous}"),
                Ok(b) => format!("RESTORE ALSO MISMATCHED (wanted {previous}, got {b})"),
                Err(e2) => format!("RESTORE FAILED: {e2}"),
            };
            return Err(format!("{e}; {restored}"));
        }
        Ok(())
    }

    pub fn carrier(&self, r: &Runner) -> Result<bool, String> {
        let out = r.short(
            &format!(
                "cat /sys/class/net/{}/carrier 2>/dev/null || echo 0",
                self.iface
            ),
            "read carrier",
        )?;
        Ok(out.trim() == "1")
    }

    /// Bounce the link so a pending tx-param override is consumed at re-link.
    /// Carrier is sampled before and after so a missing peer is distinguishable
    /// from a value that broke the link. Waits at most LINK_UP_LIMIT_S.
    pub fn bounce(&self, r: &Runner) -> Result<BounceOutcome, String> {
        let before = self.carrier(r)?;
        r.short(&format!("ip link set {} down", self.iface), "link down")?;
        r.short(&format!("ip link set {} up", self.iface), "link up")
            .map_err(|e| {
                format!(
                    "{e}\n  {} was set DOWN and is STILL DOWN - bring it up by hand",
                    self.iface
                )
            })?;
        let limit = Duration::from_secs(LINK_UP_LIMIT_S);
        let started = Instant::now();
        let after = loop {
            if self.carrier(r)? {
                break true;
            }
            if started.elapsed() >= limit {
                eprintln!(
                    "WARNING: {} did not carrier-up within {:.1}s of a bounce \
                     (elapsed {:.1}s, carrier before bounce: {before})",
                    self.iface,
                    limit.as_secs_f64(),
                    started.elapsed().as_secs_f64()
                );
                break false;
            }
            // Each carrier read is an SSH round trip; 250 ms keeps it ~4/s.
            std::thread::sleep(Duration::from_millis(250));
        };
        Ok(BounceOutcome::classify(before, after))
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
        let client = r.short("printf '%s' \"${SSH_CLIENT%% *}\"", "read SSH_CLIENT")?;
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
        o.push_str(
            "  MAIN CURSOR <= 0: the taps have consumed the whole budget; the lane cannot drive.\n",
        );
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

    #[test]
    fn hex_wire_vs_decimal_readback_round_trips() {
        // 10 must go out as "a": "10" would be parsed as 16 by the kernel.
        assert_eq!(wire_hex(10), "a");
        for v in 0..=255u32 {
            assert_eq!(kernel_store(&wire_hex(v)).map(u32::from), Some(v));
        }
        assert_eq!(kernel_store("10"), Some(16));
        assert_eq!(parse_readback("amp", "10\n").unwrap(), 10);
        assert!(parse_readback("amp", "0xa").is_err());
        assert!(parse_readback("amp", "").is_err());
    }

    #[test]
    fn validate_rejects_unrepresentable_values() {
        assert!(validate_value("c_plus_1", 255).is_ok());
        // 256 would be stored by the kernel as 0
        assert_eq!(kernel_store(&wire_hex(256)), Some(0));
        assert!(validate_value("c_plus_1", 256)
            .unwrap_err()
            .contains("would land as 0"));
        assert!(validate_value("amp", 0).is_err());
        assert!(validate_value("amp", 8).is_err());
        assert!(validate_value("amp", 7).is_ok());
        assert!(validate_value("slew_rate", 4).is_err());
        assert!(validate_value("slew_rate", 0).is_ok());
        assert!(validate_value("bogus", 1).is_err());
    }

    #[test]
    fn verify_reports_register_intended_and_actual() {
        assert!(verify_readback("c_plus_1", 5, 5).is_ok());
        let e = verify_readback("c_plus_1", 10, 16).unwrap_err();
        assert!(e.contains("serdes_tx_c_plus_1"), "{e}");
        assert!(e.contains("intended 10 (wrote 0xa)"), "{e}");
        assert!(e.contains("actual 16 (0x10)"), "{e}");
    }

    #[test]
    fn bounce_outcome_separates_no_peer_from_lost_link() {
        use BounceOutcome::*;
        assert_eq!(BounceOutcome::classify(true, true), LinkUp);
        assert_eq!(BounceOutcome::classify(true, false), LinkLost);
        assert_eq!(BounceOutcome::classify(false, false), NoPeer);
        assert_eq!(BounceOutcome::classify(false, true), LinkRaised);
        assert_eq!(LinkUp.exit_code(), 0);
        assert_eq!(LinkRaised.exit_code(), 0);
        assert_eq!(LinkLost.exit_code(), 5);
        assert_eq!(NoPeer.exit_code(), 6);
        assert!(NoPeer.message().contains("UNVERIFIED"));
    }

    #[test]
    fn plan_text_shows_register_value_and_bounce() {
        let cur = params(19, 0, 5, 0);
        let t = plan_text("enp0s2", "/sys/x", "c_plus_1", &cur, 10, true);
        assert!(t.contains("echo a > /sys/x/serdes_tx_c_plus_1"), "{t}");
        assert!(t.contains("value 10, current 5"), "{t}");
        assert!(t.contains("main cursor 14 -> 9"), "{t}");
        assert!(t.contains("poll carrier up to 8s"), "{t}");
        let t = plan_text("enp0s2", "/sys/x", "c_plus_1", &cur, 10, false);
        assert!(t.contains("--no-bounce"), "{t}");
    }
}
