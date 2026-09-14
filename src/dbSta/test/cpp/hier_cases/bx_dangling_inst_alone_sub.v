// TOP: top
// TECH: nangate45
// TARGETS: instance_all_pins_unconnected, hierarchical_instance
// CLUE: submodule instance u_alone with NO connections at all: sub u_alone ();
//       A fully floating hierarchy island; hier writer must keep it, flat
//       writer must keep its dissolved gate.
module sub (a, y);
  input a;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
  sub u_alone ();
endmodule
