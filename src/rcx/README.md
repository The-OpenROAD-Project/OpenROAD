# OpenRCX
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

OpenRCX is an parasitics Extraction tool that works on OpenDB design APIs. 
It extracts routed designs based on the LEF/DEF model.

It extracts both Resistance and Capacitance for wires, based on coupling 
distance to the nearest wire and the context over and/or under and cell abstracts.
The capacitance and resistance measurements are based on equations of 
coupling distance interpolated on exact measurements from a calibration file, 
called Extraction Rules File. The Extraction Rules file (RC technology file) is 
generated once for every process node and corner automatically.

OpenRCX stores resistance, coupling capacitance and ground capacitance on OpenDB objects with direct pointers to the associated wire and via db objects
and optionally can generate a .spef file.


#### Tests
There is a set of regression tests in /test.

```
./test/regression
```

#### Extraction Rules File Generation

This flow generates an Extraction Rules file (RC tech file/RC Table) for OpenRCX. This file provides
resistance and capacitance tables used for RC extraction for a specific process
corner.

The Extraction Rules file (RC technology file) is 
generated once for every process node and corner automatically.

The detailed documentation can be found [here](doc/calibration.txt)

#### Extract Parasitics

```
extract_parasitics
  [-ext_model_file filename]      pointer to The Extraction Rules file
  [-corner_cnt count]             process corner count
  [-max_res ohms]                 combine resistors in series up to 
                                  <max_res> value in OHMS
  [-coupling_threshold fF]        coupling below the threshold is grounded
  [-lef_res]                      use per-unit RC defined in LEF
  [-cc_model track]               calculate coupling within 
                                  <cc_model> distance
  [-context_depth depth]          caculate upper/lower coupling from 
                                  <depth> level away
  [-no_merge_via_res]             seperate via resistance
```

The `extract_parasitics` command performs parasitic extraction based on the
routed design. If there are no routed design information no parasitics are
added. Use `ext_model_file` to specify the Extraction Rules file used for the
extraction. 

The `cc_model` is used to specify the maximum number of tracks on same routing level.
The default value is 10.
The `context_depth` option is used to specify the levels that OpenRCX needs to consider 
The default value is 5.
for the over/under context overlap for capacitance calculation. 
The `max_res` command combines resistors in series up to the threshold values.
Use `no_merge_via_res` separates the via resistance from the wire resistance.

The `corner_cnt` defines the number of corners used during the parasitic
extractions.

#### Write SPEF

```
write_spef
  [-net_id net_id]                output the parasitics info for specific nets
  [filename]                      the output filename
```

The `write_spef` command writes the .spef output of the parasitics stored in the
database. Use `net_id` command to write the output for specific nets.

#### Scale RC

```
adjust_rc
  [-res_factor res]               scale the res value
  [-cc_factor cc]                 scale the coupling capacitance value
  [-gndc_factor gndc]             scale the ground cap value
```

Use the `adjust_rc` command to scale the resistance, ground, and coupling
capacitance. The `res_factor` specifies the scale factor for resistance. The
`cc_factor` specifies the scale factor for coupling cap. The `gndc_factor`
specifies the scale factor for ground cap.

#### Comparing SPEF files 

```
diff_spef
  [-file filename]                specifies the input .spef filename  
```

`diff_spef` command compares the parasitics in the database with the parasitic
information from `<file>.spef`. The output of this command is `diff_spef.out` and
contains the RC numbers from the parasitics in the database and the `<file>.spef`,
and the percentage RC difference of the two data.

#### Extraction Rules File Generation

```
bench_wires
  [-cnt count]                    specify the metal count 
                                  per pattern
  [-len wire_len]                 specify the wire length 
                                  in the pattern
  [-s_list space_multiplier_list] list of wire spacing multiplier
  [-all]                          generate all patterns  
```

`bench_wires` command produces a layout  which contains various patterns that
are used to characterize per-unit RC. The generated patterns model the
lateral, vertical, and diagonal coupling capacitance, and the ground capacitance
effects. This command generates a .def file.

The `cnt` command determines the number of wires in each pattern, the default
values is 5. Use `len` command to change the wire length in the pattern. `all` 
option is used to  specify all different pattern geometries (over, under,
over_under, and diagonal). The option `all` is required.

The `s_list` option specifies the lists of wire spacing multipliers from the
minimum spacing defined in the LEF. The list will be the input index on the 
OpenRCX RC table (Extraction Rules file).

This command is specifically intended for the Extraction Rules file generation
only.

```
bench_verilog
    [filename]                    the output verilog filename  
```

`bench_verilog` is used after the `bench_wires` command. This command generates
verilog netlist of the generated pattern layout by the `bench_wires`
command. 

This command is optional when running the Extraction Rules generation flow. This
step is required if the favorite extraction tool (i.e., reference extractor) 
requires a netlist to extract parasitics of the pattern layout.

```
bench_read_spef
    [filename]                    the input .spef filename  
```

`bench_read_spef` command reads a `<filename>.spef` file and stores the
parasitics into the database.

```
write_rules
  [-file filename]                output file name
  [-db]                           read parasitics from the databse
```

`write_rules` command writes the Extraction Rules file (RC technology file)
for OpenRCX. It processes the parasitics data from the pattern layout generated from
`bench_wires` command, and writes the Extraction Rules file with `<file>` as
the output file.

`db` command instructs OpenRCX to write the Extraction Rules file from 
the parasitics stored in the database. This option is required.

This command is specifically intended for the Extraction Rules file generation
purpose.

### Example Scripts

Example scripts demonstrating how to run OpenRCX in the OpenROAD environment on sample designs
that can be found in /test. Flow tests taking sample designs from synthesis
verilog to routed design in the open source technologies
Sky130 is shown below.

```
gcd.tcl
```

Example scripts demonstrating how to run the Extraction Rules File generation
can be found in this [directory](calibration/script).

```
generate_patterns.tcl     # generate patterns
generate_rules.tcl        # generate the Extraction Rules
                          # file
ext_patterns.tcl          # check the accuracy of OpenRCX
```
