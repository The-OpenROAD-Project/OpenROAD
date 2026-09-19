# save_image end to end under an offscreen Qt gui.
#
# Gui::saveImage lives in gui_core and has two paths.  With a window up it
# dispatches through GuiBackend::saveImage; with none it builds a script and
# reopens the gui through a GuiLauncher, because rendering a layout needs a
# window.  This covers both, and checks they agree: the same region rendered
# either way has to give the same image.
#
# It also covers the offscreen fallback to the die area, which is what makes
# a zero-area region mean "the whole design" on a machine with no display.
# gcd_nangate45's die is square, so the region below is deliberately not: a
# square one could not tell the fallback from an honoured region.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

proc read_file_bytes { path } {
  set f [open $path rb]
  set data [read $f]
  close $f
  return $data
}

# Width and height out of a PNG IHDR: an 8-byte signature, a 4-byte chunk
# length and the "IHDR" tag, then two big-endian 32-bit values.
proc png_size { path } {
  set f [open $path rb]
  set header [read $f 24]
  close $f
  if { [string length $header] != 24 } {
    fail "[file tail $path] is too short to be a PNG"
  }
  binary scan $header a8x4a4II signature tag width height
  if { $tag ne "IHDR" } {
    fail "[file tail $path] is not a PNG"
  }
  return [list $width $height]
}

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

  read_lef [file join $openroad_test_dir Nangate45 Nangate45.lef]
  read_def [file join $openroad_test_dir gcd_nangate45.def]

  # The reopened path reaches save_image through a generated Tcl script, so its
  # name carries the characters Tcl would act on -- a $ and a [command] that
  # succeeds and expands to nothing.  Unquoted, this render lands in a
  # different file and the byte comparison below finds nothing to read.
  set reopened [make_result_file {save_image_$reopened[list].png}]
  set windowed [make_result_file save_image_windowed.png]
  set die_area [make_result_file save_image_die_area.png]

  # results/ survives between runs, so clear the targets first.  Otherwise a
  # run that writes nothing at all still finds last run's files and passes.
  foreach path [list $reopened $windowed $die_area] {
    file delete $path
  }

  # No window yet, so this one goes through the launcher: it reopens the gui,
  # renders, and hides again.
  gui::save_image $reopened 0 0 30 10 500

  gui::show [subst -nocommands {
    gui::save_image $windowed 0 0 30 10 500
    # Zero area: offscreen there is no meaningful viewport, so the die area
    # is used instead.
    gui::save_image $die_area 0 0 0 0 500
    gui::hide
  }] false

  foreach path [list $reopened $windowed $die_area] {
    if { ![file exists $path] } {
      fail "[file tail $path] was not written"
    }
  }

  lassign [png_size $windowed] width height
  if { $width != 500 } {
    fail "asked for 500px wide, got $width"
  }
  if { $height >= $width } {
    fail "a 30x10 micron region should be wider than it is tall, got\
          ${width}x${height}"
  }

  # The two paths render the same region, so the images have to match byte
  # for byte.  This is what catches the launcher path drifting from the
  # dispatched one.
  if { [read_file_bytes $reopened] ne [read_file_bytes $windowed] } {
    fail "the reopened-gui image differs from the one rendered in an open\
          window"
  }

  # A zero-area region means the die area, which is square here, so at the
  # same pixel width it comes out a different shape from the 30x10 region.
  lassign [png_size $die_area] die_width die_height
  if { $die_width != 500 } {
    fail "die-area image should also honour the width, got $die_width"
  }
  if { $die_height == $height } {
    fail "die area rendered the same shape as the 30x10 region; the\
          zero-area fallback did not fire"
  }

  puts "pass"
}
