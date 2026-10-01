# Opening the Clock Tree Viewer on a design that has a virtual clock (a
# create_clock with no object list, so sta::Clock::pins() is empty) must not
# error out.  ClockTree's constructor used to dereference pins().begin()
# unconditionally; that is undefined behaviour for a virtual clock and raised
# ORD-2018 in libc++ builds while silently yielding a null net in libstdc++
# builds.  See OpenROAD-flow-scripts #4371.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

if { ![gui::supported] } {
  # Built without the GUI; nothing to exercise.
  puts "pass"
} else {
  if { ![info exists ::env(QT_QPA_PLATFORM)] } {
    set ::env(QT_QPA_PLATFORM) offscreen
  }

  suppress_message ODB 127
  suppress_message ODB 128
  suppress_message ODB 130
  suppress_message ODB 131
  suppress_message ODB 132
  suppress_message ODB 133
  suppress_message ODB 134
  suppress_message ODB 227

  read_liberty [file join $openroad_test_dir Nangate45 Nangate45_typ.lib]
  read_lef [file join $openroad_test_dir Nangate45 Nangate45.lef]
  read_def [file join $openroad_test_dir gcd_nangate45.def]

  create_clock -name core_clock -period 2 [get_ports clk]

  # No object list: this clock is virtual and has no pins.
  create_clock -name vclk_core_clock -period 2

  set image [make_result_file clocktree_virtual_clock.png]
  gui::show [subst -nocommands {
    gui::show_widget {Clock Tree Viewer}
    save_clocktree_image -clock core_clock -width 256 -height 256 $image
  }] false

  if { [file exists $image] && [file size $image] > 0 } {
    puts "pass"
  } else {
    puts "fail"
  }
}
