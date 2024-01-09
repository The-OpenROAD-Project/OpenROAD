# IR Drop Analysis

The IR Drop Analysis module in OpenROAD (`psm`) is based on PDNSim, 
an open-source static IR analyzer.

Features:

-   Report worst IR drop.
-   Report worst current density over all nodes and wire segments in the
    power distribution network, given a placed and PDN-synthesized design.
-   Check for floating PDN stripes on the power and ground nets.
-   Spice netlist writer for power distribution network wire segments.

| | |
| - | - |
| ![Image 1](doc/current_map.jpg) | ![Image 2](doc/IR_map.jpg) |
<p style="text-align: center;">(Left): Current Map, (Right): IR drop map</p>

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Analyze Power Grid

```tcl
analyze_power_grid
    [-vsrc vsrc_file]
    [-outfile out_file]
    [-error_file err_file]
    [-enable_em]
    [-em_outfile em_out_file]
    [-net net_name]
    [-dx bump_pitch_x]
    [-dy bump_pitch_y]
    [-node_density val_node_density]
    [-node_density_factor val_node_density_factor]
    [-corner corner]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-vsrc` | File to set the location of the power C4 bumps/IO pins. [Vsrc_aes.loc file](test/Vsrc_aes_vdd.loc) for an example with a description specified [here](doc/Vsrc_description.md). |
| `-dx`,`-dy` | These arguments set the bump pitch to decide the voltage source location in the absence of a vsrc file. Default bump pitch of 140um used in absence of these arguments and vsrc. |
| `-net` | Name of the net to analyze, power or ground net name. |
| `-enable_em` | Report current per power grid segment. |
| `-outfile` | Write per-instance voltage into the file. |
| `-em_outfile` | Write the per-segment current values into a file. This option is only available if used in combination with `-enable_em`. |
| `-voltage` | Sets the voltage on a specific net. If this option is not set, the Liberty file's voltage value is obtained from operating conditions. |
| `-node_density` | Node density (in microns) on the standard cell rails. It cannot be used together with `-node_density_factor`. |
| `-node_density_factor` | Factor which is multiplied by standard cell height to determine the node density on the std cell rails. It cannot be used together with `-node_density`. The default value is `5`, and the allowed values are integers `[0, MAX_INT]`. |
| `-corner` | Corner to use for analysis. | 

### Check Power Grid

```tcl
check_power_grid -net net_name
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Name of the net to analyze. Must be a power or ground net name. |

### Write Spice Power Grid

```tcl
write_pg_spice
    [-vsrc vsrc_file]
    [-outfile out_file]
    [-net net_name]
    [-dx bump_pitch_x]
    [-dy bump_pitch_y]
    [-corner corner]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-vsrc` | File to set the location of the power C4 bumps/IO pins. See [Vsrc_aes.loc file](test/Vsrc_aes_vdd.loc) for an example and its [description](doc/Vsrc_description.md). |
| `-dx`,`-dy` | Set the bump pitch to decide the voltage source location in the absence of a `vsrc` file. The default bump pitch is 140um if neither these arguments nor a `vsrc` file are given. |
| `-net` | Name of the net to analyze. Must be a power or ground net name. |
| `-outfile` | Write per-instance voltage written into the file. |
| `-corner` | Corner to use for analysis. | 

### Set PDNSim Net voltage

```tcl
set_pdnsim_net_voltage
    [-net net_name]
    [-voltage volt]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net` | Name of the net to analyze. It must be a power or ground net name. |
| `-voltage` | Sets the voltage on a specific net. If this option is not given, the Liberty file's voltage value is obtained from operating conditions. |

### Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/pdnsim.cpp) or the [swig file](./src/pdnsim.i).

| Command Name | Description |
| ----- | ----- |
| `find_net` | Get a reference to net name. | 

## Example scripts

Example scripts demonstrating how to run PDNSim on a sample design on `aes` as follows:

```tcl
./test/aes_test_vdd.tcl
./test/aes_test_vss.tcl
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+psm+in%3Atitle)
about this tool.

## References 

1. PDNSIM [documentation](doc/PDNSim-documentation.pdf)
1. Chhabria, V.A. and Sapatnekar, S.S. (no date) The-openroad-project/pdnsim: Power Grid Analysis, GitHub. Available at: https://github.com/The-OpenROAD-Project/PDNSim (Accessed: 24 July 2023). [(link)](https://github.com/The-OpenROAD-Project/PDNSim)

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
