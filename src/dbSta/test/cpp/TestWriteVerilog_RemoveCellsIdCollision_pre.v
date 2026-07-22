/*
 Bug reproduction for write_verilog -remove_cells.

 This hierarchical design contains a sub-module "sub_mod" instantiated
 in "top". When a liberty cell whose ConcreteCell ID collides with the
 dbModule computed ID is placed in remove_cells, the CellIdLess
 comparator in CellSet treats them as equal, causing writeChild to
 incorrectly drop the hierarchical instance.
 */
module top(in, out);
  input in;
  output out;
  wire w1;
  sub_mod sub_inst (.A(in), .Z(w1));
  INV_X1 u1 (.A(w1), .ZN(out));
endmodule

module sub_mod(A, Z);
  input A;
  output Z;
  INV_X1 inv1 (.A(A), .ZN(Z));
endmodule
