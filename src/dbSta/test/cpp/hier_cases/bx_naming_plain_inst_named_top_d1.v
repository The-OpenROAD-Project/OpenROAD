// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, instance_eq_top, depth_1
// CLUE: Instance named 'top' inside module top; flat path becomes top/g.
module sub (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  sub top (.i(a), .o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule
