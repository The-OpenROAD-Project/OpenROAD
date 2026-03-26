// Hierarchical design with a feedthrough buffer in child_mod.
// child_mod has data_i -> BUF_X1 -> data_o (feedthrough via buffer).
//
// In top, the child's data_i is driven by an internal wire (not a port),
// while data_o connects to an output port.  This means when
// remove_buffers removes the feedthrough buffer:
//   - input side net has NO port  (internal wire)
//   - output side net HAS a port  (data_o)
// So the output net becomes the survivor, and the merged ModNet
// takes the output port name — triggering the bug.
module child_mod (data_i, data_o, in1, out1);
 input data_i;
 output data_o;
 input in1;
 output out1;

 BUF_X1 u_ft (.A(data_i), .Z(data_o));
 BUF_X1 u1 (.A(in1), .Z(out1));
endmodule

module top (clk, in1, out1, data_o);
 input clk;
 input in1;
 output out1;
 output data_o;

 wire data_internal;

 // Drive data_internal from a register output (not a port)
 DFF_X1 r1 (.D(in1), .CK(clk), .Q(data_internal), .QN());

 child_mod u_child (.data_i(data_internal),
    .data_o(data_o),
    .in1(in1),
    .out1(out1));
endmodule
