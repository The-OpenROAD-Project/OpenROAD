# A cycle simulator of the generated block, for tests that check what an
# option is meant to do (a word written reads back, a reset clears every
# word, ...) rather than what the generator happens to emit.
#
# Combinational cells are evaluated from their Liberty functions. Flops
# and clock gates have no function OpenSTA exposes to Tcl, so the few the
# tests use are modelled below, one line each, from their datasheets.

namespace eval ram_sim {
# master -> {clock data {output_pin polarity ...} set set_active reset reset_active}
# polarity: q (the stored bit) or qn (its complement); set/reset ""
# when the flop has none; active 0 or 1.
variable flop_models
array set flop_models {
    DFFHQNx1_ASAP7_75t_R {CLK D {QN qn} "" 0 "" 0}
    DFFASRHQNx1_ASAP7_75t_R {CLK D {QN qn} SETN 0 RESETN 0}
    DFF_X1 {CK D {Q q QN qn} "" 0 "" 0}
    DFFRS_X1 {CK D {Q q QN qn} SN 0 RN 0}
    sky130_fd_sc_hd__dfxtp_1 {CLK D {Q q} "" 0 "" 0}
  }
# master -> {clock enable test_enable gated_clock}: the enable is
# latched while the clock is low
variable gate_models
array set gate_models {
    ICGx1_ASAP7_75t_R {CLK ENA SE GCLK}
    CLKGATETST_X1 {CK E SE GCK}
    CLKGATE_X1 {CK E "" GCK}
    sky130_fd_sc_hd__dlclkp_1 {CLK GATE "" GCLK}
  }

variable value
variable net_ids
variable comb_order
variable comb_all
variable port_ids
variable flops
variable gates
variable state
variable latched
variable clock_net
}

# Nets by integer id: names carry brackets, which Tcl would substitute
# inside an expression.
proc ram_sim::net_name { net } {
  variable net_ids
  set name [$net getName]
  if { ![info exists net_ids($name)] } {
    set net_ids($name) [array size net_ids]
  }
  return $net_ids($name)
}

# Liberty function syntax to a Tcl expression over value(<net>).
proc ram_sim::to_expr { function pins } {
  set e $function
  regsub -all {\&} $e {*} e
  regsub -all {\|} $e {+} e
  # tokens: identifiers become net lookups
  set out ""
  set i 0
  set n [string length $e]
  while { $i < $n } {
    set c [string index $e $i]
    if { [regexp {[A-Za-z_]} $c] } {
      regexp -start $i {[A-Za-z_][A-Za-z0-9_\[\]]*} $e tok
      if { [dict exists $pins $tok] } {
        append out "\$::ram_sim::value([dict get $pins $tok])"
      } else {
        error "ram_sim: function $function names $tok, not a pin"
      }
      incr i [string length $tok]
      continue
    }
    switch -- $c {
      "*" { append out " && " }
      "+" { append out " || " }
      "^" { append out " ^ " }
      "!" { append out "!" }
      "'" {
        error "ram_sim: postfix complement in $function is not supported"
      }
      default { append out $c }
    }
    incr i
  }
  return $out
}

