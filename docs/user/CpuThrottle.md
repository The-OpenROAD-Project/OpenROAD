# CPU Throttle

OpenROAD automatically coordinates CPU usage across parallel instances
to prevent overcommitting cores. This requires zero configuration.

## How it works

When `setThreadCount(N)` is called, OpenROAD acquires N CPU slots from
a machine-wide pool before proceeding. If not enough slots are
available (because other OpenROAD instances are using them), it waits
until they are freed.

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

## Disabling (debug only)

```
openroad -disable_throttle ...
```

This is intended only for debugging, such as tracking down
unintentional non-determinism where throttle-induced scheduling
differences might be a variable to eliminate.
