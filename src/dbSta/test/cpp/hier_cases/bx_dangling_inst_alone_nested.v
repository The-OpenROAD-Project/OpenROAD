// TOP: top
// TECH: nangate45
// TARGETS: all_pins_unconnected, nested_open_instance, depth_2
// CLUE: top instantiates mid with ALL pins open, and mid itself instantiates leaf
// with all pins open. Two levels of fully unconnected instances.
module leaf (input a, output y);
  INV_X1 g1 (.A(a), .ZN(y));
endmodule
module mid (input a, output y);
  leaf u_l ();
  INV_X1 g1 (.A(a), .ZN(y));
endmodule
module top (input in1, output out1);
  INV_X1 g0 (.A(in1), .ZN(out1));
  mid u_m ();
endmodule
