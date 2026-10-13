// TARGETS: escaped_net, naming_slash, bus, flat_path_collision, depth_2
// CLUE: the victim of the flattened name is a BUS, not a scalar. Flattening a
// module-local bus `a[1:0]` inside instance u1 gives bit nets "u1/a[0]" and
// "u1/a[1]"; writeWireDcls (VerilogWriter.cc:283-300) folds them into
// bus_ranges["u1/a"] and prints `wire [1:0] \u1/a ;`. The top-level escaped
// scalar `\u1/a ` has sta name "u1\/a" and prints as `wire \u1/a ;` -- the same
// identifier, declared twice with two different widths. The covered
// bx_naming_escaped_slashcol_net.v collides two scalars and never reaches the
// bus-range map.
module sub (i, o0, o1);
  input i;
  output o0;
  output o1;
  wire [1:0] a;
  INV_X1 g2 (.A(i), .ZN(a[0]));
  BUF_X1 g3 (.A(i), .Z(a[1]));
  BUF_X1 g4 (.A(a[0]), .Z(o0));
  INV_X1 g5 (.A(a[1]), .ZN(o1));
endmodule

module top (a, b, y0, y1, y2);
  input a;
  input b;
  output y0;
  output y1;
  output y2;
  wire \u1/a ;
  sub u1 (.i(b), .o0(y1), .o1(y2));
  INV_X1 g0 (.A(a), .ZN(\u1/a ));
  BUF_X1 g1 (.A(\u1/a ), .Z(y0));
endmodule
