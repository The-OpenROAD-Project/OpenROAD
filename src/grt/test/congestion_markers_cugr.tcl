# Verify that running CUGR on a heavily-adjusted design populates the
# "Global route" -> "Horizontal/Vertical congestion" dbMarkerCategory tree
# with markers that are layer-tagged, carry the
# "capacity:N usage:M congestion:K (wires|vias|wires + vias)" comment
# format, and list at least one source net.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -use_cugr -verbose

set block [ord::get_db_block]

set tool_category ""
foreach cat [$block getMarkerCategories] {
  if { [$cat getName] == "Global route" } {
    set tool_category $cat
  }
}

check "Global route marker category exists" {
  expr { $tool_category != "" }
} 1

set subcat_names {}
set total_markers 0
set markers_with_sources 0
set markers_with_comment 0
set markers_with_layer 0

if { $tool_category != "" } {
  foreach sub [$tool_category getMarkerCategories] {
    lappend subcat_names [$sub getName]
    foreach marker [$sub getMarkers] {
      incr total_markers
      if { [llength [$marker getSources]] > 0 } {
        incr markers_with_sources
      }
      if {
        [regexp \
          {capacity:[0-9]+ usage:[0-9]+ congestion:-?[0-9]+ \((wires|vias|wires \+ vias)\)} \
          [$marker getComment]]
      } {
        incr markers_with_comment
      }
      if { [$marker getTechLayer] != "NULL" } {
        incr markers_with_layer
      }
    }
  }
}

check "Only H/V congestion subcategories are created" {
  set valid 1
  foreach name $subcat_names {
    if { $name != "Horizontal congestion" && $name != "Vertical congestion" } {
      set valid 0
    }
  }
  expr { $valid }
} 1

check "Marker tree is non-empty" {
  expr { $total_markers > 0 }
} 1

check "Every marker comment matches capacity/usage/congestion (kind) format" {
  expr { $markers_with_comment == $total_markers }
} 1

check "Every marker is tagged with a tech layer" {
  expr { $markers_with_layer == $total_markers }
} 1

check "Every marker has at least one source net" {
  expr { $markers_with_sources == $total_markers }
} 1

exit_summary
