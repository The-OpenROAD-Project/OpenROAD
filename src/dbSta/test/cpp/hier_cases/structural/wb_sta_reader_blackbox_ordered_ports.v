// TOP: top
// TECH: nangate45
// TARGETS: black_box, ordered_connection, invented_port_names
// CLUE: makeBlackBoxOrderedPorts (VerilogReader.cc:1807) names the invented ports
// CLUE: p_0..p_n and builds buses DESCENDING (size-1,0) -- the opposite convention from
// CLUE: the named-connection path -- so the emitted interface is fabricated.
module top (a, y);
  input a;
  output y;
  wire n;
  ip2 u (a, n);
  BUF_X1 g (.A(n), .Z(y));
endmodule
