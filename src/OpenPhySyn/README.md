# OpenPhySyn

OpenPhySyn is a plugin-based physical synthesis optimization kit developed as part of the [OpenROAD](https://theopenroadproject.org/) flow.

## Default Transforms

By default, the following transforms are built with OpenPhySyn:

-   `constant propagation`: Perform constant propagation optimization across the design hierarchy.
-   `pin_swap`: performs timing-driven/power-driven commutative pin-swapping optimization.
-   `gate_clone`: performs load driven gate cloning.
-   `buffer_fanout`: adds buffers to high fan-out nets.

## Optimization Commands

The main provided commands for optimization are `optimize_design` for physical design optimization and `optimize_logic`.

Currently, `optimize_design` performs pin swapping and load-driven gate-cloning to enhance the design timing. `optimize_logic` performs constant propagation optimization across the design hierarchy.

`optimize_design` options:

-   `[-no_gate_clone]`: Disable gate-cloning.
-   `[-no_pin_swap]`: Disable pin-swap.
-   `[-clone_max_cap_factor <factor>]`: Set gate-cloning capacitance load ratio, defaults to _1.5_.
-   `[-clone_non_largest_cells]`: Allow cloning of cells that are not the largest of their cell-footprint, not recommended.


`optimize_logic` options:

-   `[-no_constant_propagation]`: Disable constant propagation.
-   `[-tihi tihi_cell_name]`: Manually specify the Logic 1 cell for constant propagation.
-   `[-tilo tilo_cell_name]`: Manually specify the Logic 0 cell for constant propagation.

## Dependencies

OpenPhySyn depends on the following libraries:

-   [Flute](https://github.com/The-OpenROAD-Project/flute3) [included]
-   [OpenSTA](https://github.com/The-OpenROAD-Project/OpenSTA) [included]
-   [OpenDB](https://github.com/The-OpenROAD-Project/OpenDB) [included]
-   [SWIG](http://www.swig.org/Doc1.3/Tcl.html)
-   [Doxygen](http://www.doxygen.nl) [included, optional]
-   [Doctests](https://github.com/onqtam/doctest) [included, optional]
