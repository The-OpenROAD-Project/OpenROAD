// TOP: top
// TECH: nangate45
// TARGETS: escaped_name_collides_with_synthesized_flat_path
// CLUE: flat write_verilog builds an instance name by joining the hierarchy path
// with '/', then escapes the result because it contains '/'. An instance whose
// name is *literally* the escaped identifier `\x/y ` therefore collides with the
// name synthesized for the path instance `x` -> instance `y`. This is the
// intersection of the two history classes (A) name collision and (H) escaped
// names, and it is invisible to a pure connectivity check: both instances keep
// their own connections, the netlist just cannot be named.
//
// The input below is legal Verilog -- `\x/y ` and the pair x/y are distinct
// identifiers -- so a correct writer must keep them distinct on the way out.
module top (a, b, y0, y1);
   input a, b;
   output y0, y1;

   INV_X1 \x/y  (.A(a), .ZN(y0));
   sub x (.i(b), .o(y1));
endmodule

module sub (i, o);
   input i;
   output o;
   INV_X1 y (.A(i), .ZN(o));
endmodule
