// TOP: top
// TECH: nangate45
// TARGETS: escaped_port_vs_flatpath
// CLUE: top PORT named \x/y vs hierarchy x containing instance y; flat output
// would hold a port/net named x/y plus an instance named x/y.
module sub (input a, output z);
  INV_X1 y (.A(a), .ZN(z));
endmodule

module top (input i1, input \x/y , output o1, output o2);
  sub x (.a(i1), .z(o1));
  INV_X1 g1 (.A(\x/y ), .ZN(o2));
endmodule
