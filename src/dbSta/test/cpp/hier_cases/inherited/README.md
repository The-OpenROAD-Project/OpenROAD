# Inherited fixtures

`inherited/` symlinks netlists owned by other test suites (src/odb/test,
src/rsz/test) into this corpus, so the corpus is a set of folders rather than
a manifest to maintain. A symlink is picked up automatically; nothing else
needs editing unless the netlist's top module is not `top`, in which case name
it in `HIER_TOP_OVERRIDES` in ../../BUILD.

## Not carried, and why

49 of the 90 inherited netlists are deliberately absent: the oracle
cannot adjudicate them, so a test over them would prove nothing about
OpenROAD. They are listed here so nobody re-adds them expecting signal.

- `TestBufferRemoval3_0.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestBufferRemoval3_1.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestBufferRemoval3_2.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestDbSta_0.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_AfterDriver_Case1_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_AfterDriver_Case1_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_AfterDriver_Case2_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_AfterDriver_Case2_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case13.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case13_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case17_post.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case17_pre.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case18_post.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case18_pre.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case19_post.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case19_pre.v` -- names an instance 'buf', a reserved Verilog gate-primitive keyword; OpenROAD's reader is permissive about it but the netlist is not legal Verilog
- `TestInsertBuffer_BeforeLoads_Case20_post.v` -- instantiates the fakeram45_64x7 macro, whose liberty is not among the libraries this suite loads
- `TestInsertBuffer_BeforeLoads_Case20_pre.v` -- instantiates the fakeram45_64x7 macro, whose liberty is not among the libraries this suite loads
- `TestInsertBuffer_BeforeLoads_Case22_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case22_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case24_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case24_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case26_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case26_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case27_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case27_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case28_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case28_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case29_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case29_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case30_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case30_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case31_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case31_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case32_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case32_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case3_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case5_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case6.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case6_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case7.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestInsertBuffer_BeforeLoads_Case7_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestReadVerilog_FeedThrough_post.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `TestReadVerilog_FeedThrough_pre.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `escape_slash_hier.v` -- SEC: sequential pin has no combinational expression in this netlist
- `hier_escape_port.v` -- SEC: sequential pin has no combinational expression in this netlist
- `hier_highest_connected_net.v` -- not provably equivalent to ITSELF under SEC; the netlist or the checker is at fault, not either link mode
- `hierclock_gate.v` -- SEC cannot run: the netlist's own outputs all sit behind no-driver/multi-driver/logical-loop cones
- `replace_hier_mod1.v` -- SEC: sequential pin has no combinational expression in this netlist
