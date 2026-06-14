# Test DRT-0349: LEF58_ENCLOSURE without CUTCLASS qualifier on a layer
# that defines CUTCLASS rules should warn, not silently skip.
source "helpers.tcl"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "obstruction.def"
read_guides "obstruction.guide"

# Add a CUTCLASS rule to mcon via ODB API.
# The existing plain ENCLOSURE rules on mcon (from sky130hs.tlef)
# lack a CUTCLASS qualifier, which should trigger DRT-0349.
set tech [ord::get_db_tech]
set mcon [$tech findLayer mcon]
odb::dbTechLayerCutClassRule_create $mcon VA
set cut_class [$mcon findTechLayerCutClassRule VA]
$cut_class setWidth 170

set_routing_layers -signal met1-met5
detailed_route -verbose 0
