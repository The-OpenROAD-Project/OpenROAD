/*
 lef: example1.lef
 lib: example1_typ.lib
 */

module leaf (a, z);
  input a;
  output z;
  INV_X1 inverter (.A(a), .ZN(z));
endmodule

module level2 (in, out);
  input in;
  output out;
  leaf leaf_inst (.a(in), .z(out));
endmodule

module level1 (in, out);
  input in;
  output out;
  level2 level2_inst (.in(in), .out(out));
endmodule

module top (a, b);
  input a;
  output b;
  level1 level1_inst (.in(a), .out(b));
endmodule