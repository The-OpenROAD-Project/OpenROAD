# generate_ram's pin placement leaves the tracks within a pitch of a die
# corner empty. A via from a pin on the first track along an edge to the
# layer below lands its enclosure outside the die (asap7: VIA34's M3
# reaches 68 nm below the first M4 track, 48 nm up), and detailed
# routing found no path (DRT-0255): 16 words of 8 bits with two read and
# four write ports put a read address pin there. The block is placed as
# generate_ram places it, up to its pins; its routing takes minutes.
source "helpers.tcl"
source "routed.tcl"

routed::read_asap7

generate_ram_netlist -mask_size 8 -word_size 8 -num_words 16 -r_ports 2 \
  -w_ports 4 -storage_cell DFFHQNx1_ASAP7_75t_R -write_style mux \
  -write_priority or -decoder word -tapcell TAPCELL_ASAP7_75t_R \
  -max_tap_dist 25
ord::design_created
ram::ram_pdngen VDD VSS M1 [ord::microns_to_dbu 0.018] \
  M3 [ord::microns_to_dbu 0.090] [ord::microns_to_dbu 2.16] \
  M4 [ord::microns_to_dbu 0.120] [ord::microns_to_dbu 1.08]
source ../../../test/asap7/asap7.tracks.tcl
ram::ram_pinplacer M3 M4

set block [ord::get_db_block]
set die [$block getDieArea]
foreach bterm [$block getBTerms] {
  if { [$bterm getSigType] in {POWER GROUND} } {
    continue
  }
  foreach bpin [$bterm getBPins] {
    foreach box [$bpin getBoxes] {
      set layer [$box getTechLayer]
      set pitch [$layer getPitch]
      if { [$layer getDirection] == "HORIZONTAL" } {
        set c [expr { ([$box yMin] + [$box yMax]) / 2 }]
        set lo [$die yMin]
        set hi [$die yMax]
      } else {
        set c [expr { ([$box xMin] + [$box xMax]) / 2 }]
        set lo [$die xMin]
        set hi [$die xMax]
      }
      if { $c <= $lo + $pitch || $c >= $hi - $pitch } {
        error "[$bterm getName] sits within a pitch of a corner"
      }
    }
  }
}

puts "pass"
