// TOP: top
// TECH: nangate45
// TARGETS: assign_alias_name_order, findNet_by_port_name, control
// CLUE: control for ..._alias_name_before_port: identical circuit with the
// internal wire renamed zz so the port name "i" sorts first and survives the
// alias merge, making findNet(inst,"i") succeed.
module top (a, y);
   input a;
   output y;
   sub u (.i(a), .o(y));
endmodule

module sub (i, o);
   input i;
   output o;
   wire zz;
   assign zz = i;
   INV_X1 g (.A(zz), .ZN(o));
endmodule
