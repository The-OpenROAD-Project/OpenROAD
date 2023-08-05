# Detailed Placement

In OpenROAD, detailed placement is performed using OpenDP, or 
Open-Source Detailed Placement Engine.

Features:

-   Fence region.
-   Fragmented ROWs.

## Commands

### Detailed Placement

The `detailed_placement` command performs detailed placement of instances
to legal locations after global placement.

```tcl
detailed_placement
    [-max_displacement disp|{disp_x disp_y}]
    [-disallow_one_site_gaps]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-max_displacement` | How far an instance (in microns) can be moved when finding a site where it can be placed. Either set one value for both directions or set {disp_x disp_y} for individual directions. (default 0, 0, integer). |
| `-disallow_one_site_gaps` | Disable one site gaps during placement. |

### Set Placement Padding

The `set_placement_padding` command sets left and right padding in multiples
of the row site width. Use the `set_placement_padding` command before
legalizing placement to leave room for routing. Use the `-global` flag
for padding that applies to all instances. Use  `-instances`
for instance-specific padding.  The instances `insts` can be a list of instance
names, or an instance object returned by the SDC `get_cells` command. To
specify padding for all instances of a common master, use the `-filter`
"ref_name == <name>" option to `get_cells`.

```tcl
set_placement_padding   
    -global|-masters masters|-instances insts
    [-right site_count]
    [-left site_count]
    [instances]
```

#### Options

| Switch Name | Description | 
| ----- | ----- |
| `-global` or `-masters` or `-instances` | Either flag must be set. For `-global`, you will set padding globally using `left` and `right` values. For `-masters`, you will set padding only for these masters using `left` and `right` values. For `-instances`, you will set padding only for these insts using `left` and `right` values. |
| `left` | Left padding (in site count). |
| `right` | Right padding (in site count). |

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
    filler_masters
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-prefix` | Prefix to name the filler cells (default "FILLER_"). |
| `filler_masters` | Filler master cells. | 

### Remove Fillers

```tcl
remove_fillers 
```

No arguments are needed for this function. 

### Check Placement

The `check_placement` command checks the placement legality. It returns
`0` if the placement is legal.

```tcl
check_placement
    [-verbose]
    [-disallow_one_site_gaps]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-verbose` | Enable verbose logging. |
| `-disallow_one_site_gaps` | Disable one site gaps during placement. |

### Optimize Mirroring

The `optimize_mirroring` command mirrors instances about the Y axis in
a weak attempt to reduce total half-perimeter wirelength (HPWL).

```tcl
optimize_mirroring
```

No arguments are needed for this function. 

### Useful developer functions

If you are a developer, you might find these useful. More details can be found in the [source file](./src/Opendp.cpp) or the [swig file](./src/Opendp.i).

```tcl
# debug detailed placement
detailed_placement_debug 
    -min_displacement disp
    -instance inst

# get masters from a design
get_masters_arg

# get bounding box of an instance
get_inst_bbox inst_name

# get grid bounding box of an instance
get_inst_grid_bbox inst_name

# format grid (takes in length x and site width w)
format_grid x w

# get row site name
get_row_site
```

## Example scripts

Examples scripts demonstrating how to run OpenDP on a sample design of `aes` as follows:

```shell
./test/aes.tcl
```

## Regression tests

There are a set of regression tests in `/test`. For more information, refer to this [section](../../README.md#regression-tests). 

Simply run the following script: 

```shell
./test/regression
```

## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+opendp+in%3Atitle)
about this tool.

## Authors

-   SangGi Do and Mingyu Woo (respective Ph. D. advisors: Seokhyeong Kang,
    Andrew B. Kahng).
-   Rewrite and port to OpenDB/OpenROAD by James Cherry, Parallax Software

## References
1. Do, S., Woo, M., & Kang, S. (2019, May). Fence-region-aware mixed-height standard cell legalization. In Proceedings of the 2019 on Great Lakes Symposium on VLSI (pp. 259-262). [(.pdf)](https://dl.acm.org/doi/10.1145/3299874.3318012)

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
