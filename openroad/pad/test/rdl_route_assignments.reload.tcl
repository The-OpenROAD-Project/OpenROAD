# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

source "helpers.tcl"

read_db [make_result_file "rdl_route_assignments.odb"]

rdl_route -layer metal10 -width 4 -spacing 4 "DVDD"

set def_file [make_result_file "rdl_route_assignments.def"]
write_def $def_file
diff_files $def_file "rdl_route_assignments.defok"
