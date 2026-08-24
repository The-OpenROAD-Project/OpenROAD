// TOP: top
// TECH: nangate45
// TARGETS: module_reuse, top_and_replicated_submodule, six_instances
// CLUE: module mm instantiated twice by top AND twice inside a submodule that
// is itself instantiated twice; 6 instances across 3 parent contexts, two of
// which are copies of the same parent type.

module mm (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module holder (input a, input b, output x, output y);
  mm u1 (.a(a), .z(x));
  mm u2 (.a(b), .z(y));
endmodule

module top (input [5:0] a, output [5:0] z);
  mm t1 (.a(a[0]), .z(z[0]));
  mm t2 (.a(a[1]), .z(z[1]));
  holder h1 (.a(a[2]), .b(a[3]), .x(z[2]), .y(z[3]));
  holder h2 (.a(a[4]), .b(a[5]), .x(z[4]), .y(z[5]));
endmodule
