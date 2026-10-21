// TOP: top
// TECH: nangate45
// TARGETS: pure_feedthrough_chain, no_logic, assign_leaf
// CLUE: 4-level chain with NO gates anywhere; the leaf is a single wire
// feedthrough (assign o = i) and every level above is a pure wrapper. Tests
// whether a gate-free hierarchy survives both writers.

module fleaf (input i, output o);
  assign o = i;
endmodule

module f3 (input i, output o);
  fleaf u (.i(i), .o(o));
endmodule

module f2 (input i, output o);
  f3 u (.i(i), .o(o));
endmodule

module f1 (input i, output o);
  f2 u (.i(i), .o(o));
endmodule

module top (input a, output z);
  f1 u (.i(a), .o(z));
endmodule
