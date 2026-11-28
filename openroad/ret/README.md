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
proposes the moves on a delay model calibrated to STA, and STA decides what
is kept: the moves are applied, the new cells placed and the design timed,
and they are kept when the setup slack improves and neither the worst nor
the total negative slack gets worse, for setup and for hold, in any scene.
Else the first of the halves, quarters, and so on that does so is kept.
Moves that are not kept are undone and leave the netlist and its constraints
as they were. Rounds repeat until one keeps nothing. A design that meets
timing is left as it is.

Only the register groups whose edge weights changed are rebuilt; every
other cell keeps its placement. Each new cell is placed at the center of the
bounding box of the placed cells it connects to, for detailed placement to
legalize, and takes the supply nets of the cell it replaces. A new register
with negative slack takes the best of its equivalent cells. Run `retime`
before clock tree synthesis and routing: a design with routing, SPEF
parasitics or annotated delays is left as it is, with a warning.

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

Latches, scan cells, negative-edge registers, registers with a live
asynchronous set or reset, and anything marked don't-touch or fixed never
move, and neither does a register chain whose output reaches a primary
output or a control pin of another register (those read the chain at its
current depth). Registers the SDC names stay too, except those it names only
as cells in a path delay or a path group when `-move_sdc_registers` is
given. Registers fed by one net are rebuilt together on one clock net, so a
register also stays when it shares its input net with one of these or with
a register on another clock net. A gate fed through such a register,
connected to a primary input or output, driving a control pin of a register,
or reachable by registers on different clock nets is never crossed, and only
plain combinational gates are. The design must have one clock defined with
`create_clock`; gated copies of it are fine.

```tcl
retime
    [-max_moves count]
    [-margin fraction]
    [-move_sdc_registers]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_moves` | Stop after this many accepted register moves. `0` means unlimited. Default unlimited. |
| `-margin` | The slack a move must gain, as a fraction of the clock period. Default `0.002`. |
| `-move_sdc_registers` | Also move registers that the SDC names as cells in a path delay or a path group, as `-to [all_registers]` does. Whatever names a register that moves is dropped with it; see Limitations. |

New registers are named `ret_ff_N`, their tie cells `ret_tie_N` and new
nets `ret_net_N`. The log shows the slack after each round. `retime` stops
with an error on designs it cannot handle: libraries without a usable base
register, inverter, AND2 or OR2 cell; more than one clock; no clock defined.
An error leaves the netlist and its constraints unchanged.

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
works on one clock domain. Initial values of moved registers are not
computed: the result is equivalent from the reset state only when the moved
registers have no defined initial value, so verify the result with
sequential equivalence checking.

With `-move_sdc_registers`, a register that moves leaves the SDC lists that
named it, and a new register is in none of them: run the commands that build
those lists again after `retime`. With several scenes the search reads the
current one, and STA judges all of them.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
