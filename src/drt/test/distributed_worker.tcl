# Single-host distributed-route worker.
# Port is provided by the leader via the DRT_WORKER_PORT environment variable
# so that CI runs never collide on a hard-coded port. See
# gcd_nangate45_distributed_local.tcl for the orchestration.
source "helpers.tcl"
set_thread_count [expr { [cpu_count] / 4 }]
if { ![info exists ::env(DRT_WORKER_PORT)] } {
  utl::error DRT 9001 "DRT_WORKER_PORT not set for distributed worker."
}
run_worker -host 127.0.0.1 -port $::env(DRT_WORKER_PORT)
