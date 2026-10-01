// TOP: top
// TECH: nangate45
// TARGETS: escaped_busbit_lookalike, real_bus_collision, depth_1
// CLUE: scalar net \data[3]  coexists with real bus wire [3:0] data; an
// unescaping writer aliases the scalar onto bit 3 of the bus (wrong logic).
module top (input a, input b, output z);
  wire [3:0] data;
  wire \data[3] ;
  BUF_X1 b0 (.A(a), .Z(data[3]));
  INV_X1 g1 (.A(b), .ZN(\data[3] ));
  XOR2_X1 x1 (.A(data[3]), .B(\data[3] ), .Z(z));
endmodule
