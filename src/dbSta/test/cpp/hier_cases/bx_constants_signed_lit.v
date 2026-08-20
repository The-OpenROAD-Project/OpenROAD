// TOP: top
// TECH: nangate45
// TARGETS: bus_const, signed_literal, probe_reader
// CLUE: signed decimal literal 4'sd5 on a bus port — legal Verilog-2001+;
// probes whether the reader's literal lexer accepts the s modifier.
module sub4 (input [3:0] bus, output y);
  wire n1, n2;
  AND2_X1 g1 (.A1(bus[0]), .A2(bus[1]), .ZN(n1));
  AND2_X1 g2 (.A1(bus[2]), .A2(bus[3]), .ZN(n2));
  OR2_X1 g3 (.A1(n1), .A2(n2), .ZN(y));
endmodule

module top (input a, output y);
  wire t;
  sub4 s1 (.bus(4'sd5), .y(t));
  XOR2_X1 gx (.A(t), .B(a), .Z(y));
endmodule
