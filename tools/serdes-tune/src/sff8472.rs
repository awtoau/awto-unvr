// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Awto / Daniel Tyrrell
//! Full SFF-8472 decode: A0h base/extended ID and A2h diagnostics.
//!
//! Field and table references are to SFF-8472 rev 12.4 (sources/README.md).
//! A2h bytes 56-91 are two different structures selected by A0h byte 92 bit 4
//! (external calibration) - both are decoded.

const CONNECTORS: [(u8, &str); 22] = [
    (0x00, "Unknown or unspecified"),
    (0x01, "SC (Subscriber Connector)"),
    (0x02, "Fibre Channel Style 1 copper"),
    (0x03, "Fibre Channel Style 2 copper"),
    (0x04, "BNC/TNC"),
    (0x05, "Fibre Channel coax headers"),
    (0x06, "Fiber Jack"),
    (0x07, "LC (Lucent Connector)"),
    (0x08, "MT-RJ"),
    (0x09, "MU"),
    (0x0a, "SG"),
    (0x0b, "Optical pigtail"),
    (0x0c, "MPO 1x12"),
    (0x0d, "MPO 2x16"),
    (0x20, "HSSDC II"),
    (0x21, "Copper pigtail"),
    (0x22, "RJ45"),
    (0x23, "No separable connector"),
    (0x24, "MXC 2x16"),
    (0x25, "CS optical connector"),
    (0x26, "SN optical connector"),
    (0x27, "MPO 2x12"),
];

const ENCODINGS: [(u8, &str); 9] = [
    (0x00, "Unspecified"),
    (0x01, "8B/10B"),
    (0x02, "4B/5B"),
    (0x03, "NRZ"),
    (0x04, "Manchester"),
    (0x05, "SONET Scrambled"),
    (0x06, "64B/66B"),
    (0x07, "256B/257B"),
    (0x08, "PAM4"),
];

/// (byte, bit, name) over A0h bytes 3-10, Table 5-3.
const XCVR_CODES: [(usize, u8, &str); 60] = [
    (3, 7, "10GBASE-ER"),
    (3, 6, "10GBASE-LRM"),
    (3, 5, "10GBASE-LR"),
    (3, 4, "10GBASE-SR"),
    (3, 3, "Infiniband 1X SX"),
    (3, 2, "Infiniband 1X LX"),
    (3, 1, "Infiniband 1X Copper Active"),
    (3, 0, "Infiniband 1X Copper Passive"),
    (4, 7, "ESCON MMF 1310nm LED"),
    (4, 6, "ESCON SMF 1310nm laser"),
    (4, 5, "SONET OC-192 short reach"),
    (4, 4, "SONET reach specifier bit 1"),
    (4, 3, "SONET reach specifier bit 2"),
    (4, 2, "SONET OC-48 long reach"),
    (4, 1, "SONET OC-48 intermediate reach"),
    (4, 0, "SONET OC-48 short reach"),
    (5, 6, "SONET OC-12 SM long reach"),
    (5, 5, "SONET OC-12 SM intermediate reach"),
    (5, 4, "SONET OC-12 short reach"),
    (5, 2, "SONET OC-3 SM long reach"),
    (5, 1, "SONET OC-3 SM intermediate reach"),
    (5, 0, "SONET OC-3 short reach"),
    (6, 7, "BASE-PX"),
    (6, 6, "BASE-BX10"),
    (6, 5, "100BASE-FX"),
    (6, 4, "100BASE-LX/LX10"),
    (6, 3, "1000BASE-T"),
    (6, 2, "1000BASE-CX"),
    (6, 1, "1000BASE-LX"),
    (6, 0, "1000BASE-SX"),
    (7, 7, "FC very long distance (V)"),
    (7, 6, "FC short distance (S)"),
    (7, 5, "FC intermediate distance (I)"),
    (7, 4, "FC long distance (L)"),
    (7, 3, "FC medium distance (M)"),
    (7, 2, "FC shortwave laser, linear Rx (SA)"),
    (7, 1, "FC longwave laser (LC)"),
    (7, 0, "FC electrical inter-enclosure (EL)"),
    (8, 7, "FC electrical intra-enclosure (EL)"),
    (8, 6, "FC shortwave laser w/o OFC (SN)"),
    (8, 5, "FC shortwave laser with OFC (SL)"),
    (8, 4, "FC longwave laser (LL)"),
    (8, 3, "SFP+ Active Cable"),
    (8, 2, "SFP+ Passive Cable"),
    (9, 7, "FC media Twin Axial Pair (TW)"),
    (9, 6, "FC media Twisted Pair (TP)"),
    (9, 5, "FC media Miniature Coax (MI)"),
    (9, 4, "FC media Video Coax (TV)"),
    (9, 3, "FC media Multimode 62.5um (M6)"),
    (9, 2, "FC media Multimode 50um (M5/M5E)"),
    (9, 0, "FC media Single Mode (SM)"),
    (10, 7, "FC speed 1200 MB/s"),
    (10, 6, "FC speed 800 MB/s"),
    (10, 5, "FC speed 1600 MB/s"),
    (10, 4, "FC speed 400 MB/s"),
    (10, 3, "FC speed 3200 MB/s"),
    (10, 2, "FC speed 200 MB/s"),
    (10, 1, "FC speed 2 (see byte 62)"),
    (10, 0, "FC speed 100 MB/s"),
    (62, 0, "64GFC"),
];

