# Each -display_option key reaches the web renderer as one JSON key, whatever
# characters it holds, in save_image -web and in save_animated_gif.
source "helpers.tcl"

proc read_file_bytes { path } {
  set f [open $path rb]
  set data [read $f]
  close $f
  return $data
}

# The bytes of the PNG save_image -web writes with `args`.
proc image { name args } {
  set path [make_result_file display_option_keys_$name.png]
  save_image -web -width 200 {*}$args $path
  return [read_file_bytes $path]
}

# The bytes of a one-frame GIF save_animated_gif writes with `args`.
proc gif { name args } {
  set path [make_result_file display_option_keys_$name.gif]
  set key [save_animated_gif -start $path]
  save_animated_gif -add -key $key -width 200 {*}$args
  save_animated_gif -end -key $key
  return [read_file_bytes $path]
}

read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45.def

# Characters outside ASCII, from the 2-byte and the 4-byte UTF-8 ranges.
set wide "[format %c 0xe9][format %c 0x1f600]"

set odd_keys [list \
  quote {a"b} \
  backslash "a\\" \
  newline "a\nb" \
  tab "a\tb" \
  unit_separator "a\u001fb" \
  surrogate "a\ud800b" \
  non_ascii "a${wide}b"]

foreach render { image gif } {
  set plain [$render plain]
  set no_cells [$render no_cells -display_option {stdcells false}]
  check "$render: stdcells false changes the image" {
    expr { $plain ne $no_cells }
  } 1

  set injected [$render injected -display_option {{stdcells":false,"x} true}]
  check "$render: a quote in a key sets no other option" {
    expr { $injected eq $plain }
  } 1

  # A key that broke the JSON would drop every option given with it.
  foreach { label key } $odd_keys {
    set odd [$render $label -display_option [list $key true] \
      -display_option {stdcells false}]
    check "$render: a $label in a key keeps the other options" {
      expr { $odd eq $no_cells }
    } 1
  }

  # visible_layers is no boolean, so WEB-0042 ignores the whole payload.
  set wrong_type [$render wrong_type -display_option {visible_layers true} \
    -display_option {stdcells false}]
  check "$render: an option WEB-0042 ignored is not applied in part" {
    expr { $wrong_type eq $plain }
  } 1
}

# json_string writes exactly this literal: a wrong escape can still be valid
# JSON, and then a site_<name> key no longer names its site.
check "json_string writes the exact literal" {
  web::json_string "x\"y\\z\nw\x00\ud800$wide"
} [format {"x\"y\\z\u000aw\u0000%c%s"} 0xfffd $wide]

exit_summary
