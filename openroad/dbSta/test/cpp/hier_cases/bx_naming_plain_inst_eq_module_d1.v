// TOP: top
// TECH: nangate45
// TARGETS: name_interplay, instance_eq_module, depth_1
// CLUE: Instance name identical to its module name: sub sub (...).
module sub (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule
module top (a, y);
  input a;
  output y;
  wire w;
  sub sub (.i(a), .o(w));
  INV_X1 u1 (.A(w), .ZN(y));
endmodule
