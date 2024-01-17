# Objective is to test man command with no arguments

source main.tcl

if {[catch {man man} result]} {
    # An error occurred
    puts "Error: $result"
} else {
    # No error occurred
    puts "Successly run man without arguments."
}