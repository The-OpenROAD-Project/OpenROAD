# Parasitics Extraction

The parasitics extraction module in OpenROAD (`rcx`) is based on the 
open-source OpenRCX, a Parasitic Extraction (PEX, or RCX) tool that 
works on OpenDB design APIs.
It extracts routed designs based on the LEF/DEF layout model.

OpenRCX extracts both Resistance and Capacitance for wires, based on coupling
distance to the nearest wire and the track density context over and/or under the
wire of interest, as well as cell
abstracts.  The capacitance and resistance measurements are based on equations
of coupling distance interpolated on exact measurements from a calibration
file, called the Extraction Rules file. The Extraction Rules file (RC technology
file) is generated once for every process node and corner, using
a provided utility for DEF wire pattern generation and regression modeling.

OpenRCX stores resistance, coupling capacitance and ground (i.e., grounded) capacitance
on OpenDB objects with direct pointers to the associated wire and via db
objects. Optionally, OpenRCX can generate a `.spef` file.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Define Process Corner

This command defines proccess corner.

```tcl
define_process_corner 
    [-ext_model_index index]
    filename
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ext_model_index` | Extraction model index. Expects 2 inputs (an index, and corner name). |
| `filename` | Path to process corner file `rcx_patterns.rules`. |

### Extract Parasitics

The `extract_parasitics` command performs parasitic extraction based on the
routed design. If there are no information on routed design, no parasitics are
returned. 

```tcl
extract_parasitics
    [-ext_model_file filename]      
    [-corner_cnt count]            
    [-max_res ohms]               
    [-coupling_threshold fF]        
    [-debug_net_id id]
    [-lef_res]                     
    [-cc_model track]             
    [-context_depth depth]      
    [-no_merge_via_res]       
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ext_model_file` | Specify the Extraction Rules file used for the extraction. |
| `-corner_cnt` | Defines the number of corners used during the parasitic extraction. |
| `-max_res` | Combines resistors in series up to the threshold value. |
| `-coupling_threshold` | Coupling below this threshold is grounded. The default value is `0.1`, units are in `fF`, accepted values are floats. |
| `-debug_net_id` | *Developer Option*: Net ID to evaluate. |
| `-lef_res` | Override LEF resistance per unit. |
| `-cc_model` | Specify the maximum number of tracks of lateral context that the tool considers on the same routing level. The default value is `10`, and the allowed values are integers `[0, MAX_INT]`. |
| `-context_depth` | Specify the number of levels of vertical context that OpenRCX needs to consider for the over/under context overlap for capacitance calculation. The default value is `5`, and the allowed values are integers `[0, MAX_INT]`. |
| `-no_merge_via_res` | Separates the via resistance from the wire resistance. |

### Write SPEF

The `write_spef` command writes the `.spef` output of the parasitics stored
in the database.

```tcl
write_spef
    [-net_id net_id]                
    [-nets nets]
    [-coordinates]
    filename                     
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-net_id` | Output the parasitics info for specific net IDs. |
| `-nets` | Net name. |
| `coordinates` | Coordinates TBC. |
| `filename` | Output filename. |

### Scale RC

Use the `adjust_rc` command to scale the resistance, ground, and coupling
capacitance. 

```tcl
adjust_rc
    [-res_factor res]               
    [-cc_factor cc]                
    [-gndc_factor gndc]          
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-res_factor` | Scale factor for resistance. |
| `-cc_factor` | Scale factor for coupling capacitance. |
| `-gndc_factor` | Scale factor for ground capacitance. |

### Comparing different SPEF files

The `diff_spef` command compares the parasitics in the reference database `<filename>.spef`.
The output of this command is `diff_spef.out`
and contains the RC numbers from the parasitics in the database and the
`<filename>.spef`, and the percentage RC difference of the two data.

```tcl
diff_spef
    [-file filename]                
    [-r_res]
    [-r_cap]
    [-r_cc_cap]
    [-r_conn]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-file` | Path to the input `.spef` filename. |
| `-r_res` | Read resistance. |
| `-r_cap` | Read capacitance. |
| `-r_cc_cap` | Read coupled capacitance. |
| `r_conn` | Read connections. |

