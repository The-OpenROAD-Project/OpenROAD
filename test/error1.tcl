# error handling
if { [catch { ord::error "catch a luser" } result] } {
  puts "caught '$result'"
}

if { [catch { sta::sta_error "sta lusing" } result] } {
  puts "caught '$result'"
}

if { [catch { read_def xxx } result] } {
  puts "caught '$result'"
}

ord::error "last chance"
puts "this message should not print because it follows an error"
