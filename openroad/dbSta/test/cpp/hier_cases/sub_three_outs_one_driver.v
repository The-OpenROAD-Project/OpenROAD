// TOP: top
// TECH: nangate45
// TARGETS: alias_three_outputs, submodule
// CLUE: three submodule outputs aliased to one internal driver: how many spurious top-level assigns does the hier writer add?

module trio (input a, output y1, output y2, output y3);
  wire w;
  INV_X1 g0 (.A(a), .ZN(w));
  assign y1 = w;
  assign y2 = w;
  assign y3 = w;
endmodule

module top (input i, output o1, output o2, output o3);
  trio u0 (.a(i), .y1(o1), .y2(o2), .y3(o3));
endmodule
