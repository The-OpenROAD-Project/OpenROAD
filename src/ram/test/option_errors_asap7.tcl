# Out-of-range options stop generation, each with its own message.

source "helpers.tcl"
source "routed.tcl"

routed::read_asap7

set base [concat $routed::asap7 {-mask_size 8 -word_size 8 -num_words 8 -r_ports 1 -w_ports 1
  -storage_cell DFFHQNx1_ASAP7_75t_R}]
foreach {id options} {
  36 {-r_ports 0 -rb_ports 0}
  96 {-zero_word -2}
  59 {-zero_word 8}
  92 {-banks 0}
  92 {-bank_columns -1}
  95 {-service_columns {0 10}}
  93 {-bit_folds 0}
  71 {-register_read_address -1}
  71 {-register_read_address 2}
  94 {-banks 2 -rb_ports -1}
  97 {-tracks no_such_tracks.tcl}
  98 {-periphery unplaced}
  79 {-cells {and2 OR2x2_ASAP7_75t_R}}
  79 {-cells {nand2 NOR2xp33_ASAP7_75t_R}}
  99 {-cells {and2 AND2x2_ASAP7_75t_R inv}}
  54 {-w_ports 2 -use_latch 1}
  55 {-w_ports 2 -column_mux_ratio 2}
  79 {-register_read_address 1 -cells {addr_flop SDFHx1_ASAP7_75t_R}}
  66 {-decoder word -column_mux_ratio 2}
  102 {-wrapper_verilog unused.v}
  64 {-storage_cell DFFASRHQNx1_ASAP7_75t_R -reset_value 12abc}
  64 {-storage_cell DFFASRHQNx1_ASAP7_75t_R -reset_value -1}
  64 {-storage_cell DFFASRHQNx1_ASAP7_75t_R -reset_value 0x}
  100 {-storage_cell DFFASRHQNx1_ASAP7_75t_R -reset_value 0x1ff}
} {
  set args [dict merge $base $options]
  if { ![catch { generate_ram {*}$args } message] } {
    error "generate_ram $options did not stop"
  }
  set want [format "RAM-%04d" $id]
  if { ![string match "*$want*" $message] } {
    error "generate_ram $options stopped with: $message, not $want"
  }
  puts "$options: $want"
}

puts "pass"
