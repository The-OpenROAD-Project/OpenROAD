// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, empty_positional, first_slot
// CLUE: Empty FIRST positional slot: sub u ( , a, y). Legal Verilog-2005
// CLUE: (ordered_port_connection ::= {attribute_instance} [expression]).

module top (a, y);
 input a;
 output y;
 sub u ( , a, y);
endmodule

module sub (di, i, o);
 input di, i;
 output o;
 BUF_X1 b (.A(i), .Z(o));
endmodule
