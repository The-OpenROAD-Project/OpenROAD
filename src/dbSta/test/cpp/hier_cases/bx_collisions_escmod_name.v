// TOP: top
// TECH: nangate45
// TARGETS: escaped_module_name
// CLUE: module named \m/1 (escaped, slash inside); hier writer must re-emit
// the escaped module name; flat writer erases module names entirely.
module \m/1  (input a, output z);
  BUF_X1 u1 (.A(a), .Z(z));
endmodule

module top (input in1, output o1);
  \m/1  x (.a(in1), .z(o1));
endmodule
