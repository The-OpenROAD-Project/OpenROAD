# Rebuffer reference for buffer_varying_lengths

Reference minimum-delay solutions for the two `buffer_varying_lengths`
tests, used to measure how far `rsz::fully_rebuffer` is from the best
buffering it could find under tight timing:

| File | Test |
| --- | --- |
| `reference_fixed.txt` | `buffer_varying_lengths` (driver fixed at BUF_X1) |
| `reference_size_driver.txt` | `buffer_varying_lengths_size_driver` (driver sized) |

Each line is one wire length (index i, length 25 um * i): the index, the
path delay in ps as the test reports it (input external delay to the load
output), the driver cell, and the buffer chain as `{cell x}` pairs in DBU,
driver side first.

## Comparing rsz with the reference

```
./run_reference.py --openroad <path to openroad> [--size-driver]
```

This runs the test's tight-timing step with that binary and prints rsz's
delay, the reference delay, and the gap for every length, followed by the
mean and maximum gap.

## Regenerating the reference

```
./run_reference.py --openroad <path to openroad> [--size-driver] --search
```

The search takes about 15 minutes per case. It times every candidate with
OpenSTA under the test's constraints:

1. `rsz_solution.tcl` records rsz's own solution (it sources the test with
   `buffer_varying_lengths_tight_only` set).
2. `sweep.tcl` times evenly spaced chains of one buffer size for every
   length (and driver size, with `--size-driver`).
3. `search.tcl` seeds each length with rsz's solution, the best chains,
   and chains starting with a buffer at the driver. It then improves every
   path by coordinate descent: move a buffer by +/-step (51.2 um down to
   0.2 um) or resize a buffer, or the driver with `--size-driver`, by one
   drive strength, keeping changes that reduce the delay.

The search is local, so the reference is an upper bound on the optimum.
Regenerate it when the test setup, liberty, or the timing engine changes,
and keep whichever of rsz and the reference is faster.
