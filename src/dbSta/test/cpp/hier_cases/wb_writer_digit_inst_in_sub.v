// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, instance_name, depth_asymmetry
// CLUE: depth changes the verdict for the same illegal name.  Flat mode renames the
// CLUE: instance to the path "u/1g", whose '/' forces staToVerilog to escape it, so the
// CLUE: bug is MASKED; hier mode keeps the bare local name "1g" and emits it unescaped.
module sub (i, o);
  input i;
  output o;
  INV_X1 \1g  (.A(i), .ZN(o));
endmodule

module top (a, y);
  input a;
  output y;
  sub u (.i(a), .o(y));
endmodule
