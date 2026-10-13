// TOP: top
// TECH: nangate45
// TARGETS: attribute, implements_operator, unused_cells_path
// CLUE: processUnusedCells (dbReadVerilog.cc:1102) only ever fires for cells
// carrying an implements_operator attribute that has no dbModule: it creates a
// CHILD BLOCK per such cell, re-runs linkNetwork on it mid-flow and then
// restoreTopBlock()s. No netlist without attributes can reach that code at all.
module top (a, y);
   input a;
   output y;
   INV_X1 g (.A(a), .ZN(y));
endmodule

(* implements_operator = "add" *)
module opmod (i, o);
   input [1:0] i;
   output o;
   AND2_X1 g (.A1(i[0]), .A2(i[1]), .ZN(o));
endmodule
