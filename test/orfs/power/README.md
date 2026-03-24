# Power Reporting: sta standalone vs openroad

This test demonstrates how to write a Tcl script (`power.tcl`) that reports
power and works with both standalone OpenSTA (`sta`) and OpenROAD (`openroad`).

## The Problem

OpenROAD requires technology LEF files to be loaded before `link_design`.
Without them, it fails with:

```
[ERROR ORD-2010] no technology has been read.
```

Standalone STA does not need LEF files for power analysis -- it only needs
liberty (`.lib`) files, a gate-level netlist, SDC constraints, and SPEF
parasitics.

## The Solution

The `power.tcl` script checks for `TECH_LEF` to detect whether LEF files
should be loaded:

```tcl
if { [info exists ::env(TECH_LEF)] } {
  read_lef $::env(TECH_LEF)
  foreach lef $::env(SC_LEF) {
    read_lef $lef
  }
}
```

The `power_report` rule in `power.bzl` invokes `openroad` or `sta` directly
(no `orfs_run` dependency). It extracts liberty and LEF files from `PdkInfo`
and only sets `TECH_LEF`/`SC_LEF` when running OpenROAD (`sta = False`).

This pattern is also used in the mock-array tests (see
`test/orfs/mock-array/power.tcl`) but with the `orfs_run` macro.

## Running the Tests

```bash
# Build both targets
bazelisk build //test/orfs/power:gcd_power_sta //test/orfs/power:gcd_power_openroad

# Run verification tests
bazelisk test //test/orfs/power:gcd_power_sta_test //test/orfs/power:gcd_power_openroad_test
```

To reproduce the failure (openroad without LEF loading), remove the
`TECH_LEF` check from `power.tcl` and rebuild the openroad target.

## Test Structure

- `power.tcl` -- Power reporting script that works with both sta and openroad
- `power.bzl` -- Minimal `power_report` rule that invokes tools directly
- `BUILD` -- Targets: `gcd_power_sta` (OpenSTA) and `gcd_power_openroad` (OpenROAD)
- `ok.sh` -- Test verifier (checks output is "OK")

Both targets use the `gcd` design's final stage output (gate-level netlist,
SDC, SPEF) from `test/orfs/gcd/`.
