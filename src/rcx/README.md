# OpenRCX

[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

OpenRCX is a Parasitic Extraction (PEX, or RCX) tool that works on OpenDB design APIs.
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

### Extract Parasitics

```
extract_parasitics
  [-ext_model_file filename]      pointer to the Extraction Rules file
  [-corner_cnt count]             process corner count
  [-max_res ohms]                 combine resistors in series up to
                                  <max_res> value in OHMS
  [-coupling_threshold fF]        coupling below the threshold is grounded
  [-lef_res]                      use per-unit RC defined in LEF
  [-cc_model track]               calculate coupling within
                                  <cc_model> distance
  [-context_depth depth]          calculate upper/lower coupling from
                                  <depth> level away
  [-no_merge_via_res]             separate via resistance
```

The `extract_parasitics` command performs parasitic extraction based on the
routed design. If there is no routed design information, then no parasitics are
returned. Use `ext_model_file` to specify the Extraction Rules file used for
the extraction.

The `cc_model` option is used to specify the maximum number of tracks of lateral context
that the tool considers on the same
routing level.  The default value is 10.  The `context_depth` option
is used to specify the number of levels of vertical context that OpenRCX needs to consider for the over/under context overlap for capacitance calculation.
The default value is 5.
The `max_res` option combines resistors in series up to the threshold value.
The `no_merge_via_res` option separates the via resistance from the wire resistance.

The `corner_cnt` defines the number of corners used during the parasitic
extraction.

### Write SPEF

```
write_spef
  [-net_id net_id]                output the parasitics info for specific nets
  [filename]                      the output filename
```

The `write_spef` command writes the .spef output of the parasitics stored
in the database. Use `net_id` option to write out .spef for specific nets.

### Scale RC

```
adjust_rc
  [-res_factor res]               scale the resistance value by this factor
  [-cc_factor cc]                 scale the coupling capacitance value by this factor
  [-gndc_factor gndc]             scale the ground capacitance value by this factor
```

Use the `adjust_rc` command to scale the resistance, ground, and coupling
capacitance. The `res_factor` specifies the scale factor for resistance. The
`cc_factor` specifies the scale factor for coupling capacitance. The `gndc_factor`
specifies the scale factor for ground capacitance.

### Comparing SPEF files

```
diff_spef
  [-file filename]                specifies the input .spef filename
```

The `diff_spef` command compares the parasitics in the database with the parasitic
information from `<file>.spef`. The output of this command is `diff_spef.out`
and contains the RC numbers from the parasitics in the database and the
`<file>.spef`, and the percentage RC difference of the two data.

### Extraction Rules File Generation

```
bench_wires
  [-cnt count]                    specify the metal count
                                  per pattern
  [-len wire_len]                 specify the wire length
                                  in the pattern
  [-s_list space_multiplier_list] list of wire spacing multipliers
  [-all]                          generate all patterns
```

The `bench_wires` command produces a layout which contains various patterns
that are used to characterize per-unit length R and C values. The generated patterns model
the lateral, vertical, and diagonal coupling capacitances, as well as ground
capacitance effects. This command generates a .def file that contains a number of wire patterns.

The `cnt` option specifies the number of wires in each pattern; the
default value of cnt is 5. Use the `len` option to change the wire length in the
pattern. The `all` option is used to  specify all different pattern geometries
(over, under, over_under, and diagonal). The option `all` is required.

The `s_list` option specifies the lists of wire spacing multipliers from
the minimum spacing defined in the LEF. The list will be the input index
on the OpenRCX RC table (Extraction Rules file).

This command is specifically intended for the Extraction Rules file
generation only.

```
bench_verilog
    [filename]                    the output verilog filename
```

`bench_verilog` is used after the `bench_wires` command. This command
generates a Verilog netlist of the generated pattern layout by the `bench_wires`
command.

This command is optional when running the Extraction Rules generation
flow. This step is required if the favorite extraction tool (i.e., reference
extractor) requires a Verilog netlist to extract parasitics of the pattern layout.

```
bench_read_spef
    [filename]                    the input .spef filename
```

The `bench_read_spef` command reads a `<filename>.spef` file and stores the
parasitics into the database.

```
write_rules
  [-file filename]                output file name
  [-db]                           read parasitics from the database
```

The `write_rules` command writes the Extraction Rules file (RC technology file)
for OpenRCX. It processes the parasitics data from the layout patterns that are
generated using the `bench_wires` command, and writes the Extraction Rules file
with `<file>` as the output file.

The `db` option instructs OpenRCX to write the Extraction Rules file from the
parasitics stored in the database. This option is required.

This command is specifically intended for the purpose of Extraction Rules file
generation.

## Example scripts

Example scripts demonstrating how to run OpenRCX in the OpenROAD environment
on sample designs can be found in /test. An example flow test taking a sample design
from synthesizable RTL Verilog to final-routed layout in an open-source SKY130 technology
is shown below.

```
gcd.tcl
```

Example scripts demonstrating how to run the
Extraction Rules file generation can be found in this
[directory](https://github.com/The-OpenROAD-Project/OpenROAD/tree/master/src/rcx/calibration/script).

```
generate_patterns.tcl     # generate patterns
generate_rules.tcl        # generate the Extraction Rules file
ext_patterns.tcl          # check the accuracy of OpenRCX
```

## Regression tests

There is a set of regression tests in /test.

```
./test/regression
```

## Extraction Rules File Generation

This flow generates an Extraction Rules file (RC tech file, or RC table) for
OpenRCX. This file provides resistance and capacitance tables used for RC
extraction for a specific process corner.

The Extraction Rules file (RC technology file) is generated once for every
process node and corner automatically.

The detailed documentation can be found [here](doc/calibration.md)

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+openrcx+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
