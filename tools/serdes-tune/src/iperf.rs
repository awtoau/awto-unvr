// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Load generation: iperf3 run ON the box, against a server on the dev host.
//!
//! Bound with -B <ip>%<iface> because an unbound client picks the wrong NIC on
//! this box (#121) and would silently measure the 1G port.

use crate::exec::Runner;
use std::net::{TcpStream, ToSocketAddrs};
use std::time::Duration;

/// iperf3 wall time is `duration` plus connect and summary exchange. 15 s of
/// headroom covers -P 4 setup and teardown; on expiry the child is killed and
/// the sweep point is recorded as failed rather than as clean.
const IPERF_OVERHEAD_S: u64 = 15;

/// TCP preflight to the iperf3 server. Two round trips on a LAN; 2 s is ~orders
/// over that but still short enough to be an obvious failure, not a hang.
const PREFLIGHT_S: u64 = 2;

pub struct Tcp {
    pub mbps: f64,
    pub retrans: i64,
}

pub struct Udp {
    pub loss_pct: f64,
    pub lost: i64,
    pub total: i64,
}

fn unit_scale(tok: &str) -> Option<f64> {
    match tok {
        "bits/sec" => Some(1e-6),
        "Kbits/sec" => Some(1e-3),
        "Mbits/sec" => Some(1.0),
        "Gbits/sec" => Some(1e3),
        _ => None,
    }
}

pub fn parse_tcp(text: &str) -> Option<Tcp> {
    let mut best: Option<Tcp> = None;
    for line in text.lines() {
        if !line.contains("sender") {
            continue;
        }
        let t: Vec<&str> = line.split_whitespace().collect();
        let Some(i) = t.iter().position(|x| unit_scale(x).is_some()) else {
            continue;
        };
        if i == 0 {
            continue;
        }
        let (Some(scale), Ok(val)) = (unit_scale(t[i]), t[i - 1].parse::<f64>()) else {
            continue;
        };
        let retrans = t
            .get(i + 1)
            .and_then(|x| x.parse::<i64>().ok())
            .unwrap_or(-1);
        let point = Tcp {
            mbps: val * scale,
            retrans,
        };
        // A [SUM] line is authoritative when -P > 1; otherwise the last sender
        // line is the stream total.
        if line.contains("SUM") {
            return Some(point);
        }
        best = Some(point);
    }
    best
}

pub fn parse_udp(text: &str) -> Option<Udp> {
    let mut out: Option<Udp> = None;
    for line in text.lines() {
        let t: Vec<&str> = line.split_whitespace().collect();
        let Some(i) = t.iter().position(|x| {
            x.contains('/')
                && x.split('/').count() == 2
                && x.split('/').all(|p| p.parse::<i64>().is_ok())
        }) else {
            continue;
        };
        let mut parts = t[i].split('/');
        let lost: i64 = parts.next()?.parse().ok()?;
        let total: i64 = parts.next()?.parse().ok()?;
        let loss_pct = t
            .get(i + 1)
            .and_then(|x| x.trim_matches(|c| c == '(' || c == ')' || c == '%').parse::<f64>().ok())
            .unwrap_or(if total > 0 {
                100.0 * lost as f64 / total as f64
            } else {
                -1.0
            });
        out = Some(Udp {
            loss_pct,
            lost,
            total,
        });
        if line.contains("receiver") {
            break;
        }
    }
    out
}

/// Prove something is listening before a sweep spends minutes measuring nothing.
pub fn preflight(server: &str, port: u16) -> Result<(), String> {
    let addrs: Vec<_> = format!("{server}:{port}")
        .to_socket_addrs()
        .map_err(|e| format!("iperf3 server {server}:{port}: cannot resolve ({e})"))?
        .collect();
    let addr = addrs
        .first()
        .ok_or_else(|| format!("iperf3 server {server}:{port}: no address"))?;
    TcpStream::connect_timeout(addr, Duration::from_secs(PREFLIGHT_S)).map_err(|e| {
        format!(
            "iperf3 server {server}:{port} not answering ({e}).\n  \
             Start it on the dev host first:  iperf3 -s -p {port}"
        )
    })?;
    Ok(())
}

