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
    [-corner cornerIndex]
    [-corner_cnt count]            
    [-max_res ohms]               
    [-coupling_threshold fF]        
    [-debug_net_id id]
    [-dbg dbg_num ]
    [-lef_res]                     
    [-lef_rc]
    [-cc_model track]             
    [-context_depth depth]      
    [-no_merge_via_res]       
    [-skip_over_cell ]
    [-version]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ext_model_file` | Specify the Extraction Rules file used for the extraction. |
| `-corner cornerIndex` | Corner to extract.  Default -1. |
| `-corner_cnt` | Defines the number of corners used during the parasitic extraction. |
| `-max_res` | Combines resistors in series up to the threshold value. |
| `-coupling_threshold` | Coupling below this threshold is grounded. The default value is `0.1`, units are in `fF`, accepted values are floats. |
| `-debug_net_id` | *Developer Option*: Net ID to evaluate. |
| `-dbg dbg_num` | Debug messaging level.  Default 0. |
| `-lef_res` | Override LEF resistance per unit. |
| `-lef_rc` | Use LEF RC values. Default false. |
| `-cc_model` | Specify the maximum number of tracks of lateral context that the tool considers on the same routing level. The default value is `10`, and the allowed values are integers `[0, MAX_INT]`. |
| `-context_depth` | Specify the number of levels of vertical context that OpenRCX needs to consider for the over/under context overlap for capacitance calculation. The default value is `5`, and the allowed values are integers `[0, MAX_INT]`. |
| `-no_merge_via_res` | Separates the via resistance from the wire resistance. |
| `-skip_over_cell` | Ignore shapes in cells.  .Default false. |
| `-version` | select between v1 and v2 modeling.  Defaults to 1.0. |

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
    [-spef_corner spef_num]
    [-ext_corner ext_num]
    [-r_res]
    [-r_cap]
    [-r_cc_cap]
    [-r_conn]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-file` | Path to the input `.spef` filename. |
