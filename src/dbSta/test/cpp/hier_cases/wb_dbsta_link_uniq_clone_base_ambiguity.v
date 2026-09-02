// TOP: top
// TECH: nangate45
// TARGETS: uniquification, clone_base_name_ambiguity
// CLUE: the clone name is a plain concatenation <module>_<inst>, so module a_b
// instantiated as c and module a instantiated as b_c both want "a_b_c". Each
// module is instantiated twice (once per parent) so each produces one clone and
// the two clones must contend; the loser gets the _1 suffix and nothing in the
// output says which source module it came from.
module top (d0, d1, d2, d3, y0, y1, y2, y3);
   input d0;
   input d1;
   input d2;
   input d3;
   output y0;
   output y1;
   output y2;
   output y3;
   p1 h1 (.x0(d0), .x1(d1), .z0(y0), .z1(y1));
   p2 h2 (.x0(d2), .x1(d3), .z0(y2), .z1(y3));
endmodule

module p1 (x0, x1, z0, z1);
   input x0;
   input x1;
   output z0;
   output z1;
   a   b_c (.i(x0), .o(z0));
   a_b c   (.i(x1), .o(z1));
endmodule

module p2 (x0, x1, z0, z1);
   input x0;
   input x1;
   output z0;
   output z1;
   a   b_c (.i(x0), .o(z0));
   a_b c   (.i(x1), .o(z1));
endmodule

module a (i, o);
   input i;
   output o;
   BUF_X1 g (.A(i), .Z(o));
endmodule

module a_b (i, o);
   input i;
   output o;
   INV_X1 g (.A(i), .ZN(o));
endmodule
