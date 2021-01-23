# slack map api
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def
read_sdc gcd.sdc

set_dont_use {AOI211_X1 OAI211_X1}
set_wire_rc -layer metal3

remove_buffers
rsz::resize_slack_preamble

rsz::find_resize_slacks
foreach net [rsz::resize_worst_slack_nets] {
  puts "[get_full_name $net] [sta::format_time [rsz::resize_net_slack $net] 3]"
}
