source $::env(SCRIPTS_DIR)/util.tcl

gui::save_display_controls

set height [[[ord::get_db_block] getBBox] getDY]
set height [ord::dbu_to_microns $height]
set resolution [expr $height / 1000]

set markerdb [[ord::get_db_block] findMarkerCategory DRC]
if {$markerdb != "NULL" && [$markerdb getMarkerCount] > 0} {
  gui::select_marker_category $markerdb
}

gui::clear_highlights -1
gui::clear_selections

gui::fit

# Setup initial visibility to avoid any previous settings
gui::set_display_controls "*" visible false
gui::set_display_controls "Layers/*" visible true
gui::set_display_controls "Nets/*" visible true
gui::set_display_controls "Instances/*" visible true
gui::set_display_controls "Shape Types/*" visible true
gui::set_display_controls "Misc/Instances/*" visible true
gui::set_display_controls "Misc/Instances/Pin Names" visible false
gui::set_display_controls "Misc/Scale bar" visible true
gui::set_display_controls "Misc/Highlight selected" visible true
gui::set_display_controls "Misc/Detailed view" visible true

# The routing view
save_image -resolution $resolution $::env(REPORTS_DIR)/final_all.webp
gui::set_display_controls "Nets/Power" visible false
gui::set_display_controls "Nets/Ground" visible false
save_image -resolution $resolution $::env(REPORTS_DIR)/final_routing.webp

# The placement view without routing
gui::set_display_controls "Layers/*" visible false
gui::set_display_controls "Instances/Physical/*" visible false
save_image -resolution $resolution $::env(REPORTS_DIR)/final_placement.webp

if {[env_var_exists_and_non_empty PWR_NETS_VOLTAGES]} {
  gui::set_display_controls "Heat Maps/IR Drop" visible true
  gui::set_heatmap IRDrop Layer $::env(IR_DROP_LAYER)
  gui::set_heatmap IRDrop ShowLegend 1
  save_image -resolution $resolution $::env(REPORTS_DIR)/final_ir_drop.webp
  gui::set_display_controls "Heat Maps/IR Drop" visible false
}

# The clock view: all clock nets and buffers
gui::set_display_controls "Layers/*" visible true
gui::set_display_controls "Nets/*" visible false
gui::set_display_controls "Nets/Clock" visible true
gui::set_display_controls "Instances/*" visible false
gui::set_display_controls "Instances/StdCells/Clock tree/*" visible true
gui::set_display_controls "Instances/StdCells/Sequential" visible true
gui::set_display_controls "Instances/Macro" visible true
gui::set_display_controls "Misc/Instances/*" visible false
select -name "clk*" -type Inst
save_image -resolution $resolution $::env(REPORTS_DIR)/final_clocks.webp
gui::clear_selections

foreach clock [get_clocks *] {
  if { [llength [get_property $clock sources]] > 0 } {
    set clock_name [get_name $clock]
    save_clocktree_image -clock $clock_name \
        -width 1024 -height 1024 \
        $::env(REPORTS_DIR)/cts_$clock_name.webp
    gui::select_clockviewer_clock $clock_name
    save_image -resolution $resolution $::env(REPORTS_DIR)/cts_${clock_name}_layout.webp
  }
}

# The resizer view: all instances created by the resizer grouped
gui::set_display_controls "Layers/*" visible false
gui::set_display_controls "Instances/*" visible true
gui::set_display_controls "Instances/Physical/*" visible false
select -name "hold*" -type Inst -highlight 0       ;# green
select -name "input*" -type Inst -highlight 1      ;# yellow
select -name "output*" -type Inst -highlight 1
select -name "repeater*" -type Inst -highlight 3   ;# magenta
select -name "fanout*" -type Inst -highlight 3
select -name "load_slew*" -type Inst -highlight 3
select -name "max_cap*" -type Inst -highlight 3
select -name "max_length*" -type Inst -highlight 3
select -name "wire*" -type Inst -highlight 3
select -name "rebuffer*" -type Inst -highlight 4   ;# red
select -name "split*" -type Inst -highlight 5      ;# dark green

save_image -resolution $resolution $::env(REPORTS_DIR)/final_resizer.webp

gui::clear_highlights -1
gui::clear_selections

gui::restore_display_controls
