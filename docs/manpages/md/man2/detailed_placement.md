---
title: detailed_placement(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

detailed_placement - detailed placement

# SYNOPSIS

detailed_placement
    [-max_displacement disp|{disp_x disp_y}]
    [-disallow_one_site_gaps]
    [-report_file_name filename]


# DESCRIPTION

The `detailed_placement` command performs detailed placement of instances
to legal locations after global placement.

# OPTIONS

`-max_displacement`:  Max distance that an instance can be moved (in microns) when finding a site where it can be placed. Either set one value for both directions or set `{disp_x disp_y}` for individual directions. The default values are `{500, 100}`, and the allowed values within are integers `[0, MAX_INT]`.

`-disallow_one_site_gaps`:  Disable one site gap during placement check.

`-report_file_name`:  File name for saving the report to (e.g. `report.json`.)

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
