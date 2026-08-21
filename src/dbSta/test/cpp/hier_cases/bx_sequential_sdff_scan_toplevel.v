// TOP: top
// TECH: nangate45
// TARGETS: sdff_scan_chain_at_top, d_cones_in_submodules
// CLUE: a 2-stage scan chain of SDFF_X1 at the TOP level (the oracle abstracts
// scan flops by instance path, so keeping them at top keeps the paths stable)
// with each D cone in its own submodule.

module dcone (input a, input b, output d);
  XOR2_X1 g (.A(a), .B(b), .Z(d));
endmodule

module top (input a, input b, input c, input ck, input se, input si,
            output q0, output q1);
  wire d0, d1, m;
  dcone u0 (.a(a), .b(b), .d(d0));
  dcone u1 (.a(b), .b(c), .d(d1));
  SDFF_X1 ff0 (.D(d0), .SE(se), .SI(si), .CK(ck), .Q(m));
  SDFF_X1 ff1 (.D(d1), .SE(se), .SI(m),  .CK(ck), .Q(q1));
  assign q0 = m;
endmodule
