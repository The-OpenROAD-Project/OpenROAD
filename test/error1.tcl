# error handling
if { [catch { ord::error ORD 1 "catch a luser" } result] } {
  puts "caught '$result'"
}

if { [catch { read_def xxx } result] } {
  puts "caught '$result'"
}

catch {ord::error ORD 1"last chance"} error
