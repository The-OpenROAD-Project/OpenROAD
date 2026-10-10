// The module generate_regfile_asap7.regfile describes, its ports
// declared several to a line, as hand-written RTL often does.
module rf8x4 (input clock,
              input  [2:0] r0_addr, r1_addr,
              output [3:0] r0_data, r1_data,
              input  [2:0] w0_addr, input [3:0] w0_data, input w0_en,
              input  [2:0] w1_addr, input [3:0] w1_data, input w1_en);
endmodule
