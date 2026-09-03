// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Sweep one serdes_tx_* parameter, score every setting, and say honestly when
//! the metric could not tell the settings apart.
//!
//! Established facts this encodes (docs/serdes-equalisation.md):
//! - over-emphasis gives an ERROR FLOOR, not a plateau (802.3dj) - so sweep
//!   past the optimum both ways and flag a winner sitting at a range edge;
//! - BER-type metrics only discriminate while the link is marginal;
//! - TCP throughput saturates and cannot rank settings that all reach line rate.

use crate::counters::{self, Sample};
use crate::env::Env;
use crate::exec::Runner;
use crate::iperf;
use crate::serdes::{self, Port};
use std::collections::BTreeMap;
use std::time::Duration;

#[derive(Clone, Copy, PartialEq)]
pub enum Metric {
    Pcs,
    Ec,
    UdpLoss,
    Throughput,
}

impl Metric {
    pub fn parse(s: &str) -> Result<Metric, String> {
        Ok(match s {
            "pcs" => Metric::Pcs,
            "ec" => Metric::Ec,
            "udp-loss" => Metric::UdpLoss,
            "throughput" => Metric::Throughput,
            _ => return Err(format!("--metric {s}: expected pcs, ec, udp-loss or throughput")),
        })
    }

    pub fn name(self) -> &'static str {
        match self {
            Metric::Pcs => "pcs",
            Metric::Ec => "ec",
            Metric::UdpLoss => "udp-loss",
            Metric::Throughput => "throughput",
        }
    }

    pub fn higher_is_better(self) -> bool {
        self == Metric::Throughput
    }

    pub fn caveat(self) -> &'static str {
        match self {
            Metric::Pcs => {
                "PCS Clause 49 errored blocks + BER events. Non-saturating across polls, \
                 but blind once the link is clean: at 10.3125 Gb/s, 100 errors take ~10 s \
                 at BER 1e-9 and ~2.7 h at 1e-12."
            }
            Metric::Ec => {
                "EC drop counters. Coarser than pcs - they count frames the controller \
                 dropped, not bit errors."
            }
            Metric::UdpLoss => {
                "Fixed-rate UDP loss. Works through a store-and-forward switch, which \
                 discards bad-CRC frames, so corruption arrives as missing datagrams."
            }
            Metric::Throughput => {
                "TCP throughput SATURATES: a 10GBASE-R link at BER 1e-9 still reaches line \
                 rate, so this metric cannot rank settings that all reach it. It finds the \
                 edges of the working window and nothing inside it."
            }
        }
    }
}

#[derive(Clone, Copy, PartialEq)]
pub enum Load {
    Tcp,
    Udp,
    None,
}

pub struct Cfg {
    pub iface: String,
    pub param: String,
    pub values: Vec<u32>,
    pub metric: Metric,
    pub duration: u64,
    pub repeat: u32,
    pub streams: u32,
    pub server: String,
    pub port: u16,
    pub udp_mbit: u32,
    pub load: Load,
    pub csv: Option<String>,
    pub restore: bool,
    pub dry_run: bool,
    pub allow_control_path: bool,
}

pub struct Point {
    pub value: u32,
    pub main_cursor: i64,
    pub linked: bool,
    pub readback: Option<u32>,
    /// None = the counter is absent from this driver build, NOT zero.
    pub pcs_blocks: Option<i64>,
    pub pcs_ber: Option<i64>,
    pub ec_delta: Option<i64>,
    pub mbps: f64,
    pub retrans: i64,
    pub loss_pct: f64,
    pub lost: i64,
    pub note: String,
}

fn show(v: Option<i64>) -> String {
    v.map(|x| x.to_string()).unwrap_or_else(|| "ABSENT".into())
}

impl Point {
    fn score(&self, m: Metric) -> Option<f64> {
        if !self.linked {
            return None;
        }
        match m {
            Metric::Pcs => match (self.pcs_blocks, self.pcs_ber) {
                (Some(b), Some(e)) => Some((b + e) as f64),
                _ => None,
            },
            Metric::Ec => self.ec_delta.map(|v| v as f64),
            Metric::UdpLoss => Some(self.loss_pct),
            Metric::Throughput => Some(self.mbps),
        }
    }
}

fn dwell(secs: u64) {
    // The measurement window itself: counters integrate over it.
    std::thread::sleep(Duration::from_secs(secs));
}

struct Measured {
    pcs_blocks: Option<i64>,
    pcs_ber: Option<i64>,
    ec_delta: Option<i64>,
    mbps: f64,
    retrans: i64,
    loss_pct: f64,
    lost: i64,
    note: String,
}

