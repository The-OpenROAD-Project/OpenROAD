# One routing_lifecycle case, sourced by a generated script in a fresh process.
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def gcd_sky130hd.def
read_guides gcd_sky130hd.guide
set_routing_layers -signal met1-met5
set_routing_watermark_strength 100
set key_a [string repeat 0 64]
set key_b [format %064x 3]

switch $case {
  early - cleared - false {
    set_routing_watermark -key_hex $key_a -fraction 1
  }
  key_a - rekey_ab {
    set_routing_watermark -key_hex $key_a -fraction 0.5
  }
  key_b - rekey_ba {
    set_routing_watermark -key_hex $key_b -fraction 0.5
  }
}
pin_access -verbose 0
switch $case {
  late - neutral {
    set_routing_watermark -key_hex $key_a -fraction 1
  }
  cleared {
    clear_routing_watermark
  }
  false {
    foreach net [[ord::get_db_block] getNets] {
      set property [odb::dbBoolProperty_find $net watermark]
      if { $property ne "NULL" } {
        $property setValue false
      }
    }
  }
  rekey_ab {
    set_routing_watermark -key_hex $key_b -fraction 0.5
  }
  rekey_ba {
    set_routing_watermark -key_hex $key_a -fraction 0.5
  }
}
if { $case eq "neutral" } {
  set_routing_watermark_strength 1
}

# No detailed-route geometry exists before the tag changes above. In
# particular, clearing tags is not expected to undo an existing route.
detailed_route -no_pin_access -verbose 0
set nets {}
foreach net [[ord::get_db_block] getNets] {
  set property [odb::dbBoolProperty_find $net watermark]
  if { $property ne "NULL" && [$property getValue] } {
    lappend nets [$net getName]
  }
}
tee -variable output { report_routing_watermark }
if {
  ![regexp {Total signal wirelength = ([0-9]+) DBU, wrong-way = ([0-9]+) DBU} \
    $output unused total wrong_way]
} {
  error "Missing signal wirelength in routing watermark report"
}
puts [list ROUTING_RESULT [dict create nets [lsort $nets] total $total wrong_way $wrong_way]]
