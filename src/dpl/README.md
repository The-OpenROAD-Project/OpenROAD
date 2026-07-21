# Detailed Placement

The detailed placement module in OpenROAD (`dpl`) legalizes the result of
global placement: it moves instances to legal sites on rows while
minimizing displacement. It is also used to re-legalize the design after
incremental changes such as resizing and buffer insertion. It originated from OpenDP (Open-Source Detailed
Placement Engine) and now uses a negotiation-based legalizer (NBLG) by
default, with the original OpenDP diamond search available as a legacy
engine. It supports:

-   Mixed-cell-height (1x–4x) designs
-   Fence regions
-   Fragmented rows
-   Placement padding and filler insertion

## Placement Engines

DPL provides two options for legalization. Negotiation legalizer is the default and is slightly slower, but can handle dense designs. Diamond search is faster (linear runtime), although it is not able to handle dense designs. Diamond search is also used at the end of Negotiation for handling potential failed corner cases.

#### NegotiationLegalizer

The default two-pass legalizer based on the NBLG paper. Used on
the `detailed_placement` command. The legacy diamond search engine can be
selected with `-use_diamond_legalizer`.

#### Diamond Search

Former engine performs a BFS-style diamond search from each cell's
global placement position, expanding outward in Manhattan order until a
legal site is found.

# Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Detailed Placement

The `detailed_placement` command performs detailed placement of instances
to legal locations after global placement.

```tcl
detailed_placement
    [-max_displacement disp|{disp_x disp_y}]
    [-disallow_one_site_gaps]
    [-report_file_name filename]
    [-use_diamond_legalizer]
    [-site_search_window sites]
    [-row_search_window rows]
    [-drc_penalty penalty]
    [-disable_window_extension]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-max_displacement` | Max distance that an instance can be moved (in microns) when finding a site where it can be placed. Either set one value for both directions or set `{disp_x disp_y}` for individual directions. The default values are `{0, 0}`, and the allowed values within are integers `[0, MAX_INT]`. |
| `-disallow_one_site_gaps` | Option is deprecated. |
| `-report_file_name` | File name for saving the report to (e.g. `report.json`.) |
| `-incremental` | By default DPL initiates with all instances unplaced. With this flag DPL will check for already legalized instances and set them as placed. |
| `-report_file_name` | File name for saving the report to (e.g. `report.json`.) |
| `-use_diamond_legalizer` | Use the legacy diamond search engine instead of the default NegotiationLegalizer. |
| `-site_search_window` | NegotiationLegalizer: base number of sites a cell may be moved left or right of its initial position, capped by `-max_displacement`. Default `20`, `0` allowed (no horizontal movement). |
| `-row_search_window` | NegotiationLegalizer: base number of rows a cell may be moved up or down from its initial position, capped by `-max_displacement`. Default `5`, `0` allowed (no row changes). |
| `-disable_window_extension` | NegotiationLegalizer: disables all search-window extensions, so the window is fixed to the base `-site_search_window`/`-row_search_window` size regardless of cell size or nearby walls. By default, the effective search window can instead grow past base sizing in two ways: (1) it's extended to at least the cell's own width/height, and (2) extended if cut short by a macro or core boundary. |
| `-drc_penalty` | NegotiationLegalizer: priority to DRC violations, ramped up each iteration to push DRC cleanup later in the run. Lower values tolerate DRC violations early on while overlaps are resolved. Default `5`, `0` allowed (disables the escalating per-candidate penalty, DRC-violating cells still accrue history cost separately). |

### Set Placement Padding

The `set_placement_padding` command sets left and right padding in multiples
of the row site width. Use the `set_placement_padding` command before
legalizing placement to leave room for routing. Use the `-global` flag
for padding that applies to all instances. Use `-instances`
for instance-specific padding.  The instances `insts` can be a list of instance
names, or an instance object returned by the SDC `get_cells` command. To
specify padding for all instances of a common master, use the `-filter`
"ref_name == <name>" option to `get_cells`.

```tcl
set_placement_padding
    -global|-masters masters|-instances insts
    [-right site_count]
    [-left site_count]
```

#### Options

```{warning}
Either one of these flags must be set: `-global | -masters | -instances`.
The order of preference is `global > masters > instances`
```

