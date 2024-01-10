---
title: set_display_controls(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

set_display_controls - set display controls

# SYNOPSIS

gui::set_display_controls 
       name 
       [display_type] 
       [value]


# DESCRIPTION

Control the visible and selected elements in the layout:

# OPTIONS

`name`:   is the name of the control. For example, for the power nets option this would be ``Signals/Power`` or could be ``Layers/*`` to set the option for all the layers.

`display_type`:  is either ``visible`` or ``selectable``

`value`: is either ``true`` or ``false``

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
