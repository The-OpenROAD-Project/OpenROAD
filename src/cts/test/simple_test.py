from openroad import Design, Tech
import helpers
import cts_aux
import os

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = Design(tech)
design.readDef("16sinks.def")

design.evalTclString("create_clock -period 5 clk")
design.evalTclString("set_wire_rc -clock -layer metal3")

cts_aux.clock_tree_synthesis(design, root_buf="CLKBUF_X3", buf_list="CLKBUF_X3",
                                 wire_unit=20, obstruction_aware=True, apply_ndr=True)

design.evalTclString("write_def simple_test_out.def")
def_file = "simple_test_out.def"
with open(def_file, 'r') as file:
    file_content = file.read()
    print(file_content)
    os.remove(def_file)

