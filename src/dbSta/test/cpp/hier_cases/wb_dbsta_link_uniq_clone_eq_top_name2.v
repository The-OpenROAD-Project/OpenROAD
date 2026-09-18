// TOP: sub_u2
// TECH: nangate45
// TARGETS: uniquification, clone_name_vs_top_module_name, visit_order
// CLUE: the bare module name goes to the FIRST-VISITED instance and the probe
// with top name sub_u1 showed the visit order is by instance name (u1 before
// u2), so the clone that actually gets requested is "sub_u2". Naming the top
// module sub_u2 therefore aims the clone straight at the block's top dbModule
// name -- the third namespace a clone name can collide with, after user modules
// and liberty cells.
module sub_u2 (a, y);
   input [1:0] a;
   output [1:0] y;
   sub u1 (.i(a[0]), .o(y[0]));
   sub u2 (.i(a[1]), .o(y[1]));
endmodule

module sub (i, o);
   input i;
   output o;
   INV_X1 g (.A(i), .ZN(o));
endmodule
