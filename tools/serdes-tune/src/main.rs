// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! serdes-tune - SFP+ and 10G SerDes TX-equalisation tooling for woomera.
//!
//! Runs on the dev host and drives the box over SSH by default; `--local` runs
//! the same code paths on the box itself. See README.md for the rationale and
//! docs/serdes-equalisation.md for the physics.

mod args;
mod counters;
mod env;
mod exec;
mod iperf;
mod jsonlite;
mod serdes;
mod sff8472;
mod sfp;
mod sweep;

use args::Args;
use exec::{Runner, Target};

/// Documented lab default for this box (docs/fedora-on-ssd.md), not a secret.
const DEFAULT_PASSWORD: &str = "unvr";
/// The 10G SFP+ port. Everything else is resolved from the interface name.
const DEFAULT_IFACE: &str = "enp0s2";
const DEFAULT_IPERF_PORT: u16 = 5701;

const VALUE_OPTS: [&str; 20] = [
    "csv",
    "set",
    "host",
    "password",
    "iface",
    "via",
    "i2c-bus",
    "save",
    "diff",
    "to",
    "window",
    "param",
    "values",
    "metric",
    "duration",
    "repeat",
    "streams",
    "server",
    "port",
    "udp-mbit",
];

const KNOWN_FLAGS: [&str; 10] = [
    "local",
    "verbose",
    "help",
    "hex",
    "json",
    "dry-run",
    "no-restore",
    "no-bounce",
    "no-load",
    "allow-control-path",
];

const USAGE: &str = "\
serdes-tune <subcommand> [options]

Where it runs
  default          drive the box over SSH from the dev host
  --local          the binary is running ON the box (direct sysfs/ethtool)
  --host <addr>    box address (default: $WOOMERA_HOST, else scripts/ssh-woomera.py --print)
  --password <pw>  use sshpass with this password (default: the lab root password;
                   pass an empty string to use keys/agent only)
  --iface <name>   interface (default enp0s2, the 10G SFP+ port)
  --verbose        echo every command executed on the box

env        what is running on the box: kernel, al_eth module srcversion, resolved
           interface + PCI address, and which expected counters are present
           exit 4 = one or more expected counters are missing (stale module)

sfp        dump and decode the whole SFF-8472 map (A0h + A2h)
  --via auto|ethtool|i2c   how to read the EEPROM (default auto)
  --i2c-bus <n>            i2c bus for the fallback path (default 2 = mux ch1)
  --hex                    also hexdump both pages
  exit 3 = a checksum did not verify

counters   read link-quality counters (pcs_*, ec_*, generic PHY)
  --json                   emit a snapshot instead of the report
  --save <file>            write the snapshot to <file>
  --diff <file>            diff <file> against a live sample
  --diff <file> --to <f2>  diff two saved snapshots
  --window <secs>          sample, dwell, sample again, print the delta

params     show the serdes_tx_* knobs and the derived main cursor
  --set <param>=<value>    write one (value is DECIMAL here, hex on the wire)
  --no-bounce              do not bounce the link (the write then does nothing)

sweep      sweep one serdes_tx_* parameter and score every setting
  --param <name>           amp|total_driver_units|c_minus_1|c_plus_1|c_plus_2|slew_rate
  --values 0,1,2,...       decimal values to try (written as hex)
  --metric pcs|ec|udp-loss|throughput   default pcs
  --duration <secs>        dwell/iperf3 seconds per point (default 10)
  --repeat <n>             measurements per point (default 1)
  --streams <n>            iperf3 TCP streams (default 4)
  --server <ip>            iperf3 server on the dev host (default: the SSH client IP)
  --port <n>               iperf3 port (default 5701)
  --udp-mbit <n>           offered rate for --metric udp-loss (default 2000)
  --no-load                do not generate traffic; just dwell and read counters
  --csv <file>             write per-point results
  --no-restore             leave the last swept value in place
  --dry-run                print the plan (and each main cursor) and stop
  --allow-control-path     permit sweeping the interface the SSH session rides on
";

