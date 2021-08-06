# Write an SDC file with clock periods that result in slightly negative slack.

# slack margin for updated clock as a percent of clock period
set margin 5

set clks [all_clocks]
if { [llength $clks] == 0 } {
  utl::warn "FLW" 6 "No clocks found."
} else {
  set clk [lindex $clks 0]
  set clk_name [get_name $clk]
  set period [get_property $clk "period"]
  # Period is in sdc/liberty units.
  utl::info "FLW" 7 "clock $clk_name period $period"

  if { [llength $clks] == 1 } {
    set slack [sta::time_sta_ui [sta::worst_slack_cmd "max"]]
    set ref_period [expr ($period - $slack) * (1.0 - $margin/100.0)]
    utl::info "FLW" 8 "Clock $clk_name period [format %.3f $ref_period]"
    utl::info "FLW" 9 "Clock $clk_name slack [format %.3f $slack]"

    set sources [$clk sources]
    # Redefine clock with updated period.
    create_clock -name $clk_name -period $ref_period $sources
    # Undo the set_propagated_clock so SDC at beginning of flow uses ideal clocks.
    unset_propagated_clock [all_clocks]
    write_sdc [file join $env(RESULTS_DIR) "updated_clks.sdc"]
  } else {
    utl::warn "FLW" 10 "more than one clock found. Skipping sdc update."
  }
}

utl::info "FLW" 11 "Path endpoint count [sta::endpoint_count]"
