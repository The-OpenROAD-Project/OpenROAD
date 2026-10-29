# Retiming

The retiming module (`ret`) moves registers across combinational gates to
shorten the worst register-to-register path without changing the function
of the design (Leiserson and Saxe, "Retiming synchronous circuitry",
Algorithmica 1991). It has two commands: `retime`, a forward retiming
search driven by the STA delays of the current (placed or unplaced)
netlist, and `unmap_sequential_cells`, the normalization step that rewrites
registers as plain D flip-flops plus explicit logic so that every movable
register is the same cell with the same (empty) control set.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Retime

Slide registers forward across combinational gates to lower the worst
arrival at a register data pin. The search builds the retiming graph of
the design (gates are vertices, registers are edge weights), reads per-gate
arc delays and per-net wire delays from the current STA results, and
repeatedly moves one register group forward across a gate when the exact
recomputed worst arrival improves. The arc delay of a gate whose output
drives a register is recomputed from the liberty table under the register
D-pin load, so a move that changes the load of a gate is costed correctly.

The result is applied with minimal perturbation: only the register groups
whose edge weights changed are rebuilt. Every other register and every
combinational cell keeps its placement. New registers are the base D
flip-flop that `unmap_sequential_cells` would choose. They are placed at
the centroid of the registers they replace (not legalized); run
`detailed_placement` afterwards and re-estimate parasitics.

A register that moves and is not the base cell (for example an enable flop)
is first unmapped with the same logic as `unmap_sequential_cells`; only the
registers that move are unmapped. Registers with asynchronous set or reset,
latches and scan cells never move, and a gate fed through such a register
or connected to a primary input or output is never crossed. The design must
have a single clock domain and a clock defined with `create_clock`.

```tcl
retime
    [-max_moves count]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_moves` | Stop after this many accepted register moves. Default unlimited. |

New instances are named `ret_ff_N` and new nets `ret_net_N`. The log shows
one line per accepted move with the worst arrival of the delay model after
the move, the number of registers in the moved group, the number of sinks
of the output net of the gate and the maximum register count on those edges.

When no move improves the worst arrival the command reports `Retime found
no slides.` and leaves the design unchanged.

The command stops with an error on designs it cannot retime yet: libraries
without a usable base register, inverter, AND2 or OR2 cell; more than one
non-virtual clock; registers on more than one clock net; negative-edge
registers; or no clock defined.
### Unmap Sequential Cells

Rewrite every synchronous register as a base D flip-flop whose D pin is
driven by combinational logic computing the liberty `next_state` function
of the original cell. An enable becomes a mux built from one inverter, two
2-input ANDs and one 2-input OR; a synchronous set or reset becomes an OR
or an AND with a tie cell; inverted data or an inverted output becomes an
inverter. Only inverters, 2-input ANDs, 2-input ORs and tie cells are used,
so the result is an explicit Boolean form that a later retiming or
resynthesis step can reason about.

The base cells are chosen from the loaded liberty libraries: the smallest
single-bit D flip-flop without asynchronous controls on each clock edge, the
smallest inverter, 2-input AND, 2-input OR and tie cells. Registers with
asynchronous set or reset pins, latches, scan cells and multi-bit cells are
left untouched and counted in the log.

```tcl
unmap_sequential_cells
```

This command takes no arguments. New instances are named `unmap_ff_N`,
`unmap_inv_N`, `unmap_and_N`, `unmap_or_N`, `unmap_tie1_N` and
`unmap_tie0_N`; new nets are named `unmap_state_N` and `unmap_net_N`. The
new cells are placed on top of the register they replace (not legalized);
run `detailed_placement` afterwards.

## Example scripts

```
./test/retime_basic.tcl
./test/retime_unmap.tcl
./test/unmap_basic.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

- Only forward moves (a register moves from the inputs of a gate to its
  output).
- Single clock domain; multi-clock designs are not supported.
- Asynchronous set/reset registers are never moved and not unmapped.
- Latches are never moved and not unmapped.
- A register unmapped on demand stays unmapped even when the search on the
  new netlist then finds no improving move (the mux cone added in front of
  it can itself become the critical path).
- Initial values of moved registers are not computed; the retimed design is
  equivalent from the reset state only when the moved registers have no
  defined initial value. Verify with sequential equivalence checking.
- A `next_state` function that needs a cell the library does not provide
  (for example a constant without a tie cell) leaves that register untouched.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
