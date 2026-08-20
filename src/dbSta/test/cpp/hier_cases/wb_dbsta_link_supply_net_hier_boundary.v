// TOP: top
// TECH: nangate45
// TARGETS: supply1, dbNet_sigtype, hier_boundary
// CLUE: hier extension of the supply-net signal-type branch
// (dbReadVerilog.cc:741): the supply1 tie crosses a hierarchy boundary, so the
// POWER dbNet must also be represented as a dbModNet on both sides. Tests
// whether the module port keeps the tie when the net is marked POWER.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   supply1 vhi;
   sub u (.i(a), .t(vhi), .o(y0));
   INV_X1 g2 (.A(a), .ZN(y1));
endmodule

module sub (i, t, o);
   input i;
   input t;
   output o;
   NAND2_X1 g (.A1(i), .A2(t), .ZN(o));
endmodule
