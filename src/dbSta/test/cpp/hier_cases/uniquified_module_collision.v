// TOP: top
// TECH: nangate45
// TARGETS: repeated_module_instantiation_uniquification
// CLUE: dbReadVerilog.cc makeUniqueDbModule stashes an original_name property
// when it renames, so one module instantiated many times stresses the
// uniquified-name space. A sibling literally named like a uniquification
// suffix makes a collision more likely.
module top (a, y);
   input [2:0] a;
   output [2:0] y;
   cell u0 (.i(a[0]), .o(y[0]));
   cell u1 (.i(a[1]), .o(y[1]));
   cell u2 (.i(a[2]), .o(y[2]));
endmodule

module cell (i, o);
   input i;
   output o;
   inner_1 u_inner (.x(i), .z(o));
endmodule

module inner_1 (x, z);
   input x;
   output z;
   BUF_X1 g (.A(x), .Z(z));
endmodule