fn locate_host() -> Result<String, String> {
    if let Ok(h) = std::env::var("WOOMERA_HOST") {
        if !h.trim().is_empty() {
            return Ok(h.trim().to_string());
        }
    }
    let out = std::process::Command::new("python3")
        .args(["scripts/ssh-woomera.py", "--print"])
        .output()
        .map_err(|e| format!("cannot run scripts/ssh-woomera.py ({e})"))?;
    let addr = String::from_utf8_lossy(&out.stdout).trim().to_string();
    if !out.status.success() || addr.is_empty() {
        return Err(
            "cannot locate the box: set --host, or $WOOMERA_HOST, or run from the repo root \
             so scripts/ssh-woomera.py can find it by MAC"
                .into(),
        );
    }
    Ok(addr)
}

fn make_runner(a: &Args) -> Result<Runner, String> {
    let verbose = a.flag("verbose");
    if a.flag("local") {
        return Ok(Runner {
            target: Target::Local,
            verbose,
        });
    }
    let host = match a.opt("host") {
        Some(h) => h.to_string(),
        None => locate_host()?,
    };
    let password = match a.opt("password") {
        Some("") => None,
        Some(p) => Some(p.to_string()),
        None => Some(DEFAULT_PASSWORD.to_string()),
    };
    Ok(Runner {
        target: Target::Ssh { host, password },
        verbose,
    })
}

fn cmd_sfp(a: &Args, r: &Runner) -> Result<i32, String> {
    let iface = a.str_or("iface", DEFAULT_IFACE);
    let via = a.str_or("via", "auto");
    let bus: u32 = a.num("i2c-bus", sfp::DEFAULT_I2C_BUS)?;
    let e = sfp::fetch(r, &iface, &via, bus)?;
    if a.flag("hex") {
        print!("{}", sff8472::hexdump("A0h (0x50):", &e.a0));
        if let Some(a2) = &e.a2 {
            print!("{}", sff8472::hexdump("A2h (0x51):", a2));
        }
        println!();
    }
    let (report, ok) = sff8472::decode(&e);
    print!("{report}");
    if !ok {
        eprintln!(
            "\nCHECKSUM MISMATCH: the EEPROM read is inconsistent. Suspect a bad i2c read \
             (retry) before suspecting the module."
        );
        return Ok(3);
    }
    Ok(0)
}

fn cmd_counters(a: &Args, r: &Runner) -> Result<i32, String> {
    let iface = a.str_or("iface", DEFAULT_IFACE);

    if let Some(path) = a.opt("diff") {
        let text = std::fs::read_to_string(path).map_err(|e| format!("read {path}: {e}"))?;
        let before = counters::from_json(&text)?;
        let after = match a.opt("to") {
            Some(p2) => {
                let t2 = std::fs::read_to_string(p2).map_err(|e| format!("read {p2}: {e}"))?;
                counters::from_json(&t2)?
            }
            None => counters::sample(r, &iface)?,
        };
        print!("{}", counters::report_diff(&before, &after));
        return Ok(0);
    }

    if let Some(w) = a.opt("window") {
        let secs: u64 = w
            .parse()
            .map_err(|_| format!("--window {w}: not a number of seconds"))?;
        let before = counters::sample(r, &iface)?;
        println!("sampled, dwelling {secs}s ...");
        std::thread::sleep(std::time::Duration::from_secs(secs));
        let after = counters::sample(r, &iface)?;
        print!("{}", counters::report_diff(&before, &after));
        return Ok(0);
    }

    let s = counters::sample(r, &iface)?;
    if let Some(path) = a.opt("save") {
        std::fs::write(path, counters::to_json(&s)).map_err(|e| format!("write {path}: {e}"))?;
        eprintln!("# snapshot written to {path}");
    }
    if a.flag("json") {
        print!("{}", counters::to_json(&s));
        return Ok(0);
    }
    print!("{}", env::Env::probe(r, &iface, Some(&s))?.render());
    println!();
    print!("{}", counters::report(&s));
    Ok(0)
}

fn cmd_env(a: &Args, r: &Runner) -> Result<i32, String> {
    let iface = a.str_or("iface", DEFAULT_IFACE);
    let s = counters::sample(r, &iface).ok();
    let e = env::Env::probe(r, &iface, s.as_ref())?;
    print!("{}", e.render());
    if s.is_none() {
        println!("  ethtool -S failed: no counter inventory could be taken");
    }
    Ok(if e.missing.is_empty() { 0 } else { 4 })
}

