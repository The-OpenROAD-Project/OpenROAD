---
title: configure_cts_characterization(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

configure_cts_characterization - configure cts characterization

# SYNOPSIS

configure_cts_characterization 
    [-max_slew max_slew]
    [-max_cap max_cap]
    [-slew_steps slew_steps]
    [-cap_steps cap_steps]


# DESCRIPTION

Configure key CTS characterization parameters, for example maximum slew and capacitance,
as well as the number of steps they will be divided for characterization.

# OPTIONS

`-max_slew`:  Max slew value (in the current time unit) that the characterization will test. If this parameter is omitted, the code would use max slew value for specified buffer in `buf_list` from liberty file.

`-max_cap`:  Max capacitance value (in the current capacitance unit) that the characterization will test. If this parameter is omitted, the code would use max cap value for specified buffer in `buf_list` from liberty file.

`-slew_steps`:  Number of steps that `max_slew` will be divided into for characterization. The default value is `12`, and the allowed values are integers `[0, MAX_INT]`.

`-cap_steps`:  Number of steps that `max_cap` will be divided into for characterization. The default value is `34`, and the allowed values are integers `[0, MAX_INT]`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
