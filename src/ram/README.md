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

The options marked **Experimental.** below, and more than one write port, were added for register files. They are a baseline to build on: their interface may change, and they may move to a command of their own, such as a `generate_register`, with these `generate_ram` options retired once flows use it. A run that uses any of them names them in RAM-0105, and `-tracks` in RAM-0106.

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
             [-read_style tristate|and_or]
             [-zero_word word]
             [-reset_value value]
             [-banks count]
             [-bank_columns count]
             [-bank_order contiguous|interleaved]
             [-rb_ports count]
             [-register_read_address 0|1]
             [-write_style clock_gate|mux]
             [-write_priority last|or]
             [-service_columns {every sites}]
             [-bank_column_layout interleaved|contiguous]
             [-cells {role master ...}]
             [-name name]
             [-decoder row|word]
             [-bit_folds count]
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
             [-periphery placed|verilog]
             [-wrapper_verilog file]
             [-tracks file]
```

#### Options

| Switch Name | Description | 
| ---------------------- | -------------------------------------- |
| `-mask_size` | Determines the number of bits which are grouped together for masking during writes. For example, a mask size of `8` will enable each 8 bits of the word to be masked when writing (commonly known as byte masking). A mask size of `1` will enable each bit to be individually masked. The write enable signal for each port will be `(word_size / mask_size)` bits wide. The word size must be a multiple of the mask size. Default: equal to `-word_size`. |
| `-word_size` | Size of each word in bits. |
| `-num_words` | Number of words in the array. |
| `-rw_ports` | Number of read-write ports for the array: 0 or 1, and a read-write port is the only write port. Default: 1. |
| `-r_ports` | Number of read ports for the array. Default: 0. |
| `-w_ports` | Number of write ports for the array. With more than one, each has its own address decoder, data (`D0`, `D1`, ...), write enables (`we0`, ...) and address (`addr_w0`, ...); a word two ports write in the same cycle keeps the last port's data. More than one write port is **experimental**. Not yet with `-use_latch` or `-column_mux_ratio` above 1. Default: 0. |
| `-column_mux_ratio` | Number of words sharing a physical column. Must be `1`, `2`, or `4`. The number of words must be divisible by the column mux ratio. Default: `1` (no column muxing). |
| `-storage_cell` | Name of the master to use for the storage device (i.e. a flip-flop). Must be positive-edge triggered. Default: auto-select from the loaded cell library. |
| `-tristate_cell` | Name of the master to use for the tristate device (i.e. a tristate inverter). It is currently assumed that the device is inverting. Default: auto-select from the loaded cell library. |
| `-read_style` | **Experimental.** The read path: `tristate`, a tristate bitline per bit, or `and_or`, a tree per bit column of an AOI22 over each pair of rows and NAND2 and NOR2 levels above, for a library without a tristate cell. `and_or` also takes a storage flop whose only output is inverted (QN). Default: `tristate` where the library has a tristate cell, `and_or` where it has none. |
| `-zero_word` | **Experimental.** A word that reads 0 and drops its writes, as a RISC-V register file's x0: its row has no flops or clock gate, and the read trees leave it out. Needs `-read_style and_or` and `-column_mux_ratio 1`. Default: none. |
| `-reset_value` | **Experimental.** Resets every word to `value`, decimal, `0x` hexadecimal or `0` octal and no wider than the word, asynchronously, through a reset port at the storage flop's own polarity (`rst_n` or `rst`); each bit's flop is cleared or preset, the other pin held off. The storage cell has both an asynchronous clear and preset, also for a value of 0. A parent adapts a reset of another polarity, or a synchronous one, to it. Not with `-use_latch`. Default: no reset. |
| `-banks`, `-bank_columns`, `-bank_order` | **Experimental.** The words in `banks` banks, contiguous or interleaved in the address, `bank_columns` of them side by side and the rest stacked. With banks each port decodes every word's whole address, so a bank need not be a power of two words. Not with `-column_mux_ratio` above 1 or `-use_latch`. Default: one bank. |
| `-rb_ports` | **Experimental.** Banked read ports, with `-banks`: each an address and a data output per bank (`addr_rb<k>_b<b>`, `Qrb<k>_b<b>`), the address the word's index in its bank, every bank read independently. The block has no mux across the banks: selecting a bank's data, and registering the selection, is the parent's. Default: 0. |
| `-register_read_address` | **Experimental.** `1` registers every read port's address, a plain positive-edge flop per address bit, so the decoders and read trees come after the flop and the data follows a cycle after the address. This is the read address register, not a register on the read data. A read whose address is registered on the clock that writes its word returns the new data (write-first). 0 and 1 only. Not with `-column_mux_ratio` above 1 or read/write ports. Default: 0. |
| `-write_style` | **Experimental.** How a word holds its value between writes: `clock_gate`, a clock gate per word; or `mux`, every flop on the clock and a mux per bit taking the new data or the flop's own value. A clock gate saves a mux per bit; a mux keeps every flop on the plain clock, so every flop is clocked every cycle and clock power grows with the block's bits, not the words written. Not with `-use_latch`. Default: `clock_gate`. |
| `-write_priority` | **Experimental.** Where two write ports write the same word in one cycle: `last`, the last port's data; or `or`, the OR of every writing port's data, as RTL that merges its writes with an OR describes it. `or` drops a word's priority logic, an inverter and an AND2 per port but the last. Default: `last`. |
| `-service_columns {every sites}` | **Experimental.** An empty column `sites` sites wide after every `every` columns of the array, its bits' and selects' alike: room inside the array for the buffers a parent flow inserts on the long nets that cross it, where the array is otherwise full. Default: none. |
| `-bank_column_layout interleaved\|contiguous` | **Experimental.** With banks, how a row's words share its columns: `interleaved`, bit by bit as the column mux lays them out; or `contiguous`, each word's bits side by side with its select column after them. Contiguous keeps a word's selects and write enables as short as its own bits, where interleaved runs them the whole row; the read trees and each write port's data nets then cross the row instead. Default: `interleaved`. |
| `-periphery`, `-pin_layer`, `-wrapper_verilog` | **Experimental.** `unplaced` and `-pin_layer` are `generate_ram_netlist`'s only, for a flow that places the periphery itself; `generate_ram` takes `placed` and `verilog`, routes the core and puts its pins on its edges. `unplaced` leaves the periphery (address inverters and registers, decoders, select buffers, each word's select cell, the input buffers) for a parent to place, size and buffer; the outline is the placed core and a row, and each pin sits where its connections land, on `-pin_layer`'s tracks, for a macro placer before the parent dissolves the core into its rows. `verilog` makes the core a block of its own, `<name>_core`, and writes the periphery to `-wrapper_verilog` as a Verilog module, `<name>`, around it: assigns of the cells' functions, for synthesis to map with the parent. Default: `placed`. |
| `-cells` | **Experimental.** The cell for a role (`inv`, `and2`, `buffer`, `clock_gate`, `aoi22`, `ao22`, `or2`, `nand2`, `nor2`, `tie`, `addr_flop`), instead of the smallest the libraries have of its function. An AOI22 or AO22 given is checked by its truth table. Default: the smallest of each. |
| `-name` | **Experimental.** The block's and the behavioral model's name. Default: `RAM<words>x<bits>`. |
| `-decoder` | **Experimental.** `row`: a decoder per row, an AND2 chain over the row address, the column mux selecting within the row. `word`: a balanced AND2 tree over each word's whole address, as with banks; shallower for the read path. Default: `row`. |
| `-bit_folds` | **Experimental.** A word in `count` rows: its bits in slices of `-mask_size` (the word divided by `count`, rounded up; the last slice may be short), one slice a row, so a wide word with many ports is not a long thin strip. A folded word is written whole, one write enable a port, its write logic built once and shared by its rows. Not with `-use_latch` or the row decoder's `-column_mux_ratio` above 1. Default: 1. |
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
| `-tracks` | **Experimental.** `generate_ram` only. A Tcl file of `make_tracks` commands, sourced in place of `make_tracks` with the technology LEF's offsets, for a technology whose offsets do not fit the block (asap7's M2 offset is negative). Default: the LEF's tracks. |

## Example scripts

See [test/make_8x8_sky130.tcl](test/make_8x8_sky130.tcl).

## Regression tests

There are a set of regression tests in `./test`. Refer to this [section](../../README.md#regression-tests) for more information.

Simply run the following command:

```shell
bazelisk test //src/ram/...
```

## Limitations

This is an experimental module and many basic features may not work yet. The register-file options are experimental on their own account, see [RAM Generation](#ram-generation). Please see [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392) for ongoing and future work.

## Authors

- Matt Liberty ([@maliberty](https://github.com/maliberty))
- Brayden Louie ([@braydenlouie](https://github.com/braydenlouie)) and Thinh Nguyen ([@tnguy19](https://github.com/tnguy19/)), advised by Austin Rovinski ([@rovinski](https://github.com/rovinski/))

## References
Inspired by the [DFFRAM](https://github.com/AUCOHL/DFFRAM) project from AUCOHL.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
