// TOP: top
// TECH: nangate45
// TARGETS: header_port_bitselect, missing_declaration, unknown_direction
// CLUE: a header port that is a bit-select gets name "y[1]" (VerilogNetBitSelect ctor),
// CLUE: which has no entry in dcl_map_, so makeCellPort takes the warn-166 fallback and
// CLUE: makes a port with DEFAULT direction; Verilog2db maps unknown -> dbIoType INPUT
// CLUE: (dbReadVerilog.cc:694), turning two outputs into inputs.
module top (y[1], y[0], a);
  output [1:0] y;
  input a;
  INV_X1 g0 (.A(a), .ZN(y[0]));
  BUF_X1 g1 (.A(a), .Z(y[1]));
endmodule
