# error handling
# cannot really test exit_on_error 1 because that will bounce us
set exit_on_error 0

if { [catch { ord::error "catch a luser" } result] } {
  puts "caught '$result'"
}

if { [catch { sta::sta_error "sta lusing" } result] } {
  puts "caught '$result'"
}

if { [catch { read_def xxx } result] } {
  puts "caught '$result'"
}
