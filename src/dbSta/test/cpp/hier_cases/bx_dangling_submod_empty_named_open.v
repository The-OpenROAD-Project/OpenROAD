// TOP: top
// TECH: nangate45
// TARGETS: all_pins_unconnected, empty_named_conn, submodule_instance
// CLUE: submodule instance with every port present but empty: sub u1 (.a(), .y());
module sub (input a, output y);
  INV_X1 g1 (.A(a), .ZN(y));
endmodule
module top (input in1, output out1);
  INV_X1 g0 (.A(in1), .ZN(out1));
  sub u1 (.a(), .y());
endmodule
