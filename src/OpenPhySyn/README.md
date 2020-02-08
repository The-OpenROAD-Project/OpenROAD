# OpenPhySyn

OpenPhySyn is a plugin-based physical synthesis optimization kit developed as part of the [OpenROAD](https://theopenroadproject.org/) flow.


## Default Transforms

By default, the following transforms are built with OpenPhySyn:

- `hello_transform`: a demo transform that adds a random wire.
- `buffer_fanout`: adds buffers to high fan-out nets.
- `gate_clone`: performs load driven gate cloning.
- `pin_swap`: performs timing-driven/power-driven commutative pin-swapping optimization.


## Optimization Command(s)

The main provided command for timing optimization is `optimize_design`.

Currently, it performs pin swapping and load-driven gate-cloning to enhance the design timing.

Available options are:
- `[-no_gate_clone]`: Disable gate-cloning.
- `[-no_pin_swap]`: Disable pin-swap.
- `[-clone_max_cap_factor <factor>]`: Set gate-cloning capacitance load ratio, defaults to *1.5*.
- `[-clone_non_largest_cells]`: Allow cloning of cells that are not the largest of their cell-footprint, not recommended.


## Dependencies

OpenPhySyn depends on the following libraries:

- [Flute](https://github.com/The-OpenROAD-Project/flute3) [included]
- [OpenSTA](https://github.com/The-OpenROAD-Project/OpenSTA) [included]
- [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB) [included]
- [SWIG](http://www.swig.org/Doc1.3/Tcl.html)
- [Doxygen](http://www.doxygen.nl) [included, optional]
- [Doctests](https://github.com/onqtam/doctest) [included, optional]

## Issues

Please open a GitHub [issue](https://github.com/The-OpenROAD-Project/OpenPhySyn/issues/new) if you find any bugs.

## Building Custom Transforms

Physical Synthesis transforms libraries are loaded from the directory referred to by the variable `PSN_TRANSFORM_PATH`, defaulting to `./transforms`.

To build a new transform, refer to the transform [template](https://github.com/The-OpenROAD-Project/OpenPhySynHelloTransform).
