// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Command execution on the box: over SSH from the dev host, or locally when
//! the binary is running on the box itself (`--local`).
//!
//! Every call is bounded and every failure names the host, the command, the
//! limit and the elapsed time - a silent hang here looks exactly like a bad
//! SerDes setting, which is the one confusion this tool must never create.

use std::io::Read;
use std::process::{Child, Command, Stdio};
use std::time::{Duration, Instant};

/// SSH TCP+auth handshake on a LAN. Expected worst case ~2 s on a busy box
/// (scripts/sfp-ddm.py uses 8). On expiry: hard error naming the host.
pub const CONNECT_TIMEOUT_S: u64 = 5;

/// sysfs/ethtool/ip commands: sub-second once the session is up. Must exceed
/// CONNECT_TIMEOUT_S so ssh's own "unreachable" error surfaces instead of this
/// wrapper killing it first. On expiry: hard error, no retry.
pub const SHORT_CMD_S: u64 = 8;

/// Poll interval while waiting on a child process or a link state.
pub const POLL_MS: u64 = 50;

#[derive(Clone)]
pub enum Target {
    Local,
    Ssh {
        host: String,
        password: Option<String>,
    },
}

pub struct Out {
    pub code: i32,
    pub stdout: String,
    pub stderr: String,
}

impl Out {
    pub fn ok(&self) -> bool {
        self.code == 0
    }
}

pub struct Runner {
    pub target: Target,
    pub verbose: bool,
}

impl Runner {
    pub fn describe(&self) -> String {
        match &self.target {
            Target::Local => "local".into(),
            Target::Ssh { host, .. } => format!("ssh root@{host}"),
        }
    }

    fn build(&self, cmd: &str) -> Command {
        match &self.target {
            Target::Local => {
                let mut c = Command::new("sh");
                c.arg("-c").arg(cmd);
                c
            }
            Target::Ssh { host, password } => {
                let ssh_args = |c: &mut Command| {
                    c.arg("-o")
                        .arg(format!("ConnectTimeout={CONNECT_TIMEOUT_S}"))
                        .arg("-o")
                        .arg("StrictHostKeyChecking=accept-new")
                        .arg("-o")
                        .arg("LogLevel=ERROR")
                        .arg(format!("root@{host}"))
                        .arg(cmd);
                };
                match password {
                    Some(pw) => {
                        let mut c = Command::new("sshpass");
                        c.arg("-p").arg(pw).arg("ssh");
                        c.arg("-o").arg("PreferredAuthentications=password")
                            .arg("-o")
                            .arg("PubkeyAuthentication=no");
                        ssh_args(&mut c);
                        c
                    }
                    None => {
                        let mut c = Command::new("ssh");
                        c.arg("-o").arg("BatchMode=yes");
                        ssh_args(&mut c);
                        c
                    }
                }
            }
        }
    }

    /// Run `cmd`, bounded by `limit`. `what` names the operation in errors.
    pub fn run(&self, cmd: &str, limit: Duration, what: &str) -> Result<Out, String> {
        if self.verbose {
            eprintln!("+ [{}] {cmd}", self.describe());
        }
        let started = Instant::now();
        let mut child = self
            .build(cmd)
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .map_err(|e| format!("{what}: cannot spawn ({e}) - is ssh/sshpass installed?"))?;

        let mut so = child.stdout.take().expect("piped");
        let mut se = child.stderr.take().expect("piped");
        let t_out = std::thread::spawn(move || {
            let mut s = String::new();
            let _ = so.read_to_string(&mut s);
            s
        });
        let t_err = std::thread::spawn(move || {
            let mut s = String::new();
            let _ = se.read_to_string(&mut s);
            s
        });

        let status = wait_bounded(&mut child, limit);
        let stdout = t_out.join().unwrap_or_default();
        let stderr = t_err.join().unwrap_or_default();

        let code = match status {
            Some(s) => s.code().unwrap_or(-1),
            None => {
                return Err(format!(
                    "{what}: TIMED OUT after {:.1}s (limit {:.1}s), killed. cmd: {cmd}",
                    started.elapsed().as_secs_f64(),
                    limit.as_secs_f64()
                ))
            }
        };
        if code == 255 && matches!(self.target, Target::Ssh { .. }) {
            return Err(format!(
                "{what}: SSH failed ({}) - box unreachable or auth refused via {}",
                stderr.trim(),
                self.describe()
            ));
        }
        Ok(Out {
            code,
            stdout,
            stderr,
        })
    }

    /// Run and require exit 0; returns trimmed stdout.
    pub fn run_ok(&self, cmd: &str, limit: Duration, what: &str) -> Result<String, String> {
        let o = self.run(cmd, limit, what)?;
        if !o.ok() {
            return Err(format!(
                "{what}: exit {} via {}\n  cmd: {cmd}\n  stderr: {}",
                o.code,
                self.describe(),
                o.stderr.trim()
            ));
        }
        Ok(o.stdout.trim_end().to_string())
    }

    pub fn short(&self, cmd: &str, what: &str) -> Result<String, String> {
        self.run_ok(cmd, Duration::from_secs(SHORT_CMD_S), what)
    }

    /// Fail immediately and clearly when the box is not there.
    pub fn preflight(&self) -> Result<(), String> {
        let host = self.short("uname -sm", "preflight: uname")?;
        if self.verbose {
            eprintln!("# target {} -> {host}", self.describe());
        }
        Ok(())
    }
}

fn wait_bounded(child: &mut Child, limit: Duration) -> Option<std::process::ExitStatus> {
    let deadline = Instant::now() + limit;
    loop {
        match child.try_wait() {
            Ok(Some(s)) => return Some(s),
            Ok(None) => {}
            Err(_) => return None,
        }
        if Instant::now() >= deadline {
            let _ = child.kill();
            let _ = child.wait();
            return None;
        }
        std::thread::sleep(Duration::from_millis(POLL_MS));
    }
}
