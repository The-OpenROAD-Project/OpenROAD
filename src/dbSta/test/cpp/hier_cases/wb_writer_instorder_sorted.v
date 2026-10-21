// TOP: top
// TECH: nangate45
// TARGETS: instance_order, writeChildren_sort
// CLUE: writeChildren copies the child iterator into a vector and sorts it by instance
// CLUE: name (VerilogWriter.cc:326-329), so instances are emitted ASCII-ordered; the
// CLUE: declared order g9,g5,g1 must come back as g1,g5,g9.
module top (a, y);
  input a;
  output y;
  wire n1;
  wire n2;
  INV_X1 g9 (.A(a), .ZN(n1));
  INV_X1 g5 (.A(n1), .ZN(n2));
  INV_X1 g1 (.A(n2), .ZN(y));
endmodule
