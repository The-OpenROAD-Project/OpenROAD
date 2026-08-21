// TOP: top
// TECH: nangate45
// TARGETS: instance_output_port_wired_to_own_input_port, hold_loop
// CLUE: at the parent, the instance's q output net IS its d input net -- a
// zero-logic hold loop that only exists because of the boundary.

module ffmod (input d, input ck, input rn, output q);
  DFFR_X1 ff (.D(d), .RN(rn), .CK(ck), .Q(q));
endmodule

module top (input ck, input rn, input c, output z);
  wire hold;
  ffmod u (.d(hold), .ck(ck), .rn(rn), .q(hold));
  XOR2_X1 g (.A(hold), .B(c), .Z(z));
endmodule
