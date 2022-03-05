############################################################################
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################

namespace eval sta {

define_cmd_args "highlight_path" {[-min|-max] pin ^|r|rise|v|f|fall}

proc highlight_path { args } {
  parse_key_args "highlight_path" args keys {} \
    flags {-max -min} 0

  if { [info exists flags(-min)] && [info exists flags(-max)] } {
    sta_error "-min and -max cannot both be specified."
  } elseif [info exists flags(-min)] {
    set min_max "min"
  } elseif [info exists flags(-max)] {
    set min_max "max"
  } else {
    # Default to max path.
    set min_max "max"
  }
  if { [llength $args] == 0 } {
    highlight_path_cmd "NULL"
  } else {
    check_argc_eq2 "highlight_path" $args

    set pin_arg [lindex $args 0]
    set tr [parse_rise_fall_arg [lindex $args 1]]

    set pin [get_port_pin_error "pin" $pin_arg]
    if { [$pin is_hierarchical] } {
      sta_error "pin '$pin_arg' is hierarchical."
    } else {
      foreach vertex [$pin vertices] {
        if { $vertex != "NULL" } {
          set worst_path [vertex_worst_arrival_path_rf $vertex $tr $min_max]
          if { $worst_path != "NULL" } {
            highlight_path_cmd $worst_path
            delete_path_ref $worst_path
          }
        }
      }
    }
  }
}

# redefine sta::sta_warn/error to call utl::warn/error
proc sta_error { id msg } {
  utl::error STA $id $msg
}

proc sta_warn { id msg } {
  utl::warn STA $id $msg
}

rename report_units report_units_raw

proc report_units { args } {

  report_units_raw

  utl::push_metrics_stage "run__flow__platform__{}_units"

  foreach unit {{"time" "timing"} {"power" "power"} {"distance" "distance"}} {
    set utype [lindex $unit 0]
    set umetric [lindex $unit 1]
    utl::metric "$umetric" "[unit_suffix $utype]"
  }

  utl::pop_metrics_stage
}

rename report_tns report_tns_raw

proc report_tns { args } {
  eval [linsert $args 0 report_tns_raw]

  utl::metric_float "timing__setup__tns" [total_negative_slack_cmd "max"]
}

rename report_worst_slack report_worst_slack_raw

proc report_worst_slack { args } {
  eval [linsert $args 0 report_worst_slack_raw]

  utl::metric_float "timing__setup__ws" [worst_slack_cmd "max"]
}

rename report_clock_skew report_clock_skew_raw

proc report_clock_skew { args } {
  global sta_report_default_digits

  parse_key_args "report_clock_skew" args \
    keys {-clock -corner -digits} flags {-setup -hold}
  check_argc_eq0 "report_clock_skew" $args

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    sta_error 419 "report_clock_skew -setup and -hold are mutually exclusive options."
  } elseif { [info exists flags(-setup)] } {
    set setup_hold "setup"
  } elseif { [info exists flags(-hold)] } {
    set setup_hold "hold"
  } else {
    set setup_hold "setup"
  }

  if [info exists keys(-clock)] {
    set clks [get_clocks_warn "-clocks" $keys(-clock)]
  } else {
    set clks [all_clocks]
  }
  set corner [parse_corner_or_all keys]
  if [info exists keys(-digits)] {
    set digits $keys(-digits)
    check_positive_integer "-digits" $digits
  } else {
    set digits $sta_report_default_digits
  }
  if { $clks != {} } {
    report_clk_skew $clks $corner $setup_hold $digits
    utl::metric_float "clock__skew__worst" [worst_clk_skew_cmd [lindex $args 2]]
  }
}

# namespace
}
