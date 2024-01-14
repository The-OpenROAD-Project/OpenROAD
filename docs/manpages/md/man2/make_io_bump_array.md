---
title: make_io_bump_array(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

make_io_bump_array - make io bump array

# SYNOPSIS

make_io_bump_array 
    -bump master
    -origin {x y}
    -rows rows
    -columns columns
    -pitch {x y}
    [-prefix prefix]


# DESCRIPTION

This command defines a bump array.

Example usage:

```
make_io_bump_array -bump BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200 200"
```

# OPTIONS

`-bump`:  Name of the bump master.

`-origin`:  Origin of the array.

`-rows`:  Number of rows to create.

`-columns`:  Number of columns to create.

`-pitch`:  Pitch of the array.

`-prefix`:  Name prefix for the bump array. The default value is `BUMP_`.

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