# Build the model of the current block.
proc ram_sim::load { } {
  variable flop_models
  variable gate_models
  variable value
  variable comb_order
  variable flops
  variable gates
  variable state
  variable latched
  array unset value
  array unset net_ids
  array unset port_ids
  array unset state
  array unset latched
  set flops {}
  set gates {}
  set block [ord::get_db_block]
  foreach term [$block getBTerms] {
    set net [$term getNet]
    if { $net ne "NULL" } {
      set value([net_name $net]) 0
    }
  }
  # combinational instances: output net -> expression, inputs
  set comb {}
  foreach inst [$block getInsts] {
    set master [[$inst getMaster] getName]
    set pins [dict create]
    foreach it [$inst getITerms] {
      set net [$it getNet]
      if { $net eq "NULL" || [$net getSigType] in {POWER GROUND} } {
        continue
      }
      set name [net_name $net]
      dict set pins [[$it getMTerm] getName] $name
      if { ![info exists value($name)] } {
        set value($name) 0
      }
    }
    if { [info exists flop_models($master)] } {
      lassign $flop_models($master) clk d outs set set_on reset reset_on
      lappend flops [list $inst $pins $clk $d $outs $set $set_on $reset $reset_on]
      set state($inst) 0
      continue
    }
    if { [info exists gate_models($master)] } {
      lappend gates [list $inst $pins {*}$gate_models($master)]
      set latched($inst) 0
      continue
    }
    if { [dict size $pins] == 0 } {
      continue
    }
    foreach it [$inst getITerms] {
      set mterm [$it getMTerm]
      if { [$mterm getIoType] ne "OUTPUT" } {
        continue
      }
      set pin [$mterm getName]
      if { ![dict exists $pins $pin] } {
        continue
      }
      set lib_pin [get_lib_pins -quiet "$master/$pin"]
      if { $lib_pin eq "" } {
        error "ram_sim: no Liberty pin $master/$pin"
      }
      set function [$lib_pin function]
      if { $function eq "" } {
        error "ram_sim: $master/$pin has no function and no model"
      }
      set enable [$lib_pin tristate_enable]
      set inputs [dict values [dict remove $pins $pin]]
      lappend comb [list [dict get $pins $pin] [to_expr $function $pins] \
        [expr { $enable eq "" ? "" : [to_expr $enable $pins] }] \
        $inputs $master]
    }
  }
  variable comb_all $comb
  order
}

# Order the combinational outputs: each after every output its inputs read.
proc ram_sim::order { } {
  variable comb_all
  variable comb_order
  set comb $comb_all
  set driven [dict create]
  foreach c $comb {
    dict lappend driven [lindex $c 0] $c
  }
  set comb_order {}
  set done [dict create]
  set visiting [dict create]
  foreach c $comb {
    visit $c $driven done visiting
  }
  # a net driven by several tristates: one entry, resolved together after
  # the last of its drivers, whose inputs are then all settled
  set last [dict create]
  set i 0
  foreach c $comb_order {
    dict set last [lindex $c 0] $i
    incr i
  }
  set ordered {}
  set i 0
  foreach c $comb_order {
    set net [lindex $c 0]
    if { [dict get $last $net] == $i } {
      lappend ordered [list $net [dict get $driven $net]]
    }
    incr i
  }
  set comb_order $ordered
}

proc ram_sim::visit { c driven done_var visiting_var } {
  upvar $done_var done
  upvar $visiting_var visiting
  variable comb_order
  set key [lindex $c 0]:[lindex $c 1]
  if { [dict exists $done $key] } {
    return
  }
  if { [dict exists $visiting $key] } {
    error "ram_sim: combinational loop through [lindex $c 0]"
  }
  dict set visiting $key 1
  foreach input [lindex $c 3] {
    if { [dict exists $driven $input] } {
      foreach d [dict get $driven $input] {
        visit $d $driven done visiting
      }
    }
  }
  dict set done $key 1
  lappend comb_order $c
}

# Evaluate every combinational output from the inputs and the flops.
proc ram_sim::settle { } {
  variable value
  variable comb_order
  variable flops
  variable state
  variable gates
  variable latched
  foreach f $flops {
    lassign $f inst pins clk d outs
    foreach {pin polarity} $outs {
      if { [dict exists $pins $pin] } {
        set value([dict get $pins $pin]) \
          [expr { $polarity eq "q" ? $state($inst) : !$state($inst) }]
      }
    }
  }
  # gated clocks follow their clock and latched enable
  foreach g $gates {
    lassign $g inst pins clk en se gclk
    if { [dict exists $pins $gclk] } {
      set value([dict get $pins $gclk]) \
        [expr { $value([dict get $pins $clk]) && $latched($inst) }]
    }
  }
  foreach entry $comb_order {
    lassign $entry net drivers
    set driving {}
    foreach c $drivers {
      lassign $c - expression enable
      if { $enable ne "" } {
        set on [expr $enable]
        if { !$on } {
          continue
        }
      }
      set bit [expr $expression]
      lappend driving [expr { $bit ? 1 : 0 }]
    }
    if { [llength [lsort -unique $driving]] > 1 } {
      error "ram_sim: $net driven both ways"
    }
    if { [llength $driving] == 1 } {
      set value($net) [lindex $driving 0]
    }
  }
}

