# OpenROAD-ICeWall

If you want to use this as part of the OpenROAD project it can be used from inside the integrated [openroad app](https://github.com/The-OpenROAD-Project/OpenROAD).

This utility aims to simplify the process of adding a padring into a floorplan. 

The definition of the padring is split into three separate parts
- A library definition which contains additional required information about the IO cells being used
- A package description which details the location of IO cells around the periphery of the design
- A signal mapping file, that associates signals in the design with the IO cells placed around the periphery

The separation of the package description from the signal mapping file allows the same IO padring to be re-used for 
different designs, reducing the amount of re-work necessary.

For more details refer to [doc/README.md](doc/README.md)