fn bind_arg(bind_ip: &str, iface: &str) -> String {
    if bind_ip.is_empty() {
        String::new()
    } else {
        format!(" -B {bind_ip}%{iface}")
    }
}

pub fn run_tcp(
    r: &Runner,
    server: &str,
    port: u16,
    secs: u64,
    bind_ip: &str,
    iface: &str,
    streams: u32,
) -> Result<Tcp, String> {
    let cmd = format!(
        "iperf3 -c {server} -p {port} -t {secs} -P {streams}{} 2>&1",
        bind_arg(bind_ip, iface)
    );
    let out = r.run(
        &cmd,
        Duration::from_secs(secs + IPERF_OVERHEAD_S),
        "iperf3 TCP",
    )?;
    parse_tcp(&out.stdout).ok_or_else(|| {
        format!(
            "iperf3 TCP: no sender summary in output (exit {}):\n{}",
            out.code,
            out.stdout.trim()
        )
    })
}

pub fn run_udp(
    r: &Runner,
    server: &str,
    port: u16,
    secs: u64,
    bind_ip: &str,
    iface: &str,
    mbit: u32,
) -> Result<Udp, String> {
    let cmd = format!(
        "iperf3 -c {server} -p {port} -u -b {mbit}M -t {secs}{} 2>&1",
        bind_arg(bind_ip, iface)
    );
    let out = r.run(
        &cmd,
        Duration::from_secs(secs + IPERF_OVERHEAD_S),
        "iperf3 UDP",
    )?;
    parse_udp(&out.stdout).ok_or_else(|| {
        format!(
            "iperf3 UDP: no loss summary in output (exit {}):\n{}",
            out.code,
            out.stdout.trim()
        )
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    const TCP_OUT: &str = "\
[SUM]   0.00-6.00   sec  6.75 GBytes  9.66 Gbits/sec  120             sender
[SUM]   0.00-6.00   sec  6.74 GBytes  9.65 Gbits/sec                  receiver
";

    const TCP_SINGLE: &str = "\
[  5]   0.00-6.00   sec  4.10 MBytes  5.73 Mbits/sec    3             sender
[  5]   0.00-6.00   sec  4.00 MBytes  5.59 Mbits/sec                  receiver
";

    const UDP_OUT: &str = "\
[  5]   0.00-6.00   sec  1.40 GBytes  2.00 Gbits/sec  0.000 ms  0/1015000 (0%)  sender
[  5]   0.00-6.00   sec  1.39 GBytes  1.99 Gbits/sec  0.011 ms  2050/1015000 (0.2%)  receiver
";

    #[test]
    fn tcp_sum_line_wins() {
        let t = parse_tcp(TCP_OUT).unwrap();
        assert!((t.mbps - 9660.0).abs() < 1.0, "{}", t.mbps);
        assert_eq!(t.retrans, 120);
    }

    #[test]
    fn tcp_single_stream_is_parsed() {
        let t = parse_tcp(TCP_SINGLE).unwrap();
        assert!((t.mbps - 5.73).abs() < 0.01, "{}", t.mbps);
        assert_eq!(t.retrans, 3);
    }

    #[test]
    fn udp_receiver_line_wins() {
        let u = parse_udp(UDP_OUT).unwrap();
        assert_eq!(u.lost, 2050);
        assert_eq!(u.total, 1_015_000);
        assert!((u.loss_pct - 0.2).abs() < 0.001, "{}", u.loss_pct);
    }

    #[test]
    fn garbage_is_rejected_not_guessed() {
        assert!(parse_tcp("iperf3: error - unable to connect").is_none());
        assert!(parse_udp("iperf3: error - unable to connect").is_none());
    }
}
