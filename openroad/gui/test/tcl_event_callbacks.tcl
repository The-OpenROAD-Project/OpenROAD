# Tcl's after, fileevent and socket callbacks while Qt runs the main loop.
#
# Qt, not Tcl, owns the main loop between gui::show and gui::hide, and the
# GUI services Tcl's event queue from it.  No case below calls gui::hide
# before the main loop starts, only from a callback, so a GUI that does not
# run the callbacks never returns from gui::show and the test times out.
#
# Callbacks run one at a time: a callback that pauses runs Qt events until
# it continues, and no other callback may run in the meantime.  The pause
# comes at the end of a burst of more events than one drain runs, so the
# periodic drain is free to fire while it waits.
#
# exit in a callback ends the process, so that case comes last; no callback
# runs after it.
#
# gui::hide and exit close the main window, which asks "Are you sure you want
# to exit?" unless its "Check on exit" setting is off.  The GUI reads its
# settings from a configuration directory under the results.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

if { ![gui::supported] } {
  # Built without the GUI; there is no Qt main loop.
  puts "pass"
} else {
  if { ![info exists ::env(QT_QPA_PLATFORM)] } {
    set ::env(QT_QPA_PLATFORM) offscreen
  }

  set config_dir [make_result_test_dir tcl_event_callbacks_config]
  set ::env(XDG_CONFIG_HOME) $config_dir
  file mkdir [file join $config_dir "OpenRoad Project"]
  set f [open [file join $config_dir "OpenRoad Project" openroad.conf] w]
  puts $f "\[main\]\ncheck_exit=false"
  close $f

  set ::order {}
  after 200 {
    lappend ::order fired
    gui::hide
  }
  gui::show {} true true
  if { $::order != {fired} } {
    fail "after callback: $::order, expected fired"
  }

  set ::order {}
  set ::burst_left 40
  proc burst { } {
    if { [incr ::burst_left -1] > 0 } {
      after 0 burst
      return
    }
    after 0 { lappend ::order other }
    lappend ::order pause_start
    gui::pause 300
    lappend ::order pause_end
    after 0 gui::hide
  }
  after 0 burst
  gui::show {} true true
  if { $::order != {pause_start pause_end other} } {
    fail "callback order: $::order, expected pause_start pause_end other"
  }

  after 100 {
    after 0 { fail "a callback ran after exit" }
    puts "pass"
    exit
  }
  gui::show {} true true
  fail "exit in a callback returned to the script"
}
