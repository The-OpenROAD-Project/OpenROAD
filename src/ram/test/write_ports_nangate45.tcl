# generate_ram with two write ports: each port writes the word it
# addresses, both may write in one cycle, and where both write the same
# word the last port's data is kept. Checked by simulating the block.
source "helpers.tcl"
source "ram_sim.tcl"
source "routed.tcl"

routed::read_nangate45

generate_ram {*}$routed::nangate45 \
  -mask_size 7 \
  -word_size 7 \
  -num_words 7 \
  -r_ports 1 \
  -w_ports 2 \
  -storage_cell DFF_X1
routed::check

ram_sim::load
ram_sim::exercise {
  words 7 bits 7
  writes {{addr_w0 D0 {we0[0]}} {addr_w1 D1 {we1[0]}}}
  reads {{addr_r0 Q}}
  cycles 300
}

puts "pass"
