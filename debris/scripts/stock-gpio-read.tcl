# Read the spare-GPIO candidates (+ known reset btn 38) in STOCK U-Boot via
# `gpio input <pin>` (stock's al_gpio, global 0-47 numbering). One persistent
# socket keeps stock busy so its ~50 s prompt-watchdog doesn't autoboot.
# Baseline for the SW1/SW2 button-press test (docs/gpio-map.md).
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
foreach p {1 2 4 5 20 28 29 32 35 36 38 39 40 41 43} {
    send "gpio input $p\r"
    catch {puts "p$p: [expect "ALPINE_UBNT_NAS_ALL>" 4]"}
}
puts "=== STOCK-READ-DONE ==="
