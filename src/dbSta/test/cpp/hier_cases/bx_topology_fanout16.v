// TOP: top
// TECH: nangate45
// TARGETS: fanout_16, same_module_16_instances_one_parent, distinct_bits
// CLUE: 16 instances of the same 1-bit module inside one parent, each wired
// to a distinct bit of a 16-bit bus; stresses per-instance uniquification and
// bit-level connectivity bookkeeping.

module bitcell (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module bank (input [15:0] a, output [15:0] z);
  bitcell u0  (.a(a[0]),  .z(z[0]));
  bitcell u1  (.a(a[1]),  .z(z[1]));
  bitcell u2  (.a(a[2]),  .z(z[2]));
  bitcell u3  (.a(a[3]),  .z(z[3]));
  bitcell u4  (.a(a[4]),  .z(z[4]));
  bitcell u5  (.a(a[5]),  .z(z[5]));
  bitcell u6  (.a(a[6]),  .z(z[6]));
  bitcell u7  (.a(a[7]),  .z(z[7]));
  bitcell u8  (.a(a[8]),  .z(z[8]));
  bitcell u9  (.a(a[9]),  .z(z[9]));
  bitcell u10 (.a(a[10]), .z(z[10]));
  bitcell u11 (.a(a[11]), .z(z[11]));
  bitcell u12 (.a(a[12]), .z(z[12]));
  bitcell u13 (.a(a[13]), .z(z[13]));
  bitcell u14 (.a(a[14]), .z(z[14]));
  bitcell u15 (.a(a[15]), .z(z[15]));
endmodule

module top (input [15:0] a, output [15:0] z);
  bank b (.a(a), .z(z));
endmodule
