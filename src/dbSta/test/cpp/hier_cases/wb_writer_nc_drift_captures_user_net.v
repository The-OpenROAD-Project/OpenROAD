// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, hier, counter_drift, silent_multi_driver
// CLUE: the drifted _NC index is never checked against existing names either, and the
// CLUE: drift DEFEATS the duplicate-declaration symptom: module mid declares fillers
// CLUE: _NC1/_NC2 but references _NC3/_NC4, so a user net named _NC3 is captured with
// CLUE: NO illegal Verilog to warn anyone.  Here the captured net is driven by INV d and
// CLUE: the open formal is a leaf OUTPUT bit, so the writer silently multi-drives it.
module leaf (s, ob, o);
  input s;
  output [1:0] ob;
  output o;
  INV_X1 g (.A(s), .ZN(o));
  BUF_X1 b0 (.A(s), .Z(ob[0]));
  BUF_X1 b1 (.A(s), .Z(ob[1]));
endmodule

module mid (a, y1, y2);
  input a;
  output y1;
  output y2;
  wire _NC3;
  INV_X1 d (.A(a), .ZN(_NC3));
  BUF_X1 e (.A(_NC3), .Z(y1));
  leaf L2 (.s(a), .o(y2));
endmodule

module top (a, b, y1, y2, y3);
  input a;
  input b;
  output y1;
  output y2;
  output y3;
  leaf L1 (.s(a), .o(y1));
  mid M (.a(b), .y1(y2), .y2(y3));
endmodule