### Extraction Rules File Generation

The `bench_wires` command produces a layout which contains various patterns
that are used to characterize per-unit length R and C values. The generated patterns model
the lateral, vertical, and diagonal coupling capacitances, as well as ground
capacitance effects. This command generates a .def file that contains a number of wire patterns.

This command is specifically intended for the Extraction Rules file generation only.

```tcl
bench_wires
    [-met_cnt mcnt]
    [-cnt count]
    [-len wire_len]
    [-over]
    [-diag]
    [-all]
    [-db_only]
    [-under_met layer]
    [-w_list width]
    [-s_list space]
    [-over_dist dist]
    [-under_dist dist]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-met_cnt` | Number of layers used in each pattern. The default value is `-1`, meaning it is not set, and the allowed values are integers `[0, MAX_INT]`. |
| `-cnt` | Number of wires in each pattern. The default value is `5`, and the default values are integers `[0, MAX_INT]`. |
| `-len` | Wirelength in microns in the pattern. The default value is `100`, and the allowed values are integers `[0, MAX_INT]`. | 
| `-all` | Consider all different pattern geometries (`over`, `under`, `over_under`, and `diagonal`). |
| `-db_only` | Run with db values only. All parameters in `bench_wires` are ignored. |
| `-under_met` | Consider under metal layer. |
| `-w_list` | Lists of wire width multipliers from the minimum spacing defined in the LEF. |
| `-s_list` | Lists of wire spacing multipliers from the minimum spacing defined in the LEF. The list will be the input index on the OpenRCX RC table (Extraction Rules file). |
| `-over_dist`, `-under_dist` | Consider over and under metal distance respectively. |

### Generate verilog netlist

`bench_verilog` is used after the `bench_wires` command. This command
generates a Verilog netlist of the generated pattern layout by the `bench_wires`
command.

This command is optional when running the Extraction Rules generation
flow. This step is required if the favorite extraction tool (i.e., reference
extractor) requires a Verilog netlist to extract parasitics of the pattern layout.


```tcl
bench_verilog
    [filename]                    
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `filename` | Name for the Verilog output file (e.g., `output.v`). |

### Read SPEF

The `bench_read_spef` command reads a `<filename>.spef` file and stores the
parasitics into the database.

```tcl
bench_read_spef
    [filename]                   
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `filename` | Path to the input `.spef` file. |

### Write Rule File

The `write_rules` command writes the Extraction Rules file (RC technology file)
for OpenRCX. It processes the parasitics data from the layout patterns that are
generated using the `bench_wires` command, and writes the Extraction Rules file
with `<filename>` as the output file.

This command is specifically intended for the purpose of Extraction Rules file
generation.

```tcl
write_rules
  [-file filename]           
  [-dir dir]
  [-name name]
  [-pattern pattern]
  [-db]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-file` | Output file name. |
| `-dir` | Output file directory. |
| `-name` | Name of rule. |
| `-pattern` | Flag to write the pattern to rulefile (0/1). | 
| `-db` | DB tbc. |

## Example scripts

Example scripts demonstrating how to run OpenRCX in the OpenROAD environment
on sample designs can be found in /test. An example flow test taking a sample design
from synthesizable RTL Verilog to final-routed layout in an open-source SKY130 technology
is shown below.

```
./test/gcd.tcl
```

Example scripts demonstrating how to run the
Extraction Rules file generation can be found in this
[directory](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/calibration/script).

```
./calibration/script/generate_patterns.tcl     # generate patterns
./calibration/script/generate_rules.tcl        # generate the Extraction Rules file
./calibration/script/ext_patterns.tcl          # check the accuracy of OpenRCX
```

## Regression tests

There are a set of regression tests in `/test`. For more information, refer to this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Extraction Rules File Generation

This flow generates an Extraction Rules file (RC tech file, or RC table) for
OpenRCX. This file provides resistance and capacitance tables used for RC
extraction for a specific process corner.

The Extraction Rules file (RC technology file) is generated once for every
process node and corner automatically.

The detailed documentation can be found [here](doc/calibration.md).

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+rcx)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
