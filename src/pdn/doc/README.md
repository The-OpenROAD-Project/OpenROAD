# OpenROAD-pdn

This utility aims to simplify the process of adding a power grid into a floorplan. The aim is to specify a small set of power grid policies to be applied to the design, such as layers to use, stripe width and spacing, then have the utility generate the actual metal straps. Grid policies can be defined over the stdcell area, and over areas occupied by macros.

PDNSim PDN checker searches for floating PDN stripes on the power and ground nets.

PDNSim reports worst IR drop and worst current density in a power wire drop given a placed and PDN synthesized design.

PDNSim spice netlist writer for power wires.

## Usage

From inside the openroad application, the Power Delivery Network Generation utility can be invoked as follows:

```
% pdngen <configuration_file> [-verbose]
```

```
set_pdnsim_net_voltage -net <net_name> -voltage <voltage_value>
check_power_grid -net <net_name>
analyze_power_grid -vsrc <voltage_source_location_file> \
                   -net <net_name> \
                   [-outfile <filename>] \
                   [-enable_em] \
                   [-em_outfile <filename>]
                   [-dx]
                   [-dy]
                   [-em_outfile <filename>]
write_pg_spice -vsrc <voltage_source_location_file> -outfile <netlist.sp> -net <net_name>
```

Options description:
- ``vsrc``: (optional) file to set the location of the power C4 bumps/IO pins. [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).
- ``dx,dy``: (optional) these arguments set the bump pitch to decide the voltage source location in the absence of a vsrc file. Default bump pitch of 140um used in absence of these arguments and vsrc
- ``net``: (mandatory) is the name of the net to analyze, power or ground net name
- ``enable_em``: (optional) is the flag to report current per power grid segment
- ``outfile``: (optional) filename specified per-instance voltage written into file
- ``em_outfile``: (optional) filename to write out the per segment current values into a file, can be specified only if enable_em is flag exists
- ``voltage``: Sets the voltage on a specific net. If this command is not run, the voltage value is obtained from operating conditions in the liberty.

Note: See the file [Vsrc_aes.loc file](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/test/aes/Vsrc.loc) for an example with a description specified [here](https://github.com/The-OpenROAD-Project/PDNSim/blob/master/doc/Vsrc_description.md).

## Configuration File

For further information on the configuration file, and to review an example configuration see the following:

* [PDN configuration help](PDN.md)
* [Sample configuration - PDN.cfg](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/pdn/doc/example_PDN.cfg)
* [Sample grid configuration - grid_strategy-M1-M4-M7.cfg](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/pdn/doc/grid_strategy-M1-M4-M7.cfg)

## Assumptions and Limitations

Currently the following assumptions are made:

1. The design is rectangular
1. The input floorplan includes the stdcell rows, placement of all macro blocks and IO pins.
1. The stdcells rows will be cut around macro placements
