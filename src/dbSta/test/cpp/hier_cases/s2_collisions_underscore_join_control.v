// TARGETS: flat_path_control, escaped_net, join_character, depth_2
// CLUE: the victim of the OBVIOUS FIX.  The flat name for net y of instance x is
// built by joining with '/', so the tempting repair for the \x/y collisions is
// to sanitize the join -- replace the divider with '_', the way
// odb::replaceBracketsWithUnderscores (util.cpp:31-52) treats brackets.  This
// design already owns a top net named x_y, so that repair would trade one silent
// merge for another.  Today the two must stay distinct.
module suby (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire x_y;
  suby x (.a(i1), .z(o1));
  INV_X1 g3 (.A(i2), .ZN(x_y));
  BUF_X1 g4 (.A(x_y), .Z(o2));
endmodule
