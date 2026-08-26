# place_pins must honor the width-dependent spacing rules to wide boundary
# PDN stripes when long pins have enough parallel run length to trigger them
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

pdngen -dont_add_pins

set_pin_length -ver_length 1.0 -hor_length 1.0

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

# count signal pin shapes closer to a stripe than the wide-shape spacing rule
proc count_stripe_spacing_violations { } {
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
              set stripe_width [expr {
                min([$sbox xMax] - [$sbox xMin], [$sbox yMax] - [$sbox yMin])
              }]
              set pin_length [expr {
                max([$box xMax] - [$box xMin], [$box yMax] - [$box yMin])
              }]
              set spacing [$layer getSpacing $stripe_width $pin_length]
              set dx [expr {
                max(0, max([$sbox xMin] - [$box xMax], [$box xMin] - [$sbox xMax]))
              }]
              set dy [expr {
                max(0, max([$sbox yMin] - [$box yMax], [$box yMin] - [$sbox yMax]))
              }]
              if { $dx < $spacing && $dy < $spacing } {
                puts "pin [$bterm getName] on layer [$layer getName] at\
                  ([ord::dbu_to_microns [$box xMin]]\
                  [ord::dbu_to_microns [$box yMin]])\
                  ([ord::dbu_to_microns [$box xMax]]\
                  [ord::dbu_to_microns [$box yMax]]) within\
                  [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of\
                  [$net getName] stripe, needs [ord::dbu_to_microns $spacing]um"
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

puts "pin to wide stripe spacing violations: [count_stripe_spacing_violations]"
