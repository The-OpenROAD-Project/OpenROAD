## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2025, The OpenROAD Authors

sta::define_cmd_args "clock_gating" { \
                                      [-min_instances min_instances]\
                                      [-max_cover max_cover]\
                                      [-dump_dir dump_dir]\
                                    }

proc clock_gating { args } {
  sta::parse_key_args "clock_gating" args \
    keys {-min_instances -max_cover -dump_dir} flags {}

  set dump_dir ""

  if { [info exists keys(-min_instances)] } {
    cgt::set_min_instances $keys(-min_instances)
  }
  if { [info exists keys(-max_cover)] } {
    cgt::set_max_cover $keys(-max_cover)
  }
  if { [info exists keys(-dump_dir)] } {
    set dump_dir $keys(-dump_dir)
  }

  cgt::clock_gating_cmd $dump_dir
}
