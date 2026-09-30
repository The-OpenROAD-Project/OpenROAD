// TOP: top
// TECH: nangate45
// TARGETS: assign_alias, merge_cycle, infinite_loop
// CLUE: mergeAssignNet (VerilogReader.cc:1853) merges lhs into rhs without checking that
// CLUE: they are already the same net.  ConcreteNet::mergeInto(this) sets merged_into_ =
// CLUE: this AND nulls pins_/terms_, so VerilogBindingTbl::find's
// CLUE: while(mergedInto(net)) loop never terminates.
module top (a, y);
  input a;
  output y;
  wire p;
  wire q;
  BUF_X1 g0 (.A(a), .Z(p));
  assign q = p;
  assign p = q;
  INV_X1 g1 (.A(q), .ZN(y));
endmodule
