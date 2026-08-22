// TOP: top
// TECH: nangate45
// TARGETS: tie0, depth_2
// CLUE: literal 1'b0 tie-off one level down inside a submodule; flat path
// must uniquify the tie into the flattened netlist, hier must keep it inside.
module top (a, y);
  input a;
  output y;
  mid m (.a(a), .y(y));
endmodule

module mid (a, y);
  input a;
  output y;
  NOR2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule
