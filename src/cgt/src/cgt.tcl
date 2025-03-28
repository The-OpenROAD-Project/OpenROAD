## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2025, The OpenROAD Authors

sta::define_cmd_args "clock_gating" { \
                                      [-instances instances]\
                                      [-gate_cond_nets gate_cond_nets]\
                                      [-min_instances min_instances]\
                                      [-max_cover max_cover]\
                                      [-group_instances group_instances]\
                                      [-dump_dir dump_dir]\
                                    }

proc clock_gating { args } {
  sta::parse_key_args "clock_gating" args \
    keys {-instances -gate_cond_nets -min_instances -max_cover -group_instances -dump_dir}

  set instances {}
  set gate_cond_nets {}
  set min_instances 10
  set max_cover 100
  set group_instances ""
  set dump_dir ""

  if { [info exists keys(-instances)] } {
    set instances $keys(-instances)
  }
  if { [info exists keys(-gate_cond_nets)] } {
    set gate_cond_nets $keys(-gate_cond_nets)
  }
  if { [info exists keys(-min_instances)] } {
    set min_instances $keys(-min_instances)
  }
  if { [info exists keys(-max_cover)] } {
    set max_cover $keys(-max_cover)
  }
  if { [info exists keys(-group_instances)] } {
    set group_instances $keys(-group_instances)
  }
  if { [info exists keys(-dump_dir)] } {
    set dump_dir $keys(-dump_dir)
  }

  cgt::clock_gating_cmd $instances $gate_cond_nets $min_instances $max_cover $group_instances \
    $dump_dir
}
