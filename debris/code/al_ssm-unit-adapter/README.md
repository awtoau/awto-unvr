# al_ssm: unit-adapter functions with no caller (retired, #325)

- `al_hal_unit_adapter-retired.c`: 25 functions cut from `modules/al_ssm/al_hal_unit_adapter.c` (AXI timeout/attr, interrupt cause, ROB/clock-gating/power-down control, SATA target-id, print helpers); 21 prototypes cut from `al_hal_unit_adapter.h`.
- Kept: `al_unit_adapter_init`, `_snoop_enable`, `_error_track_enable`, `al_pcie_perform_flr`, `_rob_cfg` (called from `al_hal_ssm.c`).
- U-Boot's copy of the header is a separate file and unchanged.
