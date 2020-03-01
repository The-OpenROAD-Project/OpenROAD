module Constants(input clock, input [5:0] io_wide_bus, output reduced, output [2: 0] out);

  reg reduce_it;
  assign reduced = reduce_it;

  AndOr mod(reduce_it, 1'b0, 1'b1, out[0], out[1], out[2]);
  
  always @(posedge clock) begin
    reduce_it = |io_wide_bus;
  end
endmodule

module AndOr(input a, input b, input c, output d, output e, output f);
    AndModule x(a, b, d);
    OrWrapper y(b, c, e, h);
    OrModule  z(1'b1, h, f);
endmodule

module AndModule(input a, input b, output c);
    assign c = a & b;
endmodule

module OrWrapper(input a, input b, output c, output d);
    OrModule z(a, b, c);
    assign d = ~a | b;
endmodule

module OrModule(input a, input b, output c);
    assign c = a | b;
endmodule