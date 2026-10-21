# Phase 1 of incremental placement is expected to recover from a divergence
# instead of aborting the whole run (see Replace::doIncrementalPlace()): it
# logs a warning and hands off to phase 2 rather than letting the ERROR
# Logger::error() would normally raise propagate. -max_phi_coef pushes the
# density penalty escalation hard enough to diverge for real (GPL-0305)
# within phase 1's 600-iteration cap, instead of needing thousands of
# iterations to overflow a float on a design that would otherwise converge.
#
# This is a passfail test, not a log_compare one: the scenario drives the
# solver into a numerically chaotic regime by design (that is the whole
# point), so the exact iteration-by-iteration HPWL/overflow numbers are not
# bit-reproducible across compilers/build systems (observed to differ
# between the CMake and Bazel builds here) even though the qualitative log
# messages this test actually checks for are. with_output_to_variable
# captures the run's utl::Logger output so those messages can be checked by
# fixed substring, independent of the numbers around them.
#
# incremental03.def is a symlink to incremental02.def (same "some gates
# unplaced to simulate rmp" aes design).

source helpers.tcl
set test_name incremental03
read_lef ./nangate45.lef
read_def ./$test_name.def

set_thread_count 4
with_output_to_variable log_text {
  global_placement -incremental -density 0.3 -pad_left 2 -pad_right 2 \
    -max_phi_coef 50
}
puts $log_text

if { [string match {*\[ERROR GPL-0305\]*} $log_text] } {
  error "GPL-0305 (phase 1's divergence) was logged as an ERROR instead of\
      a WARNING: the recoverable-divergence signaling regressed"
}
if { ![string match {*\[WARNING GPL-0305\]*} $log_text] } {
  error "Expected phase 1 to actually diverge (GPL-0305 as a WARNING); it\
      did not, so this test is not exercising the recovery path"
}
if { ![string match {*\[WARNING GPL-0195\]*} $log_text] } {
  error "Expected GPL-0195 (phase 1 diverged, continuing to phase 2 anyway)"
}

puts pass
