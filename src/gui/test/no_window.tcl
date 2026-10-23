# With no Qt window, in a build with or without Qt, the gui:: commands exist,
# and the ones that need a window log GUI-0001 rather than fail.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

suppress_message ODB 127
suppress_message ODB 128
suppress_message ODB 130
suppress_message ODB 131
suppress_message ODB 132
suppress_message ODB 133
suppress_message ODB 134
suppress_message ODB 227

read_lef [file join $openroad_test_dir Nangate45 Nangate45.lef]
read_def [file join $openroad_test_dir gcd_nangate45.def]

foreach cmd {
  gui::fit gui::clear_selections gui::clear_highlights
  gui::set_display_controls gui::save_display_controls
  gui::restore_display_controls gui::select_marker_category
  gui::show_widget gui::hide_widget gui::select_clockviewer_clock
  gui::show_worst_path gui::set_heatmap select save_clocktree_image
} {
  if { [info commands $cmd] == "" } {
    fail "$cmd is not defined"
  }
}

if { [gui::has_ui] || [gui::enabled] } {
  fail "a viewer is up before any was opened"
}

gui::save_display_controls
gui::clear_highlights -1
gui::clear_selections
gui::fit
gui::set_display_controls "*" visible false
gui::show_widget "Clock Tree Viewer"
gui::hide_widget "Clock Tree Viewer"
gui::restore_display_controls

if { ![gui::supported] } {
  # Without Qt, save_image renders with the web viewer.
  set image [make_result_file no_window.png]
  save_image -width 256 $image
  if { ![file exists $image] || [file size $image] == 0 } {
    fail "save_image wrote nothing"
  }
}

puts "pass"
