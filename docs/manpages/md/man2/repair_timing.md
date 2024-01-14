---
title: repair_timing(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

repair_timing - repair timing

# SYNOPSIS

repair_timing 
    [-setup]
    [-hold]
    [-setup_margin setup_margin]
    [-hold_margin hold_margin]
    [-allow_setup_violations]
    [-repair_tns tns_end_percent]
    [-max_utilization util]
    [-max_buffer_percent buffer_percent]
    [-verbose]


# DESCRIPTION

The `repair_timing` command repairs setup and hold violations.  It
should be run after clock tree synthesis with propagated clocks.
Setup repair is done before hold repair so that hold repair does not
cause setup checks to fail.

The worst setup path is always repaired.  Next, violating paths to
endpoints are repaired to reduced the total negative slack.

# OPTIONS

`-setup`:  Repair setup timing.

`-hold`:  Repair hold timing.

`-setup_margin`:  Add additional setup slack margin.

`-hold_margin`:  Add additional hold slack margin.

`-allow_setup_violations`:  While repairing hold violations, buffers are not inserted that will cause setup violations unless `-allow_setup_violations` is specified.

`-repair_tns`:  Percentage of violating endpoints to repair (0-100). When `tns_end_percent` is zero (the default), only the worst endpoint is repaired. When `tns_end_percent` is 100, all violating endpoints are repaired.

`-max_utilization`:  Defines the percentage of core area used.

`-max_buffer_percent`:  Specify a maximum number of buffers to insert to repair hold violations as a percentage of the number of instances in the design. The default value is `20`, and the allowed values are integers `[0, 100]`.

`-verbose`:  Enable verbose logging of the repair progress.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
