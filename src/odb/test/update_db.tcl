# This script is used to update the files required for the "read_db.tcl" test
source "helpers.tcl"

read_lef "data/gscl45nm.lef"
read_def "data/design.def"
write_db "data/design.odb"

