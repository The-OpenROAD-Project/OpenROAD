// TOP: top
// TECH: nangate45
// TARGETS: supply1, supply0, dbNet_sigtype
// CLUE: makeDbNets is the only place that sets a dbNet signal type: nets whose
// sta direction is power/ground become dbSigType POWER/GROUND
// (dbReadVerilog.cc:741-746). supply1/supply0 are the structural way to write
// a constant in gate-level Verilog, and write_verilog without -include_pwr_gnd
// suppresses power/ground nets -- so the tie should vanish on the way out.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   supply1 vhi;
   supply0 vlo;
   AND2_X1 g0 (.A1(a), .A2(vhi), .ZN(y0));
   OR2_X1 g1 (.A1(a), .A2(vlo), .ZN(y1));
endmodule
