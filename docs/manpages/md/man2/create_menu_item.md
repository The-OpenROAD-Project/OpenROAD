---
title: create_menu_item(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

create_menu_item - create menu item

# SYNOPSIS

create_menu_item [-name name]
                 [-path menu_path]
                 -text item_text
                 -script tcl_script
                 [-shortcut key_shortcut] 
                 [-echo]


# DESCRIPTION

This command add items to the menubar.
Returns: name of the new item, either ``name`` or ``actionX``.

To remove the item: 

```
gui::remove_menu_item name
```

# OPTIONS

`item_text`:  The text to put on the item

`tcl_script`:  The tcl script to evaluate when the button is pressed

`name`:  (optional) name of the item, used when deleting the item

`menu_path`:   (optional) Menu path to place the new item in (hierarchy is separated by /), defaults to "Custom Scripts", but this can also be "Tools" or "New menu/New submenu"

`key_shortcut`:  (optional) key shortcut to trigger this item

`echo`:  (optional) indicate that the commands in the ``tcl_script`` should be echoed in the log.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
