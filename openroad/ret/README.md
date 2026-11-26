# Retiming

The retiming module (`ret`) moves registers forward across combinational
gates to fix failing paths without changing the function of the design
(Leiserson and Saxe, "Retiming synchronous circuitry", Algorithmica 1991).
It runs on a globally placed design, between global placement and the
resizer, and uses the STA delays of the current netlist, including estimated
wire delays.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Retime

Slide registers forward across gates on paths that fail timing. A search
proposes the moves on a delay model calibrated to STA: gate and wire delays,
the clock-to-Q of the new register and the setup time of each endpoint. STA
then decides what is kept. The moves are applied, the new cells placed and
the design timed; they are kept when the slack improves and neither the
worst nor the total negative slack is worse than at the start, else the first
of the halves, quarters, and so on that does so is kept. The moves are listed
with the ones the delay model gains most from first, so a half keeps the best
of them. Rounds repeat from what was kept until one keeps nothing. A design
that meets timing is left as it is.

Only the register groups whose edge weights changed are rebuilt; every
other cell keeps its placement. Each new cell is placed at the center of the
placed cells it connects to, for detailed placement to legalize, and a new
register with negative slack takes the best of its equivalent cells.
Estimated parasitics of the nets that changed are refreshed. Run
`retime` before clock tree synthesis and routing: on a routed design it
warns and leaves the routing as it is.

Registers move by class, read from the liberty `next_state` function:

- A plain register is rebuilt as the library's base D flip-flop.
- An enable register keeps its cell and enable net when every register a
  move merges is the same cell on the same enable net.
- An inverting register (its loads see the complement of D) keeps its cell
  across a self-dual gate such as an inverter, a majority gate or a full
  adder.
- Any other merge first unmaps the registers involved: each becomes the
  base flip-flop plus logic (inverters, 2-input ANDs and ORs, tie cells)
  computing its `next_state`, named `unmap_*`.

Latches, scan cells, multi-bit and negative-edge registers, registers
clocked by more than one pin, registers with a live asynchronous set or
reset, registers named by `set_false_path` or `set_multicycle_path`, and
registers with a constraint on one of their pins never move. Neither does a
register chain whose output reaches a primary output, a bidirectional pin or
a control pin of another register (those read the chain at its current
depth), nor anything marked don't-touch or read by a don't-touch cell. A register whose set or reset is tied inactive moves as
the base flip-flop. Registers fed by one net are rebuilt together on one
clock net, so a register also stays when it shares its input net with one of
these or with a register on another clock net. A gate fed through such a
register, connected to a primary input or output, driving a control pin of
a register, or reachable by registers on different clock nets is never
crossed. In a hierarchical netlist (`link_design -hier`) a register stays
inside its module. New cells never use a `dont_use` cell; tie cells ignore
`set_dont_use`, as `repair_tie_fanout` does. The design must have one clock
defined with `create_clock`; gated copies of it are fine.

```tcl
retime
    [-max_moves count]
    [-margin fraction]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_moves` | Stop after this many accepted register moves. `0` means unlimited. Default unlimited. |
| `-margin` | The slack a move must gain, as a fraction of the clock period. Default `0.002`. |

New registers are named `ret_ff_N`, their tie cells `ret_tie_N` and new
nets `ret_net_N`. The log shows the slack after each round. `retime` stops
with an error on designs it cannot handle: libraries without a usable base
register, inverter, AND2 or OR2 cell; more than one clock; no clock defined.
An error leaves the netlist unchanged.

## Example scripts

```
./test/retime_basic.tcl
./test/retime_unmap.tcl
./test/retime_slack_search.tcl
./test/retime_place_size.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following command:

```shell
bazelisk test //openroad/ret/...
```

## Limitations

`retime` moves registers forward only, never backward across a gate, and
works on one clock domain. The path into a register's enable pin is not in
the delay model. Initial values of moved registers are not computed: the
result is equivalent from the reset state only when the moved registers have
no defined initial value, so verify the result with sequential equivalence
checking. A scan cell is one with a liberty `test_cell` or a LEF `USE SCAN`
pin; a scan flip-flop its library marks neither way, such as Nangate45
`SDFF_X1`, can be unmapped, which removes the scan cell. A path delay or a
path group that names a moved register (for example `-to [all_registers]`)
keeps naming only the registers that stay; read the SDC again after
`retime`, as ORFS does at every stage. STA judges the timing at the time of
`retime`; the resizer and detailed placement that run afterwards can change
it.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
