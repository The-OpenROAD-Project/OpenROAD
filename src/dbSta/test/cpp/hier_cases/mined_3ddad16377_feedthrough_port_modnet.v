// TOP: top
// TECH: nangate45
// TARGETS: net_on_two_module_ports_with_internal_tap
// CLUE: mined from 3ddad16377 "dbReadVerilog bug fix: Added dbModNet connection
// for feedthrough port", which landed with NO netlist reproducer. It walks
// network_->termIterator(inst_pin_net) and connects any dbModBTerm found by the
// net's own term name, so the trigger is a single net inside a module that
// touches BOTH module ports and a child instance pin at once.
module top (a, sel, o_ft, o_gated);
   input a, sel;
   output o_ft;
   output o_gated;
   tap_mod u_tap (.ti(a), .tsel(sel), .tft(o_ft), .tg(o_gated));
endmodule

// `ti` reaches `tft` with no gate on the path (a feedthrough) while the SAME
// net is also tapped by an internal gate driving a second output.
module tap_mod (ti, tsel, tft, tg);
   input ti, tsel;
   output tft;
   output tg;
   assign tft = ti;
   AND2_X1 g (.A1(ti), .A2(tsel), .ZN(tg));
endmodule
