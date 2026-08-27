// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, keyword_sv_logic, depth_1
// CLUE: net named \logic ; NOT a Verilog-2005 keyword, so a writer may
// legally emit plain logic -- but SV-flavored consumers lex it as a
// keyword. Probes which keyword list the writer uses.
module top (input a, output z);
  wire \logic ;
  INV_X1 u1 (.A(a), .ZN(\logic ));
  INV_X1 u2 (.A(\logic ), .ZN(z));
endmodule
