module Constants(input clock, input [5:0] io_wide_bus, output reduced, output [15: 0] out);

  reg reduce_it;
  assign reduced = reduce_it;

  FourAnd eliminateMe({io_wide_bus[0:2], reduce_it}, 4'b1010, out[0:2]);
  
  FourAnd dontEliminateMe({reduce_it, io_wide_bus[0:2]}, io_wide_bus[5:2], out[3:5]);
  
  FourAnd eliminateMePartial({io_wide_bus[0:2], reduce_it}, {2'b10, io_wide_bus[0:1]}, out[6:8]);

  FourAndNot eliminateMe2(.in1({reduce_it, io_wide_bus[0:2]}), .in2(4'b1110), .out(out[9:11]));
  
  AndAndOr deepEliminate({reduce_it, io_wide_bus[0:2]}, 4'b0110, out[12:14]);


  
  always @(posedge clock) begin
    reduce_it = |io_wide_bus;
  end
endmodule

module FourAnd(input [3:0] in1, input [3: 0] in2, output [3: 0] out);
    assign out = in1 & in2;
endmodule

module FourAndNot(input [3:0] in1, input [3: 0] in2, output [3: 0] out);
    assign out = ~(in1 & in2);
endmodule

module FourOr(input [3:0] in1, input [3: 0] in2, output [3: 0] out);
    assign out = in1 | in2;
endmodule

module AndOr(input [3:0] in1, input [3: 0] in2, output [3: 0] out);
    FourAnd a(in1, in2, temp);
    FourOr  b(temp, in1[3:0], out);
endmodule

module AndAndOr(input [3:0] in1, input [3: 0] in2, output [3: 0] out);
    AndOr a(in1, in2, temp);
    FourAnd  b(temp, in1[3:0], out);
endmodule