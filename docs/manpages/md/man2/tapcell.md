---
title: tapcell(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

tapcell - tapcell

# SYNOPSIS

tapcell 
    [-tapcell_master tapcell_master]
    [-endcap_master endcap_master]
    [-distance dist]
    [-halo_width_x halo_x]
    [-halo_width_y halo_y]
    [-tap_nwin2_master tap_nwin2_master]
    [-tap_nwin3_master tap_nwin3_master]
    [-tap_nwout2_master tap_nwout2_master]
    [-tap_nwout3_master tap_nwout3_master]
    [-tap_nwintie_master tap_nwintie_master]
    [-tap_nwouttie_master tap_nwouttie_master]
    [-cnrcap_nwin_master cnrcap_nwin_master]
    [-cnrcap_nwout_master cnrcap_nwout_master]
    [-incnrcap_nwin_master incnrcap_nwin_master]
    [-incnrcap_nwout_master incnrcap_nwout_master]
    [-tap_prefix tap_prefix]
    [-endcap_prefix endcap_prefix]


# DESCRIPTION

This command inserts tapcells or endcaps.

The figures below show two examples of tapcell insertion. When only the 
`-tapcell_master` and `-endcap_master` masters are given, the tapcell placement
is similar to Figure 1. When the remaining masters are give, the tapcell
placement is similar to Figure 2.

| <img src="./doc/image/tapcell_example1.svg" width=450px> | <img src="./doc/image/tapcell_example2.svg" width=450px> |
|:--:|:--:|
| Figure 1: Tapcell insertion representation | Figure 2:  Tapcell insertion around macro representation |

# OPTIONS

`Figure 1: Tapcell insertion representation`:  Figure 2:  Tapcell insertion around macro representation

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