const OPTIONS: [(usize, u8, &str); 15] = [
    (64, 6, "High Power Level 4 declaration (max power in A2h byte 66)"),
    (64, 5, "High Power Level 3 or 4 declaration"),
    (64, 4, "Paging implemented (A2h byte 127 page select)"),
    (64, 3, "Internal retimer or CDR"),
    (64, 2, "Cooled laser transmitter"),
    (64, 1, "Power Level 2 requirement"),
    (64, 0, "Linear receiver output"),
    (65, 7, "Receiver decision threshold (RDT) implemented"),
    (65, 6, "Tunable transmitter (SFF-8690)"),
    (65, 5, "RATE_SELECT implemented"),
    (65, 4, "TX_DISABLE implemented"),
    (65, 3, "TX_FAULT implemented"),
    (65, 2, "Loss of Signal implemented, INVERTED (non-standard)"),
    (65, 1, "Loss of Signal implemented (RX_LOS)"),
    (65, 0, "reserved bit set"),
];

const ENHANCED: [(u8, &str); 7] = [
    (7, "Alarm/warning flags for all monitored quantities"),
    (6, "Soft TX_DISABLE control and monitoring"),
    (5, "Soft TX_FAULT monitoring"),
    (4, "Soft RX_LOS monitoring"),
    (3, "Soft RATE_SELECT control and monitoring"),
    (2, "Application Select control (SFF-8079)"),
    (1, "Soft Rate Select control (SFF-8431)"),
];

const DDM_TYPE: [(u8, &str); 5] = [
    (6, "Digital diagnostic monitoring implemented"),
    (5, "Internally calibrated"),
    (4, "Externally calibrated"),
    (3, "Received power is average power (clear = OMA)"),
    (2, "Address change sequence required for A2h"),
];

const STATUS_CTL: [(u8, &str); 8] = [
    (7, "TX_DISABLE pin state"),
    (6, "Soft TX_DISABLE select"),
    (5, "RS(1) pin state"),
    (4, "Rate_Select RS(0) pin state"),
    (3, "Soft Rate_Select RS(0)"),
    (2, "TX_FAULT state"),
    (1, "RX_LOS state"),
    (0, "Data_Not_Ready"),
];

const FLAG_BITS_LOW: [(u8, &str); 8] = [
    (7, "Temp high"),
    (6, "Temp low"),
    (5, "Vcc high"),
    (4, "Vcc low"),
    (3, "TX bias high"),
    (2, "TX bias low"),
    (1, "TX power high"),
    (0, "TX power low"),
];

const FLAG_BITS_HIGH: [(u8, &str); 6] = [
    (7, "RX power high"),
    (6, "RX power low"),
    (5, "Laser temp high"),
    (4, "Laser temp low"),
    (3, "TEC current high"),
    (2, "TEC current low"),
];

pub struct Eeprom {
    pub a0: Vec<u8>,
    pub a2: Option<Vec<u8>>,
    pub source: String,
}

fn ascii(b: &[u8]) -> String {
    b.iter()
        .map(|&c| if (0x20..0x7f).contains(&c) { c as char } else { '.' })
        .collect::<String>()
        .trim_end()
        .to_string()
}

fn u16be(b: &[u8], off: usize) -> u16 {
    ((b[off] as u16) << 8) | b[off + 1] as u16
}

fn s16be(b: &[u8], off: usize) -> i16 {
    u16be(b, off) as i16
}

fn bit(v: u8, n: u8) -> bool {
    v & (1 << n) != 0
}

