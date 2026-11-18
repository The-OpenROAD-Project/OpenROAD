// TOP: top
// TECH: nangate45
// TARGETS: attribute, instance_attr, module_attr, round_trip_metadata
// CLUE: VerilogReader.cc:1565-1570 / 189-193 push (* *) attributes into
// CLUE: network_->setAttribute for instances and cells.  Do they survive read+write?
(* my_module_attr = "kept" *)
module top (a, y);
  input a;
  output y;
  wire n;
  (* dont_touch = "true" *) INV_X1 g1 (.A(a), .ZN(n));
  (* my_int_attr = 42 *) BUF_X1 g2 (.A(n), .Z(y));
endmodule
