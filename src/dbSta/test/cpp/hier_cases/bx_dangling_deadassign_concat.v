// TOP: top
// TECH: nangate45
// TARGETS: dead_assign_concat
// CLUE: concat assign {d1,d2} = {x1,x2}; with no loads on d1/d2. Dead assign
//       through a concatenation LHS.
module top (x1, x2, y);
  input x1;
  input x2;
  output y;
  wire d1;
  wire d2;
  INV_X1 u1 (.A(x1), .ZN(y));
  assign {d1, d2} = {x1, x2};
endmodule
