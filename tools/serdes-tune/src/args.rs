// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Minimal argv parser: `--key value`, `--key=value`, `--flag`, repeated keys.
//!
//! Unknown options are a hard error - a mistyped `--metrik` must not silently
//! select the default metric.

use std::collections::BTreeMap;

pub struct Args {
    pub subcommand: String,
    vals: BTreeMap<String, Vec<String>>,
    flags: Vec<String>,
    pub positional: Vec<String>,
}

impl Args {
    /// `value_opts` names the options that consume the following argv element.
    pub fn parse(argv: &[String], value_opts: &[&str]) -> Result<Args, String> {
        let mut vals: BTreeMap<String, Vec<String>> = BTreeMap::new();
        let mut flags = Vec::new();
        let mut positional = Vec::new();
        let mut subcommand = String::new();

        let mut i = 0;
        while i < argv.len() {
            let a = &argv[i];
            if let Some(body) = a.strip_prefix("--") {
                let (name, inline) = match body.split_once('=') {
                    Some((n, v)) => (n.to_string(), Some(v.to_string())),
                    None => (body.to_string(), None),
                };
                if value_opts.contains(&name.as_str()) {
                    let v = match inline {
                        Some(v) => v,
                        None => {
                            i += 1;
                            argv.get(i)
                                .ok_or_else(|| format!("--{name} needs a value"))?
                                .clone()
                        }
                    };
                    vals.entry(name).or_default().push(v);
                } else if inline.is_some() {
                    return Err(format!("--{name} does not take a value"));
                } else {
                    flags.push(name);
                }
            } else if subcommand.is_empty() {
                subcommand = a.clone();
            } else {
                positional.push(a.clone());
            }
            i += 1;
        }
        Ok(Args {
            subcommand,
            vals,
            flags,
            positional,
        })
    }

    /// Reject any option the caller never asked about, so typos fail loudly.
    pub fn check_known(&self, known_flags: &[&str]) -> Result<(), String> {
        for f in &self.flags {
            if !known_flags.contains(&f.as_str()) {
                return Err(format!("unknown option --{f}"));
            }
        }
        Ok(())
    }

    pub fn flag(&self, name: &str) -> bool {
        self.flags.iter().any(|f| f == name)
    }

    pub fn opt(&self, name: &str) -> Option<&str> {
        self.vals.get(name).and_then(|v| v.last()).map(|s| s.as_str())
    }

    pub fn str_or(&self, name: &str, default: &str) -> String {
        self.opt(name).unwrap_or(default).to_string()
    }

    pub fn num<T: std::str::FromStr>(&self, name: &str, default: T) -> Result<T, String> {
        match self.opt(name) {
            None => Ok(default),
            Some(s) => s
                .parse::<T>()
                .map_err(|_| format!("--{name}: not a number: {s:?}")),
        }
    }
}

/// Parse "0,1,2, 5" into decimal values.
pub fn parse_list(s: &str) -> Result<Vec<u32>, String> {
    let mut out = Vec::new();
    for tok in s.split(',') {
        let t = tok.trim();
        if t.is_empty() {
            continue;
        }
        out.push(
            t.parse::<u32>()
                .map_err(|_| format!("not a decimal value: {t:?}"))?,
        );
    }
    if out.is_empty() {
        return Err("empty value list".into());
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn argv(s: &[&str]) -> Vec<String> {
        s.iter().map(|x| (*x).to_string()).collect()
    }

    #[test]
    fn parses_values_flags_and_subcommand() {
        let a = Args::parse(
            &argv(&["sweep", "--param", "c_plus_1", "--values=1,2", "--dry-run"]),
            &["param", "values"],
        )
        .unwrap();
        assert_eq!(a.subcommand, "sweep");
        assert_eq!(a.opt("param"), Some("c_plus_1"));
        assert_eq!(a.opt("values"), Some("1,2"));
        assert!(a.flag("dry-run"));
    }

    #[test]
    fn typos_are_errors_not_silent_defaults() {
        let a = Args::parse(&argv(&["sweep", "--metrik"]), &["metric"]).unwrap();
        assert!(a.check_known(&["dry-run"]).is_err());
    }

    #[test]
    fn a_value_option_without_a_value_fails() {
        assert!(Args::parse(&argv(&["sweep", "--param"]), &["param"]).is_err());
    }

    #[test]
    fn value_lists_reject_junk() {
        assert_eq!(parse_list("0, 1,7").unwrap(), vec![0, 1, 7]);
        assert!(parse_list("0,0x5").is_err());
        assert!(parse_list("").is_err());
    }
}
