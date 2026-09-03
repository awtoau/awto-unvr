// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! `ethtool -S` harvesting, classification and before/after diffing.
//!
//! ABSENT IS NOT ZERO. Every accessor returns Option; a counter the driver does
//! not expose is never summed, diffed or scored as 0, because a missing counter
//! read as zero is indistinguishable from a perfect link.
//!
//! Link-quality groups:
//!   pcs_*  Clause 49 errored-block / BER counters (al_eth, #196)
//!   ec_*   Ethernet-controller drop counters (15 of them)
//!   phy    whatever generic error/drop counters the driver also exposes

use crate::exec::Runner;
use std::collections::BTreeMap;

pub const PCS_NAMES: [&str; 2] = ["pcs_errored_blocks", "pcs_ber_events"];

/// The 15 EC drop counters al_eth exposes (al_eth_main.c ethtool stat table).
/// Listed explicitly so a partial set is reported as partial, not as "some".
pub const EC_NAMES: [&str; 15] = [
    "ec_faf_in_rx_short",
    "ec_faf_in_rx_long",
    "ec_faf_out_rx_short",
    "ec_faf_out_rx_long",
    "ec_faf_out_drop",
    "ec_rxf_in_fifo_err",
    "ec_lbf_in_fifo_err",
    "ec_rxf_out_drop_1_pkt",
    "ec_rxf_out_drop_2_pkt",
    "ec_rfw_in_vlan_drop",
    "ec_rfw_in_parse_drop",
    "ec_rfw_in_mac_drop",
    "ec_rfw_in_mac_ndet_drop",
    "ec_rfw_in_ctrl_drop",
    "ec_rfw_in_prot_i_drop",
];

/// Generic error/drop counters worth watching when present. Names vary by
/// driver; these are optional, unlike PCS_NAMES/EC_NAMES which this tree's
/// al_eth is expected to provide.
const PHY_LIKE: [&str; 14] = [
    "rx_crc_errors",
    "rx_errors",
    "rx_frame_errors",
    "rx_length_errors",
    "rx_over_errors",
    "rx_missed_errors",
    "rx_symbol_err_phy",
    "rx_crc_errors_phy",
    "rx_corrected_bits_phy",
    "rx_pcs_symbol_err_phy",
    "rx_err_lane_0_phy",
    "tx_errors",
    "mac_local_faults",
    "mac_remote_faults",
];

#[derive(Clone)]
pub struct Sample {
    pub iface: String,
    pub when: String,
    pub counters: BTreeMap<String, u64>,
}

impl Sample {
    /// None means the driver does not expose this counter at all.
    pub fn get(&self, k: &str) -> Option<u64> {
        self.counters.get(k).copied()
    }

    pub fn has(&self, k: &str) -> bool {
        self.counters.contains_key(k)
    }

    /// Sum of the PCS counters, or None if neither is exposed.
    pub fn pcs_total(&self) -> Option<u64> {
        let vals: Vec<u64> = PCS_NAMES.iter().filter_map(|n| self.get(n)).collect();
        if vals.is_empty() {
            None
        } else {
            Some(vals.iter().sum())
        }
    }

    /// Sum of every ec_* counter present, or None if none are exposed.
    pub fn ec_total(&self) -> Option<u64> {
        let vals: Vec<u64> = self
            .counters
            .iter()
            .filter(|(k, _)| k.starts_with("ec_"))
            .map(|(_, v)| *v)
            .collect();
        if vals.is_empty() {
            None
        } else {
            Some(vals.iter().sum())
        }
    }

    pub fn ec_names(&self) -> Vec<String> {
        self.counters
            .keys()
            .filter(|k| k.starts_with("ec_"))
            .cloned()
            .collect()
    }

    pub fn phy_names(&self) -> Vec<String> {
        self.counters
            .keys()
            .filter(|k| PHY_LIKE.contains(&k.as_str()))
            .cloned()
            .collect()
    }
}

