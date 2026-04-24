# hopeless

Regression test for the WNS-stagnation gate in `repair_timing`
(`src/rsz/src/RepairSetup.cc`).

The design is a deliberately deep combinational chain clocked impossibly fast
(see `constraint.sdc` + `ABC_CLOCK_PERIOD_IN_PS` in `BUILD`). With the old
defaults, `repair_timing` would grind forever producing tiny TNS twitches
without moving WNS. With the gate, the phase aborts deterministically after
~1200 iterations of no WNS movement and logs `repair_timing: WNS stuck at ...`.

The test asserts only that the flow completes within the bazel timeout.
QoR is intentionally unchecked — the point is bounded runtime and determinism.

Run:

```
bazelisk test //test/orfs/hopeless:hopeless_final_base_test
```
