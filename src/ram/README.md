# RAM Generator

⚠️ **This is an experimental module currently under development. See [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392).** ⚠️

The Random Access Memory generator module in OpenROAD (`ram`) is inspired by the [DFFRAM](https://github.com/AUCOHL/DFFRAM) project from AUCOHL.
This module is designed to create dense memory arrays from standard cells when other memory compilers are not available.
Given a standard cell library with the required basic cells, the generator can produce a placed and routed RAM block.
The generated ram can be checked using the built-in static timing analyzer rather than SPICE simulation, leading to faster turnaround time.

**Major Features**:
- Support for 1rw, 1rw1r, 1r1w, 2r1w memories
- Arbitrary word size and number of words (although timing must be checked)
- Arbitrary word mask granularity
- Generated behavioral Verilog model for fast, portable simulation

**Tested Platforms**:
- sky130hd
- Nangate45

**Planned Features**:
- See [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392)

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### RAM Generation

The `generate_ram` command is an all-in-one command to place and route a RAM.
Standard cell libraries must be loaded before running this command.
The module will create a new design, therefore a design should not be loaded before running the command.

```tcl
generate_ram [-mask_size bits]
             -word_size bits
             -num_words words
             [-rw_ports count]
             [-r_ports count]
             [-w_ports count]
             [-column_mux_ratio ratio]
             [-storage_cell name]
             [-tristate_cell name]
             [-inv_cell name]
             [-power_net_name name]
             [-ground_net_name name]
             -routing_layer config
             -ver_layer config
             -hor_layer config
             -filler_cells fillers
             [-tapcell name]
             [-max_tap_dist value]
             [-write_behavioral_verilog filename]
```

#### Options

| Switch Name | Description | 
| ---------------------- | -------------------------------------- |
| `-mask_size` | Determines the number of bits which are grouped together for masking during writes. For example, a mask size of `8` will enable each 8 bits of the word to be masked when writing (commonly known as byte masking). A mask size of `1` will enable each bit to be individually masked. The write enable signal for each port will be `(word_size / mask_size)` bits wide. The word size must be a multiple of the mask size. Default: equal to `-word_size`. |
| `-word_size` | Size of each word in bits. |
| `-num_words` | Number of words in the array. |
| `-rw_ports` | Number of read-write ports for the array. Sum of read-write ports and write ports must equal 1. Default: 1. |
| `-r_ports` | Number of read ports for the array. Default: 0. |
| `-w_ports` | Number of write ports for the array. Sum of read-write ports and write ports must equal 1. Default: 0. |
| `-column_mux_ratio` | Number of words sharing a physical column. Must be `1`, `2`, or `4`. The number of words must be divisible by the column mux ratio. Default: `1` (no column muxing). |
| `-storage_cell` | Name of the master to use for the storage device (i.e. a flip-flop). Must be positive-edge triggered. Default: auto-select from the loaded cell library. |
| `-tristate_cell` | Name of the master to use for the tristate device (i.e. a tristate inverter). It is currently assumed that the device is inverting. Default: auto-select from the loaded cell library. |
| `-inv_cell` | Name of the master to use for inverters. Default: auto-select from the loaded cell library. |
| `-routing_layer` | A list of the metal layer and metal width (in microns) for generating standard cell power tracks (followpins). Example: `{met1 0.48}`. |
| `-power_net_name` | Name of the power net to create. Default: `VDD`. |
| `-ground_net_name` | Name of the ground net to create. Default: `VSS`. |
| `-ver_layer` | A list of the metal layer, metal width (in microns), and metal pitch (in microns) for generating power grid stripes on the first vertical layer above the followpin layer. Example: `{met2 0.48 40}`. North/South I/O pins are also created in this layer. |
| `-hor_layer` | A list of the metal layer, metal width (in microns), and metal pitch (in microns) for generating power grid stripes on the first horizontal layer above the followpin layer. Example: `{met3 0.48 40}`. East/West I/O pins are also created in this layer. |
| `-filler_cells` | A list of filler cells to use. Example: `{FILL_X1 FILL_X2 FILL_X4}`. |
| `-tapcell` | The name of the tapcell master to insert into the grid to obey latchup requirements. Requires `-max_tap_dist`. If this argument is not provided, no tapcells will be inserted. |
| `-max_tap_dist` | The distance (in microns) that tapcells should be placed apart. Requires `-tapcell`. |
| `-use_latch` | If set to `1`, uses a two-phase latch-based design instead of flip-flops for the storage elements. Functionally equivalent to flip-flop design with smaller storage cell area. Default: `0`. |
| `-write_behavioral_verilog` | Write a behavioral Verilog model of the RAM array to the specified file. |

### Register File Generation

The `generate_regfile` command builds a register file, many read and
write ports over few words, as an array of placed standard cells: word
rows by bit columns, each tile a flop, its write select and an AND per
read port, the read word lines decoded in a header column and the read
bit lines as OR trees in a footer row. Every cell is placed by
construction. The block is built into the current database, whose
libraries must hold the cells the spec names.

```tcl
generate_regfile -spec file
                 [-check_ports verilog_file]
                 [-verilog file]
                 [-def file]
                 [-lef file]
                 [-liberty file]
```

#### Options

| Switch Name | Description |
| ---------------------- | -------------------------------------- |
| `-spec` | The spec file, below. |
| `-check_ports` | A Verilog file declaring the spec's module. The command stops if the module's ports are not the ports the spec names, at the widths the spec implies. |
| `-verilog` | Write the cells as structural Verilog. |
| `-def` | Write the placed block as DEF. |
| `-lef` | Write an abstract LEF of the block. |
| `-liberty` | Write a timing model; a `_pre_layout.lib` beside it has ideal clocks. |

#### Spec

One key per line, `#` comments:

| Key | Description |
| ---------------------- | -------------------------------------- |
| `module name` | The module and block name. |
| `mode macro\|netlist` | `macro` (default): the block is a macro to its parent. `netlist`: the parent places the cells; the address decode is left unplaced for the parent's placer and resizer, and the abstract's pins sit where their connections land in the array. |
| `words n`, `bits n` | Depth and width. |
| `clock port` | The clock port. |
| `reset port` | A reset port the RTL has and the array ignores. |
| `read addr data` | A read port, by the module's port names. Repeat per port. |
| `read_banked addr0 data0 addr1 data1 ...` | A read port with an address and a data port per bank. |
| `write addr data [en]` | A write port. Repeat per port. |
| `cell role master` | The cells: `flop`, `and2`, `or2`, `ao22`, `inv`, `tap`. |
| `pins role pin...` | The pin names of a cell role (`flop`, `and2`, `or2`, `ao22`, `inv`), for a library whose names differ from the defaults. |
| `flop_output Q\|QN` | Whether the flop's output is inverted. |
| `pin_layer layer`, `pin_layer_v layer` | The layers of the pins on the left and right edges, and on the top and bottom. |
| `pin_track offset pitch` | The track grid the pins are centred on, in microns. |
| `banks n`, `bank_columns n`, `bank_order contiguous\|interleaved` | Word columns side by side, how many of them stand in a row, and how words fall into them. |
| `bit_folds n` | The word split into `n` bit bands stacked one above the other. |
| `read_latency 0\|1` | `1` registers each read address on the clock first. |
| `zero_word n` | A word with no storage that reads 0 and drops writes (RISC-V x0). |
| `tap_columns n`, `service_sites n` | A tap column every `n` bit columns, and free sites beside each for the clock tree. |
| `store_name pattern`, `read_reg_name pattern` | Instance names of the flops, from `{word}`, `{bit}`, `{port}` and `{bank}`. |
| `lib knob value` | The timing model's parameters: `gate_delay_ps`, `wire_factor`, `input_load_ff`, `clock_load_ff`, `leakage_nw_per_cell`, `output_max_cap_ff`, `hold_ps`. |

## Example scripts

See [test/make_8x8_sky130.tcl](test/make_8x8_sky130.tcl) and
[test/generate_regfile_asap7.tcl](test/generate_regfile_asap7.tcl).

## Regression tests

There are a set of regression tests in `./test`. Refer to this [section](../../README.md#regression-tests) for more information.

Simply run the following script: 

```shell
./test/regression
```

## Limitations

This is an experimental module and many basic features may not work yet. Please see [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392) for ongoing and future work.

## Authors

- Matt Liberty ([@maliberty](https://github.com/maliberty))
- Brayden Louie ([@braydenlouie](https://github.com/braydenlouie)) and Thinh Nguyen ([@tnguy19](https://github.com/tnguy19/)), advised by Austin Rovinski ([@rovinski](https://github.com/rovinski/))

## References
Inspired by the [DFFRAM](https://github.com/AUCOHL/DFFRAM) project from AUCOHL.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
