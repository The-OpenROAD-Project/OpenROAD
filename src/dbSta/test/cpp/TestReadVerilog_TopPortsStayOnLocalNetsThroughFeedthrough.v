module top(input [1:0] ti, output [1:0] to, output direct);
  wrapper u_top(.wi(ti), .wo(to));
  assign direct = ti[0];
endmodule

module wrapper(input [1:0] wi, output [1:0] wo);
  feedthrough u_child(.li(wi), .lo(wo));
endmodule

module feedthrough(input [1:0] li, output [1:0] lo);
  assign lo = li;
endmodule
