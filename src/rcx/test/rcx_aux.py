import utl
import rcx

# Thin wrapper over api defined in ext.i (and reused by ext-py.i)
# Ensure keywords only and provide defaults when appropriate


def define_process_corner(design, *, ext_model_index=0, filename=""):
    design.getOpenRCX().define_process_corner(ext_model_index, filename)


def extract_parasitics(
    design,
    *,
    ext_model_file=None,
    corner_cnt=1,
    max_res=50.0,
    coupling_threshold=0.1,
    debug_net_id="",
    lef_res=False,
    cc_model=10,
    context_depth=5,
    no_merge_via_res=False
):

    opts = rcx.ExtractOptions()

    opts.ext_model_file = ext_model_file
    opts.corner_cnt = corner_cnt
    opts.max_res = max_res
    opts.coupling_threshold = coupling_threshold
    opts.cc_model = cc_model
    opts.context_depth = context_depth
    opts.lef_res = lef_res
    opts.debug_net = debug_net_id
    opts.no_merge_via_res = no_merge_via_res

    design.getOpenRCX().extract(opts)


def write_spef(design, *, filename="", nets="", net_id=0, coordinates=False):
    opts = rcx.SpefOptions()
    opts.file = filename
    opts.nets = nets
    opts.net_id = net_id
    if coordinates:
        opts.N = "Y"

    design.getOpenRCX().write_spef(opts)


def bench_verilog(design, *, filename=""):
    design.getOpenRCX().bench_verilog(filename)


def bench_wires(
    design,
    *,
    met_cnt=1000,
    cnt=5,
    len=100,
    over=False,
    diag=False,
    all=False,
    db_only=False,
    under_met=-1,
    w_list="1",
    s_list="1 2 2.5 3 3.5 4 4.5 5 6 8 10 12",
    over_dist=100,
    under_dist=100
):
    opts = rcx.BenchWiresOptions()
    opts.w_list = w_list
    opts.s_list = s_list
    opts.Over = over
    opts.diag = diag
    opts.gen_def_patterns = all
    opts.cnt = cnt
    opts.len = len
    opts.under_met = under_met
    opts.met_cnt = met_cnt
    opts.db_only = db_only
    opts.over_dist = over_dist
    opts.under_dist = under_dist
    design.getOpenRCX().bench_wires(opts)


def adjust_rc(design, *, res_factor=1.0, cc_factor=1.0, gndc_factor=1.0):
    design.getOpenRCX().adjust_rc(res_factor, cc_factor, gndc_factor)


def diff_spef(
    design, *, filename="", r_conn=False, r_res=False, r_cap=False, r_cc_cap=False
):
    opts = rcx.DiffOptions()
    opts.file = file
    opts.r_res = r_res
    opts.r_cap = r_cap
    opts.r_cc_cap = r_cc_cap
    opts.r_conn = r_conn
    design.getOpenRCX().diff_spef(opts)


def write_rules(design, *, filename="extRules", dir="./", name="TYP", pattern=0):
    design.getOpenRCX().write_rules(filename, dir, name, pattern)


def read_spef(design, *, filename):
    opts = rcx.DiffOptions()
    opts.file = filename
    design.getOpenRCX().read_spef(opts)
