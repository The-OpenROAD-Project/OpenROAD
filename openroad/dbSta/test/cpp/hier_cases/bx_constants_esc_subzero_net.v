// TOP: top
// TECH: nangate45
// TARGETS: name_capture, escaped_path_name, tie0, depth_2
// CLUE: top-level DRIVEN escaped wire \s1/zero_ coexists with a 1'b0 literal
// inside submodule instance s1 — flat writer names that constant net
// s1/zero_, colliding with the user's net (known-finding-3 family, but for
// NETS via the writer's synthetic constant name).
module sub (input a, output y);
  AND2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule

module top (input a, output y, output yz);
  wire \s1/zero_ ;
  INV_X1 gi (.A(a), .ZN(\s1/zero_ ));
  BUF_X1 gb (.A(\s1/zero_ ), .Z(yz));
  sub s1 (.a(a), .y(y));
endmodule
