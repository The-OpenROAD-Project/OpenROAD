// TOP: top
// TECH: nangate45
// TARGETS: open_pin, empty_named_connection, liberty_inst
// CLUE: .A2() is a legal explicitly-unconnected named connection.  It still takes the
// CLUE: liberty fast path with an EMPTY net name string, and makeLibertyInst
// CLUE: (VerilogReader.cc:1754) makes the pin with a null net.  Is the open pin kept?
module top (a, y);
  input a;
  output y;
  wire n;
  NAND2_X1 g1 (.A1(a), .A2(), .ZN(n));
  INV_X1 g2 (.A(n), .ZN(y));
endmodule
