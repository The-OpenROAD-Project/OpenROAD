// TOP: top
// TECH: nangate45
// TARGETS: supply1, port_modifier, direction_override
// CLUE: parseDcl (VerilogReader.cc:625-630) lets a supply0/supply1 declaration REPLACE
// CLUE: the input/output dcl of a port, so the port's direction becomes power/ground;
// CLUE: Verilog2db::staToDb has no case for those and falls through to INOUT.
module top (a, t, y);
  input a;
  input t;
  supply1 t;
  output y;
  NAND2_X1 g (.A1(a), .A2(t), .ZN(y));
endmodule