impl Measured {
    fn empty() -> Measured {
        Measured {
            pcs_blocks: None,
            pcs_ber: None,
            ec_delta: None,
            mbps: 0.0,
            retrans: -1,
            loss_pct: -1.0,
            lost: -1,
            note: String::new(),
        }
    }
}

/// Absent on either side stays absent - never accumulated as zero.
fn accum(acc: &mut Option<i64>, before: Option<u64>, after: Option<u64>) {
    if let (Some(b), Some(a)) = (before, after) {
        *acc = Some(acc.unwrap_or(0) + (a as i64 - b as i64));
    }
}

fn measure(r: &Runner, cfg: &Cfg, bind_ip: &str) -> Result<Measured, String> {
    let mut m = Measured::empty();
    let mut mbps_sum = 0.0;
    let mut lost_sum: i64 = 0;
    let mut total_sum: i64 = 0;
    let mut pct_sum = 0.0;
    let mut pct_runs = 0.0;
    let mut runs = 0.0;

    for _ in 0..cfg.repeat.max(1) {
        let before = counters::sample(r, &cfg.iface)?;
        match cfg.load {
            Load::Tcp => {
                match iperf::run_tcp(
                    r,
                    &cfg.server,
                    cfg.port,
                    cfg.duration,
                    bind_ip,
                    &cfg.iface,
                    cfg.streams,
                ) {
                    Ok(t) => {
                        mbps_sum += t.mbps;
                        m.retrans = m.retrans.max(0) + t.retrans.max(0);
                    }
                    Err(e) => m.note = format!("iperf3 TCP failed: {e}"),
                }
            }
            Load::Udp => {
                match iperf::run_udp(
                    r,
                    &cfg.server,
                    cfg.port,
                    cfg.duration,
                    bind_ip,
                    &cfg.iface,
                    cfg.udp_mbit,
                ) {
                    Ok(u) => {
                        lost_sum += u.lost.max(0);
                        total_sum += u.total.max(0);
                        pct_sum += u.loss_pct;
                        pct_runs += 1.0;
                        m.lost = m.lost.max(0) + u.lost.max(0);
                    }
                    Err(e) => m.note = format!("iperf3 UDP failed: {e}"),
                }
            }
            Load::None => dwell(cfg.duration),
        }
        let after = counters::sample(r, &cfg.iface)?;
        accum(
            &mut m.pcs_blocks,
            before.get("pcs_errored_blocks"),
            after.get("pcs_errored_blocks"),
        );
        accum(
            &mut m.pcs_ber,
            before.get("pcs_ber_events"),
            after.get("pcs_ber_events"),
        );
        accum(&mut m.ec_delta, before.ec_total(), after.ec_total());
        runs += 1.0;
    }
    if runs > 0.0 {
        m.mbps = mbps_sum / runs;
    }
    // Loss over the whole dwell, not a mean of per-run percentages; iperf3's own
    // percentage is the fallback when no datagram totals were parsed.
    m.loss_pct = match (cfg.load, total_sum, pct_runs) {
        (Load::Udp, t, _) if t > 0 => 100.0 * lost_sum as f64 / t as f64,
        (Load::Udp, _, n) if n > 0.0 => pct_sum / n,
        _ => -1.0,
    };
    Ok(m)
}

fn header(cfg: &Cfg, orig: &BTreeMap<String, u32>, port: &Port, env: &Env) {
    print!("{}", env.render());
    println!("\ninterface {} -> {}", cfg.iface, port.sysfs);
    println!("sweeping serdes_tx_{} over {:?}", cfg.param, cfg.values);
    println!("metric {}: {}", cfg.metric.name(), cfg.metric.caveat());
    println!("\ncurrent TX params:");
    print!("{}", serdes::describe(orig));
    println!(
        "\nNOTE values are written as HEX (kstrtoul base 16) and read back as decimal;\n\
         \x20     each change is committed by a link bounce, not applied live."
    );
}

