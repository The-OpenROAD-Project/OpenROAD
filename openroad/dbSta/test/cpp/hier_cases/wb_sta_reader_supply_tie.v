// TOP: top
// TECH: nangate45
// TARGETS: supply0, supply1, constant_net, net_type
// CLUE: dcl_type maps supply0/supply1 to PortDirection ground/power (VerilogParse.yy:333),
// CLUE: makeModuleInstBody turns those into addConstantNet (VerilogReader.cc:1522-1529),
// CLUE: and Verilog2db only copies that to dbNet sigType (dbReadVerilog.cc:741) -- no tie
// CLUE: cell and no assign, so the logic constant may be lost on write.
module top (a, y);
  input a;
  output y;
  wire n1;
  supply1 t1;
  supply0 t0;
  NAND2_X1 g1 (.A1(a), .A2(t1), .ZN(n1));
  NOR2_X1 g2 (.A1(n1), .A2(t0), .ZN(y));
endmodule