/// (present, missing) over the counters this tree's al_eth is expected to have.
pub fn expected_split(s: &Sample) -> (Vec<String>, Vec<String>) {
    let mut present = Vec::new();
    let mut missing = Vec::new();
    for n in PCS_NAMES.iter().chain(EC_NAMES.iter()) {
        if s.has(n) {
            present.push((*n).to_string());
        } else {
            missing.push((*n).to_string());
        }
    }
    (present, missing)
}

/// Hard gate before scoring: the metric's counters must exist.
pub fn require_for_metric(s: &Sample, metric: &str) -> Result<(), String> {
    let needed: Vec<&str> = match metric {
        "pcs" => PCS_NAMES.to_vec(),
        "ec" => EC_NAMES.to_vec(),
        _ => return Ok(()),
    };
    let missing: Vec<&str> = needed.into_iter().filter(|n| !s.has(n)).collect();
    if missing.is_empty() {
        return Ok(());
    }
    Err(format!(
        "--metric {metric}: `ethtool -S {}` does not expose {}.\n  \
         The al_eth running on the box is older than this tree (these counters are new: \
         PCS Clause 49 harvest #196, EC drop mirrors). A stale module deploy is the usual \
         cause (#105 #131 #161).\n  \
         NOT falling back to another metric: scoring on an absent counter would report a \
         perfect link everywhere. Redeploy the module, or choose a metric whose counters \
         exist (`serdes-tune counters` lists them).",
        s.iface,
        missing.join(", ")
    ))
}

pub fn parse_ethtool_s(text: &str) -> BTreeMap<String, u64> {
    let mut out = BTreeMap::new();
    for line in text.lines() {
        let t = line.trim();
        let Some((name, val)) = t.split_once(':') else {
            continue;
        };
        let name = name.trim();
        let val = val.trim();
        if name.is_empty() || name.contains(char::is_whitespace) {
            continue;
        }
        if let Ok(v) = val.parse::<u64>() {
            out.insert(name.to_string(), v);
        }
    }
    out
}

pub fn sample(r: &Runner, iface: &str) -> Result<Sample, String> {
    let out = r.short(&format!("ethtool -S {iface}"), "ethtool -S")?;
    let counters = parse_ethtool_s(&out);
    if counters.is_empty() {
        return Err(format!(
            "ethtool -S {iface}: no counters parsed - wrong interface, or ethtool missing on the box"
        ));
    }
    Ok(Sample {
        iface: iface.to_string(),
        when: crate::jsonlite::now_iso8601(),
        counters,
    })
}

pub struct Delta {
    pub name: String,
    pub before: Option<u64>,
    pub after: Option<u64>,
}

impl Delta {
    /// None when either side is absent - a counter that appeared or vanished
    /// between samples has no meaningful delta.
    pub fn change(&self) -> Option<i64> {
        match (self.before, self.after) {
            (Some(b), Some(a)) => Some(a as i64 - b as i64),
            _ => None,
        }
    }
}

/// Deltas for every counter present in either sample, largest change first.
pub fn diff(before: &Sample, after: &Sample) -> Vec<Delta> {
    let mut names: Vec<&String> = before.counters.keys().collect();
    for k in after.counters.keys() {
        if !before.counters.contains_key(k) {
            names.push(k);
        }
    }
    let mut out: Vec<Delta> = names
        .into_iter()
        .map(|n| Delta {
            name: n.clone(),
            before: before.get(n),
            after: after.get(n),
        })
        .collect();
    out.sort_by_key(|d| (-d.change().unwrap_or(0).abs(), d.name.clone()));
    out
}

pub fn group_of(name: &str) -> &'static str {
    if PCS_NAMES.contains(&name) {
        "pcs"
    } else if name.starts_with("ec_") {
        "ec"
    } else if PHY_LIKE.contains(&name) {
        "phy"
    } else {
        "other"
    }
}

