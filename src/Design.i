// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

%typemap(in,numinputs=0) ord::Design* design {
  $1 = static_cast<ord::Design*>(Tcl_GetAssocData(interp, "design", nullptr));
}
