// TOP: top
// TECH: nangate45
// TARGETS: hier, name_erase_overshoot, net_victim, dup_wire_decl
// CLUE: second victim class for the erase overshoot confirmed in
// ..._netname_erase_overshoot_port: here the name the net is renamed to ("x") is
// an ordinary module-local NET rather than a port, so instead of capturing a port
// the writer should emit two `wire x;` declarations in one module body and short
// two independent cones.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m u1 (.i(a), .j(b), .y0(y0), .y1(y1));
endmodule

module m (i, j, y0, y1);
   input i;
   input j;
   output y0;
   output y1;
   wire \g/x ;
   wire x;
   INV_X1 \g/1  (.A(i), .ZN(\g/x ));
   BUF_X1 g2 (.A(\g/x ), .Z(y0));
   INV_X1 g3 (.A(j), .ZN(x));
   BUF_X1 g4 (.A(x), .Z(y1));
endmodule
