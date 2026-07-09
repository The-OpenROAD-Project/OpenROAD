# Single-host distributed-route load balancer.
# Worker/balancer ports are provided by the leader via environment variables
# (DRT_WORKER1_PORT, DRT_WORKER2_PORT, DRT_BALANCER_PORT) so CI runs never
# collide on hard-coded ports. See gcd_nangate45_distributed_local.tcl.
source "helpers.tcl"
foreach v {DRT_WORKER1_PORT DRT_WORKER2_PORT DRT_BALANCER_PORT} {
  if { ![info exists ::env($v)] } {
    utl::error DRT 9002 "$v not set for distributed balancer."
  }
}
add_worker_address -host 127.0.0.1 -port $::env(DRT_WORKER1_PORT)
add_worker_address -host 127.0.0.1 -port $::env(DRT_WORKER2_PORT)
run_load_balancer -host 127.0.0.1 -port $::env(DRT_BALANCER_PORT)
