// TOP: top
// TECH: nangate45
// TARGETS: multilevel_feedthrough_assign
// CLUE: VerilogWriter emits `assign` for feedthroughs, the construct behind
// #10343 and #9877. Here an input reaches an output through three nested
// modules with no gate on the path at all, plus one gated sibling path so the
// design still has logic.
module top (a, b, ft, o, o_ft);
   input a, b, ft;
   output o;
   output o_ft;
   lvl1 u_lvl1 (.i(ft), .o(o_ft));
   NAND2_X1 g (.A1(a), .A2(b), .ZN(o));
endmodule

module lvl1 (i, o);
   input i;
   output o;
   lvl2 u_lvl2 (.i(i), .o(o));
endmodule

module lvl2 (i, o);
   input i;
   output o;
   lvl3 u_lvl3 (.i(i), .o(o));
endmodule

module lvl3 (i, o);
   input i;
   output o;
   assign o = i;
endmodule
