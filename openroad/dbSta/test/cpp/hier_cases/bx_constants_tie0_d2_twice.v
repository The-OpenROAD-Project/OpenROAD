// TOP: top
// TECH: nangate45
// TARGETS: tie0, depth_2, multi_instance
// CLUE: submodule with an INTERNAL 1'b0 tie instantiated TWICE — hier writer
// bakes one instance's path (e.g. \s1/zero_ ) into the shared module
// definition; with two instances the emitted definition cannot be right for
// both.
module sub (input a, output y);
  AND2_X1 u1 (.A1(a), .A2(1'b0), .ZN(y));
endmodule

module top (input a, input b, output y1, output y2);
  sub s1 (.a(a), .y(y1));
  sub s2 (.a(b), .y(y2));
endmodule
