read_lef write_cdl.lef
read_liberty write_cdl.lib

read_verilog write_cdl.v

link_design top

write_cdl -masters {write_cdl.cdl} results/write_cdl_out.cdl

write_cdl -masters {write_cdl_escaped.cdl} results/write_cdl_escaped_out.cdl

puts "pass"
exit 0
