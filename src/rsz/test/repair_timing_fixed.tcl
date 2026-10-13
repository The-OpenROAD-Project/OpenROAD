# repair_timing may VT swap and pin swap a FIRM instance, as they keep its
# footprint, but it may not size one: a wider master would overlap the fixed
# cells around it, and nothing may move it. Every other instance is FIRM, the
# rest PLACED, and a tight clock makes each flow below want to size, so the
# PLACED instances show that sizing still happens.
source "helpers.tcl"
source asap7/asap7.vars
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def
read_sdc gcd.sdc
create_clock -name core_clock -period 150 [get_ports clk]
source asap7/setRC.tcl
estimate_parasitics -placement

set block [ord::get_db_block]
set i 0
foreach inst [$block getInsts] {
  if { [$inst isCore] && [incr i] % 2 } {
    $inst setPlacementStatus FIRM
  }
}

# Snapshot of each core instance's master, width, location, status and
# pin-to-net connections.
proc snapshot { } {
  set snap {}
  foreach inst [[ord::get_db_block] getInsts] {
    if { [$inst isCore] } {
      set master [$inst getMaster]
      set pins {}
      foreach iterm [$inst getITerms] {
        set net [$iterm getNet]
        if { $net ne "NULL" } {
          lappend pins [[$iterm getMTerm] getName] [$net getName]
        }
      }
      dict set snap [$inst getName] \
        [list [$master getName] [$master getWidth] [$inst getLocation] \
          [$inst getPlacementStatus] $pins]
    }
  }
  return $snap
}

# Counts the snapshotted instances with the given status whose field differs
# now.
proc changed { snap field status } {
  set index [dict get {master 0 width 1 location 2 status 3 pins 4} $field]
  set count 0
  set now [snapshot]
  dict for {name before} $snap {
    if {
      [lindex $before 3] eq $status
      && [dict exists $now $name]
      && [lindex [dict get $now $name] $index] ne [lindex $before $index]
    } {
      incr count
    }
  }
  return $count
}

proc run { cmd } {
  set failed [catch { tee -quiet -variable log $cmd } message]
  if { $failed } {
    puts $log
    puts $message
  }
  check "$cmd returns" { set failed } 0
  return $log
}

set snap [snapshot]
run {repair_timing -setup -phases GLOBAL_SIZING}
check "global sizing sizes no FIRM instance" { changed $snap width FIRM } 0
check "global sizing still VT swaps FIRM instances" \
  { expr { [changed $snap master FIRM] > 0 } } 1
check "global sizing still sizes PLACED instances" \
  { expr { [changed $snap width PLACED] > 0 } } 1

set snap [snapshot]
run {repair_timing -setup -skip_last_gasp -max_iterations 20 \
  -sequence sizeup,sizeup_match,size_down,swap}
check "repair_timing sizes no FIRM instance" { changed $snap width FIRM } 0
check "repair_timing moves no FIRM instance" { changed $snap location FIRM } 0
check "repair_timing keeps every FIRM instance FIRM" \
  { changed $snap status FIRM } 0
check "repair_timing still VT swaps FIRM instances" \
  { expr { [changed $snap master FIRM] > 0 } } 1
check "repair_timing still sizes PLACED instances" \
  { expr { [changed $snap width PLACED] > 0 } } 1
check "repair_timing still swaps pins of FIRM instances" \
  { expr { [changed $snap pins FIRM] > 0 } } 1

create_clock -name core_clock -period 400 [get_ports clk]
set snap [snapshot]
run {repair_timing -recover_power 100}
check "recover_power sizes no FIRM instance" { changed $snap width FIRM } 0
check "recover_power still VT swaps FIRM instances" \
  { expr { [changed $snap master FIRM] > 0 } } 1
check "recover_power still sizes PLACED instances" \
  { expr { [changed $snap width PLACED] > 0 } } 1

# With routing parasitics, replaceCell legalizes the new master's position,
# which must leave a FIRM instance alone even off the site grid.
set_routing_layers -signal M2-M7
global_route
estimate_parasitics -global_routing
create_clock -name core_clock -period 150 [get_ports clk]
foreach inst [$block getInsts] {
  if { [$inst isCore] && [$inst getPlacementStatus] eq "FIRM" } {
    lassign [$inst getLocation] x y
    $inst setPlacementStatus PLACED
    $inst setLocation [expr { $x + 1 }] $y
    $inst setPlacementStatus FIRM
  }
}
set snap [snapshot]
run {repair_timing -setup -skip_last_gasp -max_iterations 20 \
  -sequence sizeup,sizeup_match,size_down,swap}
check "legalization moves no FIRM instance" { changed $snap location FIRM } 0
check "routed repair_timing still VT swaps FIRM instances" \
  { expr { [changed $snap master FIRM] > 0 } } 1

exit_summary
