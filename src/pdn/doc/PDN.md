# Elements of a configuration file

A configuration file for apply_pdn consists of the following parts
1. Global variable definitions
1. Call a TCL procedure to create grid specifications

## Optional Global Variables
Global variables are prefixed by ```::``` and force the variable to be declared in the global scope, rather than the local scope. This make the values of these variables available everywhere in the TCL code.
The ```apply_pdn``` command requires the following global variables to exist

| Variable Name | Description |
|:---|:---|
| ```::power_nets``` | Name of the power net |
| ```::ground_nets``` | Name of the ground net |
| ```::rails_start_with``` | POWER\|GROUND |
| ```::stripes_start_with``` | POWER\|GROUND |
| ```::halo``` | Halo to apply around macros. Specify one, two or four values. If a HALO is defined in the floorplan DEF, then this will be ignored. |


## Power grid strategy definitions
A set of power grid specifications are provided by calling the ```pdngen::specify_grid``` command.
At least one power grid specification must be defined.

The command has the following form

``` tcl
pdngen::specify_grid (stdcell|macro) <specification>
```

Where specification is a list of key value pairs

| Key | Description |
|:---|:---|
| ```rails``` | The specification of the layer(s) which should be used to draw the horizontal stdcell rails. Each layer will specify a value for ```width``` and optionally ```offset``` |
| ```straps``` | List of layers on which to draw stripes. Each layer will specify a value for ```width```, ```offset```, ```pitch``` and optionally ```spacing```. By default ```spacing = pitch / 2``` |
| ```connect``` | List of connections to be made between layers. Macro pin connections are on layer \<layer_name>_PIN_\<direction> |

Additionally, for macro grids
| Key | Description |
|:---|:---|
| ```orient``` | If the orientation of the macro matches an entry in this list, then apply this grid specification |
| ```instance``` | If the instance name of the macro matches an entry in this list, then apply this grid specification  |
| ```macro``` | If the macro name of the macro matches an entry in this list, then apply this grid specification |
| ```power_pins``` | List of power pins on the macro to connect to the grid |
| ```ground_pins``` | List of ground pins on the macro to connect to the grid |
| ```blockages``` | Layers which are blocked by a macro using this grid specification |

The key connect specifies two layers to be connected together where the stripes the same net of the first layer overlaps with stripes of the second layer

Macro pins are extracted from the LEF/DEF and are specified on a layer called ```<layer_name>\_PIN\_<dir>```, where ```<layer_name>``` is the name of the layer and ```<dir>``` is ```hor``` to indicate horizontal pins in the floorplan and ```ver``` to indicate that the pins are oriented vertically in the floorplan.

A separate grid is built for each macro. The list of macro specifications that have been defined are searched to find a specification with a matcing instance name key, failing that a macro specification with a matching macro name key, or else the first specification with neither an instance or macro key is used. Furthermore, if orient is specified, then the orientation of the macro must match one of the entries in the orient field

### Examples of grid specifications

1. Stdcell grid specification
``` tcl
pdngen::specify_grid stdcell {
    name grid
    rails {
        metal1 {width 0.17}
    }
    straps {
        metal4 {width 0.48 pitch 56.0 offset 2}
        metal7 {width 1.40 pitch 40.0 offset 2}
    }
    connect {{metal1 metal4} {metal4 metal7}}
}

```
This specification adds a grid over the stdcell area, with an metal1 followpin width of 0.17,connecting to metal4 stripes of 0.48um every 56.0um, connecting in turn to metal7 stripes, also 1.40um wide and 40.0 pitch

2. Macro grid specification
``` tcl
pdngen::specify_grid macro {
    orient {R0 R180 MX MY}
    power_pins "VDD VDDPE VDDCE"
    ground_pins "VSS VSSE"
    blockages "metal1 metal2 metal3 metal4 metal5 metal6"
    straps {
        metal5 {width 0.93 pitch 10.0 offset 2}
        metal6 {width 0.93 pitch 10.0 offset 2}
    }
    connect {{metal4_PIN_ver metal5} {metal5 metal6} {metal6 metal7}}
}

```

If this the only macro grid specification defined, then it will be applied over all the macros in the design that match one of the entries in the orient field.

All vertical metal4 pins on the macros are connected to metal5, which is then connected to metal6, which is connected in turn to metal7.

For macros that have their pins oriented in non-preferred routing direction the grid specification would be as follows.

We define blockages on metal1 to metal6 - although the macro itself only blocks from metal1 to metal4, we need to consider metal5 and metal6 to be blocked in case the stdcell grid specification tries to use those layers for its power straps. In that case we need those straps to be cut in order to keep the space for the macro grid.

