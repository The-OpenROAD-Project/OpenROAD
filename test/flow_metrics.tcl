## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2019-2026, The OpenROAD Authors

# Functions/variables common to metrics scripts.

proc define_metric { name header1 header2 field_width fmt cmp_op limit_expr } {
  variable metrics
  dict set metrics $name [list $header1 $header2 $field_width $fmt $cmp_op $limit_expr]
}

proc metric_names { } {
  variable metrics
  return [dict keys $metrics]
}

proc metric_header1 { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $header1
}

proc metric_header2 { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $header2
}

proc metric_tool_name { name } {
  regexp "(...)::.*" $name ignore tool
  return $tool
}

proc metric_field_width { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $field_width
}

proc metric_format { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $fmt
}

proc metric_json_key { name } {
  return $name
}

proc metric_cmp_op { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $cmp_op
}

proc metric_limit_expr { name } {
  variable metrics
  lassign [dict get $metrics $name] header1 header2 field_width fmt cmp_op limit_expr
  return $limit_expr
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
define_metric "IFP::instance_count" "" "insts" 7 "%7d" "<" {$value * 1.2}

define_metric "DPL::design_area" "" "area" 7 "%7.0f" "<" {$value * 1.2}
define_metric "DPL::utilization" "" "util" 4 "%4.1f" "<" {$value * 1.2}

define_metric "RSZ::repair_design_buffer_count" "drv" "bufs" 4 "%4d" "<=" {int($value * 1.2)}
define_metric "RSZ::max_slew_slack" "max" "slew" 4 "%3.0f%%" ">=" {min(0, $value * 1.2)}
define_metric "RSZ::max_capacitance_slack" "max" "cap" 4 "%3.0f%%" ">=" {min(0, $value * 1.2)}
define_metric "RSZ::max_fanout_slack" "max" "fanout" 6 "%5.0f%%" ">=" {min(0, $value * 1.2)}

define_metric "RSZ::worst_slack_min" "slack" "min" 5 "%5.2f" ">" {$value - $clock_period * .1}
define_metric "RSZ::worst_slack_max" "slack" "max" 5 "%5.2f" ">" {$value  - $clock_period * .1}
define_metric "RSZ::tns_max" "tns" "max" 5 "%5.1f" ">" \
  {$value - $clock_period * .1 * $instance_count * .1}
define_metric "RSZ::hold_buffer_count" "hold" "bufs" 4 "%4d" "<=" {int($value * 1.2)}

define_metric "GRT::ANT::errors" "" "ANT" 3 "%3d" "<=" {$value}

define_metric "DRT::drv" "" "drv" 3 "%3d" "<=" {$value}
define_metric "DRT::worst_slack_min" "slack" "min" 5 "%5.2f" ">" {$value - $clock_period * .1}
define_metric "DRT::worst_slack_max" "slack" "max" 5 "%5.2f" ">" {$value  - $clock_period * .1}
define_metric "DRT::tns_max" "tns" "max" 5 "%5.1f" ">" \
  {$value - $clock_period * .1 * $instance_count * .1}
define_metric "DRT::clock_skew" "clk" "skew" 5 "%5.2f" "<=" {$value * 1.2}
define_metric "DRT::max_slew_slack" "max" "slew" 4 "%3.0f%%" ">=" {min(0, $value * 1.2)}
define_metric "DRT::max_capacitance_slack" "max" "cap" 4 "%3.0f%%" ">=" {min(0, $value * 1.2)}
define_metric "DRT::max_fanout_slack" "max" "fanout" 6 "%5.0f%%" ">=" {min(0, $value * 1.2)}
define_metric "DRT::clock_period" "" "" 0 "%5.2f" "<=" {$value}
define_metric "DRT::ANT::errors" "" "ANT" 3 "%3d" "<=" {$value}

################################################################

# Used by regression.tcl to check pass/fail metrics test.
# Returns "pass" or failing metric comparison string.
proc check_test_metrics { test lang } {
  # Don't require json until it is really needed.
  package require json

  set metrics_file [test_metrics_result_file $test $lang]
  set metrics_limits_file [test_metrics_limits_file $test]
  if { ![file exists $metrics_file] } {
    return "missing metrics file"
  }
  set stream [open $metrics_file r]
  set json_string [read $stream]
  close $stream
  if { [catch { json::json2dict $json_string } metrics_dict] } {
    return "error parsing metrics json"
  }

  if { ![file exists $metrics_limits_file] } {
    return "missing metrics limits file"
  }
  set stream [open $metrics_limits_file r]
  set json_string [read $stream]
  close $stream
  if { [catch { json::json2dict $json_string } metrics_limits_dict] } {
    return "error parsing metrics limits json"
  }

  set failures ""
  foreach name [metric_names] {
    set json_key [metric_json_key $name]
    set cmp_op [metric_cmp_op $name]
    if { [dict exists $metrics_dict $json_key] } {
      set value [dict get $metrics_dict $json_key]
      if { [dict exists $metrics_limits_dict $json_key] } {
        set limit [dict get $metrics_limits_dict $json_key]
        # tclint-disable-next-line redundant-expr
        if { ![expr $value $cmp_op $limit] } {
          fail "$name [format [metric_format $name] $value]\
                [cmp_op_negated $cmp_op] [format [metric_format $name] $limit]"
        }
      } else {
        fail "missing $name in metric limits"
      }
    } else {
      fail "missing $name in metrics"
    }
  }
  if { $failures == "" } {
    return "pass"
  } else {
    return $failures
  }
}

proc fail { msg } {
  upvar 1 failures failures

  if { $failures != "" } {
    set failures [concat $failures "; $msg"]
  } else {
    set failures $msg
  }
}

################################################################

proc report_flow_metrics_main { } {
  global argc argv test_groups test_langs
  if { $argc == 0 } {
    set tests $test_groups(flow)
  } else {
    set tests [expand_tests $argv]
  }

  report_metrics_header
  foreach test $tests {
    report_test_metrics $test $test_langs($test)
  }
}

proc report_metrics_header { } {
  puts -nonewline [format "%20s" ""]
  foreach name [metric_names] {
    set tool_name [metric_tool_name $name]
    set field_width [metric_field_width $name]
    if { $field_width > 0 } {
      puts -nonewline " [format %${field_width}s $tool_name]"
    }
  }
  puts ""

  puts -nonewline [format "%20s" ""]
  foreach name [metric_names] {
    set header1 [metric_header1 $name]
    set field_width [metric_field_width $name]
    if { $field_width > 0 } {
      puts -nonewline " [format %${field_width}s $header1]"
    }
  }
  puts ""

  puts -nonewline [format "%20s" ""]
  foreach name [metric_names] {
    set header2 [metric_header2 $name]
    set field_width [metric_field_width $name]
    if { $field_width > 0 } {
      puts -nonewline " [format %${field_width}s $header2]"
    }
  }
  puts ""
}

proc report_test_metrics { test $lang } {
  # Don't require json until it is really needed.
  package require json

  set metrics_result_file [test_metrics_result_file $test $lang]
  if { [file exists $metrics_result_file] } {
    set stream [open $metrics_result_file r]
    set json_string [read $stream]
    close $stream
    puts -nonewline [format "%-20s" $test]
    if { ![catch { json::json2dict $json_string } metrics_dict] } {
      foreach name [metric_names] {
        set field_width [metric_field_width $name]
        if { $field_width != 0 } {
          set key [metric_json_key $name]
          if { [dict exists $metrics_dict $key] } {
            set value [dict get $metrics_dict $key]
            if { $value > 1E+20 } {
              set value "INF"
            }
            set field [format [metric_format $name] $value]
          } else {
            set field [format "%${field_width}s" "N/A"]
          }
          puts -nonewline " $field"
        }
      }
    }
    puts ""
  }
}

################################################################

proc report_flow_metric_limits_main { } {
  global argc argv
  if { $argv == "help" || $argv == "-help" } {
    puts {Usage: report_flow_metric_limits test1 [test2]...}
  } else {
    if { $argc == 0 } {
      set tests [expand_tests "flow"]
    } else {
      set tests [expand_tests $argv]
    }
    report_metrics_header
    foreach test $tests {
      report_test_metric_limits $test
    }
  }
}

proc report_test_metric_limits { test } {
  # Don't require json until it is really needed.
  package require json

  set metrics_limits_file [test_metrics_limits_file $test]
  if { ![file exists $metrics_limits_file] } {
    return "missing metrics limits file"
  }
  set stream [open $metrics_limits_file r]
  set json_string [read $stream]
  close $stream
  if { [catch { json::json2dict $json_string } metrics_limits_dict] } {
    return "error parsing metrics limits json"
  }

  puts -nonewline [format "%-20s" $test]
  foreach name [metric_names] {
    set field_width [metric_field_width $name]
    if { $field_width != 0 } {
      set key [metric_json_key $name]
      if { [dict exists $metrics_limits_dict $key] } {
        set limit [dict get $metrics_limits_dict $key]
        set format [metric_format $name]
        if { [string index $format end] == "d" } {
          set limit [expr round($limit)]
        }
        set field [format $format $limit]
      } else {
        set field [format "%${field_width}s" "N/A"]
      }
      puts -nonewline " $field"
    }
  }
  puts ""
}

################################################################

proc compare_flow_metrics_main { } {
  global argc argv test_groups test_langs
  if { $argv == "help" || $argv == "-help" } {
    puts {Usage: save_flow_metrics [test1]...}
  } else {
    set relative 0
    if { $argc >= 1 && [lindex $argv 0] == "-relative" } {
      set relative 1
      set argc [expr $argc - 1]
      set argv [lrange $argv 1 end]
    }
    if { $argc == 0 } {
      set tests $test_groups(flow)
    } else {
      set tests [expand_tests $argv]
    }

    report_metrics_header
    foreach test $tests {
      compare_test_metrics $test $relative $test_langs($test)
    }
  }
}

proc compare_test_metrics { test relative lang } {
  # Don't require json until it is really needed.
  package require json

  set metrics_file [test_metrics_file $test]
  set result_file [test_metrics_result_file $test $lang]
  if {
    [file exists $metrics_file]
    && [file exists $result_file]
  } {
    set metrics_stream [open $metrics_file r]
    set results_stream [open $result_file r]
    set metrics_json [read $metrics_stream]
    set results_json [read $results_stream]
    close $metrics_stream
    close $results_stream
    puts -nonewline [format "%-20s" $test]
    if {
      ![catch { json::json2dict $metrics_json } metrics_dict]
      && ![catch { json::json2dict $results_json } results_dict]
    } {
      foreach name [metric_names] {
        set field_width [metric_field_width $name]
        if { $field_width != 0 } {
          set key [metric_json_key $name]
          if {
            [dict exists $metrics_dict $key]
            && [dict exists $results_dict $key]
          } {
            set metrics_value [dict get $metrics_dict $key]
            set result_value [dict get $results_dict $key]
            if { $metrics_value != 0 && $relative } {
              set delta [expr ($result_value - $metrics_value) * 100.0 / abs($metrics_value)]
              set field [format "%+.1f%%" $delta]
              set field [format "%[string length $short_name]s" $field]
            } else {
              set delta [expr $result_value - $metrics_value]
              set field [format [metric_format $name] $delta]
            }
          } else {
            set field [format "%${field_width}s" "N/A"]
          }
          puts -nonewline " $field"
        }
      }
    }
    puts ""
  }
}

################################################################

# Copy result metrics to test results saved in the repository.
proc save_flow_metrics_main { } {
  global argc argv

  if { $argv == "help" || $argv == "-help" } {
    puts {Usage: save_flow_metrics [test1]...}
  } else {
    if { $argc == 0 } {
      set tests [expand_tests "flow"]
    } else {
      set tests [expand_tests $argv]
    }
    foreach test $tests {
      save_metrics $test
    }
  }
}

proc save_metrics { test } {
  if { [lsearch [group_tests "all"] $test] == -1 } {
    puts "Error: test $test not found."
  } else {
    set metrics_result_file [test_metrics_result_file $test \
      [result_lang $test]]
    set metrics_file [test_metrics_file $test]
    file copy -force $metrics_result_file $metrics_file
  }
}

################################################################

# Save result metrics + margins as metric limits.
proc save_flow_metric_limits_main { } {
  global argc argv
  if { $argv == "help" || $argv == "-help" } {
    puts {Usage: save_flow_metric_limits [failures] test1 [test2]...}
  } else {
    if { $argc == 0 } {
      set tests [expand_tests "flow"]
    } else {
      set tests [expand_tests $argv]
    }
    foreach test $tests {
      save_metric_limits $test [result_lang $test]
    }
  }
}

proc save_metric_limits { test lang } {
  global test_langs
  # Don't require json until it is really needed.
  package require json

  set metrics_file [test_metrics_result_file $test $test_langs($test)]
  set metrics_limits [test_metrics_limits_file $test]
  if { ![file exists $metrics_file] } {
    puts "Error: metrics file $metrics_file not found."
  } else {
    set stream [open $metrics_file r]
    set json_string [read $stream]
    close $stream
    if { ![catch { json::json2dict $json_string } metrics_dict] } {
      set limits_stream [open $metrics_limits w]
      puts $limits_stream "{"
      set first 1
      # Find value of variables used in margin expr's.
      foreach metric {"DRT::clock_period" "IFP::instance_count"} {
        regexp "(...)::(.*)" $metric ignore tool var
        set key [metric_json_key $metric]
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
          set limit_expr [metric_limit_expr $name]
          set value_limit [expr $limit_expr]
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

################################################################

proc test_metrics_file { test } {
  global test_dir
  return [file join $test_dir "$test.metrics"]
}

proc test_metrics_result_file { test lang } {
  global test_dir
  return [file join $test_dir "results" "$test-$lang.metrics"]
}

proc test_metrics_limits_file { test } {
  global test_dir
  return [file join $test_dir "$test.metrics_limits"]
}
