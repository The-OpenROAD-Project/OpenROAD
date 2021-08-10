# Challenging net from nangate45/swerv with segment overlaps.
# We have to fallback to Flute on this net.
source "pdrev_helpers.tcl"

set_debug_level STT check 1

set alpha 0.3

set net {clk 20 
  {X 48300 34000}
  {X 50700 34500}
  {X 48100 35300}
  {X 51200 35300}
  {X 51700 34100}
  {X 48000 35700}
  {X 52300 34200}
  {X 48600 35700}
  {X 51200 34100}
  {X 52700 34900}
  {X 49200 35700}
  {X 52800 35400}
  {X 52700 35700}
  {X 51900 35700}
  {X 48500 35000}
  {X 49900 34500}
  {X 50100 35700}
  {X 50600 35200}
  {X 50400 35600}
  {X 52600 34100}
  {X 48000 34100}
}

report_stt_net $net $alpha