fn shown(v: Option<u64>) -> String {
    v.map(|x| x.to_string()).unwrap_or_else(|| "ABSENT".into())
}

pub fn report(s: &Sample) -> String {
    let mut o = String::new();
    o.push_str(&format!("interface {}  sampled {}\n\n", s.iface, s.when));

    o.push_str("PCS Clause 49 (al_eth, #196) - the discriminating metric\n");
    for n in PCS_NAMES {
        o.push_str(&format!("  {n:<28} {}\n", shown(s.get(n))));
    }
    if s.pcs_total().is_none() {
        o.push_str(
            "  ABSENT - the running al_eth predates the #196 PCS harvest, or this is not\n\
             \x20 an al_eth port. `--metric pcs` will refuse to run against it.\n",
        );
    } else {
        o.push_str(
            "  note: MDIO 3.33 is clear-on-read and saturates (255 blocks / 63 BER) per\n\
             \x20 poll, so a large value is a lower bound, not a count.\n",
        );
    }

    let ec_present: Vec<&str> = EC_NAMES.into_iter().filter(|n| s.has(n)).collect();
    o.push_str(&format!(
        "\nEC drop counters ({} of {} expected present)\n",
        ec_present.len(),
        EC_NAMES.len()
    ));
    for n in EC_NAMES {
        o.push_str(&format!("  {n:<28} {}\n", shown(s.get(n))));
    }
    for n in s.ec_names() {
        if !EC_NAMES.contains(&n.as_str()) {
            o.push_str(&format!("  {n:<28} {} (unexpected name)\n", shown(s.get(&n))));
        }
    }
    o.push_str(&format!("  {:<28} {}\n", "(sum)", shown(s.ec_total())));

    let phy = s.phy_names();
    o.push_str("\nGeneric PHY/MAC error counters\n");
    if phy.is_empty() {
        o.push_str("  none of the known names present\n");
    }
    for n in &phy {
        o.push_str(&format!("  {n:<28} {}\n", shown(s.get(n))));
    }
    o
}

pub fn report_diff(before: &Sample, after: &Sample) -> String {
    let mut o = String::new();
    o.push_str(&format!(
        "diff {} -> {} on {}\n\n",
        before.when, after.when, after.iface
    ));
    o.push_str(&format!(
        "{:<32} {:>14} {:>14} {:>12}  {}\n",
        "counter", "before", "after", "delta", "group"
    ));
    let mut moved = 0;
    let mut mismatched = Vec::new();
    for d in diff(before, after) {
        match d.change() {
            Some(0) => continue,
            Some(c) => {
                moved += 1;
                o.push_str(&format!(
                    "{:<32} {:>14} {:>14} {:>+12}  {}\n",
                    d.name,
                    shown(d.before),
                    shown(d.after),
                    c,
                    group_of(&d.name)
                ));
            }
            None => {
                mismatched.push(d.name.clone());
                o.push_str(&format!(
                    "{:<32} {:>14} {:>14} {:>12}  {}\n",
                    d.name,
                    shown(d.before),
                    shown(d.after),
                    "n/a",
                    group_of(&d.name)
                ));
            }
        }
    }
    if moved == 0 {
        o.push_str("(no counter changed)\n");
    }
    if !mismatched.is_empty() {
        o.push_str(&format!(
            "\nWARNING: {} counter(s) exist in only one sample: {}.\n  \
             The two samples came from different driver builds - the module was reloaded or \
             redeployed between them (#105 #131 #161). No delta is computed for those.\n",
            mismatched.len(),
            mismatched.join(", ")
        ));
    }

    let pair = |name: &str, b: Option<u64>, a: Option<u64>| match (b, a) {
        (Some(b), Some(a)) => format!("{name} delta {:+}", a as i64 - b as i64),
        _ => format!("{name} delta n/a (ABSENT)"),
    };
    o.push_str(&format!(
        "\n{}   {}\n",
        pair("pcs", before.pcs_total(), after.pcs_total()),
        pair("ec", before.ec_total(), after.ec_total())
    ));
    o
}

