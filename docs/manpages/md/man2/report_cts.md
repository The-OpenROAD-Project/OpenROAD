---
title: report_cts(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

report_cts - report cts

# SYNOPSIS

report_cts 
    [-out_file file]


# DESCRIPTION

This command is used to extract the following metrics after a successful `clock_tree_synthesis` run. 
- Number of Clock Roots
- Number of Buffers Inserted
- Number of Clock Subnets
- Number of Sinks.

# OPTIONS

`-out_file`:  The file to save `cts` reports. If this parameter is omitted, the report is streamed to `stdout` and not saved.

`Command Name`:  Description

`clock_tree_synthesis_debug`:  Option to plot the CTS to GUI.

# ARGUMENTS

This command has no arguments.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
