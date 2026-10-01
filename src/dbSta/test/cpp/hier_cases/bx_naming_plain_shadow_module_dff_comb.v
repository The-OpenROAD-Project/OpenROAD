// TOP: top
// TECH: nangate45
// TARGETS: cellname_shadow, module_name, sequential_cell_name, depth_1
// CLUE: user module named DFF_X1 with the cell's port names but purely
// combinational contents; binding the liberty flop instead of the module
// makes the design sequential and LEC-distinguishable.
module DFF_X1 (D, CK, Q, QN);
  input D, CK;
  output Q, QN;
  wire t;
  AND2_X1 g0 (.A1(D), .A2(CK), .ZN(t));
  BUF_X1 g1 (.A(t), .Z(Q));
  INV_X1 g2 (.A(t), .ZN(QN));
endmodule
module top (d, ck, q, qn);
  input d, ck;
  output q, qn;
  DFF_X1 u1 (.D(d), .CK(ck), .Q(q), .QN(qn));
endmodule
