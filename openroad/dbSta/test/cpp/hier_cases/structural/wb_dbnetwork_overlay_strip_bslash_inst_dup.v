// TOP: top
// TECH: nangate45
// TARGETS: hier, stripParentPrefix, instance_name_collision, dup_instance_name
// CLUE: turns the stripParentPrefix guard misfire into illegal Verilog. With the
// parent instance named `\u1\ ` the leaf g1 keeps its whole path (sta
// "u1\\/g1") and staToVerilog emits `\u1\/g1 `; a sibling leaf whose source name
// IS `\u1\/g1 ` (sta "u1\\\/g1") emits the identical text. Two instances in one
// module definition then carry one name -- the hier-mode twin of known finding 3,
// with no flattening involved.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m \u1\  (.i(a), .j(b), .o(y0), .p(y1));
endmodule

module m (i, j, o, p);
   input i;
   input j;
   output o;
   output p;
   INV_X1 g1 (.A(i), .ZN(o));
   INV_X1 \u1\/g1  (.A(j), .ZN(p));
endmodule
