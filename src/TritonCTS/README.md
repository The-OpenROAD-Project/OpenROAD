# TritonCTS 2.0
TritonCTS 2.0 is the clock tree synthesis engine of The OpenROAD Project.

### Usage
TritonCTS 2.0 is available under the OpenROAD app as ``clock_tree_synthesis`` command. There are currently two ways one can run this command.
The first is if the user does not have a characterization file. TritonCTS 2.0 creates the wire segments manually based on the user parameters. 
The following tcl snippet shows how to call TritonCTS and exporting a characterization file..

```
read_lef "mylef.lef"
read_liberty "myliberty.lib"
read_def "mydef.def"
read_verilog "myverilog.v"
read_sdc "mysdc.sdc"

report_checks

clock_tree_synthesis -buf_list <list_of_buffers> \
                     -sqr_cap <cap_per_sqr> \
                     -sqr_res <res_per_sqr> \
                     [-root_buf <root_buf>] \
                     [-max_slew <max_slew>] \
                     [-max_cap <max_cap>] \
                     [-slew_inter <slew_inter>] \
                     [-cap_inter <cap_inter>] \
                     [-wire_unit <wire_unit>] \
                     [-clk_nets <list_of_clk_nets>] \
                     [-out_path <lut_path>] \
                     [-characterization_only]

write_def "final.def"
```
Argument description:
- ``-buf_list`` are the master cells (buffers) that will be considered when making the wire segments.
- ``-sqr_cap`` is the capacitance (in picofarad) per micrometer (thus, the same unit that is used in the LEF syntax) to be used in the wire segments. 
- ``-sqr_res`` is the resistance (in ohm) per micrometer (thus, the same unit that is used in the LEF syntax) to be used in the wire segments. 
- ``-root_buffer`` is the master cell of the buffer that serves as root for the clock tree. 
If this parameter is omitted, the first master cell from ``-buf_list`` is taken.
- ``-max_slew`` is the max slew value (in seconds) that the characterization will test. 
If this parameter is omitted, the code tries to obtain the value from the liberty file.
- ``-max_cap`` is the max capacitance value (in farad) that the characterization will test. 
If this parameter is omitted, the code tries to obtain the value from the liberty file.
- ``-slew_inter`` is the time value (in seconds) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-12). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-cap_inter`` is the capacitance value (in farad) that the characterization will consider for results. 
If this parameter is omitted, the code gets the default value (5.0e-15). Be careful that this value can be quite low for bigger technologies (>65nm).
- ``-wire_unit`` is the minimum unit distance between buffers for a specific wire. 
If this parameter is omitted, the code gets the value from ten times the height of ``-root_buffer``.
- ``-clk_nets`` is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.
- ``-out_path`` is the output path (full) that the lut.txt and sol_list.txt files will be saved. This is used to load an existing characterization, without creating one from scratch.
- ``-characterization_only`` is a flag that, when specified, makes so that only the library characterization step is run and no clock tree is inserted in the design.

Instead of creating a characterization, you can use the following tcl snippet to call TritonCTS and load the characterization file..

```
read_lef "mylef.lef"
read_liberty "myliberty.lib"
read_def "mydef.def"
read_verilog "myverilog.v"
read_sdc "mysdc.sdc"

report_checks

clock_tree_synthesis -lut_file <lut_file> \
                     -sol_list <sol_list_file> \
                     -root_buf <root_buf> \
                     [-wire_unit <wire_unit>] \
                     [-clk_nets <list_of_clk_nets>] 

write_def "final.def"
```
Argument description:
- ``-lut_file`` is the file containing delay, power and other metrics for each segment.
- ``-sol_list`` is the file containing the information on the topology of each segment (wirelengths and buffer masters).
- ``-sqr_res`` is the resistance (in ohm) per database units to be used in the wire segments. 
- ``-root_buffer`` is the master cell of the buffer that serves as root for the clock tree. 
If this parameter is omitted, you can use the ``-buf_list`` argument, using the first master cell. If both arguments are omitted, an error is raised.
- ``-wire_unit`` is the minimum unit distance between buffers for a specific wire, based on your ``-lut_file``. 
If this parameter is omitted, the code gets the value from the header of the ``-lut_file``. For the old technology characterization, described [here](https://github.com/The-OpenROAD-Project/TritonCTS/blob/master/doc/Technology_characterization.md), this argument is mandatory, and omitting it raises an error.
- ``-clk_nets`` is a string containing the names of the clock roots. 
If this parameter is omitted, TritonCTS looks for the clock roots automatically.

Another command available from TritonCTS is ``report_cts``. It is used to extract metrics after a successful ``clock_tree_synthesis`` run. These are: Number of Clock Roots, Number of Buffers Inserted, Number of Clock Subnets, and Number of Sinks.
The following tcl snippet shows how to call ``report_cts``.

```
read_lef "mylef.lef"
read_liberty "myliberty.lib"
read_def "mydef.def"
read_verilog "myverilog.v"
read_sdc "mysdc.sdc"

report_checks

clock_tree_synthesis -lut_file "lut.txt" \
                     -sol_list "sol_list.txt" \
                     -root_buf "BUF_X4" \
                     -wire_unit 20 

report_cts [-out_file "file.txt"]
```

Argument description:
- ``-out_file`` is the file containing the TritonCTS reports.
If this parameter is omitted, the metrics are shown on the standard output.

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
