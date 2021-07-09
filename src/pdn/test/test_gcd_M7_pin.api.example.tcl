read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd_M7_pin/floorplan.def

define_pdn_grid -name grid \
  -type stdcell \
  -rails {
    metal1 {width 0.17 pitch  2.4 offset 0}
    metal2 {width 0.17 pitch  2.4 offset 0}
   } \
  -straps {
    metal4 {width 0.48 pitch 56.0 offset 2}
    metal7 {width 1.40 pitch 40.0 offset 2 -starts_with POWER}
   } \
  -connect {{metal1 metal2 constraints {cut_pitch 0.16}} {metal2 metal4} {metal4 metal7}} \
  -pins metal7 \
  -starts_with POWER

define_pdn_grid \
  -name ram \
  -type macro \
  -orient {R0 R180 MX MY} \
  -power_pins "VDD VDDPE VDDCE" \
  -ground_pins "VSS VSSE" \
  -blockages "metal1 metal2 metal3 metal4 metal5 metal6" \
  -straps {
    metal5 {width 0.93 pitch 10.0 offset 2}
    metal6 {width 0.93 pitch 10.0 offset 2}
   } \
  -connect {{metal4_PIN_ver metal5} {metal5 metal6} {metal6 metal7}} \
  -starts_with POWER

define_pdn_grid \
  -name rotated_rams \
  -type macro \
  -orient {R90 R270 MXR90 MYR90} \
  -power_pins "VDD VDDPE VDDCE" \
  -ground_pins "VSS VSSE" \
  -blockages "metal1 metal2 metal3 metal4 metal5 metal6" \
  -straps {
     metal6 {width 0.93 pitch 10.0 offset 2}
   } \
  -connect {{metal4_PIN_hor metal6} {metal6 metal7}} \
  -starts_with POWER

 
# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# RAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$} 
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}

pdngen -verbose

write_def gcd/power_grid.def 

