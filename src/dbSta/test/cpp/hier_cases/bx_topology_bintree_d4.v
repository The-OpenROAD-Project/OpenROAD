// TOP: top
// TECH: nangate45
// TARGETS: binary_tree_depth_4, 15_instances_one_combiner
// CLUE: binary XOR-combiner tree of depth 4: 15 instances of one 2-input
// module wired as a reduction tree; stresses many-instance uniquification
// with non-trivial inter-instance nets.

module comb (input a, input b, output z);
  XOR2_X1 g (.A(a), .B(b), .Z(z));
endmodule

module top (input [15:0] a, output z);
  wire [7:0] l1;
  wire [3:0] l2;
  wire [1:0] l3;
  comb n0 (.a(a[0]),  .b(a[1]),  .z(l1[0]));
  comb n1 (.a(a[2]),  .b(a[3]),  .z(l1[1]));
  comb n2 (.a(a[4]),  .b(a[5]),  .z(l1[2]));
  comb n3 (.a(a[6]),  .b(a[7]),  .z(l1[3]));
  comb n4 (.a(a[8]),  .b(a[9]),  .z(l1[4]));
  comb n5 (.a(a[10]), .b(a[11]), .z(l1[5]));
  comb n6 (.a(a[12]), .b(a[13]), .z(l1[6]));
  comb n7 (.a(a[14]), .b(a[15]), .z(l1[7]));
  comb m0 (.a(l1[0]), .b(l1[1]), .z(l2[0]));
  comb m1 (.a(l1[2]), .b(l1[3]), .z(l2[1]));
  comb m2 (.a(l1[4]), .b(l1[5]), .z(l2[2]));
  comb m3 (.a(l1[6]), .b(l1[7]), .z(l2[3]));
  comb k0 (.a(l2[0]), .b(l2[1]), .z(l3[0]));
  comb k1 (.a(l2[2]), .b(l2[3]), .z(l3[1]));
  comb r  (.a(l3[0]), .b(l3[1]), .z(z));
endmodule