pub fn run(r: &Runner, cfg: &Cfg) -> Result<i32, String> {
    if !serdes::TX_PARAMS.contains(&cfg.param.as_str()) {
        return Err(format!(
            "--param {}: expected one of {:?}",
            cfg.param,
            serdes::TX_PARAMS
        ));
    }
    if cfg.load == Load::None && matches!(cfg.metric, Metric::UdpLoss | Metric::Throughput) {
        return Err(format!(
            "--no-load with --metric {}: that metric IS the load generator, so every point \
             would score identically on no data. Use --metric pcs or ec with --no-load.",
            cfg.metric.name()
        ));
    }
    let port = Port::resolve(r, &cfg.iface)?;
    if !cfg.allow_control_path {
        port.check_not_control_path(r)?;
    }
    let orig = port.read_params(r)?;

    // Capability probe BEFORE anything is written: a metric whose counters are
    // absent must abort, never silently fall back or score absent as zero.
    let probe: Sample = counters::sample(r, &cfg.iface)?;
    let env = Env::probe(r, &cfg.iface, Some(&probe))?;
    header(cfg, &orig, &port, &env);
    counters::require_for_metric(&probe, cfg.metric.name())?;

    let bind_ip = port.ipv4(r)?.unwrap_or_default();
    if cfg.load != Load::None {
        if bind_ip.is_empty() {
            return Err(format!(
                "{} has no IPv4 address; iperf3 cannot be bound to it and would measure \
                 the wrong NIC. Address the interface, or use --load none.",
                cfg.iface
            ));
        }
        iperf::preflight(&cfg.server, cfg.port)?;
        r.short("command -v iperf3", "check iperf3 on the box")
            .map_err(|_| "iperf3 is not installed on the box".to_string())?;
    }

    if cfg.dry_run {
        println!("\n--dry-run: no writes, no link bounces. Planned points:");
        for v in &cfg.values {
            let mut p = orig.clone();
            p.insert(cfg.param.clone(), *v);
            println!(
                "  {}=0x{v:x} ({v})  main cursor {}",
                cfg.param,
                serdes::main_cursor(&p)
            );
        }
        return Ok(0);
    }

    let mut points: Vec<Point> = Vec::new();
    println!(
        "\n{:>6} {:>4} {:>8} {:>10} {:>10} {:>10} {:>10} {:>8}  note",
        "value", "hex", "main", "pcs_blocks", "pcs_ber", "ec_delta", "Mbit/s", "loss%"
    );

    for &v in &cfg.values {
        let mut proj = orig.clone();
        proj.insert(cfg.param.clone(), v);
        let mc = serdes::main_cursor(&proj);
        if mc <= 0 {
            println!(
                "{v:>6} {v:>4x} {mc:>8} {:>10} {:>10} {:>10} {:>10} {:>8}  SKIPPED: main cursor <= 0",
                "-", "-", "-", "-", "-"
            );
            points.push(Point {
                value: v,
                main_cursor: mc,
                linked: false,
                readback: None,
                pcs_blocks: None,
                pcs_ber: None,
                ec_delta: None,
                mbps: 0.0,
                retrans: -1,
                loss_pct: -1.0,
                lost: -1,
                note: "skipped: main cursor <= 0".into(),
            });
            continue;
        }

        let readback = port.write_param(r, &cfg.param, v)?;
        let linked = port.bounce(r)?;
        let mut note = String::new();
        if readback != v {
            note.push_str(&format!("READBACK {readback} != {v} (hex/dec confusion?) "));
        }
        if !linked {
            note.push_str("LANE DID NOT LINK ");
        }

        let m = if linked {
            measure(r, cfg, &bind_ip)?
        } else {
            Measured::empty()
        };
        note.push_str(&m.note);
        println!(
            "{v:>6} {v:>4x} {mc:>8} {:>10} {:>10} {:>10} {:>10.1} {:>8.3}  {note}",
            show(m.pcs_blocks),
            show(m.pcs_ber),
            show(m.ec_delta),
            m.mbps,
            m.loss_pct
        );
        points.push(Point {
            value: v,
            main_cursor: mc,
            linked,
            readback: Some(readback),
            pcs_blocks: m.pcs_blocks,
            pcs_ber: m.pcs_ber,
            ec_delta: m.ec_delta,
            mbps: m.mbps,
            retrans: m.retrans,
            loss_pct: m.loss_pct,
            lost: m.lost,
            note,
        });
    }

    if cfg.restore {
        let want = orig.get(&cfg.param).copied().unwrap_or(0);
        let back = port.write_param(r, &cfg.param, want)?;
        let up = port.bounce(r)?;
        println!(
            "\nrestored serdes_tx_{}={want} (readback {back}), link {}",
            cfg.param,
            if up { "up" } else { "DOWN - check the box" }
        );
    } else {
        println!(
            "\n--no-restore: serdes_tx_{} is left at the last swept value.",
            cfg.param
        );
    }

    if let Some(path) = &cfg.csv {
        write_csv(path, cfg, &points)?;
        println!("csv: {path}");
    }
    analyse(cfg, &points);
    Ok(0)
}

