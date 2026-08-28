// TOP: top
// TECH: nangate45
// TARGETS: tie1, name_capture, one_
// CLUE: user wire legitimately NAMED one_ (driven by a buffer) coexists with a
// literal 1'b1 tie — probes collision between the writer's synthetic one_
// constant name and a real user net.
module top (input a, output y, output yc);
  wire one_;
  BUF_X1 gb (.A(a), .Z(one_));
  OR2_X1 g (.A1(one_), .A2(1'b1), .ZN(y));
  INV_X1 gi (.A(one_), .ZN(yc));
endmodule
