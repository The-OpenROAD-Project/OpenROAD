############################################################################
##
## Copyright (c) 2019, OpenROAD
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

# Functions/variables common to metrics scripts.

package require json

proc define_metric { name short_name fmt cmp_op margin_expr } {
  variable metrics
  dict set metrics $name [list $short_name $fmt $cmp_op $margin_expr]
}

proc metric_names {} {
  variable metrics
  return [dict keys $metrics]
}

proc metric_short_name { name } {
  variable metrics
  lassign [dict get $metrics $name] short_name fmt cmp_op margin_expr
  return $short_name
}

proc metric_format { name } {
  variable metrics
  lassign [dict get $metrics $name] short_name fmt cmp_op margin_expr
  return $fmt
}

proc metric_json_key { name } {
  return $name
}

proc metric_cmp_op { name } {
  variable metrics
  lassign [dict get $metrics $name] short_name fmt cmp_op margin_expr
  return $cmp_op
}

proc metric_margin_expr { name } {
  variable metrics
  lassign [dict get $metrics $name] short_name fmt cmp_op margin_expr
  return $margin_expr
}

proc cmp_op_negated { cmp_op } {
  if { $cmp_op == "<" } {
    return ">="
  } elseif { $cmp_op == "<=" } {
    return ">"
  } elseif { $cmp_op == ">" } {
    return "<="
  } elseif { $cmp_op == ">=" } {
    return "<"
  } elseif { $cmp_op == "==" } {
    return "!="
  } else {
    error "unknown comparison operator"
  }
}

# make format field width to match short name width
define_metric "instance_count" "  insts" "%7d" "<" {$value * .2}
define_metric "design_area" "   area" "%7.0f" "<" {$value * .2}
define_metric "utilization" "util" "%4.1f" "<" {$value * .2}
define_metric "worst_slack_min" "slack_min" "%9.3f" ">" {-$clock_period * .1}
define_metric "worst_slack_max" "slack_max" "%9.3f" ">" {-$clock_period * .1}
define_metric "tns_max" " tns_max" "%8.1f" ">" {-$clock_period * .1 * $instance_count * .1}
define_metric "skew_setup" "skew_setup" "%10.3f" "<=" {0}
define_metric "skew_hold" "skew_hold" "%9.3f" "<=" {0}
define_metric "max_slew_violations" "max_slew" "%8d" "<=" {0}
define_metric "max_capacitance_violations" "max_cap" "%7d" "<=" {0}
define_metric "max_fanout_violations" "max_fanout" "%10d" "<=" {0}
define_metric "DPL::errors" "DPL" "%3d" "<=" {0}
define_metric "ANT::errors" "ANT" "%3d" "<=" {0}
define_metric "DRT::drv" "drv" "%3d" "<=" {0}
define_metric "clock_period" "" "%d" "<=" {0}

################################################################

proc save_metrics_limits_main {} {
  global argv
  if { $argv == "help" || $argv == "-help" } {
    puts {Usage: save_flow_metrics [failures] test1 [test2]...}
  } elseif { $argv == "failures" } {
    global failure_file
    if [file exists $failure_file] {
      set fail_ch [open $failure_file "r"]
      while { ! [eof $fail_ch] } {
	set test [gets $fail_ch]
	if { $test != "" } {
	  save_metrics $test
	}
      }
      close $fail_ch
    }
  } else {
    set tests [expand_tests $argv]
    foreach test $tests {
      save_metrics_limits $test
    }
  }
}