| Switch Name | Description |
| ----- | ----- |
| `-global` | Set padding globally using `left` and `right` values. |
| `-masters` |  Set padding only for these masters using `left` and `right` values. | 
| `-instances` | For `-instances`, you will set padding only for these insts using `left` and `right` values. |
| `-left` | Left padding (in site count). |
| `-right` | Right padding (in site count). |
| `instances` | Set padding for these list of instances. Not to be confused with the `-instances` switch above. |

### Filler Placement

The `filler_placement` command fills gaps between detail-placed instances
to connect the power and ground rails in the rows. `filler_masters` is a
list of master/macro names to use for filling the gaps. Wildcard matching
is supported, so `FILL*` will match, e.g., `FILLCELL_X1 FILLCELL_X16 FILLCELL_X2
FILLCELL_X32 FILLCELL_X4 FILLCELL_X8`.  To specify a different naming prefix
from `FILLER_` use `-prefix <new prefix>`.

```tcl
filler_placement
    [-prefix prefix]
    [-verbose]
    filler_masters
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-prefix` | Prefix to name the filler cells. The default value is `FILLER_`. |
| `-verbose` | Print the filler cell usage. |
| `filler_masters` | Filler master cells. |

### Remove Fillers

This command removes all filler cells.

```tcl
remove_fillers
```

### Check Placement

The `check_placement` command checks the placement legality. It returns
`0` if the placement is legal.

```tcl
check_placement
    [-verbose]
    [-disallow_one_site_gaps]
    [-report_file_name filename]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-verbose` | Enable verbose logging. |
| `-disallow_one_site_gaps` | Option is deprecated. |
| `-report_file_name` | File name for saving the report to (e.g. `report.json`.) |

### Optimize Mirroring

The `optimize_mirroring` command mirrors instances about the Y axis in
a weak attempt to reduce the total half-perimeter wirelength (HPWL).

```tcl
optimize_mirroring
```

### Improve Placement

The `improve_placement` command optimizes a given placed design.

```tcl
improve_placement
    [-random_seed seed]
    [-max_displacement disp|{disp_x disp_y}]
    [-disallow_one_site_gaps]
```

## Useful Developer Commands

If you are a developer, you might find these useful. More details can be found in the [source file](./src/Opendp.cpp) or the [swig file](./src/Opendp.i).

| Command Name | Description |
| ----- | ----- |
| `detailed_placement_debug` | Debug detailed placement. |
| `get_masters_arg` | Get masters from a design. |
| `get_inst_bbox` | Get bounding box of an instance. |
| `get_inst_grid_bbox` | Get grid bounding box of an instance. |
| `format_grid` | Format grid (takes in length `x` and site width `w` as inputs). |
| `get_row_site` | Get row site name. |

## Example scripts

Examples scripts demonstrating how to run `dpl` on a sample design of `aes` as follows:

```shell
./test/aes.tcl
```

## Regression tests

There are a set of regression tests in `./test`. Refer to this [section](../../README.md#regression-tests) for more information.

Simply run the following script:

```shell
./test/regression
```

## Limitations

The following limitations apply when using the NegotiationLegalizer (default):

1. **Multithreading**: The negotiation pass is single-threaded.
   Extend with the inter-region parallelism from NBLG (Algorithm 2, dynamic
   region adjustment) using OpenMP or std::thread.

2. **Fence region R-tree**: Replace linear scan in `FenceRegion::nearestRect()`
   with a spatial index (Boost.Geometry rtree or OpenROAD's existing RTree)
   for large designs with many fence sub-rectangles.

3. **Row rail inference**: Currently uses row-index parity as a proxy for
   VDD/VSS. Replace with actual LEF pg_pin parsing once available in the
   build context.

## Authors

-   SangGi Do and Mingyu Woo (respective Ph. D. advisors: Seokhyeong Kang,
    Andrew B. Kahng).
-   Rewrite and port to OpenDB/OpenROAD by James Cherry, Parallax Software

## References

1. Do, S., Woo, M., & Kang, S. (2019, May). Fence-region-aware mixed-height standard cell legalization. In Proceedings of the 2019 on Great Lakes Symposium on VLSI (pp. 259-262). [(.pdf)](https://dl.acm.org/doi/10.1145/3299874.3318012)
2. J. Chen et al., "NBLG: A Robust Legalizer for Mixed-Cell-Height Modern Design," IEEE TCAD, vol. 41, no. 11, 2022.
3. L. McMurchie and C. Ebeling, "PathFinder: A negotiation-based performance-driven router for FPGAs," 1995.

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
