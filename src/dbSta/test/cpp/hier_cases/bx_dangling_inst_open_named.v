// TOP: top
// TECH: nangate45
// TARGETS: all_pins_unconnected, empty_named_conn
// CLUE: INV_X1 with both pins present but empty in named form: (.A(), .ZN()).
module top (input in1, output out1);
  INV_X1 g1 (.A(in1), .ZN(out1));
  INV_X1 u_alone (.A(), .ZN());
endmodule
