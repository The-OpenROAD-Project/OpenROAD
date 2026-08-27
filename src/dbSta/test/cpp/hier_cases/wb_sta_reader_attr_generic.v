// TOP: top
// TECH: nangate45
// TARGETS: attribute, module_attr, metadata_round_trip
// CLUE: attributes with names OpenROAD does not special case go through
// CLUE: network_->setAttribute for the cell (VerilogReader.cc:189-193) and the instance.
// CLUE: Is any of that metadata preserved by write_verilog?
(* my_module_attr = "kept" *)
module top (a, y);
  input a;
  output y;
  wire n;
  (* my_str_attr = "keepme" *) INV_X1 g1 (.A(a), .ZN(n));
  (* my_int_attr = 42 *) BUF_X1 g2 (.A(n), .Z(y));
endmodule
