import utl
import rcx

# Thin wrapper over api defined in ext.i (and reused by ext-py.i)
# Ensure keywords only and provide defaults when appropriate


def define_process_corner(*, ext_model_index=0, filename=""):
    rcx.define_process_corner(ext_model_index, filename)


def extract_parasitics(*, ext_model_file=None,
                       corner_cnt=1,
                       max_res=50.0,
                       coupling_threshold=0.1,
                       debug_net_id="",
                       lef_res=False,
                       cc_model=10,
                       context_depth=5,
                       no_merge_via_res=False
                       ):
    # NOTE: This is position dependent
    rcx.extract(ext_model_file,
                corner_cnt,
                max_res,
                coupling_threshold,
                cc_model,                
                context_depth,
                debug_net_id,
                lef_res,
                no_merge_via_res)


def write_spef(*, filename="", nets="", net_id=0, coordinates=False):
    rcx.write_spef(filename, nets, net_id, coordinates)


def bench_verilog(*, filename=""):
    rcx.bench_verilog(filename)


def bench_wires(*,
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
                under_dist=100):
    rcx.bench_wires(db_only, over, diag, all, met_cnt, cnt, len, under_met,
                    w_list, s_list, over_dist, under_dist)


def adjust_rc(*, res_factor=1.0,
              cc_factor=1.0,
              gndc_factor=1.0):
    rcx.adjust_rc(res_factor, cc_factor, gndc_factor)


def diff_spef(*, filename="",
              r_conn=False,
              r_res=False,
              r_cap=False,
              r_cc_cap=False):
    rcx.diff_spef(filename, r_conn, r_res, r_cap, r_cc_cap)    


def write_rules(*,
                filename="extRules",
                dir="./",
                name="TYP",
                pattern=0):
    rcx.write_rules(filename, dir, name, pattern)


def read_spef(*, filename):
    rcx.read_spef(filename)

