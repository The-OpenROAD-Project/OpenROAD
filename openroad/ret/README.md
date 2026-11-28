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
the design timed; they are kept when the setup slack improves and neither
the worst nor the total negative slack is worse than at the start of the
round or of the command, for setup and for hold, in any scene (corner or
mode). Else the first of the halves, quarters, and so on that does so is
kept. The moves are listed with the ones the delay model gains most from
first, so a half keeps the best of them. Moves that are not kept are undone
and leave the netlist and its constraints as they were. Rounds repeat from
what was kept until one keeps nothing. A design that meets timing is left as
it is.

Only the register groups whose edge weights changed are rebuilt; every
other cell keeps its placement. Each new cell is placed at the center of the
bounding box of the placed cells it connects to, for detailed placement to
legalize, and a new register with negative slack takes the best of its
equivalent cells. Estimated parasitics of the nets that changed are
refreshed. A new cell takes the supply nets of the cell it replaces: a
supply pin the net of the pin of that name, and a pin with no such name the
net of the pins of its kind, power or ground. `global_connect` connects any
that remain. Run `retime` before
clock tree synthesis and routing. A design is left as it is, with a warning,
when it has routing, route guides or special wires on a net that is not a
supply, parasitics read from SPEF, or annotated delays (`read_sdf`,
`set_assigned_delay`): STA cannot bring those back for a move that is undone.

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

These registers never move:

- latches, scan cells, multi-bit and negative-edge registers, and registers
  clocked by more than one pin or with a second pin marked as a clock (LEF
  `USE CLOCK`, liberty `clock : true`);
- registers with a tri-state output (liberty `three_state`), registers of a
  cell whose LEF pins are not its liberty pins, and registers whose power
  pins, or ground pins, are on different nets when a cell `retime` creates
  has a supply pin of another name;
- registers with a live asynchronous set or reset, with an input left
  unconnected or on a net with a second driver (a tri-state bus), or with a
  data input that is a constant (a tie cell, case analysis) or that a
  primary input drives with no gate in between;
- registers marked don't-touch, fixed in place, in a group or a region (a
  power domain, for example), in a scan chain, with a pin on a don't-touch
  net, driving a net with a non-default routing rule, or read by a
  don't-touch cell;
- registers named by `set_false_path`, `set_multicycle_path` or
  `set_path_margin`, with a constraint on one of their pins or on a net
  they drive, with a timing edge that `set_disable_timing` disables, or with
  the `-reference_pin` of a port delay;
- registers the SDC names as cells: in the `-from` or `-to` of
  `set_max_delay`, `set_min_delay` or `group_path`, or in a constraint on
  the cell such as a derate, unless `-move_sdc_registers` is given;
- a register chain whose output reaches a primary output, a bidirectional
  pin, a control pin of another register or a net with a second driver
  (those read the chain at its current depth), one that meets a second
  chain from the same gate in a register with two data pins, and a run of
  more than 256 registers with no gate in between.

Registers that one net feeds and gates read are rebuilt together on one
clock net and the same supply nets, so such a register also stays when
another of them stays, is on another clock net or has a supply pin on
another net. In a hierarchical netlist (`link_design -hier`) a register
stays inside its module.

These gates are never crossed:

- a gate fed through a register that stays, connected to a primary input or
  output, driving a control pin of a register, or reachable by registers on
  different clock nets or different supply nets;
- a gate that drives or reads a net with a second driver, and a cell that
  is not a plain combinational gate: a tri-state driver, an isolation cell,
  a level shifter, a macro, a pad, or a cell without a liberty function;
- a gate marked don't-touch, driving a don't-touch net or a net with a
  non-default routing rule, in a group or a region, named by any of
  those exceptions, with a constraint on one of its pins or on the net it
  drives (case analysis, for example), or with a timing edge that
  `set_disable_timing` disables; `-move_sdc_registers` changes none of
  this.

A register whose set or reset is tied inactive moves as the base flip-flop,
and the tie cells only it read are removed unless one of the reasons above
holds them in place. New cells never use a cell whose LEF pins are not its
liberty pins, a `dont_use` cell or a cell that would be left with an open
input; an enable or inverting register of one of the last two is unmapped
when it moves. Tie cells ignore `set_dont_use`, as `repair_tie_fanout` does.
The design must have one clock defined with `create_clock`; gated copies of
it are fine.

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
| `-move_sdc_registers` | Also move registers that the SDC names as cells in the `-from` or `-to` of a path delay or a path group, or in a constraint on the cell. Whatever names a register that moves is dropped with it. Meant for an SDC that lists every register, as `-to [all_registers]` does; see Limitations. |

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
sequential equivalence checking. A scan cell is one with a liberty
`test_cell` or a LEF `USE SCAN` pin; a scan flip-flop its library marks
neither way, such as Nangate45 `SDFF_X1`, can be unmapped, which removes the
scan cell.

An SDC that lists every register (`group_path -to [all_registers]`, or the
path delays of the asap7 platform constraints) keeps every register in place
unless `-move_sdc_registers` is given. With that option a register that
moves leaves those lists, and a constraint on that cell goes with it. A new
register is in none of them, so the paths that start or end at it are timed
without that path delay, and `retime` judges its moves without it: a move
can lengthen a path from an input to a new register past a `set_max_delay`
that `retime` does not see. Run the commands that build the lists again
after `retime`, on the retimed netlist. Reading a file that `write_sdc`
wrote does not do that: it lists the cells by name.

`retime` deletes the cells and nets it replaced when it returns. Undoing it
afterwards through an ECO opened before the command (`dbDatabase::beginEco`)
brings them back without the SDC constraints that named them.

STA judges the timing at the time of `retime`, setup and hold, with the
ideal clocks of that stage; the resizer, clock tree synthesis and detailed
placement that run afterwards can change it. With several scenes the search
reads the delays, the slacks and the clock of the current one, and STA
judges all of them: nothing is proposed for a scene that fails while the
current one meets timing, and `retime` says so. `set_scene` makes a scene
the current one.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
