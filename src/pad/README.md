# ICeWall

At the top level of the chip, special padcells are use to connect signals
to the external package. Additional commands are provided to [specify the
placement of padcells, bondpads and bumps](doc/TCL_Interface.md)


The definition of the padring is split into three separate parts:

- A library definition which contains additional required information about the IO cells
being used
- A package description which details the location of IO cells
around the periphery of the design
- A signal mapping file that associates
signals in the design with the IO cells placed around the periphery

The separation of the package description from the signal mapping file
allows the same IO padring to be reused for different designs, reducing
the amount of rework needed.

For more details refer to [doc/README.md](doc/README.md)
