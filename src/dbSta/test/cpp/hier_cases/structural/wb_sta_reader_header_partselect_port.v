// TOP: top
// TECH: nangate45
// TARGETS: header_port_partselect, control
// CLUE: contrast with the bit-select header port: VerilogNetPartSelect keeps the BASE
// CLUE: name, so makeCellPort finds the declaration and builds a correct bus port.
module top (y[1:0], a);
  output [1:0] y;
  input a;
  INV_X1 g0 (.A(a), .ZN(y[0]));
  BUF_X1 g1 (.A(a), .Z(y[1]));
endmodule
