---
title: report_floating_nets(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

report_floating_nets - report floating nets

# SYNOPSIS

report_floating_nets 
    [-verbose]


# DESCRIPTION

The `report_floating_nets` command reports nets with only one pin connection.

# OPTIONS

`-verbose`:  Print the net names.

`Command Name`:  Description

`repair_setup_pin`:  Repair setup pin violation.

`check_parasitics`:  Check if the `estimate_parasitics` command has been called.

`parse_time_margin_arg`:  Get the raw value for timing margin (e.g. `slack_margin`, `setup_margin`, `hold_margin`)

`parse_percent_margin_arg`:  Get the above margin in perentage format.

`parse_margin_arg`:  Same as `parse_percent_margin_arg`.

`parse_max_util`:  Check maximum utilization.

`parse_max_wire_length`:  Get maximum wirelength.

`check_corner_wire_caps`:  Check wire capacitance for corner.

`check_max_wire_length`:  Check if wirelength is allowed by rsz for minimum delay.

`dblayer_wire_rc`:  Get layer RC values.

`set_dblayer_wire_rc`:  Set layer RC values.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
