# incremental02 with a separate phase 2 target: phase 1 stops at the looser
# -overflow, the new cells are spread over the free space before the fillers
# are handed out, and phase 2 brings the whole design to
# -incremental_phase2_overflow.
source helpers.tcl
read_lef ./nangate45.lef
read_def ./incremental02.def

# -incremental_phase2_overflow needs -incremental_phase2 and a value in [0, 1].
if { ![catch { global_placement -incremental -incremental_phase2_overflow 0.1 } \
         msg] } {
  error "Expected -incremental_phase2_overflow without -incremental_phase2 to\
      be rejected"
}
if { ![string match {*GPL-0199*} $msg] } {
  error "Expected GPL-0199, got: $msg"
}
foreach value {2 -0.1 abc} {
  if { ![catch { global_placement -incremental -incremental_phase2 \
                   -incremental_phase2_overflow $value } msg] } {
    error "Expected -incremental_phase2_overflow $value to be rejected"
  }
  if { ![string match {*GPL-0200*} $msg] } {
    error "Expected GPL-0200 for $value, got: $msg"
  }
}

set_thread_count 4
set_debug_level GPL spread 1
set phase2_overflow 0.1
# with_output_to_variable runs its body in its own scope: read the global.
with_output_to_variable log_text {
  global_placement -incremental -incremental_phase2 -density 0.3 \
    -overflow 0.3 -incremental_phase2_overflow $::phase2_overflow \
    -pad_left 2 -pad_right 2
}
puts $log_text

if { ![regexp {Spread (\d+) new cells} $log_text -> spread] || $spread == 0 } {
  error "Expected the new cells to be spread before the fillers are placed"
}
if { ![string match {*\[INFO GPL-0191\]*} $log_text] } {
  error "Expected phase 2 to run (GPL-0191)"
}

# Only rows after GPL-0191 belong to phase 2; its last row holds the final
# overflow, which must meet phase 2's own target.
set phase2_log [string range $log_text [string first GPL-0191 $log_text] end]
if { [string match {*GPL-1010*} $phase2_log] } {
  error "Expected phase 2 to converge before its iteration limit (GPL-1010)"
}
set rows [regexp -all -inline -line {^\s*\d+ \|\s+([0-9.]+) \|} $phase2_log]
set final_overflow [lindex $rows end]
if { $final_overflow eq "" || $final_overflow > $phase2_overflow } {
  error "Expected a final overflow at or below $phase2_overflow, got\
      '$final_overflow'"
}

puts pass