pub fn to_json(s: &Sample) -> String {
    let mut meta = BTreeMap::new();
    meta.insert("iface".into(), s.iface.clone());
    meta.insert("when".into(), s.when.clone());
    crate::jsonlite::emit_snapshot(&meta, &s.counters)
}

pub fn from_json(text: &str) -> Result<Sample, String> {
    let snap = crate::jsonlite::parse_snapshot(text)?;
    Ok(Sample {
        iface: snap.meta.get("iface").cloned().unwrap_or_default(),
        when: snap.meta.get("when").cloned().unwrap_or_default(),
        counters: snap.counters,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn s(iface: &str, pairs: &[(&str, u64)]) -> Sample {
        Sample {
            iface: iface.into(),
            when: "test".into(),
            counters: pairs.iter().map(|(k, v)| ((*k).to_string(), *v)).collect(),
        }
    }

    #[test]
    fn parses_ethtool_output() {
        let text = "NIC statistics:\n     rx_packets: 12\n     pcs_errored_blocks: 3\n\
                    \x20    a name with spaces: 9\n     tx_bytes: 0\n";
        let m = parse_ethtool_s(text);
        assert_eq!(m.get("rx_packets"), Some(&12));
        assert_eq!(m.get("pcs_errored_blocks"), Some(&3));
        assert_eq!(m.get("tx_bytes"), Some(&0));
        assert_eq!(m.len(), 3, "the malformed line must be skipped");
    }

    #[test]
    fn absent_is_not_zero() {
        let old = s("enp0s2", &[("rx_packets", 5)]);
        assert_eq!(old.get("pcs_errored_blocks"), None);
        assert_eq!(old.pcs_total(), None);
        assert_eq!(old.ec_total(), None);
        assert!(report(&old).contains("ABSENT"));
    }

    #[test]
    fn metric_gate_rejects_an_old_driver() {
        let old = s("enp0s2", &[("rx_packets", 5)]);
        let e = require_for_metric(&old, "pcs").unwrap_err();
        assert!(e.contains("pcs_errored_blocks"), "{e}");
        assert!(e.contains("older than this tree"), "{e}");
        assert!(require_for_metric(&old, "udp-loss").is_ok());

        let new = s(
            "enp0s2",
            &[("pcs_errored_blocks", 0), ("pcs_ber_events", 0)],
        );
        assert!(require_for_metric(&new, "pcs").is_ok());
        assert!(require_for_metric(&new, "ec").is_err());
    }

    #[test]
    fn expected_split_lists_every_missing_counter() {
        let new = s("enp0s2", &[("pcs_errored_blocks", 1)]);
        let (present, missing) = expected_split(&new);
        assert_eq!(present, vec!["pcs_errored_blocks".to_string()]);
        assert_eq!(missing.len(), 1 + EC_NAMES.len());
    }

    #[test]
    fn diff_flags_a_vanished_counter_instead_of_zeroing_it() {
        let before = s("enp0s2", &[("pcs_errored_blocks", 4), ("rx_packets", 1)]);
        let after = s("enp0s2", &[("rx_packets", 3)]);
        let d = diff(&before, &after);
        let pcs = d.iter().find(|x| x.name == "pcs_errored_blocks").unwrap();
        assert_eq!(pcs.change(), None);
        let rep = report_diff(&before, &after);
        assert!(rep.contains("exist in only one sample"), "{rep}");
        assert!(rep.contains("pcs delta n/a (ABSENT)"), "{rep}");
    }

    #[test]
    fn json_round_trips() {
        let a = s("enp0s2", &[("pcs_ber_events", 7), ("ec_faf_out_drop", 2)]);
        let b = from_json(&to_json(&a)).unwrap();
        assert_eq!(b.iface, "enp0s2");
        assert_eq!(b.get("pcs_ber_events"), Some(7));
        assert_eq!(b.ec_total(), Some(2));
    }
}
