---
title: assign_io_bump(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/11
---

# NAME

assign_io_bump - assign io bump

# SYNOPSIS

assign_io_bump 
    -net net
    [-terminal iterm]
    [-dont_route]
    instance


# DESCRIPTION

This command assigns a net to a bump instance.

Example usage:

```
assign_io_bump -net p_ddr_addr_9_o BUMP_6_0
assign_io_bump -net p_ddr_addr_8_o BUMP_6_2
assign_io_bump -net DVSS BUMP_6_4
assign_io_bump -net DVDD BUMP_7_3
assign_io_bump -net DVDD -terminal u_dvdd/DVDD BUMP_8_3
assign_io_bump -net p_ddr_addr_7_o BUMP_7_1
assign_io_bump -net p_ddr_addr_6_o BUMP_7_0
```

# OPTIONS

`-net`:  Net to connect to.

`-terminal`:  Instance terminal to route to.

`-dont_route`:  Flag to indicate that this bump should not be routed, only perform assignment.

`instance`:  Name of the bump.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
