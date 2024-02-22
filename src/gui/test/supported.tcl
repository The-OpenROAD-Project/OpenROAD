# check for presence of supported

if { [info commands gui::supported] == "::gui::supported" } {
  puts "Found / Pass"
} else {
  puts "Not found / Fail"
}
