// TOP: top
// TECH: nangate45
// TARGETS: binary_tree_depth_3, 7_instances_one_combiner
// CLUE: binary XOR-combiner tree of depth 3 (7 instances of one module) in
// top; shallower bracket variant of the depth-4 tree.

module comb (input a, input b, output z);
  XOR2_X1 g (.A(a), .B(b), .Z(z));
endmodule

module top (input [7:0] a, output z);
  wire [3:0] l1;
  wire [1:0] l2;
  comb n0 (.a(a[0]), .b(a[1]), .z(l1[0]));
  comb n1 (.a(a[2]), .b(a[3]), .z(l1[1]));
  comb n2 (.a(a[4]), .b(a[5]), .z(l1[2]));
  comb n3 (.a(a[6]), .b(a[7]), .z(l1[3]));
  comb m0 (.a(l1[0]), .b(l1[1]), .z(l2[0]));
  comb m1 (.a(l1[2]), .b(l1[3]), .z(l2[1]));
  comb r  (.a(l2[0]), .b(l2[1]), .z(z));
endmodule
