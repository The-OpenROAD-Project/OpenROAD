# Ensure that running commands without loading a design doesn't crash.
source "helpers.tcl"

set skip {
  exit_summary
  run_unit_test_and_exit
  vwait
  exit
}

set arg [make_result_file commands_without_load]

foreach command [info commands] {
  if { [lsearch $skip $command] != -1 } {
    puts "skip $command"
  } else {
    puts "TEST: $command"
    catch { $command } msg
    catch { $command $arg } msg
    catch { $command $arg $arg } msg
    catch { $command $arg $arg $arg } msg
    file delete $arg
  }
}

# If we got here without crashing then we passed
puts pass
