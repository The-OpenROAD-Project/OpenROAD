# OpenROAD-pdn

This utility aims to simplify the process of adding a power grid into a
floorplan. The aim is to specify a small set of power grid policies to be
applied to the design, such as layers to use, stripe width and spacing,
then have the utility generate the actual metal straps. Grid policies can
be defined over the stdcell area, and over areas occupied by macros.

## Installation and Setup

This package runs as a utility within the `openraod` application.

## Usage

From inside the `openroad` application, the Power Delivery Network Generation
utility can be invoked as follows:

```
% pdngen <configuration_file> [-verbose]
```

All inputs and power grid policies are specified in a TCL format in the
configuration file.

``` tcl
% pdngen PDN.cfg -verbose
```

## Configuration File

For further information on the configuration file, and to review an example
configuration see the following:

* [PDN configuration help](PDN.md)
* [Sample configuration - PDN.cfg](doc/example_PDN.cfg)
* [Sample grid configuration - grid_strategy-M1-M4-M7.cfg](doc/grid_strategy-M1-M4-M7.cfg)

## Assumptions and Limitations

Currently the following assumptions are made:

1. The design is rectangular
1. The input floorplan includes the stdcell rows, placement of all macro blocks and IO pins.
1. The stdcells rows will be cut around macro placements
