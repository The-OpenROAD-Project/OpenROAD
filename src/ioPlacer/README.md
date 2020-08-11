ioPlacer
======================

Assign I/O pins to on-track locations at the boundaries of the 
core while optimizing I/O nets wirelength. I/O pin assignment also 
creates a metal shape for each I/O pin using min-area rules.

Use the following command to perform I/O pin assignment:
```
place_ios [-hor_layer h_layer]  
          [-ver_layer v_layer] 
	  [-random_seed seed] 
          [-random] 
```
- ``-hor_layer`` (mandatory). Set the layer to create the metal shapes 
of I/O pins assigned to horizontal tracks. 
- ``-ver_layer`` (mandatory). Set the layer to create the metal shapes
of I/O pins assigned to vertical tracks. 
- ``-random_seed``. Set the seed for random operations.
- ``-random``. When this flag is enabled, the I/O pin assignment is 
random.

## Getting Started
You can find usage information inside OpenROAD app repository, in the "I/O pin assignment" section of the [README file](https://github.com/The-OpenROAD-Project/OpenROAD/blob/develop/README.md).

Copyright (c) 2019--2020, Federal University of Rio Grande do Sul (UFRGS)
All rights reserved.
