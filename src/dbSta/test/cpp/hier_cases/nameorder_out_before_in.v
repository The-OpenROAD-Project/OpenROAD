// TOP: top
// TECH: nangate45
// TARGETS: name_order_probe, port_to_port_feedthrough, no_top_wire
// CLUE: pure port-to-port feedthrough with NO top-level wire: the top OUTPUT port sorts before the top INPUT port, so the alias group is {a_out, z_in}.

module ft (input a, output y);
  assign y = a;
endmodule

module top (input z_in, output a_out);
  ft u0 (.a(z_in), .y(a_out));
endmodule
