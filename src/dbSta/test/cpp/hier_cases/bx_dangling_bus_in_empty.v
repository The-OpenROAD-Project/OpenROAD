// TOP: top
// TECH: nangate45
// TARGETS: dangling_bus_input, empty_named_conn, dead_cone
// CLUE: whole 4-bit sub input bus b left explicitly empty (.b()); b feeds only a dead
// AND2 cone inside. Bus port + empty conn survival.
module sub (input a, input [3:0] b, output y);
  wire d0;
  INV_X1 g1 (.A(a), .ZN(y));
  AND2_X1 g2 (.A1(b[0]), .A2(b[3]), .ZN(d0));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .b(), .y(out1));
endmodule
