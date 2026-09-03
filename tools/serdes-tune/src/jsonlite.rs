// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Just enough JSON to write and read this tool's own counter snapshots.
//! Not a general parser: it reads the flat `{"meta":{...},"counters":{...}}`
//! shape emitted by `emit_snapshot()` and rejects anything else.

use std::collections::BTreeMap;
use std::process::Command;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub fn esc(s: &str) -> String {
    let mut o = String::with_capacity(s.len() + 2);
    for c in s.chars() {
        match c {
            '"' => o.push_str("\\\""),
            '\\' => o.push_str("\\\\"),
            '\n' => o.push_str("\\n"),
            c if (c as u32) < 0x20 => o.push_str(&format!("\\u{:04x}", c as u32)),
            c => o.push(c),
        }
    }
    o
}

pub fn emit_snapshot(meta: &BTreeMap<String, String>, counters: &BTreeMap<String, u64>) -> String {
    let mut s = String::from("{\n  \"meta\": {\n");
    let m: Vec<String> = meta
        .iter()
        .map(|(k, v)| format!("    \"{}\": \"{}\"", esc(k), esc(v)))
        .collect();
    s.push_str(&m.join(",\n"));
    s.push_str("\n  },\n  \"counters\": {\n");
    let c: Vec<String> = counters
        .iter()
        .map(|(k, v)| format!("    \"{}\": {}", esc(k), v))
        .collect();
    s.push_str(&c.join(",\n"));
    s.push_str("\n  }\n}\n");
    s
}

pub struct Snapshot {
    pub meta: BTreeMap<String, String>,
    pub counters: BTreeMap<String, u64>,
}

pub fn parse_snapshot(text: &str) -> Result<Snapshot, String> {
    let mut meta = BTreeMap::new();
    let mut counters = BTreeMap::new();
    let mut section = "";
    for raw in text.lines() {
        let line = raw.trim().trim_end_matches(',');
        if line.starts_with("\"meta\"") {
            section = "meta";
            continue;
        }
        if line.starts_with("\"counters\"") {
            section = "counters";
            continue;
        }
        if !line.starts_with('"') {
            continue;
        }
        let rest = &line[1..];
        let (key, tail) = rest
            .split_once("\":")
            .ok_or_else(|| format!("snapshot: unparseable line {raw:?}"))?;
        let val = tail.trim();
        match section {
            "meta" => {
                meta.insert(
                    key.to_string(),
                    val.trim_matches('"').replace("\\\"", "\"").to_string(),
                );
            }
            "counters" => {
                let v: u64 = val
                    .parse()
                    .map_err(|_| format!("snapshot: counter {key} is not an integer: {val:?}"))?;
                counters.insert(key.to_string(), v);
            }
            _ => {}
        }
    }
    if counters.is_empty() {
        return Err("snapshot: no counters found - is this a serdes-tune snapshot?".into());
    }
    Ok(Snapshot { meta, counters })
}

/// ISO 8601 with the local offset for human-facing output; falls back to UTC
/// (machine-log form) when `date` is unavailable.
pub fn now_iso8601() -> String {
    if let Ok(o) = Command::new("date").arg("+%Y-%m-%dT%H:%M:%S%:z").output() {
        if o.status.success() {
            let s = String::from_utf8_lossy(&o.stdout).trim().to_string();
            if !s.is_empty() {
                return s;
            }
        }
    }
    utc_iso8601(SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default())
}

fn utc_iso8601(d: Duration) -> String {
    let secs = d.as_secs() as i64;
    let days = secs.div_euclid(86_400);
    let tod = secs.rem_euclid(86_400);
    let (y, m, dd) = civil_from_days(days);
    format!(
        "{y:04}-{m:02}-{dd:02}T{:02}:{:02}:{:02}Z",
        tod / 3600,
        (tod % 3600) / 60,
        tod % 60
    )
}

// Howard Hinnant's civil_from_days, days since 1970-01-01.
fn civil_from_days(z: i64) -> (i64, u32, u32) {
    let z = z + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z.rem_euclid(146_097);
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146_096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    (if m <= 2 { y + 1 } else { y }, m as u32, d as u32)
}