fn lookup(table: &[(u8, &'static str)], v: u8) -> String {
    table
        .iter()
        .find(|(k, _)| *k == v)
        .map(|(_, s)| (*s).to_string())
        .unwrap_or_else(|| "unknown/vendor specific".to_string())
}

fn mw_to_dbm(mw: f64) -> String {
    if mw <= 0.0 {
        "-inf".to_string()
    } else {
        format!("{:.2}", 10.0 * mw.log10())
    }
}

fn checksum(b: &[u8], from: usize, to: usize) -> u8 {
    b[from..=to].iter().fold(0u8, |a, &x| a.wrapping_add(x))
}

pub fn hexdump(label: &str, b: &[u8]) -> String {
    let mut o = format!("{label}\n");
    for (i, chunk) in b.chunks(16).enumerate() {
        let hex: Vec<String> = chunk.iter().map(|c| format!("{c:02x}")).collect();
        o.push_str(&format!(
            "  {:3}: {:<48}  |{}|\n",
            i * 16,
            hex.join(" "),
            ascii(chunk)
        ));
    }
    o
}

fn identity(b: &[u8], o: &mut String) {
    o.push_str("== A0h base ID ==\n");
    o.push_str(&format!(
        "  identifier            0x{:02x}  {}\n",
        b[0],
        match b[0] {
            0x00 => "unknown/unspecified",
            0x01 => "GBIC",
            0x02 => "module soldered to motherboard",
            0x03 => "SFP or SFP+",
            0x80..=0xff => "vendor specific",
            _ => "see SFF-8024",
        }
    ));
    o.push_str(&format!(
        "  ext identifier        0x{:02x}  {}\n",
        b[1],
        if b[1] == 0x04 {
            "function defined by 2-wire interface ID only"
        } else {
            "see Table 5-2"
        }
    ));
    o.push_str(&format!(
        "  connector             0x{:02x}  {}\n",
        b[2],
        lookup(&CONNECTORS, b[2])
    ));
    o.push_str(&format!(
        "  encoding              0x{:02x}  {}\n",
        b[11],
        lookup(&ENCODINGS, b[11])
    ));
    if b[12] == 0xff {
        o.push_str(&format!(
            "  signalling rate       >25.4 GBd; nominal {} MBd (byte 66 x 250)\n",
            b[66] as u32 * 250
        ));
    } else {
        o.push_str(&format!(
            "  signalling rate       {} MBd nominal (+{}% / -{}%)\n",
            b[12] as u32 * 100,
            b[66],
            b[67]
        ));
    }
    o.push_str(&format!(
        "  rate identifier       0x{:02x}  {}\n",
        b[13],
        match b[13] {
            0x00 => "unspecified",
            0x01 => "SFF-8079 (4/2/1G Rate_Select & AS0/AS1)",
            0x02 => "SFF-8431 (8/4/2G Rx Rate_Select only)",
            0x04 => "SFF-8431 (8/4/2G Tx Rate_Select only)",
            0x06 => "SFF-8431 (8/4/2G independent Rx & Tx Rate_Select)",
            0x08 => "FC-PI-5 (16/8/4G Rx Rate_Select only)",
            0x0a => "FC-PI-5 (16/8/4G independent Rx, Tx)",
            0x0c => "FC-PI-6 (32/16/8G independent Rx, Tx)",
            0x0e => "10/8G Rx and Tx Rate_Select controlling internal retimer/CDR",
            0x10 => "FC-PI-7 (64/32/16G independent Rx, Tx)",
            0x20 => "Rate select based on PMDs (A0h byte 36, A2h byte 67)",
            _ => "unspecified/reserved",
        }
    ));

    let copper = bit(b[8], 2) || bit(b[8], 3);
    if copper {
        o.push_str(&format!(
            "  cable attenuation     {} dB @12.9 GHz, {} dB @25.78 GHz (bytes 14,15)\n",
            b[14], b[15]
        ));
        o.push_str(&format!("  cable length          {} m (byte 18)\n", b[18]));
        o.push_str(&format!(
            "  cable spec compliance bytes 60,61 = 0x{:02x}{:02x}\n",
            b[60], b[61]
        ));
    } else {
        o.push_str(&format!(
            "  lengths               SMF {} km / {} m, OM2 {} m, OM1 {} m, OM4 {} m, OM3 {} m\n",
            b[14],
            b[15] as u32 * 100,
            b[16] as u32 * 10,
            b[17] as u32 * 10,
            b[18] as u32 * 10,
            b[19] as u32 * 10
        ));
        o.push_str(&format!("  wavelength            {} nm\n", u16be(b, 60)));
    }

    o.push_str(&format!("  vendor                {:?}\n", ascii(&b[20..36])));
    o.push_str(&format!(
        "  vendor OUI            {:02x}:{:02x}:{:02x}\n",
        b[37], b[38], b[39]
    ));
    o.push_str(&format!("  vendor PN             {:?}\n", ascii(&b[40..56])));
    o.push_str(&format!("  vendor rev            {:?}\n", ascii(&b[56..60])));
    o.push_str(&format!("  vendor SN             {:?}\n", ascii(&b[68..84])));
    let d = ascii(&b[84..92]);
    o.push_str(&format!(
        "  date code             {d:?}  (20{}-{}-{} lot {:?})\n",
        d.get(0..2).unwrap_or("??"),
        d.get(2..4).unwrap_or("??"),
        d.get(4..6).unwrap_or("??"),
        d.get(6..8).unwrap_or("")
    ));
    o.push_str(&format!(
        "  SFF-8472 compliance   0x{:02x}  {}\n",
        b[94],
        match b[94] {
            0x00 => "not specified",
            0x01 => "rev 9.3",
            0x02 => "rev 9.5",
            0x03 => "rev 10.2",
            0x04 => "rev 10.4",
            0x05 => "rev 11.0",
            0x06 => "rev 11.3",
            0x07 => "rev 11.4",
            0x08 => "rev 12.3",
            0x09 => "rev 12.4",
            _ => "reserved",
        }
    ));
    o.push_str(&format!(
        "  ext spec compliance   0x{:02x}  (SFF-8024 Table 4-4)\n",
        b[36]
    ));
}

fn bitlists(b: &[u8], o: &mut String) {
    o.push_str("\n-- transceiver codes (bytes 3-10, 36, 62) --\n");
    let mut any = false;
    for (byte, n, name) in XCVR_CODES {
        if byte < b.len() && bit(b[byte], n) {
            any = true;
            o.push_str(&format!("  byte {byte:>2} bit {n}   {name}\n"));
        }
    }
    if !any {
        o.push_str("  none set - SFF-8472 requires at least one\n");
    }

    o.push_str("\n-- options (bytes 64-65, Table 8-3) --\n");
    let mut any = false;
    for (byte, n, name) in OPTIONS {
        if bit(b[byte], n) {
            any = true;
            o.push_str(&format!("  byte {byte} bit {n}    {name}\n"));
        }
    }
    if !any {
        o.push_str("  none\n");
    }

    o.push_str(&format!(
        "\n-- diagnostic monitoring type (byte 92 = 0x{:02x}, Table 8-5) --\n",
        b[92]
    ));
    for (n, name) in DDM_TYPE {
        if bit(b[92], n) {
            o.push_str(&format!("  bit {n}   {name}\n"));
        }
    }
    if bit(b[92], 7) {
        o.push_str("  bit 7   SET - legacy diagnostics, must be 0 for SFF-8472 compliance\n");
    }

    o.push_str(&format!(
        "\n-- enhanced options (byte 93 = 0x{:02x}, Table 8-6) --\n",
        b[93]
    ));
    for (n, name) in ENHANCED {
        if bit(b[93], n) {
            o.push_str(&format!("  bit {n}   {name}\n"));
        }
    }
}

fn checksums_a0(b: &[u8], o: &mut String) -> bool {
    let cc_base = checksum(b, 0, 62);
    let cc_ext = checksum(b, 64, 94);
    let base_ok = cc_base == b[63];
    let ext_ok = cc_ext == b[95];
    o.push_str("\n-- checksums --\n");
    o.push_str(&format!(
        "  CC_BASE (bytes 0-62)   stored 0x{:02x}  computed 0x{cc_base:02x}  {}\n",
        b[63],
        if base_ok { "OK" } else { "*** MISMATCH ***" }
    ));
    o.push_str(&format!(
        "  CC_EXT  (bytes 64-94)  stored 0x{:02x}  computed 0x{cc_ext:02x}  {}\n",
        b[95],
        if ext_ok { "OK" } else { "*** MISMATCH ***" }
    ));
    base_ok && ext_ok
}

struct Cal {
    external: bool,
    rx_pwr: [f64; 5],
    tx_i: (f64, f64),
    tx_pwr: (f64, f64),
    t: (f64, f64),
    v: (f64, f64),
}

fn slope(b: &[u8], off: usize) -> f64 {
    b[off] as f64 + b[off + 1] as f64 / 256.0
}

fn f32be(b: &[u8], off: usize) -> f64 {
    f32::from_bits(
        ((b[off] as u32) << 24) | ((b[off + 1] as u32) << 16) | ((b[off + 2] as u32) << 8)
            | b[off + 3] as u32,
    ) as f64
}

fn calibration(a0: &[u8], a2: &[u8]) -> Cal {
    let external = bit(a0[92], 4);
    if !external {
        return Cal {
            external,
            // Identity conversion: raw counts are already 0.1 uW units.
            rx_pwr: [0.0, 1.0, 0.0, 0.0, 0.0],
            tx_i: (1.0, 0.0),
            tx_pwr: (1.0, 0.0),
            t: (1.0, 0.0),
            v: (1.0, 0.0),
        };
    }
    Cal {
        external,
        // Rx_PWR(4)..Rx_PWR(0), bytes 56-75, single-precision floats.
        rx_pwr: [
            f32be(a2, 72),
            f32be(a2, 68),
            f32be(a2, 64),
            f32be(a2, 60),
            f32be(a2, 56),
        ],
        tx_i: (slope(a2, 76), s16be(a2, 78) as f64),
        tx_pwr: (slope(a2, 80), s16be(a2, 82) as f64),
        t: (slope(a2, 84), s16be(a2, 86) as f64),
        v: (slope(a2, 88), s16be(a2, 90) as f64),
    }
}

impl Cal {
    fn temp_c(&self, raw: i16) -> f64 {
        (self.t.0 * raw as f64 + self.t.1) / 256.0
    }
    fn vcc_v(&self, raw: u16) -> f64 {
        (self.v.0 * raw as f64 + self.v.1) / 10_000.0
    }
    fn bias_ma(&self, raw: u16) -> f64 {
        (self.tx_i.0 * raw as f64 + self.tx_i.1) / 500.0
    }
    fn txpwr_mw(&self, raw: u16) -> f64 {
        (self.tx_pwr.0 * raw as f64 + self.tx_pwr.1) / 10_000.0
    }
    fn rxpwr_mw(&self, raw: u16) -> f64 {
        let x = raw as f64;
        // rx_pwr[0] is the 0th-order term through rx_pwr[4], the 4th.
        let uw = self.rx_pwr[0]
            + self.rx_pwr[1] * x
            + self.rx_pwr[2] * x * x
            + self.rx_pwr[3] * x * x * x
            + self.rx_pwr[4] * x * x * x * x;
        uw / 10_000.0
    }
}

fn thresholds(a2: &[u8], cal: &Cal, o: &mut String) {
    o.push_str("\n-- alarm / warning thresholds (A2h bytes 0-39, Table 9-5) --\n");
    o.push_str(&format!(
        "  {:<12} {:>12} {:>12} {:>12} {:>12}\n",
        "quantity", "low alarm", "low warn", "high warn", "high alarm"
    ));
    let row = |o: &mut String, name: &str, base: usize, f: &dyn Fn(&[u8], usize) -> String| {
        o.push_str(&format!(
            "  {:<12} {:>12} {:>12} {:>12} {:>12}\n",
            name,
            f(a2, base + 2),
            f(a2, base + 6),
            f(a2, base + 4),
            f(a2, base)
        ));
    };
    let temp = |b: &[u8], off: usize| format!("{:.2} C", cal.temp_c(s16be(b, off)));
    let volt = |b: &[u8], off: usize| format!("{:.3} V", cal.vcc_v(u16be(b, off)));
    let bias = |b: &[u8], off: usize| format!("{:.2} mA", cal.bias_ma(u16be(b, off)));
    let txp = |b: &[u8], off: usize| {
        format!("{} dBm", mw_to_dbm(cal.txpwr_mw(u16be(b, off))))
    };
    let rxp = |b: &[u8], off: usize| {
        format!("{} dBm", mw_to_dbm(cal.rxpwr_mw(u16be(b, off))))
    };
    row(o, "temperature", 0, &temp);
    row(o, "Vcc", 8, &volt);
    row(o, "TX bias", 16, &bias);
    row(o, "TX power", 24, &txp);
    row(o, "RX power", 32, &rxp);
    let laser: u32 = a2[40..56].iter().map(|&x| x as u32).sum();
    o.push_str(&format!(
        "  optional laser-temp / TEC thresholds (bytes 40-55): {}\n",
        if laser == 0 { "all zero (not implemented)" } else { "present, see --hex" }
    ));
}

fn realtime(a2: &[u8], cal: &Cal, o: &mut String) {
    o.push_str("\n-- real-time diagnostics (A2h bytes 96-109) --\n");
    let inside = |v: f64, lo: f64, hi: f64| {
        if v < lo || v > hi {
            "  OUTSIDE ALARM RANGE"
        } else {
            ""
        }
    };
    let t = cal.temp_c(s16be(a2, 96));
    o.push_str(&format!(
        "  temperature   {t:>10.2} C{}\n",
        inside(
            t,
            cal.temp_c(s16be(a2, 2)),
            cal.temp_c(s16be(a2, 0))
        )
    ));
    let v = cal.vcc_v(u16be(a2, 98));
    o.push_str(&format!(
        "  Vcc           {v:>10.3} V{}\n",
        inside(v, cal.vcc_v(u16be(a2, 10)), cal.vcc_v(u16be(a2, 8)))
    ));
    let i = cal.bias_ma(u16be(a2, 100));
    o.push_str(&format!(
        "  TX bias       {i:>10.2} mA{}\n",
        inside(i, cal.bias_ma(u16be(a2, 18)), cal.bias_ma(u16be(a2, 16)))
    ));
    let tp = cal.txpwr_mw(u16be(a2, 102));
    o.push_str(&format!(
        "  TX power      {:>10} dBm ({tp:.4} mW){}\n",
        mw_to_dbm(tp),
        inside(
            tp,
            cal.txpwr_mw(u16be(a2, 26)),
            cal.txpwr_mw(u16be(a2, 24))
        )
    ));
    let rp = cal.rxpwr_mw(u16be(a2, 104));
    o.push_str(&format!(
        "  RX power      {:>10} dBm ({rp:.4} mW){}\n",
        mw_to_dbm(rp),
        inside(
            rp,
            cal.rxpwr_mw(u16be(a2, 34)),
            cal.rxpwr_mw(u16be(a2, 32))
        )
    ));
    if u16be(a2, 106) != 0 || u16be(a2, 108) != 0 {
        o.push_str(&format!(
            "  laser temp/wavelength raw 0x{:04x}, TEC current raw 0x{:04x}\n",
            u16be(a2, 106),
            u16be(a2, 108)
        ));
    }
}

fn status_and_control(a2: &[u8], o: &mut String) {
    o.push_str(&format!(
        "\n-- status / control (A2h byte 110 = 0x{:02x}, Table 9-16) --\n",
        a2[110]
    ));
    for (n, name) in STATUS_CTL {
        o.push_str(&format!(
            "  bit {n}  {:<32} {}\n",
            name,
            u8::from(bit(a2[110], n))
        ));
    }

    let flags = |o: &mut String, label: &str, lo: u8, hi: u8| {
        let mut set: Vec<&str> = Vec::new();
        for (n, name) in FLAG_BITS_LOW {
            if bit(lo, n) {
                set.push(name);
            }
        }
        for (n, name) in FLAG_BITS_HIGH {
            if bit(hi, n) {
                set.push(name);
            }
        }
        o.push_str(&format!(
            "  {label} 0x{lo:02x}{hi:02x}  {}\n",
            if set.is_empty() {
                "(clear)".to_string()
            } else {
                set.join(", ")
            }
        ));
    };
    o.push_str("\n-- alarm / warning flags (A2h bytes 112-113, 116-117) --\n");
    flags(o, "alarms  ", a2[112], a2[113]);
    flags(o, "warnings", a2[116], a2[117]);

    let eq_db = |code: u8| match code {
        0 => "no EQ".to_string(),
        1..=10 => format!("{code} dB"),
        _ => "reserved".to_string(),
    };
    o.push_str(&format!(
        "\n-- signal-integrity control (A2h bytes 114-115) --\n\
         \x20 byte 114 Tx input EQ      0x{:02x}  RATE=HIGH {}, RATE=LOW {}\n\
         \x20 byte 115 Rx out emphasis  0x{:02x}  RATE=HIGH {}, RATE=LOW {}\n",
        a2[114],
        eq_db(a2[114] >> 4),
        eq_db(a2[114] & 0xf),
        a2[115],
        eq_db(a2[115] >> 4),
        eq_db(a2[115] & 0xf),
    ));

    o.push_str(&format!(
        "\n-- extended status / control (A2h bytes 118-119, Table 10-1) --\n\
         \x20 byte 118 = 0x{:02x}   Adaptive Input EQ fail {}, soft RS(1) {}, \
         power level 4 enable {}, power level state {}, power level select {}\n\
         \x20 byte 119 = 0x{:02x}   PAM4 Tx cfg {}, PAM4 Rx cfg {}, 64GFC mode {}, \
         Tx CDR unlocked {}, Rx CDR unlocked {}\n",
        a2[118],
        u8::from(bit(a2[118], 4)),
        u8::from(bit(a2[118], 3)),
        u8::from(bit(a2[118], 2)),
        u8::from(bit(a2[118], 1)),
        u8::from(bit(a2[118], 0)),
        a2[119],
        u8::from(bit(a2[119], 4)),
        u8::from(bit(a2[119], 3)),
        u8::from(bit(a2[119], 2)),
        u8::from(bit(a2[119], 1)),
        u8::from(bit(a2[119], 0)),
    ));

    let vendor = &a2[120..127];
    o.push_str(&format!(
        "\n-- vendor area / password (A2h bytes 120-126, section 10.2) --\n\
         \x20 bytes {:02x?} {:?}\n\
         \x20 (this is where vendors put a password field for protected functions)\n\
         \x20 byte 127 page select 0x{:02x}{}\n",
        vendor,
        ascii(vendor),
        a2[127],
        if a2[127] == 0 { " (user EEPROM)" } else { "" }
    ));
}

fn enhanced_features(a2: &[u8], o: &mut String) {
    o.push_str(
        "\n-- additional enhanced features (A2h bytes 56-91, Table 9-11)\n\
         \x20  [selected because A0h byte 92 bit 4 'externally calibrated' is clear] --\n",
    );
    o.push_str(&format!(
        "  byte 56 = 0x{:02x}  Tx squelch impl {:02b}, force-squelch {}, squelch-disable {}, \
         RS0/1 ignore {}\n",
        a2[56],
        (a2[56] >> 2) & 0x3,
        u8::from(bit(a2[56], 1)),
        u8::from(bit(a2[56], 0)),
        u8::from(bit(a2[56], 4))
    ));
    o.push_str(&format!(
        "  byte 57 = 0x{:02x}  Rx force-squelch {}, Rx squelch-disable {}\n",
        a2[57],
        u8::from(bit(a2[57], 1)),
        u8::from(bit(a2[57], 0))
    ));
    o.push_str(&format!(
        "  byte 58 = 0x{:02x}  Tx Adaptive Input EQ fail flag implemented {}\n",
        a2[58],
        u8::from(bit(a2[58], 0))
    ));
    o.push_str(&format!(
        "  byte 60 = 0x{:02x}  Tx Input EQ: manual control {}, adaptive {}, freeze {}, \
         store/recall {:02b}\n",
        a2[60],
        u8::from(bit(a2[60], 0)),
        u8::from(bit(a2[60], 1)),
        u8::from(bit(a2[60], 2)),
        (a2[60] >> 3) & 0x3
    ));
    if !bit(a2[60], 0) && !bit(a2[60], 1) {
        o.push_str(
            "           -> module implements NO Tx input EQ control; byte 114 is inert.\n",
        );
    }
    o.push_str(&format!(
        "  byte 61 = 0x{:02x}  max adaptive Tx input EQ settling time {} ms\n",
        a2[61],
        a2[61] as u32 * 100
    ));
    o.push_str(&format!(
        "  byte 62 = 0x{:02x}  Rx output EQ type {:02b}, enhanced output EQ ctl {:02b}, \
         output amplitude ctl {}\n",
        a2[62],
        (a2[62] >> 3) & 0x3,
        (a2[62] >> 1) & 0x3,
        u8::from(bit(a2[62], 0))
    ));
    o.push_str(&format!(
        "  byte 63 = 0x{:02x}  max Tx input EQ {} dB, Rx output amplitude codes 0x{:x}\n",
        a2[63],
        a2[63] & 0xf,
        (a2[63] >> 4) & 0xf
    ));
    o.push_str(&format!(
        "  byte 64 = 0x{:02x}  max Rx output EQ post-cursor {}, pre-cursor {}\n",
        a2[64],
        (a2[64] >> 4) & 0xf,
        a2[64] & 0xf
    ));
    o.push_str(&format!(
        "  byte 66 = 0x{:02x}  max power consumption {:.1} W\n",
        a2[66],
        a2[66] as f64 * 0.1
    ));
    o.push_str(&format!(
        "  byte 67 = 0x{:02x}  secondary extended spec compliance (SFF-8024 Table 4-4)\n",
        a2[67]
    ));
    o.push_str(&format!(
        "  bytes 71-74 enhanced control = {:02x?}\n",
        &a2[71..75]
    ));
}

fn calibration_constants(a2: &[u8], cal: &Cal, o: &mut String) {
    o.push_str(
        "\n-- external calibration constants (A2h bytes 56-91, Table 9-6)\n\
         \x20  [selected because A0h byte 92 bit 4 'externally calibrated' is set] --\n",
    );
    o.push_str(&format!(
        "  Rx_PWR(0..4)  {:?}\n",
        cal.rx_pwr.iter().map(|v| format!("{v:.6e}")).collect::<Vec<_>>()
    ));
    o.push_str(&format!(
        "  Tx_I   slope {:.4} offset {:.0}\n  Tx_PWR slope {:.4} offset {:.0}\n",
        cal.tx_i.0, cal.tx_i.1, cal.tx_pwr.0, cal.tx_pwr.1
    ));
    o.push_str(&format!(
        "  T      slope {:.4} offset {:.0}\n  V      slope {:.4} offset {:.0}\n",
        cal.t.0, cal.t.1, cal.v.0, cal.v.1
    ));
    o.push_str(&format!("  raw bytes 56-91 {:02x?}\n", &a2[56..92]));
}

/// Full decode. Returns (report, all_checksums_ok).
pub fn decode(e: &Eeprom) -> (String, bool) {
    let mut o = String::new();
    let b = &e.a0;
    o.push_str(&format!("source: {}\n\n", e.source));
    identity(b, &mut o);
    bitlists(b, &mut o);
    let mut ok = checksums_a0(b, &mut o);

    let Some(a2) = &e.a2 else {
        o.push_str(
            "\n== A2h ==\n  NOT READ. Either the module has no diagnostics (A0h byte 92\n\
             \x20 bit 6 clear) or only 256 bytes were available. DDM, thresholds,\n\
             \x20 status/control and CC_DMI are all unavailable.\n",
        );
        return (o, ok);
    };
    if !bit(b[92], 6) {
        o.push_str(
            "\n== A2h ==\n  A0h byte 92 bit 6 is clear: the module does not claim digital\n\
             \x20 diagnostics. The bytes below are decoded anyway but may be meaningless.\n",
        );
    } else {
        o.push_str("\n== A2h diagnostics ==\n");
    }

    let cal = calibration(b, a2);
    o.push_str(&format!(
        "  calibration: {}\n",
        if cal.external {
            "EXTERNAL - raw A/D counts converted with bytes 56-91"
        } else {
            "INTERNAL - values are already in physical units"
        }
    ));
    thresholds(a2, &cal, &mut o);
    if cal.external {
        calibration_constants(a2, &cal, &mut o);
    } else {
        enhanced_features(a2, &mut o);
    }
    realtime(a2, &cal, &mut o);
    status_and_control(a2, &mut o);

    let cc_dmi = checksum(a2, 0, 94);
    let dmi_ok = cc_dmi == a2[95];
    ok = ok && dmi_ok;
    o.push_str(&format!(
        "\n-- checksum --\n  CC_DMI (A2h bytes 0-94)  stored 0x{:02x}  computed 0x{cc_dmi:02x}  {}\n",
        a2[95],
        if dmi_ok { "OK" } else { "*** MISMATCH ***" }
    ));

    let user: u32 = a2[128..248].iter().map(|&x| x as u32).sum();
    o.push_str(&format!(
        "\n  user EEPROM (A2h 128-247): {}\n  vendor control (A2h 248-255): {:02x?}\n",
        if user == 0 {
            "all zero".to_string()
        } else {
            format!("non-zero, {:?}", ascii(&a2[128..248]))
        },
        &a2[248..256]
    ));
    (o, ok)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn put(b: &mut [u8], at: usize, s: &str) {
        for (i, c) in s.bytes().enumerate() {
            b[at + i] = c;
        }
    }

    /// A plausible 10GBASE-SR module: internally calibrated, LC, 850 nm.
    fn synth() -> Eeprom {
        let mut a0 = vec![0u8; 256];
        a0[0] = 0x03; // SFP or SFP+
        a0[1] = 0x04;
        a0[2] = 0x07; // LC
        a0[3] = 0x10; // 10GBASE-SR
        a0[11] = 0x06; // 64B/66B
        a0[12] = 103; // 10300 MBd
        a0[16] = 80; // OM2 800 m
        a0[19] = 30; // OM3 300 m
        put(&mut a0, 20, "AWTO TEST       ");
        put(&mut a0, 40, "SFP-10GSR-85    ");
        put(&mut a0, 56, "A   ");
        a0[60] = 0x03;
        a0[61] = 0x52; // 850 nm
        a0[63] = a0[0..63].iter().fold(0u8, |a, &x| a.wrapping_add(x));
        a0[65] = 0x1a; // TX_DISABLE, TX_FAULT, RX_LOS
        put(&mut a0, 68, "SN0001          ");
        put(&mut a0, 84, "260101AB");
        a0[92] = 0x68; // DDM, internally calibrated, avg power
        a0[93] = 0xf0;
        a0[94] = 0x08;
        a0[95] = a0[64..95].iter().fold(0u8, |a, &x| a.wrapping_add(x));

        let mut a2 = vec![0u8; 256];
        let put16 = |b: &mut [u8], at: usize, v: u16| {
            b[at] = (v >> 8) as u8;
            b[at + 1] = v as u8;
        };
        put16(&mut a2, 0, 75 * 256); // temp high alarm 75 C
        put16(&mut a2, 2, 0xfb00); // temp low alarm -5 C
        put16(&mut a2, 8, 36300); // Vcc high alarm 3.63 V
        put16(&mut a2, 10, 29700); // Vcc low alarm 2.97 V
        put16(&mut a2, 16, 5000); // bias high alarm 10 mA
        put16(&mut a2, 24, 10000); // TX power high alarm 1 mW
        put16(&mut a2, 32, 10000); // RX power high alarm 1 mW
        put16(&mut a2, 96, 35 * 256); // 35 C
        put16(&mut a2, 98, 33000); // 3.3 V
        put16(&mut a2, 100, 3000); // 6 mA
        put16(&mut a2, 102, 5670); // 0.567 mW
        put16(&mut a2, 104, 5610);
        a2[95] = a2[0..95].iter().fold(0u8, |a, &x| a.wrapping_add(x));
        Eeprom {
            a0,
            a2: Some(a2),
            source: "synthetic".into(),
        }
    }

    #[test]
    fn decodes_a_plausible_sr_module() {
        let (rep, ok) = decode(&synth());
        assert!(ok, "checksums should verify:\n{rep}");
        assert!(rep.contains("10GBASE-SR"), "{rep}");
        assert!(rep.contains("LC (Lucent Connector)"), "{rep}");
        assert!(rep.contains("64B/66B"), "{rep}");
        assert!(rep.contains("10300 MBd"), "{rep}");
        assert!(rep.contains("850 nm"), "{rep}");
        assert!(rep.contains("\"AWTO TEST\""), "{rep}");
        assert!(rep.contains("35.00 C"), "{rep}");
        assert!(rep.contains("3.300 V"), "{rep}");
        assert!(rep.contains("6.00 mA"), "{rep}");
        assert!(rep.contains("-2.46 dBm (0.5670 mW)"), "{rep}");
        assert!(rep.contains("INTERNAL"), "{rep}");
        assert!(!rep.contains("MISMATCH"), "{rep}");
    }

    #[test]
    fn flags_a_corrupt_checksum() {
        let mut e = synth();
        e.a0[63] ^= 0xff;
        let (rep, ok) = decode(&e);
        assert!(!ok);
        assert!(rep.contains("*** MISMATCH ***"), "{rep}");
    }

    #[test]
    fn missing_a2_is_reported_not_invented() {
        let mut e = synth();
        e.a2 = None;
        let (rep, _) = decode(&e);
        assert!(rep.contains("NOT READ"), "{rep}");
        assert!(!rep.contains("real-time diagnostics"), "{rep}");
    }

    #[test]
    fn tx_input_eq_capability_is_decoded() {
        let mut e = synth();
        // Byte 60 bit 0 = manual Tx input EQ control, byte 63 low nibble = max dB.
        let a2 = e.a2.as_mut().unwrap();
        a2[60] = 0x01;
        a2[63] = 0x0a;
        a2[114] = 0x53; // RATE=HIGH 5 dB, RATE=LOW 3 dB
        a2[95] = a2[0..95].iter().fold(0u8, |a, &x| a.wrapping_add(x));
        let (rep, ok) = decode(&e);
        assert!(ok, "{rep}");
        assert!(rep.contains("max Tx input EQ 10 dB"), "{rep}");
        assert!(rep.contains("RATE=HIGH 5 dB, RATE=LOW 3 dB"), "{rep}");
    }
}
