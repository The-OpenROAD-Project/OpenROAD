/*
Hierarchical clock example (rtl source for hierclock_gate.v)

Generated using:
gdb ~/yosys/yosys
read_verilog hierclock.v
opt
wreduce
opt
synth
write_verilog post_booth.v
techmap -map adder_map_file.v
techmap -map ./techmap.v
dfflibmap -liberty NangateOpenCellLibrary_typical.lib 
abc -liberty NangateOpenCellLibrary_typical.lib
stat -liberty NangateOpenCellLibrary_typical.lib    
write_verilog hierclock_gate.v
*/

module hierclock(
                 input        clk_i,
                 input        rst_n_i,
                 input [3:0]  a_i,
                 input [3:0]  b_i,
                 input        a_ld_i,
                 input        b_ld_i,
                 output [3:0] a_count_o,
                 output a_count_valid_o,
                 output [3:0] b_count_o,
                 output b_count_valid_o
                 );
   wire                 clk1_int;
   wire                 clk2_int;

   clockgen U1 (.clk_i(clk_i), 
                .rst_n_i(rst_n_i), 
                .clk1_o(clk1_int), 
                .clk2_o(clk2_int));
   
   counter  U2(.clk_i(clk1_int),.rst_n_i(rst_n_i),.load_i(a_ld_i), 
               .load_value_i(a_i), 
               .count_value_o(a_count_o), 
               .count_valid_o(a_count_valid_o)
               );
   counter  U3(.clk_i(clk2_int), .rst_n_i(rst_n_i), .load_i(b_ld_i), 
               .load_value_i(b_i), 
               .count_value_o(b_count_o), 
               .count_valid_o(b_count_valid_o)
               );
   
endmodule // hierclock

module counter (input clk_i,
                input  rst_n_i,
                input  load_i,
                input[3:0]  load_value_i,
                output[3:0] count_value_o,
                output count_valid_o);

   reg [3:0]                counter_q;
   reg                      count_valid_q;

   assign count_value_o = counter_q;
   assign count_valid_o = count_valid_q;
   
   always @(posedge clk_i)
     begin
        if (~rst_n_i)
          begin
             counter_q <= 4'b0;
             count_valid_q <= 1'b0;
          end
        else
          begin
             if (load_i)
               begin
                  counter_q <= load_value_i;
                  count_valid_q <= 1'b0;
               end
             else
               begin
                  counter_q <= counter_q + 1'b1;
                  count_valid_q <= 1'b1;
               end
          end
     end
endmodule // counter




module clockgen (input clk_i,
                 input  rst_n_i,
                 output clk1_o,
                 output clk2_o);
   reg [3:0]            counter_q;

   always @(posedge clk_i)
     begin
        if (~rst_n_i)
          counter_q <= 4'b0;
        else
          begin
             counter_q <= counter_q + 1'b1;
          end
     end
   assign clk1_o = counter_q[1];
   assign clk2_o = counter_q[3];
endmodule // clockgen


