#!/usr/bin/env python3
"""Read the Alpine V2 SoC die temperature directly via /dev/mem (runs on woomera).

Interim CPU-temp readout until al_thermal is built into the kernel (#44). The
on-die thermal sense unit @ 0xfd860a00 has a status register (unit.status @ +0xc)
carrying a 12-bit ADC readout; convert with the vendor formula (offsets from
al_hal_thermal_sensor.c, Alpine V2). Root only.
"""
import mmap, struct, sys

BASE = 0xfd860000            # mmap page
STATUS_OFF = 0x0a0c          # unit.status = 0xfd860a00 + 0x0c
OFFSET_V2, MULT_V2 = 1090, 3520
T_RESULT_MASK = 0xfff
T_VALID, T_INIT_DONE, T_PWR_OK = 1 << 15, 1 << 29, 1 << 30


def main() -> int:
    try:
        with open("/dev/mem", "rb", 0) as f:
            m = mmap.mmap(f.fileno(), 4096, offset=BASE, prot=mmap.PROT_READ)
            s = struct.unpack("<I", m[STATUS_OFF:STATUS_OFF + 4])[0]
    except (PermissionError, OSError) as e:
        print(f"/dev/mem read failed ({e}) — kernel likely has STRICT_DEVMEM; "
              f"build al_thermal instead (#44)", file=sys.stderr)
        return 2
    raw = s & T_RESULT_MASK
    celsius = ((raw * MULT_V2) // 4096 - OFFSET_V2) // 10
    print(f"status=0x{s:08x} pwr_ok={bool(s & T_PWR_OK)} init_done={bool(s & T_INIT_DONE)} "
          f"valid={bool(s & T_VALID)} raw={raw}  ->  SoC die {celsius} C")
    return 0


if __name__ == "__main__":
    sys.exit(main())
