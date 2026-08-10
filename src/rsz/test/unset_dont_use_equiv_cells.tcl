# Liberty dont_use only seeds the resizer dont_use policy, so a cell family
# that is dont_use in liberty and re-enabled with unset_dont_use has to become
# a sizing candidate.  Vendor flows rely on this to size multibit flops that
# their liberty marks dont_use.

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def "gcd_nangate45_placed.def"

set_wire_rc -layer metal3
estimate_parasitics -placement

# FILLCELL_X* are marked dont_use in the Nangate45 liberty.
report_equiv_cells FILLCELL_X1

unset_dont_use "FILLCELL_*"

report_equiv_cells FILLCELL_X1

# The equivalence classes are built without dont_use filtering, so check that
# set_dont_use still removes a cell from the candidate list.
report_equiv_cells BUF_X2

set_dont_use "BUF_X4"

report_equiv_cells BUF_X2
