// TOP: top
// TECH: nangate45
// TARGETS: hier_uniquify, port_tied_differently
// CLUE: two instances of sub2 with input b tied differently (constant vs
// signal); probes whether divergent tying changes uniquification behavior.
module sub2 (input a, input b, output z);
  AND2_X1 u1 (.A1(a), .A2(b), .ZN(z));
endmodule

module top (input in1, input in2, input in3, output o1, output o2);
  wire one;
  LOGIC1_X1 c1 (.Z(one));
  sub2 i1 (.a(in1), .b(one), .z(o1));
  sub2 i2 (.a(in2), .b(in3), .z(o2));
endmodule
