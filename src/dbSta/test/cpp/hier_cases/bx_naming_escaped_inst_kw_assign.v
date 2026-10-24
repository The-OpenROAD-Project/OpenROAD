// TOP: top
// TECH: nangate45
// TARGETS: escaped_instance, keyword_assign, depth_1
// CLUE: instance named \assign ; unescaped it becomes the assign keyword
// mid-instantiation.
module top (input a, output z);
  INV_X1 \assign (.A(a), .ZN(z));
endmodule
