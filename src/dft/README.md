# DFT: Design for Testing

The Design for Testing module in OpenROAD (`dft`) is an implementation of Design For Testing.
New nets and logic are added to allow IC designs to be tested for errors in manufacturing.
Physical imperfections can cause hard failures and variability can cause timing errors.

A simple DFT insertion consist of the following parts:

* A scan_in pin where the test patterns are shifted in.
* A scan_out pin where the test patterns are read from.
* Scan cells that replace flops with registers that allow for testing.
* One or more scan chains (shift registers created from your scan cells).
* A scan_enable pin to allow your design to enter and leave the test mode.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Set DFT Config 

The command `set_dft_config` sets the DFT configuration variables.

```tcl
set_dft_config 
    [-max_length <int>]
    [-max_chains <int>]
    [-clock_mixing <string>]
    [-scan_enable_name_pattern <string>]
    [-scan_in_name_pattern <string>]
    [-scan_out_name_pattern <string>]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-max_length` | The maximum number of bits that can be in each scan chain. |
| `-max_chains` | The maximum number of scan chains that will be generated. This takes priority over `max_length`,
in `no_mix` clock mode it specifies a maximum number of chains per clock-edge pair. |
| `-clock_mixing` | How architect will mix the scan flops based on the clock driver. `no_mix`: Creates scan chains with only one type of clock and edge. This may create unbalanced chains. `clock_mix`: Creates scan chains mixing clocks and edges. Falling edge flops are going to be stitched before rising edge. |
| `-scan_enable_name_pattern` | A format pattern with one or less set of braces (`{}`) to use to find or create scan enable drivers during scan chain stitching. The braces, if found, will be set to `0` as DFT architectures typically use a single shift-enable for all scan chains. If an un-escaped forward slash (`/`) is found, instead of searching for and/or creating a top-level port, an instance's pin will be searched for instead where the part of the string preceding the `/` is interpreted as the instance name and part succeeding it will be interpreted as the pin's name. |
| `-scan_in_name_pattern` | A format pattern with one or less braces (`{}`) to use to find or create scan in drivers during scan chain stitching. The braces will be replaced with the chain's ordinal number (starting at `0`). If an un-escaped forward slash (`/`) is found, instead of searching for and/or creating a top-level port, an instance's pin will be searched for instead where the part of the string preceding the `/` is interpreted as the instance name and part succeeding it will be interpreted as the pin's name. |
| `-scan_out_name_pattern` | A format pattern with one or less braces (`{}`) to use to find or create scan in loads during scan chain stitching. The braces will be replaced with the chain's ordinal number (starting at `0`). If an un-escaped forward slash (`/`) is found, instead of searching for and/or creating a top-level port, an instance's pin will be searched for instead where the part of the string preceding the `/` is interpreted as the instance name and part succeeding it will be interpreted as the pin's name. |

### Report DFT Config

Prints the current DFT configuration to be used by `preview_dft` and
`insert_dft`.

```tcl
report_dft_config
```

### Scan replace

Replaces flipflops with equivalent scan flipflops. This will generally be called before
placement, as it changes the area of cells.

```tcl
scan_replace
```

### Preview DFT

Prints a preview of the scan chains that will be stitched by `insert_dft`. Use
this command to iterate and try different DFT configurations. This command does
not perform any modification to the design, and should be run after `scan_replace`
and global placement.

```tcl
preview_dft
    [-verbose]
```

#### Options

| Switch Name | Description |
| ---- | ---- |
| `-verbose` | Shows more information about each one of the scan chains that will be created. |

### Insert DFT

Architect scan chains and connect them up in a way that minimises wirelength. As
a result, this should be run after placement, and after `scan_replace`.

```tcl
insert_dft
```

## Example scripts

This example will create scan chains with a max length of 10 bits mixing all the
scan flops in the scan chains.

```
set_dft_config -max_length 10 -clock_mixing clock_mix
report_dft_config
scan_replace
# Run global placement...
preview_dft -verbose
insert_dft
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

* There are no optimizations for the scan chains. This is a WIP.
* There is no way to specify existing scan ports to be used by scan insertion.
* There is currently no way to define a user defined scan path.
* We can only work with one bit cells.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
