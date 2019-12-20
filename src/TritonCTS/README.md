# TritonCTS 2.0
TritonCTS 2.0 is the clock tree synthesis engine of The OpenROAD Project.

### Usage
TritonCTS 2.0 is available under the OpenROAD app as ``clock_tree_synthesis`` command.
The following tcl snippet shows how to call TritonCTS.

```
read_lef "mylef.lef"
read_liberty "myliberty.lib"
read_def "mydef.def"
read_verilog "myverilog.v"
read_sdc "mysdc.sdc"

report_checks

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf CLKBUF_X3 \
                     -wire_unit 20

write_def "final.def"
```

Argument description:
- ```lut_file```, ```sol_list``` and ```wire_unit``` are parameters related to 
the technology characterization described [here](https://github.com/The-OpenROAD-Project/TritonCTS/blob/master/doc/Technology_characterization.md).
- ``root_buffer`` is the master cell of the buffer that serves as root for the clock tree.
- ``clk_nets`` is a string containing the names of the clock roots. 
If this parameter is ommitted, TritonCTS looks for the clock roots automatically.

### Third party packages
[LEMON](https://lemon.cs.elte.hu/trac/lemon) - **L**ibrary for **E**fficient **M**odeling and **O**ptimization in **N**etworks

Capacitated k-means package from Dr. Jiajia Li (UCSD). 
Published [here](https://vlsicad.ucsd.edu/Publications/Conferences/344/c344.pdf).

### Authors
TritonCTS 2.0 is written by Mateus Fogaça, PhD student in the Graduate Program on Microelectronics from
the Federal University of Rio Grande do Sul (UFRGS), Brazil. Mr. Fogaça advisor is Prof. Ricardo Reis

Many guidance provided by (alphabetic order):
* Kwangsoo Han 
*  Andrew B. Kahng
*  Jiajia Li
*  Tom Spyrou
