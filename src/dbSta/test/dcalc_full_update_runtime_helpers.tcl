# Shared full timing-update runtime comparison for delay models.
proc dcalc_measure_full_update { } {
  set start_us [clock microseconds]
  sta::find_requireds
  set elapsed_us [expr { [clock microseconds] - $start_us }]
  set wns [sta::worst_slack -max]
  set tns [sta::total_negative_slack -max]
  return [list $elapsed_us $wns $tns]
}

proc dcalc_check_close { metric actual expected tolerance } {
  if { abs($actual - $expected) > $tolerance } {
    error "$metric changed: actual=$actual expected=$expected"
  }
}

proc run_dcalc_full_update_runtime { { sample_count 3 } } {
  set models {
    dmp_ceff_elmore
    dmp_ceff_lambert_w
  }

  # Warm both implementations before collecting interleaved samples.
  foreach model $models {
    set_delay_calculator $model
    sta::find_requireds
  }

  array set runtime_sum_us {}
  array set reference_wns {}
  array set reference_tns {}

  # Interleave samples without asserting a machine-dependent speed relationship.
  for { set round 1 } { $round <= $sample_count } { incr round } {
    foreach model $models {
      set_delay_calculator $model
      lassign [dcalc_measure_full_update] elapsed_us wns tns

      if { $elapsed_us <= 0 } {
        error "$model full timing update reported a non-positive runtime"
      }
      if { $round == 1 } {
        set reference_wns($model) $wns
        set reference_tns($model) $tns
        set runtime_sum_us($model) 0
      } else {
        dcalc_check_close \
          "$model WNS" $wns $reference_wns($model) 1e-12
        dcalc_check_close \
          "$model TNS" $tns $reference_tns($model) 1e-12
      }

      incr runtime_sum_us($model) $elapsed_us
      puts [format \
        "round=%2d model=%-20s runtime_us=%10d wns=%16.6f tns=%16.6f" \
        $round $model $elapsed_us $wns $tns]
    }
  }

  # Report both averages
  foreach model $models {
    set average_us [expr { $runtime_sum_us($model) / double($sample_count) }]
    puts [format \
      "average  model=%-20s runtime_us=%10.0f wns=%16.6f tns=%16.6f" \
      $model $average_us $reference_wns($model) $reference_tns($model)]
  }

  puts "pass"
}
