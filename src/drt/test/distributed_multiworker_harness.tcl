# Reusable single-host MULTI-WORKER distributed detailed-route harness with a
# built-in correctness oracle (Workstream C).
#
# It (1) routes a design with the NON-distributed router to produce a baseline
# DEF, then (2) routes the SAME pre-route design with the DISTRIBUTED router
# using a leader + N local worker processes + 1 balancer over localhost, with a
# local dir as the shared volume, and (3) asserts the distributed routed DEF is
# byte-identical to the baseline. Wall-times for both the baseline and the
# distributed run are printed so 1-vs-N-worker scaling can be measured.
#
# Robustness (same approach as Workstream A's gcd harness, generalized to N):
#   * All ports are allocated dynamically (bind :0) -- never hard-coded, so
#     concurrent / leftover processes can't cause DST-0009.
#   * The leader POLLS each server port until it accepts connections (no sleep).
#   * Every child process is tracked and killed on every exit path.
#   * The baseline runs in a separate BLOCKING child so the oracle is
#     deterministic (no background process to race against).
#
# Inputs (set by the design-specific caller BEFORE sourcing this file):
#   DIST_DESIGN_NAME   - e.g. "aes_nangate45"
#   DIST_PREROUTE_DEF  - pre-route DEF to route (e.g. aes_nangate45_preroute.def)
#   DIST_GUIDE         - route guide file
#   DIST_BASELINE_TCL  - non-distributed baseline script (writes <name>.defok)
#   DIST_NUM_WORKERS   - number of worker processes to launch (env, default 2)
#   DIST_CLOUD_SIZE    - -cloud_size value (env, default = DIST_NUM_WORKERS)

source "helpers.tcl"

set OR $argv0

proc alloc_free_port {} {
  set s [socket -server {apply {{args {}}}} 0]
  set port [lindex [fconfigure $s -sockname] 2]
  close $s
  return $port
}

proc port_is_up { port } {
  if { [catch { set s [socket 127.0.0.1 $port] } err] } {
    return 0
  }
  close $s
  return 1
}

proc wait_for_port { port timeout_ms } {
  set waited 0
  while { $waited < $timeout_ms } {
    if { [port_is_up $port] } {
      return 1
    }
    exec sleep 0.1
    incr waited 100
  }
  return 0
}

set children {}
proc kill_children {} {
  global children
  foreach pid $children {
    catch { exec kill $pid }
  }
  set children {}
}

# ---- parameters --------------------------------------------------------------
set num_workers 2
if { [info exists ::env(DIST_NUM_WORKERS)] } {
  set num_workers $::env(DIST_NUM_WORKERS)
}
set cloud_size $num_workers
if { [info exists ::env(DIST_CLOUD_SIZE)] } {
  set cloud_size $::env(DIST_CLOUD_SIZE)
}
puts "DIST: design=$DIST_DESIGN_NAME num_workers=$num_workers cloud_size=$cloud_size"

# ---- 1. baseline (non-distributed) = the oracle ------------------------------
set base_start [clock milliseconds]
exec $OR -no_splash -no_init -exit $DIST_BASELINE_TCL \
  > [make_result_file ${DIST_DESIGN_NAME}_base.log] 2>@1
set base_end [clock milliseconds]
set base_def [make_result_file ${DIST_DESIGN_NAME}.defok]
if { ![file exists $base_def] } {
  utl::error DRT 9008 "Baseline routed DEF $base_def was not produced."
}
puts "DIST: baseline (non-distributed) wall-time = [expr {$base_end - $base_start}] ms"

# ---- 2. launch N workers + balancer on dynamic ports -------------------------
set worker_ports {}
for { set i 0 } { $i < $num_workers } { incr i } {
  lappend worker_ports [alloc_free_port]
}
set balancer_port [alloc_free_port]
puts "DIST: worker_ports={$worker_ports} balancer=$balancer_port"

set wi 0
foreach wp $worker_ports {
  set env(DRT_WORKER_PORT) $wp
  lappend children [exec $OR -no_init distributed_worker.tcl \
    > [make_result_file ${DIST_DESIGN_NAME}_worker${wi}.log] 2>@1 &]
  incr wi
}
foreach wp $worker_ports {
  if { ![wait_for_port $wp 60000] } {
    kill_children
    utl::error DRT 9003 "Worker did not come up on port $wp."
  }
}

set env(DRT_WORKER_PORTS) [join $worker_ports ","]
set env(DRT_BALANCER_PORT) $balancer_port
lappend children [exec $OR -no_init distributed_balancer.tcl \
  > [make_result_file ${DIST_DESIGN_NAME}_balancer.log] 2>@1 &]
if { ![wait_for_port $balancer_port 60000] } {
  kill_children
  utl::error DRT 9005 "Balancer did not come up on port $balancer_port."
}

# ---- 3. distributed detailed_route on the SAME pre-route design --------------
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def $DIST_PREROUTE_DEF
read_guides $DIST_GUIDE
set_thread_count [expr { max(1, [cpu_count] / 4) }]

set dist_start [clock milliseconds]
if { [catch {
  detailed_route \
    -output_drc [make_result_file ${DIST_DESIGN_NAME}_dist.output.drc.rpt] \
    -verbose 1 \
    -distributed \
    -remote_host 127.0.0.1 \
    -remote_port $balancer_port \
    -cloud_size $cloud_size \
    -shared_volume [make_result_dir]
} err] } {
  kill_children
  utl::error DRT 9006 "Distributed detailed_route failed: $err"
}
set dist_end [clock milliseconds]
puts "DIST: distributed ($num_workers workers, cloud_size $cloud_size) wall-time = [expr {$dist_end - $dist_start}] ms"

set dist_def [make_result_file ${DIST_DESIGN_NAME}_dist.def]
write_def $dist_def

# ---- 4. tear down + verify oracle --------------------------------------------
kill_children

if { [diff_files $base_def $dist_def] } {
  utl::error DRT 9007 \
    "Distributed routed DEF differs from non-distributed baseline."
}
puts "DIST: distributed routed DEF == non-distributed baseline."
puts "pass"
