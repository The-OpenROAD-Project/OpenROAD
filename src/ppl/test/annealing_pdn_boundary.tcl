# the annealing placer must also keep pins away from boundary PDN stripes
source "helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal2 -width 2.0 -pitch 20.0 -offset 5.0 \
  -extend_to_boundary
add_pdn_stripe -layer metal3 -width 2.0 -pitch 20.0 -offset 5.0 \
  -extend_to_boundary
add_pdn_connect -layers {metal1 metal2}
add_pdn_connect -layers {metal2 metal3}

pdngen

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12 -annealing

# count signal pin shapes overlapping special net wires on the same layer
proc count_pin_stripe_overlaps { } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        foreach net [$block getNets] {
          if { ![$net isSpecial] } {
            continue
          }
          foreach swire [$net getSWires] {
            foreach sbox [$swire getWires] {
              set slayer [$sbox getTechLayer]
              if { $slayer == "NULL" || $slayer != $layer } {
                continue
              }
              if {
                [$box xMin] < [$sbox xMax] && [$sbox xMin] < [$box xMax]
                && [$box yMin] < [$sbox yMax] && [$sbox yMin] < [$box yMax]
              } {
                puts "pin [$bterm getName] on layer [$layer getName] at\
                  ([ord::dbu_to_microns [$box xMin]]\
                  [ord::dbu_to_microns [$box yMin]])\
                  ([ord::dbu_to_microns [$box xMax]]\
                  [ord::dbu_to_microns [$box yMax]]) overlaps\
                  [$net getName] stripe"
                incr violations
              }
            }
          }
        }
      }
    }
  }
  return $violations
}

puts "pin to boundary stripe violations: [count_pin_stripe_overlaps]"
