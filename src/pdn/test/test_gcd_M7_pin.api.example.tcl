read_lef ../../../test/Nangate45/Nangate45.lef
read_def gcd_M7_pin/floorplan.def

# Stdcell power/ground pins
add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

# RAM power ground pins
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSSE$}

# Define voltage domains
set_voltage_domain -name CORE -power VDD  -ground VSS

# Define power grid over the stdcell region
define_pdn_grid -name main_grid -pins {metal7} -voltage_domains CORE
add_pdn_stripe  -grid main_grid -layer metal1 -width 0.17 -followpins
add_pdn_stripe  -grid main_grid -layer metal2 -width 0.17 -followpins
add_pdn_stripe  -grid main_grid -layer metal4 -width 0.48 -pitch 56.0 -offset 2 -starts_with POWER
add_pdn_stripe  -grid main_grid -layer metal7 -width 1.40 -pitch 40.0 -offset 2 -starts_with POWER
add_pdn_ring    -grid main_grid -layers {metal6 metal7} -widths 5.0 -spacings  3.0 -core_offset 5

add_pdn_connect -grid main_grid -layers {metal1 metal2} -cut_pitch 0.16
add_pdn_connect -grid main_grid -layers {metal2 metal4}
add_pdn_connect -grid main_grid -layers {metal4 metal7}

# Define power grid over unrotated RAMs when the pins are vertical
define_pdn_grid -macro -name ram -orient {R0 R180 MX MY} -pin_direction vertical -starts_with POWER -halo 4.0
add_pdn_stripe  -grid ram -layer metal5 -width 0.93 -pitch 10.0 -offset 2
add_pdn_stripe  -grid ram -layer metal6 -width 0.93 -pitch 10.0 -offset 2
add_pdn_connect -grid ram -layers {metal4 metal5}
add_pdn_connect -grid ram -layers {metal5 metal6}
add_pdn_connect -grid ram -layers {metal6 metal7}

# Define power grid over rotated RAMs when the power ground pins a horizontal
define_pdn_grid -macro -name rotated_rams -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal -halo 4.0
add_pdn_stripe  -grid rotated_rams -layer metal6 -width 0.93 -pitch 10.0 -offset 2
add_pdn_connect -grid rotated_rams -layers {metal4 metal6}
add_pdn_connect -grid rotated_rams -layers {metal6 metal7}

pdngen -verbose


write_def gcd/power_grid.def 

