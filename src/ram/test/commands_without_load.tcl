# Ensure that running commands without loading a design doesn't crash.

set skip {
  define_corners
  exit
}

foreach command [info commands] {
  if {[lsearch $skip $command] != -1} {
    puts "skip $command"
  } else {
    catch {$command} msg
  }
}

# If we got here without crashing then we passed
puts pass
