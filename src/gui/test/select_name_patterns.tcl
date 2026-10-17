# What -name means for gui's select command.
#
# Gui::select globs object names through fnmatch, the same matcher the web
# viewer's find uses, so the two agree and so do a Qt5 and a Qt6 build.  The
# cases below are the ones where a glob is not simply a literal: a * that has
# to cross the / in a hierarchical name, a bus bit whose brackets have to be
# escaped to mean themselves, and the same brackets left unescaped meaning a
# character class.
#
# The counts are for gcd_nangate45 and are what make the test meaningful --
# an assertion that a pattern merely returns something would pass on a
# pattern that matched everything.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

proc expect { label pattern_script want } {
  set got [uplevel 1 $pattern_script]
  if { $got != $want } {
    fail "$label selected $got, expected $want"
  }
}

if { ![gui::supported] } {
  # Built without the GUI; select is not available.
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

  read_lef [file join $openroad_test_dir Nangate45 Nangate45.lef]
  read_def [file join $openroad_test_dir gcd_nangate45.def]

  gui::show {
    # An ITerm is named inst/mterm, so this only matches if * crosses the /.
    # A Qt6 build used to answer 0 here while a Qt5 build answered 179.
    expect "*A1" { select -type ITerm -name {*A1} } 179

    # A * that does not have to cross the separator, which both agreed on.
    expect "*_/A1" { select -type ITerm -name {*_/A1} } 177

    expect "literal" { select -type Inst -name {FILLER_0_0_1} } 1
    expect "prefix" { select -type Inst -name {FILLER*} } 266
    expect "case insensitive" \
      { select -type Inst -name {filler*} -case_insensitive } 266

    # A literal pattern skips fnmatch, so the case-insensitive compare on that
    # path is a separate one and needs its own case.
    expect "case insensitive literal" \
      { select -type Inst -name {filler_0_0_1} -case_insensitive } 1
    expect "character class" { select -type Inst -name {[Ff]ILLER*} } 266

    # A bus bit: the brackets are the name, so they have to be escaped.  This
    # also guards the literal fast path: a pattern containing a backslash is
    # not literal, and treating it as one is the bug this used to have.
    expect "escaped bus bit" { select -type Net -name {req_msg\[0\]} } 1

    # Unescaped they are a character class, so this asks for req_msg0,
    # which no net is called.
    expect "unescaped bus bit" { select -type Net -name {req_msg[0]} } 0

    gui::hide
  } false

  puts "pass"
}
