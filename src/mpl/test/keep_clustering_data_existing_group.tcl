# An instance that already belongs to someone else's group — a UPF power domain,
# a region the placer honors — must stay in it.  A dbInst belongs to exactly one
# dbGroup and dbGroup::addInst silently moves it, so writing the clustering data
# over such an instance would take it out of the group dpl/gpl read (see
# dbToOpendp.cpp, which builds its regions from dbGroup::getInsts).
#
# The DEF's GROUPS section carries each group's exact membership, so the golden
# file is the check: pd_always_on keeps _001_/_002_ and no cluster claims them.
# MPL-0078 in the log says the same thing in one line.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

# Stand-in for a power domain: a non-VISUAL_DEBUG group holding two instances.
set block [ord::get_db_block]
set domain [odb::dbGroup_create $block "pd_always_on"]
foreach inst_name {_001_ _002_} {
  $domain addInst [$block findInst $inst_name]
}

set_thread_count 0
rtl_macro_placer -keep_clustering_data \
  -report_directory results/keep_clustering_data_existing_group

set def_file [make_result_file "keep_clustering_data_existing_group.def"]
write_def $def_file
diff_files $def_file "keep_clustering_data_existing_group.defok"
