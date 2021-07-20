# TCL Commands to build chip level padrings

The following commands can be used from the command prompt to define the padring structure for the chip.
 - [add_pad](add_pad.md)
 - [define_pad_cell](define_pad_cell.md)
 - [set_padring_options](set_padring_options.md)
 - [set_bump_options](set_bump_options.md)
 - [set_bump](set_bump.md)

Once the padcells have been added, the padring can be built using the initialize_padring command.
 - [initialize_padring](initialize_padring.md)

Use the place_cell command to pre-place additional cells in the floorplan
 - [place_cell](place_cell.md)


Full examples of how to use these commands together to build a chip
 - Example for chip with [wirebond padring](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/ICeWall/test/tcl_interface.example.tcl)
 - Example for chip with [flipchip bumps](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/src/ICeWall/test/tcl_interface.flipchip.example.tcl)


Alternatively, the information needed to build chip level pad rings can be [bundled up into a separate file and loaded in batch fashion](README.md)

