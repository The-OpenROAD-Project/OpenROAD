// TOP: top
// TECH: nangate45
// TARGETS: uniquification, name_id_map_counter, cascade
// CLUE: when <module>_<inst> is taken, makeUniqueDbModule appends _<id> from
// the per-block module_name_id_map_ counter and retries. Here BOTH the base
// clone name sub_u1 and the first fallback sub_u1_1 are real user modules, so
// the counter has to walk past two occupied names -- and the two user modules
// may themselves be renamed out of the way.
module top (d, y);
   input [4:0] d;
   output [4:0] y;
   sub u1 (.i(d[0]), .o(y[0]));
   sub u2 (.i(d[1]), .o(y[1]));
   sub u3 (.i(d[2]), .o(y[2]));
   sub_u1 w1 (.i(d[3]), .o(y[3]));
   sub_u1_1 w2 (.i(d[4]), .o(y[4]));
endmodule

module sub (i, o);
   input i;
   output o;
   INV_X1 g (.A(i), .ZN(o));
endmodule

module sub_u1 (i, o);
   input i;
   output o;
   BUF_X1 g (.A(i), .Z(o));
endmodule

module sub_u1_1 (i, o);
   input i;
   output o;
   wire m;
   INV_X1 g0 (.A(i), .ZN(m));
   INV_X1 g1 (.A(m), .ZN(o));
endmodule
