# RAM Generator

⚠️ **This is an experimental module currently under development. See [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392).** ⚠️

The Random Access Memory generator module in OpenROAD (`ram`) is inspired by the [DFFRAM](https://github.com/AUCOHL/DFFRAM) project from AUCOHL.
This module is designed to create dense memory arrays from standard cells when other memory compilers are not available.
Given a standard cell library with the required basic cells, the generator can produce a placed and routed RAM block.
The generated ram can be checked using the built-in static timing analyzer rather than SPICE simulation, leading to faster turnaround time.

**Major Features**:
- Support for 1rw memories
- Arbitrary word size and number of words (although timing must be checked)
- Arbitrary word mask granularity
- Generated behavioral Verilog model for fast, portable simulation

**Tested Platforms**:
- sky130hd

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
generate_ram -mask_size bits
             -word_size bits
             -num_words words
             [-read_ports count]
             [-storage_cell name]
             [-tristate_cell name]
             [-inv_cell name]
             -power_pin name
             -ground_pin name
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
| ----- | ----- |
| `-mask_size` | Determines the number of bits which are grouped together for masking during writes. For example, a mask size of `8` will enable each 8 bits of the word to be masked when writing (commonly known as byte masking). A mask size of `1` will enable each bit to be individually masked. The write enable signal for each port will be `(word_size / mask_size)` bits wide. The word size must be a multiple of the mask size. |
| `-word_size` | Size of each word in bits. |
| `-num_words` | Number of words in the array. |
| `-read_ports` | Number of read ports for the array. Default: 1. |
| `-storage_cell` | Name of the master to use for the storage device (i.e. a flip-flop). Must be positive-edge triggered. Default: auto-select from the loaded cell library. |
| `-tristate_cell` | Name of the master to use for the tristate device (i.e. a tristate inverter). It is currently assumed that the device is inverting. Default: auto-select from the loaded cell library. |
| `-inv_cell` | Name of the master to use for inverters. Default: auto-select from the loaded cell library. |
| `-power_pin` | Name of the power pin in each standard cell used. Only one name is currently supported. |
| `-ground_pin` | Name of the ground pin in each standard cell used. Only one name is currently supported. |
| `-routing_layer` | A list of the metal layer and metal width (in microns) for generating standard cell power tracks (followpins). Example: `{met1 0.48}`. |
| `-ver_layer` | A list of the metal layer, metal width (in microns), and metal pitch (in microns) for generating power grid stripes on the first vertical layer above the followpin layer. Example: `{met2 0.48 40}`. North/South I/O pins are also created in this layer. |
| `-hor_layer` | A list of the metal layer, metal width (in microns), and metal pitch (in microns) for generating power grid stripes on the first horizontal layer above the followpin layer. Example: `{met3 0.48 40}`. East/West I/O pins are also created in this layer. |
| `-filler_cells` | A list of filler cells to use. Example: `{FILL_X1 FILL_X2 FILL_X4}`. |
| `-tapcell` | The name of the tapcell master to insert into the grid to obey latchup requirements. Requires `-max_tap_dist`. If this argument is not provided, no tapcells will be inserted. |
| `-max_tap_dist` | The distance (in microns) that tapcells should be placed apart. Requires `-tapcell`. |
| `-write_behavioral_verilog` | Write a behavioral Verilog model of the RAM array to the specified file. |

## Example scripts

See [test/make_8x8.tcl](test/make_8x8.tcl).

## Regression tests

There are a set of regression tests in `./test`. Refer to this [section](../../README.md#regression-tests) for more information.

Simply run the following script: 

```shell
./test/regression
```

## Limitations

This is an experimental module and many basic features may not work yet. Please see [#9392](https://github.com/The-OpenROAD-Project/OpenROAD/issues/9392) for ongoing and future work.

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+ram+in%3Atitle)
about this tool.

## Authors

- Matt Liberty ([@maliberty](https://github.com/maliberty))
- Brayden Louie ([@braydenlouie](https://github.com/braydenlouie)) and Thinh Nguyen ([@tnguy19](https://github.com/tnguy19/)), advised by Austin Rovinski ([@rovinski](https://github.com/rovinski/))

## References
Inspired by the [DFFRAM](https://github.com/AUCOHL/DFFRAM) project from AUCOHL.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
