from openroad import Design, Tech
import helpers
import cts_aux
import os

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")
tech.readLiberty("sky130hs/sky130hs_tt.lib")

design = helpers.make_design(tech)

# avoid potential name clash with tcl test
def_file = "max_cap-py.def"
design.evalTclString("source ../../rsz/test/hi_fanout.tcl")

tcl_strg1 = f'write_hi_fanout_def1 {def_file} 20 \
  "rdrv" "sky130_fd_sc_hs__buf_1" "" "X" \
  "r" "sky130_fd_sc_hs__dfxtp_1" "CLK" "D" 200000 \
  "met1" 1000'

design.evalTclString(tcl_strg1)
design.readDef(def_file)
os.remove(def_file)

tcl_strg2 = """create_clock -period 5 clk1
source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer met2
set_wire_rc -clock  -layer met3"""

design.evalTclString(tcl_strg2)

cts_aux.clock_tree_synthesis(
    design,
    root_buf="sky130_fd_sc_hs__clkbuf_1",
    buf_list="sky130_fd_sc_hs__clkbuf_1",
)

tcl_strg3 = """set_propagated_clock clk1
estimate_parasitics -placement
report_check_types -max_cap -max_slew -net [get_net -of_object [get_pin r1/CLK]]"""

design.evalTclString(tcl_strg3)
