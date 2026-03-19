# Test: vertical pin on bottom die edge should be DRT-accessible.
#
# This reproduces DRT-0073 "No access point" when place_pins puts a
# vertical-layer pin (M5) exactly at the bottom die boundary. The pin
# center is ~42nm from the edge, leaving no room for via access.
#
# A macro with such pins cannot be used in a hierarchical design because
# the parent's router cannot reach the pin.
#
# Expected: place_pins should not create inaccessible pin positions,
# or should warn when a pin placement would be DRT-inaccessible.

source "helpers.tcl"

# Use ASAP7 tech
read_lef "asap7/asap7_tech_1x_201209.lef"
read_lef "asap7/asap7sc7p5t_28_R_1x_220121a.lef"

# Create a tiny synthetic design with a few I/O pins
read_verilog pin_die_edge_access1.v
link_design tiny_macro

# Small die — typical for a macro in hierarchical design
initialize_floorplan -die_area "0 0 25.920 25.920" \
  -core_area "1.116 1.116 24.828 24.828" \
  -site asap7sc7p5t

# Make tracks (ASAP7 standard)
make_tracks M1 -x_offset 0.012 -x_pitch 0.036 -y_offset 0.012 -y_pitch 0.036
make_tracks M2 -x_offset 0.012 -x_pitch 0.036 -y_offset 0.012 -y_pitch 0.036
make_tracks M3 -x_offset 0.012 -x_pitch 0.048 -y_offset 0.012 -y_pitch 0.048
make_tracks M4 -x_offset 0.012 -x_pitch 0.048 -y_offset 0.012 -y_pitch 0.048
make_tracks M5 -x_offset 0.012 -x_pitch 0.048 -y_offset 0.012 -y_pitch 0.048

# Constrain test_pin to bottom edge
set_io_pin_constraint -region bottom:* -pin_names {test_pin}

# Place pins with vertical layers M3/M5 (reproduces the issue)
place_pins -hor_layers "M2 M4" -ver_layers "M3 M5" -corner_avoidance 0

# Verify: check that test_pin is accessible
# Get pin position
set block [ord::get_db_block]
foreach bterm [$block getBTerms] {
    set name [$bterm getName]
    set bpins [$bterm getBPins]
    if {[llength $bpins] == 0} continue
    set bpin [lindex $bpins 0]
    set boxes [$bpin getBoxes]
    if {[llength $boxes] == 0} continue
    set box [lindex $boxes 0]
    set layer_name [[$box getTechLayer] getName]
    set y_min [ord::dbu_to_microns [$box yMin]]
    set y_max [ord::dbu_to_microns [$box yMax]]
    puts "Pin $name: layer=$layer_name y_range=($y_min, $y_max)"

    # Flag if vertical-layer pin is at die boundary
    if {$layer_name eq "M5" || $layer_name eq "M3"} {
        if {$y_min < 0.001} {
            puts "WARNING: Pin $name on vertical layer $layer_name at die bottom edge (y_min=$y_min)"
            puts "  This pin may be inaccessible from outside the macro (DRT-0073)"
        }
    }
}
