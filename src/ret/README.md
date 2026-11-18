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
other cell keeps its placement. The new registers are left unplaced: run
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

Registers with asynchronous set or reset, latches and scan cells never
move, and neither does a register chain whose output reaches a primary
output or a control pin of another register (those read the chain at its
current depth), nor anything marked don't-touch. Registers fed by one net
are rebuilt together on one clock net, so a register also stays when it
shares its input net with one of these or with a register on another clock
net. A gate fed through such a register, connected to a primary input or
output, driving a control pin of a register, or reachable by registers on
different clock nets is never crossed. The design must have one clock
defined with `create_clock`; gated copies of it are fine.

In a hierarchical netlist (`link_design -hier`) a register stays inside its
module: a gate is crossed only when the gate that drives its registers, the
registers, the gate itself and the gates it drives are all in one module.
The cells that replace an unmapped register are created in its module. A
flat netlist has no boundaries to keep.

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
OR2 cell; more than one clock; negative-edge registers; no clock defined. An
error leaves the netlist unchanged.

## Example scripts

```
./test/retime_basic.tcl
./test/retime_unmap.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following command:

```shell
bazelisk test //src/ret/...
```

## Limitations

`retime` moves registers forward only, never backward across a gate, and
works on one clock domain. In a hierarchical netlist it does not move a
register across a module boundary. The path into a register's
enable pin is not in the delay model. Initial values of moved registers are
not computed: the result is equivalent from the reset state only when the
moved registers have no defined initial value, so verify the result with
sequential equivalence checking.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
