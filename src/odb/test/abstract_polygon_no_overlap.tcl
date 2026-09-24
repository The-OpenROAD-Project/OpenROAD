# write_abstract_lef on a design with a non-rectangular (polygon) die area
# whose tech has no TYPE OVERLAP layer.  The shape cannot be described, so
# ODB-0033 is issued and the OVERLAP section is dropped, but the rest of the
# macro -- including the -bloat_occupied_layers polygons -- is still written
# from the polygon die area rather than its bounding box.
source "helpers.tcl"

read_lef "abstract_polygon_no_overlap.lef"
read_def "abstract_polygon.def"

set lef_file [make_result_file abstract_polygon_no_overlap.lef]
write_abstract_lef $lef_file
diff_file $lef_file "abstract_polygon_no_overlap.lefok"

set bloat_lef_file [make_result_file abstract_polygon_no_overlap_bloat.lef]
write_abstract_lef -bloat_occupied_layers $bloat_lef_file
diff_file $bloat_lef_file "abstract_polygon_no_overlap_bloat.lefok"
