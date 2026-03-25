source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_def "abstract_bloat_pin.def"

set lef_file [make_result_file abstract_bloat_pin.lef]

write_abstract_lef -bloat_occupied_layers $lef_file

# Verify the OBS does not cover the signal pin
set f [open $lef_file r]
set lef_content [read $f]
close $f

# Parse the pin rect and OBS rects on metal1
set pin_rects [list]
set obs_rects [list]
set in_pin 0
set in_obs 0
set current_layer ""
foreach line [split $lef_content "\n"] {
    set line [string trim $line]
    if {[string match "PIN sig_in" $line]} {
        set in_pin 1
    } elseif {$in_pin && [string match "LAYER *" $line]} {
        set current_layer [lindex $line 1]
    } elseif {$in_pin && [string match "RECT *" $line] && $current_layer eq "metal1"} {
        lappend pin_rects $line
    } elseif {$in_pin && [string match "END sig_in" $line]} {
        set in_pin 0
    }
    if {[string match "OBS" $line]} {
        set in_obs 1
        set current_layer ""
    } elseif {$in_obs && [string match "LAYER *" $line]} {
        set current_layer [lindex [split $line " "] 1]
    } elseif {$in_obs && [string match "RECT *" $line] && $current_layer eq "metal1"} {
        lappend obs_rects $line
    } elseif {$in_obs && [string match "END" $line]} {
        set in_obs 0
    }
}

puts "Pin rects on metal1: [llength $pin_rects]"
foreach r $pin_rects {
    puts "  $r"
}
puts "OBS rects on metal1: [llength $obs_rects]"
foreach r $obs_rects {
    puts "  $r"
}

# Check that no OBS rect fully contains the pin rect
# Pin is at x: -0.07 to 0.0, y: 2.43 to 2.57 (in LEF coords)
# The key check: pin x_min (negative or 0) should not be inside any OBS rect
set pin_covered 0
if {[llength $pin_rects] > 0} {
    # Extract pin bounds from first pin rect
    set pr [lindex $pin_rects 0]
    regexp {RECT\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)} $pr -> px1 py1 px2 py2
    foreach or_rect $obs_rects {
        regexp {RECT\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)} $or_rect -> ox1 oy1 ox2 oy2
        if {$ox1 <= $px1 && $oy1 <= $py1 && $ox2 >= $px2 && $oy2 >= $py2} {
            set pin_covered 1
            puts "FAIL: OBS rect ($ox1 $oy1 $ox2 $oy2) covers pin rect ($px1 $py1 $px2 $py2)"
        }
    }
}

if {!$pin_covered} {
    puts "pass: OBS does not cover signal pin"
} else {
    puts "FAIL: OBS covers signal pin - pin_access will fail with DRT-0073"
}
