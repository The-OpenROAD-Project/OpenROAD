// TOP: top
// TECH: nangate45
// TARGETS: tie0, depth_2
// CLUE: 1'b0 tie-off inside a level-1 submodule; flat path must uniquify the
// constant into the flattened netlist, hier path must keep it in the sub.
module sub (a, y);
  input a;
  output y;
  AND2_X1 g1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule

module top (a, y);
  input a;
  output y;
  sub s1 (.a(a), .y(y));
endmodule
