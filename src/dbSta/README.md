# dbSta (OpenDB + OpenSTA integration)

dbSta wires OpenSTA's timing and power engines onto the OpenROAD/OpenDB
database. The timing commands (`report_checks`, `report_tns`, etc.) are
documented with OpenSTA. This page documents the **power sign-off** commands
that are available in the `openroad` shell through dbSta/OpenSTA.

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param param` are required.
```

## Power Analysis

OpenROAD reports a design's power consumption (internal + switching/dynamic +
leakage) using OpenSTA's power engine and the Liberty `internal_power`,
`leakage_power`, and capacitance models that are already loaded with
`read_liberty`. No extra input files are required for a first-pass estimate;
switching activity is taken from explicit assumptions (see
`set_power_activity`) or from simulation activity (`read_vcd` / `read_saif`).

### Report Power

The `report_power` command reports total and grouped power. With no instance
selection it prints a design summary table broken into the
`Sequential`, `Combinational`, `Clock`, `Macro`, and `Pad` groups, each split
into `Internal`, `Switching`, `Leakage`, and `Total` columns (Watts), followed
by a `Total` row.

```tcl
report_power
    [-instances instances]
    [-highest_power_instances count]
    [-corner corner]
    [-digits digits]
    [-format text|json]
    [> filename]
```

| Switch Name | Description |
| ----------- | ----------- |
| `-instances` | Report power for the listed instances (per-instance table) instead of the design summary. |
| `-highest_power_instances` | Report the `count` instances with the highest total power. |
| `-corner` | Analysis corner (defaults to the single/default corner). |
| `-digits` | Number of significant digits in the report (default from `sta_report_default_digits`). |
| `-format` | `text` (default) or `json`. |

Example design summary:

```tcl
read_liberty Nangate45_typ.lib
read_lef     Nangate45.lef
read_verilog design.v
link_design  top
create_clock -name clk -period 10 {clk}
set_power_activity -global -activity 0.1
set_power_activity -input  -activity 0.1
report_power
```

### Set Power Activity

The `set_power_activity` command sets the switching activity assumptions used
for dynamic (switching) power. Activity may be given as a toggle `-activity`
(toggles per clock period, requires a defined clock) or directly as a
`-density` (toggles per unit time).

```tcl
set_power_activity
    [-global]
    [-input]
    [-input_ports ports]
    [-pins pins]
    [-activity activity | -density density]
    [-duty duty]
    [-clock clock]
```

| Switch Name | Description |
| ----------- | ----------- |
| `-global` | Apply to all nets without a more specific annotation. |
| `-input` | Apply to primary input ports. |
| `-input_ports` | Apply to the listed input ports. |
| `-pins` | Apply to the listed pins. |
| `-activity` | Switching activity (toggles per clock period). Requires a clock. |
| `-density` | Toggle density (toggles per unit time). |
| `-duty` | Duty cycle (0.0 - 1.0, default 0.5). |
| `-clock` | Clock used to convert `-activity` to a density. |

Use `unset_power_activity` to clear annotations.

### Activity From Simulation (VCD/SAIF)

For accurate dynamic power, annotate real switching activity from simulation:

```tcl
read_vcd  [-scope scope] [-mode mode] filename
read_saif [-scope scope] filename
report_activity_annotation [-report_unannotated] [-report_annotated]
```

## Power Model

- **Switching (dynamic) power** per net is approximately
  `0.5 * activity * f * C * Vdd^2`, expressed internally as a toggle density
  (toggles/second). `set_power_activity -activity a` converts to a density via
  the clock period.
- **Internal power** comes from Liberty `internal_power` arcs (state- and
  slew/load-dependent), evaluated at the analysis corner.
- **Leakage power** comes from Liberty `leakage_power` tables
  (state-dependent).

Activity is resolved per pin/net in order of specificity: VCD/SAIF annotation,
then explicit `set_power_activity`, then clock-derived toggling, then a default
propagated activity.
