# Nets read from a design: two sky130 buffers, routed nets on met2 and
# met3, power rails with a via, a net beyond the window, and a buffer with
# no connections, whose pins are grounded with the obstructions.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def field_solver_nets.def
read_itf sky130.itf

# A 1 um halo: c's wire crosses the window's edge and is clipped; d lies
# beyond it.
add_field_solver_nets -nets a -halo 1 -margin 0.2
set result [solve_capacitance -min_walks 32768 -max_walks 32768]
check "only the requested net is solved" { dict keys $result } a
check "b runs alongside and couples most" {
  set c [dict get $result a couplings]
  expr { [dict get $c b] > [dict get $c VPWR] && [dict get $c b] > [dict get $c c] }
} 1
check "d is outside the window" {
  dict exists $result a couplings d
} 0
check "obstructions are not a net" {
  dict exists $result a couplings <obstructions>
} 0
check "the window's leakage is reported" {
  expr { [dict get $result a window_leakage] > 0 }
} 1

# The default halo, three times the height of a's met3, reaches d.
add_field_solver_nets -nets a
set wide [solve_capacitance -nets a -min_walks 32768 -max_walks 32768]
check "the default halo includes d" {
  dict exists $wide a couplings d
} 1

add_field_solver_nets -nets {a a} -halo 1
check "a net named twice is read once" { fsv::default_targets_cmd } a

check_error "an unknown net" { add_field_solver_nets -nets nope } FSV-0030
check_error "a layer map needs pairs" {
  add_field_solver_nets -nets a -layer_map {met2}
} FSV-0032
check_error "a halo is a distance" {
  add_field_solver_nets -nets a -halo -1
} FSV-0033
check_error "an unmapped layer" {
  add_field_solver_nets -nets a -layer_map {met2 met9}
} FSV-0007

# A pin made of a via alone: its layers come from the via's boxes.
set block [ord::get_db_block]
set lib [lindex [[ord::get_db] getLibs] 0]
set master [odb::dbMaster_create $lib fsv_via_pin]
$master setWidth 460
$master setHeight 2720
$master setType CORE
set mterm [odb::dbMTerm_create $master P INPUT]
odb::dbBox_create [odb::dbMPin_create $mterm] \
  [[ord::get_db_tech] findVia M1M2_PR] 230 230
$master setFrozen
set inst [odb::dbInst_create $block $master u4]
$inst setLocation 16000 10000
$inst setPlacementStatus PLACED
[$inst findITerm P] connect [odb::dbNet_create $block e]
add_field_solver_nets -nets e -halo 0.5
set pin [solve_capacitance -min_walks 8192 -max_walks 8192]
check "a via pin has its shapes" { expr { [dict get $pin e total] > 0 } } 1

# A macro whose met2 obstruction covers it whole, its own pin included, and
# which d's met2 wire crosses: the obstruction is trimmed clear of both.
set macro [odb::dbMaster_create $lib fsv_macro]
$macro setWidth 3000
$macro setHeight 2000
$macro setType BLOCK
set met2 [[ord::get_db_tech] findLayer met2]
odb::dbBox_create [odb::dbMPin_create [odb::dbMTerm_create $macro P INPUT]] \
  $met2 1000 800 1400 1200
odb::dbBox_create $macro $met2 0 0 3000 2000
$macro setFrozen
set m1 [odb::dbInst_create $block $macro m1]
$m1 setLocation 13500 500
$m1 setPlacementStatus PLACED
[$m1 findITerm P] connect [odb::dbNet_create $block h]
add_field_solver_nets -nets h -halo 2
set macro_row [solve_capacitance -min_walks 8192 -max_walks 8192]
check "a pin inside an obstruction solves" {
  expr { [dict get $macro_row h total] > 0 }
} 1

# A neighbour that only touches the window, with no halo or margin: it is
# left out. 2007 database units do not survive the round trip through um
# exactly, which once let it in and clip to nothing.
set met4 [[ord::get_db_tech] findLayer met4]
odb::dbSBox_create [odb::dbSWire_create [odb::dbNet_create $block f] ROUTED] \
  $met4 1000 11000 2007 11500 STRIPE
odb::dbSBox_create [odb::dbSWire_create [odb::dbNet_create $block g] ROUTED] \
  $met4 2007 11000 3000 11500 STRIPE
add_field_solver_nets -nets f -halo 0 -margin 0
set touch [solve_capacitance -min_walks 8192 -max_walks 8192]
check "a neighbour touching the window is left out" {
  dict exists $touch f couplings g
} 0

# A met2 box of d across a's wire: a short.
set swire [odb::dbSWire_create [$block findNet d] ROUTED]
odb::dbSBox_create $swire [[ord::get_db_tech] findLayer met2] \
  5000 6900 5500 7100 STRIPE
check_error "overlapping nets" { add_field_solver_nets -nets a } FSV-0031
odb::dbSWire_destroy $swire

# An octilinear special wire would be read as its bounding box.
set swire [odb::dbSWire_create [$block findNet b] ROUTED]
odb::dbSBox_create $swire [[ord::get_db_tech] findLayer met2] \
  1000 1000 2000 2000 STRIPE $odb::dbSBox_OCTILINEAR 200
check_error "a non-Manhattan special wire" {
  add_field_solver_nets -nets b
} FSV-0031
odb::dbSWire_destroy $swire

# Shapes of an instance or block pin without a placement would sit at the
# origin.
set unplaced [odb::dbInst_create $block \
  [[ord::get_db] findMaster sky130_fd_sc_hd__buf_1] u5]
check_error "an unplaced instance" { add_field_solver_nets -nets a } FSV-0031
odb::dbInst_destroy $unplaced
set bterm [odb::dbBTerm_create [$block findNet c] in2]
odb::dbBox_create [odb::dbBPin_create $bterm] \
  [[ord::get_db_tech] findLayer met2] 0 0 140 140
check_error "an unplaced block pin" { add_field_solver_nets -nets a } FSV-0031

exit_summary
