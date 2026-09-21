# write_abstract_lef on a design with a non-rectangular (polygon) die area.
# The MACRO SIZE covers the bounding box of the polygon, and the actual
# shape is described by an OVERLAP layer POLYGON in the OBS.  The virtual
# blockages odb creates outside the polygon must not be written as
# obstructions.
source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_lef "Nangate45/Nangate45_stdcell.lef"
read_def "abstract_polygon.def"

set lef_file [make_result_file abstract_polygon.lef]
write_abstract_lef $lef_file
diff_file $lef_file "abstract_polygon.lefok"

set bloat_lef_file [make_result_file abstract_polygon_bloat.lef]
write_abstract_lef -bloat_occupied_layers $bloat_lef_file
diff_file $bloat_lef_file "abstract_polygon_bloat.lefok"
