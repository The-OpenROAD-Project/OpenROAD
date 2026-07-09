# Single-host distributed detailed-route regression with a built-in
# correctness oracle: route gcd_nangate45 with the NON-distributed router to
# produce a baseline DEF, then route the SAME pre-route design with the
# DISTRIBUTED router (leader + 2 local workers + balancer over localhost using
# a local dir as the shared volume) and assert the two routed DEFs are
# identical.
#
# Robustness fixes vs the original gcd_nangate45_distributed.tcl:
#   * Ports are allocated dynamically (bind to :0) instead of hard-coded
#     1111/1112/1234, so concurrent / leftover processes on a shared host can
#     never cause "Address already in use" (DST-0009).
#   * The leader POLLS each server's port until it is actually accepting
#     connections, instead of a fixed `sleep 3` race.
#   * All child processes are tracked and killed on every exit path.
#   * The baseline is produced in THIS process (no separate background process
#     to race against), making the oracle deterministic.

source "helpers.tcl"

set OR $argv0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Allocate a free TCP port by binding to port 0 and reading back the assigned
# port. There is a small window between closing here and the child binding, so
# callers must tolerate a failed bind and the leader polls for readiness.
proc alloc_free_port {} {
  set s [socket -server {apply {{args {}}}} 0]
  set port [lindex [fconfigure $s -sockname] 2]
  close $s
  return $port
}

# Return 1 if a server is accepting connections on 127.0.0.1:$port.
proc port_is_up { port } {
  if { [catch { set s [socket 127.0.0.1 $port] } err] } {
    return 0
  }
  close $s
  return 1
}

# Block (up to $timeout_ms) until $port accepts a connection.
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

# ---------------------------------------------------------------------------
# 1. Baseline: non-distributed detailed_route on gcd_nangate45 (the oracle).
#    Run in a SEPARATE, BLOCKING child process so the leader process starts
#    the distributed run from a clean state, and so there is no background
#    process to race against (the original harness polled /proc/<pid>).
# ---------------------------------------------------------------------------
set base_def [make_result_file gcd_nangate45_base.def]
exec $OR -no_splash -no_init -exit gcd_nangate45.tcl \
  > [make_result_file gcd_nangate45_base.log] 2>@1
# gcd_nangate45.tcl writes the baseline routed DEF here:
set base_def [make_result_file gcd_nangate45.defok]
if { ![file exists $base_def] } {
  utl::error DRT 9008 "Baseline routed DEF $base_def was not produced."
}

# ---------------------------------------------------------------------------
# 2. Launch the local distributed cluster on dynamically-chosen free ports.
# ---------------------------------------------------------------------------
set worker1_port [alloc_free_port]
set worker2_port [alloc_free_port]
set balancer_port [alloc_free_port]
puts "DIST: worker1=$worker1_port worker2=$worker2_port balancer=$balancer_port"

set env(DRT_WORKER1_PORT) $worker1_port
set env(DRT_WORKER2_PORT) $worker2_port
set env(DRT_BALANCER_PORT) $balancer_port

# Workers first, then the balancer (the balancer probes workers when added).
set env(DRT_WORKER_PORT) $worker1_port
lappend children [exec $OR -no_init distributed_worker.tcl \
  > [make_result_file distributed_worker1.log] 2>@1 &]
set env(DRT_WORKER_PORT) $worker2_port
lappend children [exec $OR -no_init distributed_worker.tcl \
  > [make_result_file distributed_worker2.log] 2>@1 &]

if { ![wait_for_port $worker1_port 30000] } {
  kill_children
  utl::error DRT 9003 "Worker 1 did not come up on port $worker1_port."
}
if { ![wait_for_port $worker2_port 30000] } {
  kill_children
  utl::error DRT 9004 "Worker 2 did not come up on port $worker2_port."
}

lappend children [exec $OR -no_init distributed_balancer.tcl \
  > [make_result_file distributed_balancer.log] 2>@1 &]
if { ![wait_for_port $balancer_port 30000] } {
  kill_children
  utl::error DRT 9005 "Balancer did not come up on port $balancer_port."
}

# ---------------------------------------------------------------------------
# 3. Distributed detailed_route on the SAME pre-route design.
# ---------------------------------------------------------------------------
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
read_guides gcd_nangate45.route_guide
set_thread_count [expr { [cpu_count] / 4 }]

if { [catch {
  detailed_route -output_drc [make_result_file gcd_nangate45_dist.output.drc.rpt] \
    -verbose 1 \
    -distributed \
    -remote_host 127.0.0.1 \
    -remote_port $balancer_port \
    -cloud_size 2 \
    -shared_volume [make_result_dir]
} err] } {
  kill_children
  utl::error DRT 9006 "Distributed detailed_route failed: $err"
}

set dist_def [make_result_file gcd_nangate45_dist.def]
write_def $dist_def

# ---------------------------------------------------------------------------
# 4. Tear down the cluster and verify the oracle.
# ---------------------------------------------------------------------------
kill_children

if { [diff_files $base_def $dist_def] } {
  utl::error DRT 9007 \
    "Distributed routed DEF differs from non-distributed baseline."
}
puts "DIST: distributed routed DEF == non-distributed baseline."
# Last line must start with "pass" for the PASSFAIL test harness.
puts "pass"