fn cmd_params(a: &Args, r: &Runner) -> Result<i32, String> {
    let iface = a.str_or("iface", DEFAULT_IFACE);
    let port = serdes::Port::resolve(r, &iface)?;

    if let Some(spec) = a.opt("set") {
        let (param, val) = spec
            .split_once('=')
            .ok_or_else(|| format!("--set {spec}: expected <param>=<value>"))?;
        if !serdes::TX_PARAMS.contains(&param) {
            return Err(format!(
                "{param}: expected one of {:?}",
                serdes::TX_PARAMS
            ));
        }
        let v: u32 = val
            .parse()
            .map_err(|_| format!("{val}: not a decimal value"))?;
        let back = port.write_param(r, param, v)?;
        println!("serdes_tx_{param} <- {v} (wrote 0x{v:x}), readback {back}");
        if back != v {
            println!("READBACK MISMATCH - the store parses hex, the show prints decimal");
        }
        if a.flag("no-bounce") {
            println!(
                "--no-bounce: the override is staged but NOT applied; it takes effect at the \
                 next link event (al_eth_serdes_static_tx_params_set from pcs_config)."
            );
        } else {
            port.check_not_control_path(r)?;
            let up = port.bounce(r)?;
            println!("link bounced, carrier {}", if up { "up" } else { "DOWN" });
        }
    }

    let cur = port.read_params(r)?;
    println!("{} -> {}", iface, port.sysfs);
    print!("{}", serdes::describe(&cur));
    Ok(0)
}

fn cmd_sweep(a: &Args, r: &Runner) -> Result<i32, String> {
    let iface = a.str_or("iface", DEFAULT_IFACE);
    let metric = sweep::Metric::parse(&a.str_or("metric", "pcs"))?;
    let load = if a.flag("no-load") {
        sweep::Load::None
    } else if metric == sweep::Metric::UdpLoss {
        sweep::Load::Udp
    } else {
        sweep::Load::Tcp
    };
    let server = match a.opt("server") {
        Some(s) => s.to_string(),
        None => {
            let c = r.short("printf '%s' \"${SSH_CLIENT%% *}\"", "read SSH_CLIENT")?;
            let c = c.trim().to_string();
            if c.is_empty() && load != sweep::Load::None && !a.flag("dry-run") {
                return Err(
                    "--server is required: the iperf3 server address could not be inferred \
                     (no SSH_CLIENT, e.g. under --local)"
                        .into(),
                );
            }
            c
        }
    };
    let csv = a.opt("csv").map(|s| s.to_string());
    let cfg = sweep::Cfg {
        iface,
        param: a.str_or("param", "c_plus_1"),
        values: args::parse_list(&a.str_or("values", "0,1,2,3,4,5,6,7"))?,
        metric,
        duration: a.num("duration", 10u64)?,
        repeat: a.num("repeat", 1u32)?,
        streams: a.num("streams", 4u32)?,
        server,
        port: a.num("port", DEFAULT_IPERF_PORT)?,
        udp_mbit: a.num("udp-mbit", 2000u32)?,
        load,
        csv,
        restore: !a.flag("no-restore"),
        dry_run: a.flag("dry-run"),
        allow_control_path: a.flag("allow-control-path"),
    };
    sweep::run(r, &cfg)
}

fn run() -> Result<i32, String> {
    let argv: Vec<String> = std::env::args().skip(1).collect();
    let a = Args::parse(&argv, &VALUE_OPTS)?;
    a.check_known(&KNOWN_FLAGS)?;
    if !a.positional.is_empty() {
        return Err(format!(
            "unexpected argument(s) {:?} - every setting is a --option\n\n{USAGE}",
            a.positional
        ));
    }
    if a.subcommand.is_empty() || a.flag("help") || a.subcommand == "help" {
        print!("{USAGE}");
        return Ok(if a.subcommand.is_empty() { 2 } else { 0 });
    }
    let r = make_runner(&a)?;
    r.preflight()?;
    match a.subcommand.as_str() {
        "sfp" => cmd_sfp(&a, &r),
        "env" => cmd_env(&a, &r),
        "counters" => cmd_counters(&a, &r),
        "params" => cmd_params(&a, &r),
        "sweep" => cmd_sweep(&a, &r),
        other => Err(format!("unknown subcommand {other:?}\n\n{USAGE}")),
    }
}

fn main() {
    match run() {
        Ok(code) => std::process::exit(code),
        Err(e) => {
            eprintln!("serdes-tune: {e}");
            std::process::exit(1);
        }
    }
}
