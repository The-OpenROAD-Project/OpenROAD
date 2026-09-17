// TOP: top
// TECH: nangate45
// TARGETS: name_capture, escaped_path_name, tie0, depth_2
// CLUE: variant of bx_constants_esc_subzero_net with a BUF (not INV) driving
// the user's \s1/zero_ net, so a capture of the s1-internal 1'b0 tie by the
// user net gives y = a & a = a instead of 0 — functionally detectable.
module sub (input a, output y);
  AND2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule

module top (input a, output y, output yz);
  wire \s1/zero_ ;
  BUF_X1 gb0 (.A(a), .Z(\s1/zero_ ));
  BUF_X1 gb (.A(\s1/zero_ ), .Z(yz));
  sub s1 (.a(a), .y(y));
endmodule
