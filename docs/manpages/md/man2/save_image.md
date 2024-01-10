---
title: save_image(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/10
---

# NAME

save_image - save image

# SYNOPSIS

save_image [-resolution microns_per_pixel]
           [-area {x0 y0 x1 y1}]
           [-width width]
           [-display_option {option value}]
           filename


# DESCRIPTION

This command can be both be used when the GUI is active and not active
to save a screenshot with various options.

# OPTIONS

`filename`:  path to save the image to.

`x0, y0`:  first corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block

`x1, y1`:  second corner of the layout area (in microns) to be saved, default is to save what is visible on the screen unless called when gui is not active and then it selected the whole block

`microns_per_pixel`:  resolution in microns per pixel to use when saving the image, default will match what the GUI has selected

`width`:  width of the output image in pixels, default will be computed from the resolution. Cannot be used with ``-resolution``

`option`:  specific setting for a display option to show or hide specific elements. For example, to hide metal1 ``-display_option {Layers/metal1 false}``, to show routing tracks ``-display_option {Tracks/Pref true}``, or to show everthing ``-display_option {* true}``

# ARGUMENTS

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
