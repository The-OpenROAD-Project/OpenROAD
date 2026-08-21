// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, escaped_port_name
// CLUE: sub has escaped-name input \b! left unconnected via explicit-empty
//       named connection .\b! (). Escaped formal + dangling in one.
module sub (a, \b! , y);
  input a;
  input \b! ;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .\b! (), .y(y));
endmodule
