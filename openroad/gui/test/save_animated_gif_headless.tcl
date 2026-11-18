# save_animated_gif end to end under an offscreen Qt gui.
#
# The gif encoder lives in gui_core and gets its pixels from
# GuiBackend::renderImage(), so this covers the whole path: the offscreen
# fallback to the die area for a zero-area region, the first frame fixing the
# canvas size, and a later frame whose aspect ratio differs being scaled into
# that canvas rather than resizing it.

set script_dir [file dirname [info script]]
set openroad_test_dir [file normalize [file join $script_dir .. .. .. test]]

source [file join $openroad_test_dir helpers.tcl]

proc fail { msg } {
  puts "fail: $msg"
  exit 0
}

# Logical screen width/height from the gif header: a 6-byte signature followed
# by two little-endian 16-bit values.
proc gif_header { path } {
  set f [open $path rb]
  set header [read $f 10]
  close $f
  if { [string length $header] != 10 } {
    fail "gif shorter than its own header: $path"
  }
  binary scan $header a6ss signature width height
  if { $signature ne "GIF89a" } {
    fail "expected a GIF89a signature in $path, got '$signature'"
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

  set animation [make_result_file save_animated_gif_headless.gif]
  set single [make_result_file save_animated_gif_headless_single.gif]
  set no_frames [make_result_file save_animated_gif_headless_empty.gif]

  # results/ survives between runs, so clear the targets first.  Otherwise a
  # run that writes nothing at all still finds last run's files and passes.
  foreach path [list $animation $single $no_frames] {
    file delete $path
  }

  gui::show [subst -nocommands {
    save_animated_gif -start $animation
    # Zero area: offscreen there is no meaningful viewport, so the die area is
    # used.  This first frame fixes the canvas at 200 px wide.
    save_animated_gif -add -width 200 -delay 10
    save_animated_gif -add -width 200 -delay 10
    # A 5:1 area cannot fill a square canvas once the aspect ratio is kept, so
    # this frame is scaled down and padded rather than changing the canvas.
    save_animated_gif -add -area {0 0 100000 20000} -width 200 -delay 10
    save_animated_gif -end

    # The same first frame on its own, as a baseline to size against.
    save_animated_gif -start $single
    save_animated_gif -add -width 200 -delay 10
    save_animated_gif -end

    set empty_key [save_animated_gif -start $no_frames]
    save_animated_gif -end -key \$empty_key
  }] false

  foreach path [list $animation $single] {
    if { ![file exists $path] || [file size $path] == 0 } {
      fail "no gif written to $path"
    }
  }

  # The canvas is the first frame's size, and the later frames did not move it.
  lassign [gif_header $animation] width height
  if { $width != 200 } {
    fail "expected a 200 px wide canvas, got $width"
  }
  if { $height <= 0 } {
    fail "expected a positive canvas height, got $height"
  }

  lassign [gif_header $single] single_width single_height
  if { $single_width != $width || $single_height != $height } {
    fail "baseline canvas ${single_width}x${single_height} != ${width}x${height}"
  }

  # Three frames have to encode to more than one.  Catches frames being
  # silently dropped, which a valid-looking header alone would not.
  if { [file size $animation] <= [file size $single] } {
    fail "3-frame gif ([file size $animation] bytes) is not larger than\
          the 1-frame baseline ([file size $single] bytes)"
  }

  if { [file exists $no_frames] } {
    fail "a gif with no frames should not have been written"
  }

  puts "pass"
}