# Asynchronous set and reset take effect at once.
proc ram_sim::apply_async { } {
  variable value
  variable flops
  variable state
  foreach f $flops {
    lassign $f inst pins clk d outs set set_on reset reset_on
    if {
      $reset ne "" && [dict exists $pins $reset]
      && $value([dict get $pins $reset]) == $reset_on
    } {
      set state($inst) 0
    } elseif {
      $set ne "" && [dict exists $pins $set]
      && $value([dict get $pins $set]) == $set_on
    } {
      set state($inst) 1
    }
  }
}

# One clock cycle on `clock`: low (gates latch their enables), then a
# rising edge on which every flop whose clock rose samples its data.
proc ram_sim::cycle { clock } {
  variable value
  variable flops
  variable state
  variable gates
  variable latched
  set clock [net_of_port $clock]
  set value($clock) 0
  apply_async
  settle
  foreach g $gates {
    lassign $g inst pins clk en se
    set on [expr { $value([dict get $pins $en]) }]
    if { $se ne "" && [dict exists $pins $se] } {
      set on [expr { $on || $value([dict get $pins $se]) }]
    }
    set latched($inst) $on
  }
  settle
  set before [array get value]
  set value($clock) 1
  settle
  set sampled {}
  array set was $before
  foreach f $flops {
    lassign $f inst pins clk d
    set c [dict get $pins $clk]
    if { !$was($c) && $value($c) } {
      lappend sampled $inst $was([dict get $pins $d])
    }
  }
  foreach {inst bit} $sampled {
    set state($inst) $bit
  }
  apply_async
  settle
}

proc ram_sim::net_of_port { name } {
  variable port_ids
  if { [info exists port_ids($name)] } {
    return $port_ids($name)
  }
  set term [[ord::get_db_block] findBTerm $name]
  if { $term eq "NULL" } {
    error "ram_sim: no port $name"
  }
  return [net_name [$term getNet]]
}

proc ram_sim::set_port { name bit } {
  variable value
  set value([net_of_port $name]) $bit
}

proc ram_sim::get_port { name } {
  variable value
  return $value([net_of_port $name])
}

# A bus `base`[0..width-1] to an integer value and back.
proc ram_sim::set_bus { base width int } {
  for { set i 0 } { $i < $width } { incr i } {
    set_port "$base\[$i\]" [expr { ($int >> $i) & 1 }]
  }
}

proc ram_sim::get_bus { base width } {
  set int 0
  for { set i 0 } { $i < $width } { incr i } {
    set int [expr { $int | ([get_port "$base\[$i\]"] << $i) }]
  }
  return $int
}

proc ram_sim::check { condition message } {
  if { ![uplevel 1 [list expr $condition]] } {
    error "ram_sim: $message"
  }
}

