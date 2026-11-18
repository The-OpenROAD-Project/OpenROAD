// TOP: top
// TECH: nangate45
// TARGETS: hier, flattened_names_inside_module, name_erase_overshoot, depth_1
// CLUE: same flattened-name shape as ..._dcflat_pathnames_short but one level
// down, where the module-local net's flat db name is "s1/u1\/n": the honest
// prefix "s1" and the malformed tail "u1\" are BOTH candidates for find(). Tests
// which one wins and whether the in-module result still collides for two
// flattened siblings inside one real module.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   sub s1 (.i(a), .j(b), .o(y0), .p(y1));
endmodule

module sub (i, j, o, p);
   input i;
   input j;
   output o;
   output p;
   wire \u1/n ;
   wire \u2/n ;
   INV_X1 \u1/g1  (.A(i), .ZN(\u1/n ));
   BUF_X1 \u1/g2  (.A(\u1/n ), .Z(o));
   INV_X1 \u2/g1  (.A(j), .ZN(\u2/n ));
   BUF_X1 \u2/g2  (.A(\u2/n ), .Z(p));
endmodule
