# TCL Commands to build chip level padrings

The following commands can be used from the command prompt to define the padring structure for the chip.
 - [add_pad](add_pad.md)
 - [add_libcell](add_libcell.md)
 - [set_padring_options](set_padring_options.md)
 - [set_bump_options](set_bump_options.md)

Once the padcells have been added, the padring can be built using the initialize_padring command.
 - [initialize_padring](initialize_padring.md)

Use the place_cell command to pre-place additional cells in the floorplan
 - [place_cell](place_cell.md)


Full examples of how to use these commands together to build a chip
 - Example for chip with [wirebond padring](../test/tcl_interface.tcl)
 - Example for chip with [flipchip bumps](../test/tcl_interface.flipchip.tcl)

