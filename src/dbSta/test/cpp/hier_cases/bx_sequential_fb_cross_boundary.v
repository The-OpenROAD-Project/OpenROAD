// TOP: top
// TECH: nangate45
// TARGETS: feedback_loop_crossing_module_boundary, toggle_flop
// CLUE: the toggle loop CROSSES a boundary: Q leaves the submodule, is
// inverted at the parent, and re-enters the same instance as D.

module ffmod (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input ck, input rn, output z);
  wire q, n;
  ffmod u (.d(n), .ck(ck), .rn(rn), .q(q));
  INV_X1 i0 (.A(q), .ZN(n));
  assign z = q;
endmodule
