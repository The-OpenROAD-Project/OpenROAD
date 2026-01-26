// A standard cell definition for the test.
module and2 (input A, input B, output Z);
endmodule

// Module to test ORD-2043: defined but never instantiated
module unused_module (input A, output Z);
   assign Z = A;
endmodule

// Module to test ORD-2038: module with no instances
module empty_module ();
endmodule

module top (
    input clk,
    input top_in,
    output top_out,
    output out_unconnected // For ORD-2040
);

    // [WARNING ORD-2038] SanityCheck: Module 'i_empty' has no instances.
    empty_module i_empty();

    // [ODB-0050] SanityCheck: 'w2' has no driver.
    // [ODB-0051] SanityCheck: 'w2' has less than 2 connections
    wire w1, w2;
    and2 u1 (.A(w1), .B(), .Z(w2));

    // [ODB-0051] SanityCheck: 'single_conn_wire' has less than 2 connections
    wire single_conn_wire;
    and2 u2 (.A(single_conn_wire), .B(top_in), .Z());

    wire dangling_wire;

    // Normal connection to make the design valid for linking.
    assign top_out = w2;
    assign w1 = top_in;

endmodule