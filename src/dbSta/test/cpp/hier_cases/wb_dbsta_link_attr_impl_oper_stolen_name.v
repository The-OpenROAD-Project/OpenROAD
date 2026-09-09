// TOP: top
// TECH: nangate45
// TARGETS: attribute, implements_operator, uniquification_interaction
// CLUE: processUnusedCells decides a module is unused via
// block_->findModule(cell->name()). Uniquification can hand a user module's
// name to a clone, and the victim is renamed -- so an INSTANTIATED module can
// become invisible under its own name and be re-linked as if it were unused,
// mid-way through building the real design.
module top (d, y);
   input [2:0] d;
   output [2:0] y;
   m u1 (.i(d[0]), .o(y[0]));
   m u2 (.i(d[1]), .o(y[1]));
   m_u1 w1 (.i(d[2]), .o(y[2]));
endmodule

module m (i, o);
   input i;
   output o;
   INV_X1 g (.A(i), .ZN(o));
endmodule

(* implements_operator = "add" *)
module m_u1 (i, o);
   input i;
   output o;
   BUF_X1 g (.A(i), .Z(o));
endmodule
