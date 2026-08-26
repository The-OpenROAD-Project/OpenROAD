# Check that DMP Ceff delay calculators propagate load delay and slew.
source "helpers.tcl"

proc wire_delay { from_pin to_pin } {
  set edges [get_timing_edges -from $from_pin -to $to_pin]
  if { [llength $edges] != 1 } {
    error "expected one timing edge from $from_pin to $to_pin"
  }
  return [get_property [lindex $edges 0] delay_max_rise]
}

proc report_model { label calculator } {
  puts "\n=== $label ==="
  set_delay_calculator $calculator
  estimate_parasitics -placement
  sta::find_requireds
  report_checks \
    -unconstrained \
    -from [get_ports in] \
    -to [get_ports out] \
    -format full_clock_expanded \
    -fields {capacitance slew fanout input_pin net} \
    -digits 6
}

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def dmp_ceff_dcalc_load_propagation.def

# Configure placement parasitics with nonzero resistance and capacitance.
set_input_transition 0 [get_ports in]
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3

set input_port [get_ports in]
set input_load [get_pins u1/A]
set internal_driver [get_pins u1/Z]
set internal_load [get_pins u2/A]

# Show physical propagation with the default delay calculator.
report_model "Default (dmp_ceff_elmore)" dmp_ceff_elmore
set elmore_input_slew [get_property $input_load slew_max_rise]
set elmore_input_delay [wire_delay $input_port $input_load]
set elmore_internal_delay [wire_delay $internal_driver $internal_load]

if {
  $elmore_input_slew <= 0.0
  || $elmore_input_delay <= 0.0
  || $elmore_internal_delay <= 0.0
} {
  error "Elmore control did not produce positive slew and net delays"
}

# Verify physical propagation with TwoPole.
set failures {}
report_model "TwoPole (dmp_ceff_two_pole)" dmp_ceff_two_pole
set two_pole_input_slew [get_property $input_load slew_max_rise]
set two_pole_input_delay [wire_delay $input_port $input_load]
set two_pole_driver_slew [get_property $internal_driver slew_max_rise]
set two_pole_load_slew [get_property $internal_load slew_max_rise]
set two_pole_internal_delay [wire_delay $internal_driver $internal_load]

if { $two_pole_input_slew <= 0.0 } {
  lappend failures "TwoPole: u1/A load slew is zero"
}
if { $two_pole_input_delay <= 0.0 } {
  lappend failures "TwoPole: in -> u1/A net delay is zero"
}
if { $two_pole_internal_delay <= 0.0 } {
  lappend failures "TwoPole: u1/Z -> u2/A net delay is zero"
}
if { $two_pole_driver_slew == $two_pole_load_slew } {
  lappend failures "TwoPole: u2/A load slew equals u1/Z driver slew"
}

# Verify physical propagation with Lambert-W.
report_model "Lambert-W (dmp_ceff_lambert_w)" dmp_ceff_lambert_w
set lambert_input_slew [get_property $input_load slew_max_rise]
set lambert_input_delay [wire_delay $input_port $input_load]
set lambert_driver_slew [get_property $internal_driver slew_max_rise]
set lambert_load_slew [get_property $internal_load slew_max_rise]
set lambert_internal_delay [wire_delay $internal_driver $internal_load]

if { $lambert_input_slew <= 0.0 } {
  lappend failures "Lambert-W: u1/A load slew is zero"
}
if { $lambert_input_delay <= 0.0 } {
  lappend failures "Lambert-W: in -> u1/A net delay is zero"
}
if { $lambert_internal_delay <= 0.0 } {
  lappend failures "Lambert-W: u1/Z -> u2/A net delay is zero"
}
if { $lambert_driver_slew == $lambert_load_slew } {
  lappend failures "Lambert-W: u2/A load slew equals u1/Z driver slew"
}

# Verify both models propagate input-port and internal-net RC effects.
if { [llength $failures] > 0 } {
  set failure_details [join $failures "\n  - "]
  error "DMP Ceff load propagation failed:\n  - $failure_details"
}

puts "pass"
