// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! What is actually running on the box, printed with every measurement.
//!
//! Kernel/module mismatch is this project's most expensive recurring bug
//! (#105 #131 #161), and the counters this tool scores on are new (#196). A
//! result without its kernel release, module srcversion and the present/missing
//! counter list is not reproducible - and an absent counter read as 0 looks
//! exactly like a healthy link.

use crate::counters::{self, Sample};
use crate::exec::Runner;

pub struct Env {
    pub uname_r: String,
    pub uname_sm: String,
    pub iface: String,
    pub pci: String,
    pub driver: String,
    pub module: String,
    pub srcversion: String,
    pub module_version: String,
    pub present: Vec<String>,
    pub missing: Vec<String>,
}

fn first_line(s: &str) -> String {
    s.lines().next().unwrap_or("").trim().to_string()
}

impl Env {
    pub fn probe(r: &Runner, iface: &str, sample: Option<&Sample>) -> Result<Env, String> {
        let uname_r = first_line(&r.short("uname -r", "uname -r")?);
        let uname_sm = first_line(&r.short("uname -sm", "uname -sm")?);
        let dev = format!("/sys/class/net/{iface}/device");
        let pci = first_line(&r.short(
            &format!("basename $(readlink -f {dev} 2>/dev/null) 2>/dev/null || true"),
            "resolve PCI address",
        )?);
        let driver = first_line(&r.short(
            &format!("basename $(readlink -f {dev}/driver 2>/dev/null) 2>/dev/null || true"),
            "resolve driver",
        )?);
        let module = first_line(&r.short(
            &format!("basename $(readlink -f {dev}/driver/module 2>/dev/null) 2>/dev/null || true"),
            "resolve module",
        )?);
        let (srcversion, module_version) = if module.is_empty() {
            (String::new(), String::new())
        } else {
            (
                first_line(&r.short(
                    &format!("cat /sys/module/{module}/srcversion 2>/dev/null || true"),
                    "module srcversion",
                )?),
                first_line(&r.short(
                    &format!("cat /sys/module/{module}/version 2>/dev/null || true"),
                    "module version",
                )?),
            )
        };
        let (present, missing) = match sample {
            Some(s) => counters::expected_split(s),
            None => (Vec::new(), Vec::new()),
        };
        Ok(Env {
            uname_r,
            uname_sm,
            iface: iface.to_string(),
            pci,
            driver,
            module,
            srcversion,
            module_version,
            present,
            missing,
        })
    }

    fn or_unknown(s: &str) -> &str {
        if s.is_empty() {
            "UNRESOLVED"
        } else {
            s
        }
    }

    pub fn render(&self) -> String {
        let mut o = String::from("== environment (provenance for every number below) ==\n");
        o.push_str(&format!(
            "  kernel      {} ({})\n",
            Self::or_unknown(&self.uname_r),
            Self::or_unknown(&self.uname_sm)
        ));
        o.push_str(&format!(
            "  interface   {}  PCI {}  driver {}\n",
            self.iface,
            Self::or_unknown(&self.pci),
            Self::or_unknown(&self.driver)
        ));
        o.push_str(&format!(
            "  module      {}  srcversion {}  version {}\n",
            Self::or_unknown(&self.module),
            Self::or_unknown(&self.srcversion),
            Self::or_unknown(&self.module_version)
        ));
        if !self.present.is_empty() || !self.missing.is_empty() {
            o.push_str(&format!(
                "  counters    {} of {} expected present\n",
                self.present.len(),
                self.present.len() + self.missing.len()
            ));
            if !self.missing.is_empty() {
                o.push_str(&format!("  MISSING     {}\n", self.missing.join(", ")));
                if !self.driver.is_empty() && !self.driver.starts_with("al_eth") {
                    o.push_str(&format!(
                        "  Driver is {}, not al_eth: these counters are al_eth-specific and will\n\
                         \x20 never appear here. pcs/ec scoring is not available on this port.\n",
                        self.driver
                    ));
                    return o;
                }
                o.push_str(
                    "  These counters are new (#196 PCS harvest, EC drop mirrors). Their absence\n\
                     \x20 means the al_eth on the box is older than this tree - a stale module\n\
                     \x20 deploy is the usual cause (#105 #131 #161). Anything scored on a missing\n\
                     \x20 counter would read as a clean link. Redeploy the module before trusting\n\
                     \x20 any result here.\n",
                );
            }
        }
        o
    }
}
