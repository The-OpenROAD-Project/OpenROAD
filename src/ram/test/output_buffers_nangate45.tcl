# Check custom output buffers through placement, PDN, pin placement and routing.
source "helpers.tcl"
set_thread_count 2
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45.lef

generate_ram \
  -word_size 7 -mask_size 1 -num_words 7 -rw_ports 1 \
  -storage_cell DFF_X1 -output_buffer_cell BUF_X4 \
  -routing_layer {metal1 0.08} -ver_layer {metal4 0.14 9} \
  -hor_layer {metal3 0.08 8} \
  -filler_cells {FILLCELL_X1 FILLCELL_X2 FILLCELL_X4 FILLCELL_X8} \
  -tapcell TAPCELL_X1 -max_tap_dist 10

set block [ord::get_db_block]
set count 0
foreach inst [$block getInsts] {
  if { [string match "output_buffer.*" [$inst getName]] } {
    incr count
    if { [[$inst getMaster] getName] ne "BUF_X4" } {
      error "wrong output buffer master"
    }
    set input [$inst findITerm A]
    set output [$inst findITerm Z]
    if { [[$input getNet] getBTermCount] != 0 } {
      error "array net still connected to external pins"
    }
    if { [[$output getNet] getITermCount] != 1 } {
      error "output has more than one driver"
    }
    foreach term [list $input $output] {
      if { [[$term getNet] getWire] eq "NULL" } {
        error "output buffer signal net is unrouted"
      }
    }
    foreach term [$inst getITerms] {
      if { [$term getSigType] in {POWER GROUND} && [$term getNet] eq "NULL" } {
        error "unconnected output buffer supply"
      }
    }
  }
}
if { $count != 7 } { error "expected seven output buffers, found $count" }
check_placement -verbose
if { [detailed_route_num_drvs] != 0 } { error "RAM has routing violations" }
puts "pass"
