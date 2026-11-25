# Rebuffer reference for buffer_varying_lengths

Reference minimum-delay solutions for the `buffer_varying_lengths` tests,
used to measure how far `rsz::fully_rebuffer` is from the best buffering it
could find under tight timing:

| File | Test |
| --- | --- |
| `reference_nangate45_fixed.txt` | `buffer_varying_lengths` (driver fixed at BUF_X1) |
| `reference_nangate45_size_driver.txt` | `buffer_varying_lengths_size_driver` (driver sized) |
| `reference_asap7_fixed.txt` | `buffer_varying_lengths_asap7` (driver fixed at BUFx2) |
| `reference_asap7_size_driver.txt` | `buffer_varying_lengths_asap7_size_driver` (driver sized) |

Each line is one wire length (index i, length i times the PDK's step: 25 um
on nangate45, 3.75 um on asap7): the index, the path delay in ps as the test
reports it (input external delay to the load output), the driver cell, and
the buffer chain as `{cell x}` pairs in DBU, driver side first. The PDK
settings, shared with the tests, are in `../buffer_varying_lengths_pdk.tcl`.

## Comparing rsz with the reference

```
./run_reference.py --openroad <path to openroad> [--pdk asap7] [--size-driver]
```

This runs the test's tight-timing step with that binary and prints rsz's
delay, the reference delay, and the gap for every length, followed by the
mean and maximum gap.

## Regenerating the reference

```
./run_reference.py --openroad <path to openroad> [--pdk asap7] [--size-driver] --search
```

The search takes about 15 minutes per case. It times every candidate with
OpenSTA under the test's constraints:

1. `rsz_solution.tcl` records rsz's own solution (it sources the test with
   `buffer_varying_lengths_tight_only` set).
2. `sweep.tcl` times evenly spaced chains of one buffer size for every
   length (and driver size, with `--size-driver`).
3. `search.tcl` seeds each length with rsz's solution, the best chains,
   and chains starting with a buffer at the driver. It then improves every
   path by coordinate descent: move a buffer by +/-step (about two length
   steps down to 1/128 of one) or resize a buffer, or the driver with
   `--size-driver`, by one drive strength, keeping changes that reduce the
   delay.

The search is local, so the reference is an upper bound on the optimum.
Regenerate it when the test setup, liberty, or the timing engine changes,
and keep whichever of rsz and the reference is faster.