fn write_csv(path: &str, cfg: &Cfg, points: &[Point]) -> Result<(), String> {
    let mut s = String::from(
        "param,value,hex,main_cursor,linked,readback,pcs_errored_blocks,pcs_ber_events,\
         ec_delta,mbps,retrans,loss_pct,lost,score,note\n",
    );
    for p in points {
        s.push_str(&format!(
            "{},{},{:x},{},{},{},{},{},{},{:.3},{},{:.4},{},{},{}\n",
            cfg.param,
            p.value,
            p.value,
            p.main_cursor,
            p.linked,
            p.readback.map(|v| v.to_string()).unwrap_or_default(),
            show(p.pcs_blocks),
            show(p.pcs_ber),
            show(p.ec_delta),
            p.mbps,
            p.retrans,
            p.loss_pct,
            p.lost,
            p.score(cfg.metric)
                .map(|v| format!("{v:.4}"))
                .unwrap_or_default(),
            p.note.trim().replace(',', ";")
        ));
    }
    std::fs::write(path, s).map_err(|e| format!("write {path}: {e}"))
}

fn analyse(cfg: &Cfg, points: &[Point]) {
    println!("\n== analysis ==");
    let scored: Vec<(&Point, f64)> = points
        .iter()
        .filter_map(|p| p.score(cfg.metric).map(|s| (p, s)))
        .collect();
    let dead: Vec<&Point> = points.iter().filter(|p| !p.linked).collect();
    if !dead.is_empty() {
        println!(
            "{} of {} points did not link: {:?}. A dead lane is a result, not a gap - \
             it bounds the usable range.",
            dead.len(),
            points.len(),
            dead.iter().map(|p| p.value).collect::<Vec<_>>()
        );
    }
    let vanished: Vec<u32> = points
        .iter()
        .filter(|p| p.linked && p.score(cfg.metric).is_none())
        .map(|p| p.value)
        .collect();
    if !vanished.is_empty() {
        println!(
            "WARNING: the {} counter was ABSENT at {vanished:?} despite the link being up. \
             The module changed under the sweep (#105 #131 #161); those points are not \
             scored - they are NOT zero-error results.",
            cfg.metric.name()
        );
    }
    if scored.is_empty() {
        println!("No point produced a usable measurement. Nothing can be concluded.");
        return;
    }

    if !cfg.metric.higher_is_better() && scored.iter().all(|(_, s)| *s <= 0.0) {
        println!(
            "METRIC READ ZERO AT EVERY POINT. {} cannot discriminate here and NO WINNER \
             is reported.\n  \
             A BER-type metric only has gradient while the link is marginal. Either start \
             from a deliberately bad tap so there is an error floor to climb out of, dwell \
             far longer (--duration), or use a metric with margin resolution (eye/PRBS - \
             not implemented in userspace, see docs/serdes-equalisation.md sections 3.2-3.3).",
            cfg.metric.name()
        );
        return;
    }
    if cfg.metric == Metric::Throughput {
        let max = scored.iter().map(|(_, s)| *s).fold(f64::MIN, f64::max);
        let min = scored.iter().map(|(_, s)| *s).fold(f64::MAX, f64::min);
        if max > 0.0 && (max - min) / max < 0.02 {
            println!(
                "All settings landed within 2% of each other ({min:.0}-{max:.0} Mbit/s). \
                 This is the saturation the metric is known for - it is NOT evidence that \
                 the settings are equivalent. Re-run with --metric pcs."
            );
        }
    }

    let best = scored
        .iter()
        .copied()
        .reduce(|a, b| {
            let better = if cfg.metric.higher_is_better() {
                b.1 > a.1
            } else {
                b.1 < a.1
            };
            if better {
                b
            } else {
                a
            }
        })
        .expect("non-empty");
    println!(
        "best: {}={} (0x{:x}) score {:.4} [{} is better], main cursor {}",
        cfg.param,
        best.0.value,
        best.0.value,
        best.1,
        if cfg.metric.higher_is_better() {
            "higher"
        } else {
            "lower"
        },
        best.0.main_cursor
    );

    let ties: Vec<u32> = scored
        .iter()
        .filter(|(_, s)| (*s - best.1).abs() < f64::EPSILON)
        .map(|(p, _)| p.value)
        .collect();
    if ties.len() > 1 {
        println!(
            "TIED at that score: {ties:?}. The metric cannot separate them; do not pick one \
             and call it characterised (#207)."
        );
    }

    let first = points.first().map(|p| p.value);
    let last = points.last().map(|p| p.value);
    if Some(best.0.value) == first || Some(best.0.value) == last {
        println!(
            "WARNING: the best point sits at an EDGE of the swept range. Extend the sweep in \
             that direction before believing it.\n  \
             Over-emphasis produces an ERROR FLOOR rather than a plateau (802.3dj), so a \
             monotone hill-climb can walk straight into it; sweep past the apparent optimum \
             in BOTH directions."
        );
    }

    if cfg.metric == Metric::Pcs {
        println!(
            "Reminder: MDIO 3.33 saturates at 255 errored blocks / 63 BER events per poll, \
             so a large delta is a lower bound. Values flatten exactly where the link is worst."
        );
    }
}
