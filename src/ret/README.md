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
other cell keeps its placement. New registers are placed at the centroid of
the registers they replace and legalized only when parasitics come from a
routing, as the resizer does; otherwise run `detailed_placement` afterwards.
Estimated parasitics of the nets that changed are refreshed.

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

Registers with asynchronous set or reset, latches and scan cells never
move, and neither does a register chain whose output reaches a primary
output or a control pin of another register (those read the chain at its
current depth), nor anything marked don't-touch. A gate fed through such a
register, connected to a primary input or output, or reachable by registers
on different clock nets is never crossed. The design must have one clock
defined with `create_clock`; gated copies of it are fine.

```tcl
retime
    [-max_moves count]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_moves` | Stop after this many accepted register moves. Default unlimited. |

New instances are named `ret_ff_N` and new nets `ret_net_N`. The log shows
one line per accepted move. `retime` stops with an error on designs it
cannot handle: libraries without a usable base register, inverter, AND2 or
OR2 cell; more than one clock; negative-edge registers; no clock defined; a
hierarchical netlist (`link_design -hier`).

## Example scripts

```
./test/retime_basic.tcl
./test/retime_unmap.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

- Forward moves only; a register never moves backward across a gate.
- One clock domain.
- Flat netlists only.
- The path into an enable pin is not in the delay model.
- Registers of different cells on the same enable net are different
  classes and are unmapped when merged; so is an inverting register
  crossing a gate that is not self-dual.
- A register unmapped on demand stays unmapped even when the search then
  finds no improving move.
- The search restarts from the first gate after every accepted move and
  recomputes the whole delay model for every trial, including rejected
  ones.
- The cone estimate that pre-filters candidates costs the fanin gates of a
  slide with their pre-slide arcs, so it can skip a move the exact recompute
  would accept.
- Initial values of moved registers are not computed: the result is
  equivalent from the reset state only when the moved registers have no
  defined initial value. Verify with sequential equivalence checking.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
