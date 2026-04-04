# CPU Throttle

OpenROAD automatically coordinates CPU usage across parallel instances
to prevent overcommitting cores. This requires zero configuration.

## How it works

At startup, OpenROAD acquires 1 CPU slot from a machine-wide pool.
When a tool enters a parallel section (e.g., detailed routing, global
placement), it temporarily acquires additional slots up to the
configured thread count. When the parallel section completes, the
extra slots are released back to the pool for other instances to use.

The coordination uses per-CPU file locks in `/tmp/openroad_cpu_sem/`.
The total pool size equals the number of hardware threads
(`std::thread::hardware_concurrency()`). If a process crashes, the OS
automatically releases its locks — no stale state accumulates.

## Why this exists

Bazel (and other parallel build systems) may launch many OpenROAD
instances simultaneously. Without coordination, each instance creates
its own thread pool, leading to severe CPU oversubscription. For
example, 48 tests each requesting 4 threads on a 48-core machine
creates 192 active threads competing for 48 cores. The resulting
context-switch overhead slows everything down.

With throttle enabled (the default), total active threads across all
OpenROAD instances never exceeds the available cores.

## Scope

There are more advanced approaches to CPU resource management (e.g.,
cgroups, container-level CPU limits, workload-aware schedulers). This
implementation focuses on solving the immediate problem: preventing
CPU overcommit when running `bazelisk test ...` in ORFS and
lightening the burden on current CI infrastructure without requiring
additional configuration. No plans beyond that.

## Disabling (debug only)

```
openroad -disable_throttle ...
```

This is intended only for debugging, such as tracking down
unintentional non-determinism where throttle-induced scheduling
differences might be a variable to eliminate.

## Monitoring

To observe throttle behavior during a build, run the monitor script
alongside your build in a separate terminal:

```
python3 etc/cpu_throttle_monitor.py --watch 1
```

This shows a live view of CPU slot utilization, including which
processes hold which slots.

To record a utilization trace for post-build analysis:

```
python3 etc/cpu_throttle_monitor.py --watch 1 --history /tmp/throttle.csv
```

The CSV records slot counts each second. Use `--json` for
machine-readable output, or `-v` for per-slot details.
