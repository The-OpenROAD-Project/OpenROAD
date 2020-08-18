module gcd (clk,
    req_msg,
    req_rdy,
    req_val,
    reset,
    resp_msg,
    resp_rdy,
    resp_val);
 input clk;
 input [31:0] req_msg;
 output req_rdy;
 input req_val;
 input reset;
 output [15:0] resp_msg;
 input resp_rdy;
 output resp_val;

endmodule
