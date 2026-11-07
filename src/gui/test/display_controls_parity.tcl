# With no Qt window, gui::set_display_controls acts on the web viewer's
# script view, which keeps its own copy of the Qt display-control tree.  This
# checks that copy against the real one: the same paths, and the same
# defaults.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

# Path -> default visibility for every control the current viewer has.
proc control_defaults { } {
  set defaults [dict create]
  foreach path [gui::display_control_paths] {
    dict set defaults $path [gui::check_display_controls $path visible]
  }
  return $defaults
}

if { ![gui::supported] } {
  # Registered only where the Qt gui is built in, so no tree to compare with
  # is a failure.
  fail "OpenROAD was built without the Qt gui"
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

  read_lef [file join $openroad_test_dir Nangate45 Nangate45.lef]
  read_def [file join $openroad_test_dir gcd_nangate45.def]

  set web [control_defaults]
  # Without the saved settings, so the window starts from its defaults.
  set ::qt {}
  gui::show {
    set ::qt [control_defaults]
    gui::hide
  } false false

  if { [dict size $::qt] == 0 } {
    fail "the Qt gui reported no controls"
  }
  foreach path [dict keys $::qt] {
    if { ![dict exists $web $path] } {
      fail "missing without a window: $path"
    }
    if { [dict get $web $path] != [dict get $::qt $path] } {
      fail "$path defaults to [dict get $web $path] without a window, \
            [dict get $::qt $path] in Qt"
    }
  }
  foreach path [dict keys $web] {
    if { ![dict exists $::qt $path] } {
      fail "not in the Qt gui: $path"
    }
  }

  puts "pass"
}
