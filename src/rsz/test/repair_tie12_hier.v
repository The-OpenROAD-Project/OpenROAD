module top (input top_in);
  wire main_net;

  HIER1 hier1 (.in(top_in), .MI0(main_net));

  BUFx2_ASAP7_75t_R load0 (.A(main_net), .Y());
  ALU_module alu0 (.MI1(main_net));
  HIER3 hier3 (.MI3(main_net));
endmodule


module HIER1 (input in, output MI0);
  TIEHIx1_ASAP7_75t_R tie0 (.H(MI0));
endmodule


module ALU_module (input MI1);
  HIER2 hier2 (.MI2(MI1));
  BUFx2_ASAP7_75t_R load2 (.A(MI1), .Y());
endmodule


module HIER2 (input MI2);
  BUFx2_ASAP7_75t_R load1 ( .A(MI2), .Y() );
endmodule


module HIER3 (input MI3);
  ALU_module alu1 (.MI1(MI3));
endmodule