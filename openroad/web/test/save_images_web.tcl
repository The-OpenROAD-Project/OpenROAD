# The views of ORFS's save_images.tcl, with no Qt window: the
# gui:: display controls drive save_image -web, so each view is its own image.

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

# Width and height out of a PNG IHDR.
proc png_size { path } {
  set f [open $path rb]
  set header [read $f 24]
  close $f
  binary scan $header a8x4a4II signature tag width height
  if { $tag ne "IHDR" } {
    fail "[file tail $path] is not a PNG"
  }
  return [list $width $height]
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

set all [make_result_file save_images_all.png]
set routing [make_result_file save_images_routing.png]
set placement [make_result_file save_images_placement.png]
set webp [make_result_file save_images_ext.webp]
set escaped [make_result_file save_images_escaped_key.png]
set unescaped [make_result_file save_images_unescaped_key.png]
# results/ survives between runs, so a run that writes nothing must not find
# last run's files.
foreach path [list $all $routing $placement $webp.png $escaped $unescaped] {
  file delete $path
}

gui::save_display_controls
gui::set_display_controls "*" visible false
gui::set_display_controls "Layers/*" visible true
gui::set_display_controls "Nets/*" visible true
gui::set_display_controls "Instances/*" visible true
gui::set_display_controls "Shape Types/*" visible true
gui::set_display_controls "Misc/Instances/*" visible false
gui::set_display_controls "Misc/Instances/Pins" visible true
gui::set_display_controls "Misc/Instances/Blockages" visible true
save_image -web -width 400 $all

gui::set_display_controls "Nets/Power" visible false
gui::set_display_controls "Nets/Ground" visible false
save_image -web -width 400 $routing

gui::set_display_controls "Shape Types/Routing/*" visible false
gui::set_display_controls "Instances/Physical/Fill cell" visible false
gui::set_display_controls "Misc/Instances/*" visible false
save_image -web -width 400 $placement
gui::restore_display_controls

# Only PNG is written, so another extension gets one added.
save_image -web -width 100 $webp

# A pattern's backslash escapes, as in gui::set_display_controls.
save_image -web -width 100 -display_option {{Nets/\Power} false} $escaped
save_image -web -width 100 -display_option {{Nets/Power} false} $unescaped

foreach path [list $all $routing $placement $webp.png] {
  if { ![file exists $path] } {
    fail "[file tail $path] was not written"
  }
}
if { [file exists $webp] } {
  fail "PNG bytes were written under the .webp name"
}
lassign [png_size $all] width height
if { $width != 400 } {
  fail "asked for 400px wide, got $width"
}
if { [read_file_bytes $all] eq [read_file_bytes $routing] } {
  fail "hiding the power and ground nets did not change the image"
}
if { [read_file_bytes $routing] eq [read_file_bytes $placement] } {
  fail "hiding the routing did not change the image"
}
if { [read_file_bytes $escaped] ne [read_file_bytes $unescaped] } {
  fail "a backslash in a display option did not match as an escape"
}
if { [gui::enabled] } {
  fail "the script view made gui::enabled true"
}

puts "pass"
