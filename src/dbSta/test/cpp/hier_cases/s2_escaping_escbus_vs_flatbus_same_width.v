// TARGETS: escaped_net, naming_slash, bus, flat_path_collision, depth_2
// CLUE: the same-width mirror of the scalar/bus clash. The top declares an
// escaped BUS `\u1/a ` (sta bits "u1\/a[0]", "u1\/a[1]") and the submodule
// declares a module-local bus `a[1:0]` that flattens to "u1/a[0]", "u1/a[1]".
// writeWireDcls keys bus_ranges on the sta base name (VerilogWriter.cc:283-295)
// so the two buses stay separate map entries, but netVerilogName prints both
// bases as `\u1/a ` (VerilogNamespace.cc:59-64 drops the escape marker before
// '/'). Two `wire [1:0] \u1/a ;` of the SAME width can alias silently instead
// of failing to parse, so this asks whether the logic survives, not just the
// syntax.
module sub (i0, i1, o0, o1);
  input i0;
  input i1;
  output o0;
  output o1;
  wire [1:0] a;
  INV_X1 g2 (.A(i0), .ZN(a[0]));
  BUF_X1 g3 (.A(i1), .Z(a[1]));
  BUF_X1 g4 (.A(a[0]), .Z(o0));
  INV_X1 g5 (.A(a[1]), .ZN(o1));
endmodule

module top (a, b, c, y0, y1, y2, y3);
  input a;
  input b;
  input c;
  output y0;
  output y1;
  output y2;
  output y3;
  wire [1:0] \u1/a ;
  sub u1 (.i0(b), .i1(c), .o0(y2), .o1(y3));
  INV_X1 g0 (.A(a), .ZN(\u1/a [0]));
  BUF_X1 g1 (.A(b), .Z(\u1/a [1]));
  BUF_X1 h0 (.A(\u1/a [0]), .Z(y0));
  INV_X1 h1 (.A(\u1/a [1]), .ZN(y1));
endmodule
