source "helpers.tcl"

# example.3dbx instantiates one chiplet master (SoC) twice (soc_inst,
# soc_inst_duplicate). Duplicated masters are NOT supported in this version:
# descending a block placed more than once would alias its inner dbInsts
# across placements, and downstream code assumes a chiplet block is placed
# once. read_3dbx must abort with STA-3004 rather than build a wrong graph.
# (Real duplicated-master support needs per-unfold-path identity -- Track A'.)
catch { read_3dbx ../../odb/test/data/example.3dbx }
