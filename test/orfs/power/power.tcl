# When running in OpenROAD (not standalone STA), technology LEF files
# must be loaded before read_verilog/link_design. Without this,
# OpenROAD fails with: [ERROR ORD-2010] no technology has been read.
# Standalone STA does not require LEF files for power analysis.
if { [info exists ::env(TECH_LEF)] } {
  read_lef $::env(TECH_LEF)
  foreach lef $::env(SC_LEF) {
    read_lef $lef
  }
}

# Read liberty files
foreach libFile $::env(LIB_FILES) {
  read_liberty $libFile
}

# Read design
read_verilog $::env(POWER_VERILOG)
link_design gcd

# Read constraints and parasitics
read_sdc $::env(POWER_SDC)
read_spef $::env(POWER_SPEF)

# Report power
report_power

set f [open $::env(OUTPUT) w]
puts $f OK
close $f
