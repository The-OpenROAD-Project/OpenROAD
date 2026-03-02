source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 2

if {[grt::is_multicore]} {
  utl::error GRT 707 "FastRoute should start with multicore routing disabled."
}

global_route -verbose

if {[grt::is_multicore]} {
  utl::error GRT 708 \
    "Serial global_route unexpectedly left multicore routing enabled."
}

global_route -start_incremental
global_route -end_incremental -multicore

if {![grt::is_multicore]} {
  utl::error GRT 709 \
    "Incremental global_route -multicore did not reach FastRoute."
}

global_route -start_incremental
global_route -end_incremental

if {[grt::is_multicore]} {
  utl::error GRT 710 \
    "Incremental global_route without -multicore did not disable FastRoute multicore routing."
}

puts "pass"
exit 0