| `-spef_corner spef_num` | The spef corner to diff. |
| `-ext_corner ext_num` | The extraction corner to diff. |
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
    [-v1]
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
| `-v1` | Generation version one patterns. |
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
  [-db]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-file` | Output file name. |
| `-dir` | Output file directory. |
| `-name` | Name of rule. |
| `-db` | DB tbc. |

### Write RCX Model

The `write_rcx_model` command write the model file after reading capacitance/resistance
Tables from Field Solver.

```tcl
write_rcx_model
  [-file filename]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-file` | Output file name. |

### Init RCX Model

The `init_rcx_model` command is used for initialization for creating the model file.

```tcl
init_rcx_model
  [-corner_names names]
  [-met_cnt met_cnt]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-corner_names` | List of corner names. |
| `-met_cnt` | Number of metal layers. |

### Read RCX Tables

The `read_rcx_tables` command reads the capacitance and resistance tables from Field
Solver Output.

```tcl
read_rcx_tables
  [-corner_name corner_name]
  [-file in_file_name]
  [-wire_index wire]
  [-over]
  [-under]
  [-over_under]
  [-diag]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-corner_name` | Corner name. |
| `-file` | Input file name. |
| `-wire_index` | Target wire index. |
| `-over` | Over pattern family only. |
| `-under` | Under pattern family only. |
| `-over_under` | OverUnder pattern family only. |
| `-diag` | Diagonal pattern family only. |

### Generate Solver Patterns

The `gen_solver_patterns` command generates 3D wire patterns not targeting any particular
Field Solver.

```tcl
gen_solver_patterns
  [-process_file process_file]
  [-process_name process_name]
  [-version version]
  [-wire_cnt wire_count]
  [-len wire_len]
  [-w_list widths]
  [-s_list spacings]
  [-over_dist dist]
  [-under_dist dist]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-process_file` | File that contains full process stack with conductor and dielctric dimensions. |
| `-process_name` | Name of the process. |
| `-version` | 2 if normalized wires, 1 not normalized; deafult is 1. |
| `-wire_cnt` | Number of wires. Default is 3. |
| `-len` | Wire length in microns. Default is 10. |
| `-w_list` | Min layer Width multiplier list. Default is 1. |
| `-s_list` | Min layer spacing multiplier list. Default is "1.0 1.5 2.0 3 5". |
| `-over_dist` | Max number of levels for Under patterns. Default is 4. |
| `-under_dist` | Max number of levels for Over patterns. Default is 4. |

### Define RCX Corners

The `define_rcx_corners` command defines specific corners to extract parasitics.

```tcl
define_rcx_corners
  [-corner_list corner_list]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-corner_list` | List of corner names. |

### Get Model Corners

The `get_model_corners` command lists all the corner names from a model file.

```tcl
get_model_corners
  [-ext_model_file file_name]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-ext_model_file` | Input file name. |

### Generate RCX Model

The `gen_rcx_model` command 

```tcl
gen_rcx_model
  [-spef_file_list spefList]
  [-corner_list cornerList]
  [-out_file outfilename]
  [-comment comment]
  [-version version]
  [-pattern pattern]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-spef_file_list`| List of spef file names. |
| `-corner_list`| List of corners. |
| `-out_file`| Output file name. |
| `-comment`| Comment on the model file. Default: "RCX Model File". |
| `-version`| Version of the model file. |
| `-pattern`| Pattern of the model file. |

### Bench Wires Generation

The `bench_wires_gen` command generates comprehensive benchmarking patterns
(not for model generation).

```tcl
bench_wires_gen
  [ -len length_in_min_widths ]
  [ -met metal ]
  [ -mlist metal_list ]
  [ -width multiplier_width_list ]
  [ -spacing multiplier_spacing_list ]
  [ -couple_width multiplier_coupling_width_list ]
  [ -couple_spacing multiplier_coupling_spacing_list ]
  [ -over_width multiplier_over_width_list ]
  [ -over_spacing multiplier_over_spacing_list ]
  [ -under_width multiplier_under_width_list ]
  [ -under_spacing multiplier_under_spacing_list ]
  [ -over2_width multiplier_over2_width_list ]
  [ -over2_spacing multiplier_over2_spacing_list ]
  [ -under2_width multiplier_under2_width_list ]
  [ -under2_spacing multiplier_under2_spacing_list ]
  [ -dbg dbg_flag ]
  [ -wire_cnt wire_count ]
  [ -offset_over offset_over ]
  [ -offset_under offset_under ]
  [ -under_dist max_dist_to_under_met ]
  [ -over_dist max_dist_to_over_met ]
  [ -diag ]
  [ -over ]
  [ -under ]
  [ -over_under ]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-len` | length_in_min_widths.
| `-met` | target metal.
| `-mlist` | target metal_list.
| `-width` | multiplier_width_list.
| `-spacing` | multiplier_spacing_list.
| `-couple_width` | multiplier_coupling_width_list.
| `-couple_spacing` | multiplier_coupling_spacing_list.
| `-over_width` | multiplier_over_width_list.
| `-over_spacing` | multiplier_over_spacing_list.
| `-under_width` | multiplier_under_width_list.
| `-under_spacing` | multiplier_under_spacing_list.
| `-over2_width` | multiplier_over2_width_list.
| `-over2_spacing` | multiplier_over2_spacing_list.	 
| `-under2_width` | multiplier_under2_width_list.
| `-under2_spacing` | multiplier_under2_spacing_list.
| `-dbg` |  dbg_flag.
| `-wire_cnt` | wire_count.
| `-offset_over` | offset_over.
| `-offset_under` | offset_under.
| `-under_dist` | max_dist_to_under_met.
| `-over_dist` | max_dist_to_over_met.	  
| `-diag` | Diag pattern family only.
| `-over` | Over pattern family only.
| `-under` | Under pattern family only.
| `-over_under` | OverUnder pattern family only.

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