proc save_metrics_limits { test } {
  if { [lsearch [group_tests "all"] $test] == -1 } {
    puts "Error: test $test not found."
  } else {
    set metrics_file [test_metrics_file $test]
    set metrics_limits [test_metrics_limits_file $test]
    if { ! [file exists $metrics_file] } {
      puts "Error: metrics file $metrics_file not found."
    } else {
      set stream [open $metrics_file r]
      set json_string [read $stream]
      close $stream
      if { ![catch {json::json2dict $json_string} metrics_dict] } {
        set limits_stream [open $metrics_limits w]
        puts $limits_stream "{"
        set first 1
        # Find value of variables used in margin expr's.
        foreach var {"clock_period" "instance_count"} {
          set key [metric_json_key $var]
          if { [dict exists $metrics_dict $key] } {
            set value [dict get $metrics_dict $key]
            set $var $value
          } else {
            puts "Error: $test missing $var metric."
            return
          }
        }
        foreach name [metric_names] {
          set key [metric_json_key $name]
          if { [dict exists $metrics_dict $key] } {
            set value [dict get $metrics_dict $key]
            set margin_expr [metric_margin_expr $name]
            set margin [expr $margin_expr]
            set value_limit [expr $value + $margin]
            if { $first } {
              puts -nonewline $limits_stream "  "
            } else {
              puts -nonewline $limits_stream " ,"
            }
            puts $limits_stream "\"$key\" : \"$value_limit\""
            set first 0
          }
        }

        puts $limits_stream "}"
        close $limits_stream
      } else {
        puts "Error: json parse error $metrics_dict"
      }
    }
  }
}

proc test_metrics_file { test } {
  global test_dir
  return [file join $test_dir "results" "$test.metrics"]
}

proc test_metrics_limits_file { test } {
  global test_dir
  return [file join $test_dir "$test.metrics_limits"]
}

################################################################

proc check_flow_metrics { test } {
  set metrics_file [test_metrics_file $test]
  set metrics_limits_file [test_metrics_limits_file $test]
  if { ![file exists $metrics_file] } {
    return "missing metrics file"
  }
  set stream [open $metrics_file r]
  set json_string [read $stream]
  close $stream
  if { [catch {json::json2dict $json_string} metrics_dict] } {
    return "error parsing metrics json"
  }

  if { ![file exists $metrics_limits_file] } {
    return "missing metrics limits file"
  }
  set stream [open $metrics_limits_file r]
  set json_string [read $stream]
  close $stream
  if { [catch {json::json2dict $json_string} metrics_limits_dict] } {
    return "error parsing metrics limits json"
  }

  foreach name [metric_names] {
    set json_key [metric_json_key $name]
    set cmp_op [metric_cmp_op $name]
    if { ![dict exists $metrics_dict $json_key] } {
      return "missing $name in metrics"
    }
    set value [dict get $metrics_dict $json_key]
    if { ![dict exists $metrics_limits_dict $json_key] } {
      return "missing $name in metric limits"
    }
    set limit [dict get $metrics_limits_dict $json_key]
    if { ![expr $value $cmp_op $limit] } {
      return "$name [format [metric_format $name] $value] [cmp_op_negated $cmp_op] [format [metric_format $name] $limit]"
    }
  }
  return "pass"
}

################################################################

proc report_flow_metrics { tests } {
  report_metrics_header
  foreach test $tests {
    report_test_metrics $test
  }
}

proc report_metrics_header {} {
  puts -nonewline [format "%-20s" ""]
  foreach name [metric_names] {
    set short_name [metric_short_name $name]
    if { $short_name != "" } {
      puts -nonewline " $short_name"
    }
  }
  puts ""
}

proc report_test_metrics { test } {
  set metrics_file [test_metrics_file $test]
  if { [file exists $metrics_file] } {
    set stream [open $metrics_file r]
    set json_string [read $stream]
    close $stream
    puts -nonewline [format "%-20s" $test]
    if { ![catch {json::json2dict $json_string} metrics_dict] } {
      foreach name [metric_names] {
        set short_name [metric_short_name $name]
        if { $short_name != "" } {
          set key [metric_json_key $name]
          if { [dict exists $metrics_dict $key] } {
            set value [dict get $metrics_dict $key]
            set field [format [metric_format $name] $value]
          } else {
            set field "N/A"
          }
          puts -nonewline " $field"
        }
      }
    }
    puts ""
  }
}
