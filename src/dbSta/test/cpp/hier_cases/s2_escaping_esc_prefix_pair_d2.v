// TARGETS: escaped_net, name_is_prefix_of_other, depth_2
// CLUE: `\n+x ` is a strict PREFIX of `\n+xy `, and both live one level down so
// the flat names are "u1/n+x" and "u1/n+xy". dbNetwork::name(Net) recovers the
// local name with an UNANCHORED substring search followed by
// erase(pos, header.length() + 1) (dbNetwork.cc:2628-2630) -- a find that hits
// the wrong occurrence, or the +1 that assumes a divider follows, mangles one
// of the pair into the other. The instance name "u1" is also a prefix of the
// net names' flat form, so both prefix relations are live at once.
module sub (i0, i1, o0, o1);
  input i0;
  input i1;
  output o0;
  output o1;
  wire \n+x ;
  wire \n+xy ;
  INV_X1 g1 (.A(i0), .ZN(\n+x ));
  BUF_X1 g2 (.A(i1), .Z(\n+xy ));
  BUF_X1 g3 (.A(\n+x ), .Z(o0));
  INV_X1 g4 (.A(\n+xy ), .ZN(o1));
endmodule

module top (a, b, y0, y1);
  input a;
  input b;
  output y0;
  output y1;
  sub u1 (.i0(a), .i1(b), .o0(y0), .o1(y1));
endmodule
