# incremental02 with -incremental_phase2: phase 1 leaves the overflow above
# the target, so all instances are unlocked and phase 2 brings it down.
source helpers.tcl
read_lef ./nangate45.lef
read_def ./incremental02.def

# -incremental_phase2 only applies to incremental placement.
if { ![catch { global_placement -incremental_phase2 } msg] } {
  error "Expected -incremental_phase2 without -incremental to be rejected"
}
if { ![string match {*GPL-0198*} $msg] } {
  error "Expected GPL-0198, got: $msg"
}

set_thread_count 4
set overflow 0.1
# with_output_to_variable runs its body in its own scope: read the global.
with_output_to_variable log_text {
  global_placement -incremental -incremental_phase2 -density 0.3 \
    -overflow $::overflow -pad_left 2 -pad_right 2
}
puts $log_text

if { ![string match {*\[INFO GPL-0191\]*} $log_text] } {
  error "Expected phase 2 to run (GPL-0191)"
}
if { [string match {*GPL-0197*} $log_text] } {
  error "Expected no GPL-0197 with -incremental_phase2"
}

# The last row of phase 2's progress table holds the final overflow. Only
# rows after GPL-0191 belong to phase 2; phase 1's measure only the
# instances that were unplaced.
set phase2_start [string first GPL-0191 $log_text]
set phase2_log ""
if { $phase2_start >= 0 } {
  set phase2_log [string range $log_text $phase2_start end]
}
if { [string match {*GPL-1010*} $phase2_log] } {
  error "Expected phase 2 to converge before its iteration limit (GPL-1010)"
}
set rows [regexp -all -inline -line {^\s*\d+ \|\s+([0-9.]+) \|} $phase2_log]
set final_overflow [lindex $rows end]
if { $final_overflow eq "" || $final_overflow > $overflow } {
  error "Expected a final overflow at or below $overflow, got\
      '$final_overflow'"
}

puts pass
