// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, hier, cross_module_counter
// CLUE: unconnected_net_index_ is a VerilogWriter MEMBER that never resets, but
// CLUE: writeWireDcls declares _NC1.._NCn from 1 in EVERY module.  Two different parent
// CLUE: modules each holding an open vector formal must therefore desynchronise:
// CLUE: the second module declares _NC1,_NC2 but references _NC3,_NC4.
module leaf (s, i, o);
  input s;
  input [1:0] i;
  output o;
  INV_X1 g (.A(s), .ZN(o));
endmodule

module mid (a, y);
  input a;
  output y;
  leaf L2 (.s(a), .o(y));
endmodule

module top (a, b, y1, y2);
  input a;
  input b;
  output y1;
  output y2;
  leaf L1 (.s(a), .o(y1));
  mid M (.a(b), .y(y2));
endmodule
