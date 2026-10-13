// TOP: top
// TECH: nangate45
// TARGETS: const_feedthrough, depth_3
// CLUE: 1'b0 applied at top, fed through THREE pure feedthrough module levels
// (innermost uses assign q = p) before reaching a gate — probes the
// feedthrough-assign dropping pattern with a constant source.
module l3 (input p, output q);
  assign q = p;
endmodule

module l2 (input p, output q);
  l3 i3 (.p(p), .q(q));
endmodule

module l1 (input p, output q);
  l2 i2 (.p(p), .q(q));
endmodule

module top (input a, output y);
  wire t;
  l1 i1 (.p(1'b0), .q(t));
  OR2_X1 g (.A1(a), .A2(t), .ZN(y));
endmodule
