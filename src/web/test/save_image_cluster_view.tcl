# End-to-end check of `save_image -web -display_option ...`, including the
# cluster overlay over MPL's clustering data.
#
# The options travel from Tcl to the renderer as a JSON payload, and a
# wrongly-typed value makes save_image_cmd drop the whole payload with a
# WEB-0042 warning, silently rendering the default image.  Asserting on the
# rendered bytes is what catches that.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45.def

set base [make_result_file "save_image_cluster_base.png"]
set no_stdcells [make_result_file "save_image_cluster_no_stdcells.png"]
set colored [make_result_file "save_image_cluster_colored.png"]

proc read_png { path } {
  set fh [open $path rb]
  fconfigure $fh -translation binary
  set data [read $fh]
  close $fh
  return $data
}

save_image -web -width 200 $base
save_image -web -width 200 -display_option {stdcells false} $no_stdcells

# Cluster coloring needs dbGroups; build a couple by hand so the test does not
# depend on running the macro placer.
set block [ord::get_db_block]
set insts [$block getInsts]
set half [expr { [llength $insts] / 2 }]
set group_a [odb::dbGroup_create $block "cluster_a"]
$group_a setType "VISUAL_DEBUG"
set group_b [odb::dbGroup_create $block "cluster_b"]
$group_b setType "VISUAL_DEBUG"
set i 0
foreach inst $insts {
  if { $i < $half } {
    $group_a addInst $inst
  } else {
    $group_b addInst $inst
  }
  incr i
}

save_image -web -width 200 \
  -display_option {cluster_view true} \
  $colored

set failures {}

# A display option that hides most of the design must change the image.
if { [read_png $base] eq [read_png $no_stdcells] } {
  lappend failures "-display_option {stdcells false} did not change the image"
}

if { [read_png $base] eq [read_png $colored] } {
  lappend failures "cluster_view did not color the image"
}

if { [llength $failures] > 0 } {
  foreach failure $failures {
    puts "FAILURE: $failure"
  }
} else {
  puts pass
}
