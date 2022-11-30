# exclude horizontal edges
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("group_pins3.def")


ppl_aux.set_io_pin_constraint(design,
                              pin_names="req_msg\[10\] req_msg\[11\] req_msg\[12\] req_msg\[13\] " +
                              "req_msg\[14\] req_msg\[15\] req_msg\[16\] req_msg\[17\] " +
                              "req_msg\[18\] req_msg\[19\] req_msg\[20\] req_msg\[21\] " +
                              "req_msg\[22\] req_msg\[23\] req_msg\[0\] req_msg\[1\]",
                              region="left:*")

ppl_aux.set_io_pin_constraint(design,
                              pin_names="req_msg\[24\] req_msg\[25\] req_msg\[26\] " +
                              "req_msg\[27\] req_msg\[28\] req_msg\[29\] req_msg\[2\] " +
                              "req_msg\[30\] req_msg\[31\] req_msg\[3\] req_msg\[4\] " +
                              "req_msg\[5\] req_msg\[6\] req_msg\[7\] req_msg\[8\] req_msg\[9\]",
                              region="bottom:*")

ppl_aux.set_io_pin_constraint(design,
                              pin_names="clk req_rdy req_val reset resp_msg\[0\] " +
                              "resp_msg\[10\] resp_msg\[11\] resp_msg\[12\] " +
                              "resp_msg\[13\] resp_msg\[14\] resp_msg\[15\]",
                              region="top:*")

ppl_aux.set_io_pin_constraint(design,
                              pin_names="resp_rdy resp_val resp_msg\[1\] " +
                              "resp_msg\[2\] resp_msg\[3\] resp_msg\[4\] " +
                              "resp_msg\[5\] resp_msg\[6\] resp_msg\[7\] " +
                              "resp_msg\[8\] resp_msg\[9\]",
                              region="right:*")


ppl_aux.place_pins(design, 
                   hor_layers="metal3",
                   ver_layers="metal2",
                   corner_avoidance=0,
                   min_distance=0.24,
                   group_pins=["req_msg[10] req_msg[11] req_msg[12] req_msg[13] " +
                               "req_msg[14] req_msg[15] req_msg[16] req_msg[17] " +
                               "req_msg[18] req_msg[19] req_msg[20] req_msg[21] " +
                               "req_msg[22] req_msg[23] req_msg[0] req_msg[1]",

                               "req_msg[24] req_msg[25] req_msg[26] req_msg[27] " +
                               "req_msg[28] req_msg[29] req_msg[2] req_msg[30] "  +
                               "req_msg[31] req_msg[3] req_msg[4] req_msg[5] " +
                               "req_msg[6] req_msg[7] req_msg[8] req_msg[9]",
                               
                               "clk req_rdy req_val reset resp_msg[0] resp_msg[10] " +
                               "resp_msg[11] resp_msg[12] resp_msg[13] resp_msg[14] " +
                               "resp_msg[15]",
                               
                               "resp_rdy resp_val resp_msg[1] resp_msg[2] resp_msg[3] " +
                               "resp_msg[4] resp_msg[5] resp_msg[6] resp_msg[7] " +
                               "resp_msg[8] resp_msg[9]" ]
                   )

def_file = helpers.make_result_file("group_pins7.def")
design.writeDef(def_file)
helpers.diff_files("group_pins7.defok", def_file)
