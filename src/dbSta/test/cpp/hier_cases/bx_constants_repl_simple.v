// TOP: top
// TECH: nangate45
// TARGETS: concat_const, replication, probe_reader
// CLUE: simplest replication {4{1'b1}} driving a bus port — brackets
// bx_constants_repl_concat (STA-0171 on {2{2'b01}}): is replication per se
// rejected, or only replication of a multi-bit constant?
module sub4 (input [3:0] bus, output y);
  wire n1, n2;
  AND2_X1 g1 (.A1(bus[0]), .A2(bus[1]), .ZN(n1));
  AND2_X1 g2 (.A1(bus[2]), .A2(bus[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(y));
endmodule

module top (input a, output y);
  wire t;
  sub4 s1 (.bus({4{1'b1}}), .y(t));
  XOR2_X1 gx (.A(t), .B(a), .Z(y));
endmodule
