// TOP: top
// TECH: nangate45
// TARGETS: blackbox, unresolved_module, ORD_2013
// CLUE: dbLinkDesign passes link_make_black_boxes = true, so the sta linker
// happily builds a black-box cell for an undefined module. Verilog2db then
// asks db_->findMaster() for it and errors ORD-2013 -- the reader supports
// black boxes but the db builder rejects them.
module top (a, y);
   input a;
   output y;
   missing_mod u (.i(a), .o(y));
endmodule
