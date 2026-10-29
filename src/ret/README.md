# Retiming

The retiming module (`ret`) will host register retiming for OpenROAD. This
first piece is the normalization step every retimer needs: rewriting the
registers of a design as plain D flip-flops plus explicit logic, so that
every movable register is the same cell with the same (empty) control set.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

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
new cells are not placed; run placement afterwards.

## Example scripts

```
./test/unmap_basic.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

- Asynchronous set/reset registers are not unmapped.
- Latches are not unmapped.
- A `next_state` function that needs a cell the library does not provide
  (for example a constant without a tie cell) leaves that register untouched.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
