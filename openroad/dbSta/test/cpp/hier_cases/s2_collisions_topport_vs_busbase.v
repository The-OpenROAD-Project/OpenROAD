// TARGETS: flat_bus_base_collision, escaped_port, port_vs_net, depth_2
// CLUE: the victim is a top-level OUTPUT PORT named \x/b , and the name the
// flat writer synthesizes for sub x's bus b is the same string.  A port and a
// wire of a different width are then declared for one name: writeWireDcls skips
// a net only when findPort() matches the sta name (VerilogWriter.cc:282), and
// the bus base name x/b is not the port's sta name x\/b, so the guard misses.
module pbus (input a, output z);
  wire [1:0] b;
  INV_X1 g1 (.A(a), .ZN(b[0]));
  INV_X1 g2 (.A(b[0]), .ZN(b[1]));
  BUF_X1 g3 (.A(b[1]), .Z(z));
endmodule

module top (i1, i2, \x/b , o1);
  input i1;
  input i2;
  output \x/b ;
  output o1;
  pbus x (.a(i1), .z(o1));
  INV_X1 g4 (.A(i2), .ZN(\x/b ));
endmodule