# Random writes and reads against a model of what the RAM is meant to
# hold. Options (a dict):
#   words, bits         the RAM's shape
#   writes              ports: {addr data we}, `we` the enable pins, every
#                       mask slice's, set together: a whole word written
#   reads               ports: {addr data}
#   banked_reads        ports: {addr data bank}, the word that address in
#                       that bank holds given by `bank_word`
#   bank_word           a command: <bank_word> bank index -> word
#   zero_word           a word that reads 0 and drops writes, or -1
#   latency             0, or 1 for a read one clock after its address
#   priority            last (the last port writing a word wins) or or
#   mem                 the words' contents to start from
#   cycles              how many
proc ram_sim::exercise { options } {
  set o [dict merge {writes {} reads {} banked_reads {} bank_word {}
                     zero_word -1 latency 0 priority last mem {}
                     cycles 200} $options]
  set words [dict get $o words]
  set bits [dict get $o bits]
  set abits [expr { max(1, int(ceil(log($words) / log(2)))) }]
  set mem [dict get $o mem]
  if { $mem eq "" } {
    set mem [lrepeat $words 0]
  }
  set zero [dict get $o zero_word]
  expr { srand(1) }
  for { set cycle 0 } { $cycle < [dict get $o cycles] } { incr cycle } {
    # writes: a third of the time every port writes the same word
    set same [expr { int(rand() * $words) }]
    set written {}
    foreach port [dict get $o writes] {
      lassign $port addr data we
      set a [expr { rand() < 0.33 ? $same : int(rand() * $words) }]
      set d [expr { int(rand() * (1 << $bits)) }]
      set e [expr { rand() < 0.7 }]
      set_bus $addr $abits $a
      set_bus $data $bits $d
      foreach pin $we {
        set_port $pin $e
      }
      if { $e } {
        lappend written $a $d
      }
    }
    cycle clk
    foreach port [dict get $o writes] {
      foreach pin [lindex $port 2] {
        set_port $pin 0
      }
    }
    set next $mem
    set hit [dict create]
    foreach {a d} $written {
      if { $a == $zero } {
        continue
      }
      if { [dict get $o priority] eq "or" && [dict exists $hit $a] } {
        lset next $a [expr { [lindex $next $a] | $d }]
      } else {
        lset next $a $d
      }
      dict set hit $a 1
    }
    set mem $next
    # reads
    set expect {}
    foreach port [dict get $o reads] {
      lassign $port addr data
      set a [expr { int(rand() * $words) }]
      set_bus $addr $abits $a
      lappend expect $data $a $a
    }
    foreach port [dict get $o banked_reads] {
      lassign $port addr data bank
      set per_bank [expr { $words / [llength [dict get $o banked_reads]] }]
      set i [expr { int(rand() * $per_bank) }]
      set_bus $addr [expr { max(1, int(ceil(log($per_bank) / log(2)))) }] $i
      lappend expect $data [{*}[dict get $o bank_word] $bank $i] "$bank/$i"
    }
    if { [dict get $o latency] } {
      cycle clk
    } else {
      settle
    }
    foreach {data a where} $expect {
      set q [get_bus $data $bits]
      set want [expr { $a == $zero ? 0 : [lindex $mem $a] }]
      check { $q == $want } \
        "cycle $cycle: $data at $where (word $a) reads $q, expected $want"
    }
  }
  return $mem
}

# Instances whose master is `pattern` (glob).
proc ram_sim::insts_of { pattern } {
  set out {}
  foreach inst [[ord::get_db_block] getInsts] {
    if { [string match $pattern [[$inst getMaster] getName]] } {
      lappend out $inst
    }
  }
  return $out
}

# Outputs driven through a tristate enable.
proc ram_sim::tristate_count { } {
  variable comb_order
  set n 0
  foreach entry $comb_order {
    foreach c [lindex $entry 1] {
      if { [lindex $c 2] ne "" } {
        incr n
      }
    }
  }
  return $n
}

