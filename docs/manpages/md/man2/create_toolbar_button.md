---
title: create_toolbar_button(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

create_toolbar_button - create toolbar button

# SYNOPSIS

create_toolbar_button [-name name]
                      -text button_text
                      -script tcl_script 
                      [-echo]


# DESCRIPTION

This command creates toolbar button with name set using the
`-text` flag and accompanying logic in the `-script` flag.

Returns: name of the new button, either ``name`` or ``buttonX``.

To remove the button: 

```
gui::remove_toolbar_button name
```

# OPTIONS

`button_text`:  The text to put on the button.

`tcl_script`:  The tcl script to evaluate when the button is pressed.

`name`:  The name of the button, used when deleting the button.

`echo`:  This indicate that the commands in the ``tcl_script`` should be echoed in the log.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
