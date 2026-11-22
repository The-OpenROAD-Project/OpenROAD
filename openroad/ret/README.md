# Retiming

The retiming module (`ret`) moves registers forward across combinational
gates to shorten the worst register-to-register path without changing the
function of the design (Leiserson and Saxe, "Retiming synchronous
circuitry", Algorithmica 1991). The search is driven by the STA delays of
the current netlist, placed or not, including estimated wire delays.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Retime

Slide registers forward across gates while the worst register-to-register
arrival improves. Each accepted move is checked with an exact recompute of
the delay model, which reads gate arcs and wire delays from STA. Paths that
start at a primary input, at a gate driving an output, or at a fenced gate
cannot be shortened by a forward move; they are constraints instead, and a
move that pushes one past the clock period is rejected.

Only the register groups whose edge weights changed are rebuilt; every
other cell keeps its placement. A group is the registers of one class that
start on one net. The new registers are left unplaced: run
`global_placement -incremental` after `retime` to place them. Estimated
parasitics of the nets that changed are refreshed. Run `retime` before clock
tree synthesis and routing: on a routed design it warns and leaves the
routing as it is.

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

A register whose asynchronous set or reset is tied to its inactive value
moves like one without it: it is rebuilt as the base flip-flop, and a tie
cell that only it read is removed with it. When the libraries have no
flip-flop without asynchronous controls, the base cell is the register that
takes the least area once tie cells hold its controls inactive, and every
new register gets its own tie cells.

Registers with a live asynchronous set or reset, latches and scan cells never
move, and neither does a register chain whose output reaches a primary
output or a control pin of another register (those read the chain at its
current depth), nor anything marked don't-touch. A register also stays when
it shares its input net with one of these or with a register on another
clock net. A gate fed through such a register or driving such a chain,
connected to a primary input or output, reading a register that no gate
drives (one fed straight from a primary input), driving a control pin of a
register, or reachable by registers on different clock nets is never crossed.
The design must have one clock defined with `create_clock`; gated copies of
it are fine.

Hierarchical netlists (`link_design -hier`) have initial support: retiming is
local to a module. A register stays inside its module: a gate is crossed
only when the gate that drives its registers, the registers, the gate itself
and the gates it drives are all in one module. The cells that replace an
unmapped register are created in its module. A flat netlist has no
boundaries to keep.

```tcl
retime
    [-max_moves count]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_moves` | Stop after this many accepted register moves. Default unlimited. |

New registers are named `ret_ff_N`, their tie cells `ret_tie_N` and new nets
`ret_net_N`. The log shows one line per accepted move. `retime` stops with
an error on designs it cannot handle: libraries without a usable base
register, inverter, AND2 or OR2 cell; more than one clock; negative-edge
registers; no clock defined. An error leaves the netlist unchanged.

## Experimental: slack search with an STA check

A second search, for a placed design between global placement and the
resizer, is behind debug switches while it is evaluated:

```tcl
set_debug_level RET slack_search 1
set_debug_level RET sta_check 1
set_debug_level RET place_size 1
retime
```

| Switch | Description |
| ----- | ----- |
| `slack_search 1` | Take moves by slack, worst failing spot first. A path starts at what its register launches, gate arcs are calibrated to STA and an endpoint is charged its setup time. A move must raise the slack through its spot by more than 0.2% of the clock period without lowering the total negative slack; when none is left, moves that raise the total without lowering the worst slack are taken. |
| `sta_check 1` | Let STA decide what is kept. The moves are applied, the new cells placed and the design timed. The moves are kept when neither the worst nor the total negative slack got worse; else the first half of them is tried, then a quarter, and so on. Short rounds of 16 moves follow, each from what was kept, until a round keeps nothing or 16 rounds have kept moves. |
| `place_size 1` | For each new register with negative slack, try the three fastest cells of the same function and keep the one STA prefers. The slack search then prices a new register with its fastest cell. |
| `why 1` | Report where the failing endpoints of STA are and what stops a move on the worst path into each failing endpoint of the delay model. |

Under `sta_check` each new cell is placed at the center of the placed cells
on its data and output nets and left for detailed placement to legalize;
`global_placement -incremental` is not needed. `-max_moves` limits the
moves of the whole run.

## Example scripts

```
./test/retime_basic.tcl
./test/retime_unmap.tcl
./test/retime_slack_search.tcl
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
checking.

In a hierarchical netlist a register does not cross a module boundary, so a
path that runs through another module, such as an arithmetic operator kept
as its own module, is not shortened. A tie cell in another module than the
register it held is left in place when that register is removed.

The STA check guards the timing at the time of `retime`; the resizer and
detailed placement that run afterwards can still make it worse than without
retiming. The delay model of the slack search counts one endpoint for a gate
however many registers it feeds, so its total negative slack underweights
gates that feed many registers.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