``` tcl
pdngen::specify_grid macro {
    orient {R90 R270 MXR90 MYR90}
    power_pins "VDD VDDPE VDDCE"
    ground_pins "VSS VSSE"
    blockages "metal1 metal2 metal3 metal4 metal5 metal6"
    straps {
        metal6 {width 0.93 pitch 10.0 offset 2}
    }
    connect {{metal4_PIN_hor metal6} {metal6 metal7}}
}
```

Macros with orientations R90, R270, MXR90 or MYR90 will have their metal4 pins in the vertical (non-preferred) direction - this specification connects these pins directly to the metal6 layer, then on to metal7.

In both of thees cases we have specified that the macros the pins VDD, VDDPE and VDDCE will be connected to the power net, and the pins VSS and VSSE will be connected to the ground net. Any straps specified for other grids will be blocked for layers metal1 to metal6 - no actual blockage is added to the design in this case.

## Connection constraints

Further constraints can be applied for the connections between layers that have been specified

| Key | Description |
|:---|:---|
| ```cut_pitch``` | Specify a value greater than minimum cut pitch, e.g. for connecting a dual layer stdcell rail |
| ```max_rows``` | For a large via, limit the maximum number of rows allowed |
| ```max_columns``` | For a large via, limit the maximum number of columns allowed |
| ```split_cuts``` | Use individual vias instead of a via array |
| ```ongrid``` | Force intermediate metal layers to be drawn on the routing grid |

### Examples

``` tcl
pdngen::specify_grid stdcell {
    name grid
    rails {
        metal1 {width 0.17 pitch  2.4 offset 0}
        metal2 {width 0.17 pitch  2.4 offset 0}
    }
    connect {
      {metal1 metal2 constraints {cut_pitch 0.16}}
    }
}
```

## Using fixed VIAs from the tech file

Normally pdngen uses VIARULEs defined in the LEF, along with design rules for spacing, enclosure etc to build vias to connect between the layers. Some technologies may not have VIARULEs set up in the LEF file, others may define a set of fixed vias to be used in the power grid. In these cases you used the fixed_vias property to specify a list of fixed vias to be used for a given connection. The specified fixed_vias will be used to connect between layers. For any required vias that do not have a fixed_vias specified, pdngen will revert to building a via from a VIARULE instead

### Example

``` tcl
pdngen::specify_grid stdcell {
    name grid
    rails {
        metal1 {width 0.17 pitch  2.4 offset 0}
        metal2 {width 0.17 pitch  2.4 offset 0}
    }
    connect {
      {metal1 metal2 fixed_vias VIA12}
      {metal2 metal5 fixed_vias {VIA23 VIA34}}
    }
}
```

## Additional features for top level power grid

At the top level of the SoC, there is often a requirement to connect the core power and ground pads to the core power grid. This is done by specifying a core power ground ring around between the stdcell area and the pad cell placement.

### Example

``` tcl
pdngen::specify_grid stdcell {
  name grid

  pwr_pads {PADCELL_VDD_V PADCELL_VDD_H}
  gnd_pads {PADCELL_VSS_V PADCELL_VSS_H}

  core_ring {
    metal9  {width 5.0 spacing 2.0 core_offset 4.5}
    metal10 {width 5.0 spacing 2.0 core_offset 4.5}
  }
  rails {
    metal1  {width 0.17 pitch  2.4 offset 0}
  }
  straps {
    metal4  {width 0.48 pitch 56.0 offset 2}
    metal7  {width 1.40 pitch 40.0 offset 2}
    metal8  {width 1.40 pitch 40.0 offset 2}
    metal9  {width 1.40 pitch 40.0 offset 2}
    metal10 {width 1.40 pitch 40.0 offset 2}
  }
  connect {
    {metal1 metal4}
    {metal4 metal7}
    {metal7 metal8}
    {metal8 metal9}
    {metal9 metal10}
  }
}

```

When inserting a grid for a hierarchical sub-block, the top layers are omitted to be added at the SoC leve. So, at the SoC level, we have straps for layers metal8 to metal 10. We also specify core rings for two of the layers. The core rings are constructed as concentric rings around the stdcell area using the specified metals in their preferred routing directions. Straps from the stdcell area in the these layers will be extended out to connect to the core rings.

The stdcell rails may also be extended out to the core rings by using the extend_to_core_rings property in the rail definition.

### Example

``` tcl
pdngen::specify_grid stdcell {
  name grid

  core_ring {
    metal4  {width 5.0 spacing 2.0 core_offset 4.5}
    metal5  {width 5.0 spacing 2.0 core_offset 4.5}
  }
  rails {
    metal1  {width 0.17 pitch  2.4 offset 0 extend_to_core_ring 1}
  }
  straps {
    metal4  {width 0.48 pitch 56.0 offset 2}
  }
  connect {
    {metal1 metal4}
  }
}

```


It is assumed that the power pads specified have core facing pins for power/ground in the same layers used in the core rings. Straps, the same width as the pins are used to connect to the core rings. These connections from the pad cells have priority over connections from the core area power straps.
