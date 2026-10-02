# Phase 1 of incremental placement is expected to recover from a divergence
# instead of aborting the whole run (see Replace::doIncrementalPlace()): it
# logs a warning and hands off to phase 2 rather than letting the ERROR
# Logger::error() would normally raise propagate. -max_phi_coef pushes the
# density penalty escalation hard enough to diverge for real (GPL-0305)
# within phase 1's 600-iteration cap, instead of needing thousands of
# iterations to overflow a float on a design that would otherwise converge.
# incremental03.def is a symlink to incremental02.def (same "some gates
# unplaced to simulate rmp" aes design).

source helpers.tcl
set test_name incremental03
read_lef ./nangate45.lef
read_def ./$test_name.def

set_thread_count 4
global_placement -incremental -density 0.3 -pad_left 2 -pad_right 2 \
  -max_phi_coef 50
