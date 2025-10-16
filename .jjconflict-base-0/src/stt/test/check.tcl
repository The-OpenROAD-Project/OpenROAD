# Nets with cross over/overlap
source "stt_helpers.tcl"

# squack when segment overlap is detected
set_debug_level STT check 1

set cross1 {cross1 0
  {p0 72 61}
  {p1 86 50}
  {p2 84 51}
  {p3 89 51}
  {p4 86 53}
  {p5 86 58}
  {p6 85 59}
  {p7 80 53}
  {p8 79 56}
  {p9 81 58}
  {p10 79 60}
}

report_stt_net $cross1 0.3