# A -periphery verilog wrapper around the loaded core: its assigns, its
# clock gates and its instance of the core, joined to the core through
# the core's ports. The wrapper's own ports then stand for the RAM's.
proc ram_sim::add_wrapper { file } {
  variable comb_all
  variable gates
  variable gate_models
  variable latched
  variable value
  variable port_ids
  set text [read [set f [open $file r]]]
  close $f
  regsub -all {//[^\n]*} $text {} text
  # wrapper net name -> id; a net joined to a core port is that port's net
  set ids [dict create]
  # the core instance: .port(net) pairs
  if { ![regexp {\n\s*(\S+)\s+core\s*\(([^;]*)\);} $text - core_master pairs] } {
    error "ram_sim: $file instantiates no core"
  }
  foreach {- port conn} [regexp -all -inline {\.(\w+)\(([^)]*)\)} $pairs] {
    set conn [unescape $conn]
    set term [[ord::get_db_block] findBTerm $port]
    if { $term ne "NULL" } {
      dict set ids $conn [net_name [$term getNet]]
      continue
    }
    # a bus: each bit of the core's port to the same bit of the net
    foreach term [[ord::get_db_block] getBTerms] {
      if {
        [regexp {^(.+)\[(\d+)\]$} [$term getName] - base bit]
        && $base eq $port
      } {
        dict set ids "$conn\[$bit\]" [net_name [$term getNet]]
      }
    }
  }
  # the wrapper's ports
  foreach {- dir range name} [regexp -all -inline {(input|output)\s*(\[\d+:\d+\])?\s*(\w+)} $text] {
    if { $range eq "" } {
      set port_ids($name) [wrapper_net ids $name]
    } else {
      regexp {\[(\d+):(\d+)\]} $range - hi lo
      for { set b $lo } { $b <= $hi } { incr b } {
        set port_ids($name\[$b\]) [wrapper_net ids "$name\[$b\]"]
      }
    }
  }
  # assigns
  foreach {- lhs rhs} [regexp -all -inline {assign\s+([^=]*?)\s*=\s*([^;]*);} $text] {
    set out [wrapper_net ids [unescape $lhs]]
    set expr ""
    set inputs {}
    set rest $rhs
    while { [string length [string trim $rest]] } {
      set rest [string trimleft $rest]
      if {
        [regexp {^\\(\S+)\s(.*)$} $rest - tok rest] ||
        [regexp {^([A-Za-z_][\w.]*(?:\[\d+\])?)(.*)$} $rest - tok rest]
      } {
        set id [wrapper_net ids $tok]
        lappend inputs $id
        append expr "\$::ram_sim::value($id)"
      } elseif { [regexp {^1'b([01])(.*)$} $rest - bit rest] } {
        append expr $bit
      } else {
        set c [string index $rest 0]
        set rest [string range $rest 1 end]
        append expr [string map {~ ! & " && " | " || " ^ " ^ "} $c]
      }
    }
    lappend comb_all [list $out $expr "" $inputs assign]
  }
  # clock gates
  set cell {\n\s*(\w+)\s+(\\\S+\s|\w+)\s*\(([^;]*)\);}
  foreach {- master name pins} [regexp -all -inline $cell $text] {
    if { ![info exists gate_models($master)] } {
      continue
    }
    set p [dict create]
    foreach {- pin conn} [regexp -all -inline {\.(\w+)\(([^)]*)\)} $pins] {
      dict set p $pin [wrapper_net ids [unescape $conn]]
    }
    set inst "wrapper:$name"
    lappend gates [list $inst $p {*}$gate_models($master)]
    set latched($inst) 0
  }
  foreach id [dict values $ids] {
    if { ![info exists value($id)] } {
      set value($id) 0
    }
  }
  order
}

# An escaped Verilog identifier (\\name followed by a space) as a name.
proc ram_sim::unescape { s } {
  return [string trim [regsub {^\\} [string trim $s] {}]]
}

# The id of a wrapper net, new unless the net is joined to a core port.
proc ram_sim::wrapper_net { ids_var name } {
  upvar $ids_var ids
  if { ![dict exists $ids $name] } {
    dict set ids $name [fresh "w:$name"]
  }
  return [dict get $ids $name]
}

proc ram_sim::fresh { name } {
  variable net_ids
  if { ![info exists net_ids($name)] } {
    set net_ids($name) [array size net_ids]
  }
  return $net_ids($name)
}

# The most cells matching `pattern` (glob, by master) on any path from a
# flop or port to `net`: a decoder's depth in AND2s, say.
proc ram_sim::depth { pattern } {
  variable comb_order
  set d [dict create]
  set most 0
  foreach entry $comb_order {
    lassign $entry net drivers
    set here 0
    foreach c $drivers {
      lassign $c - - - inputs master
      set in 0
      foreach i $inputs {
        if { [dict exists $d $i] } {
          set in [expr { max($in, [dict get $d $i]) }]
        }
      }
      set here [expr { max($here, $in + [string match $pattern $master]) }]
    }
    dict set d $net $here
    set most [expr { max($most, $here) }]
  }
  return $most
}

# Storage flops by word: {row word} -> flops, from their names
# (storage_<row>_<word in row>_<first bit>.bit<n>).
proc ram_sim::words_of_flops { } {
  variable flops
  set out [dict create]
  foreach f $flops {
    set inst [lindex $f 0]
    if { [regexp {^storage_(\d+)_(\d+)_\d+\.} [$inst getName] - row word] } {
      dict lappend out [list $row $word] $inst
    }
  }
  return $out
}
