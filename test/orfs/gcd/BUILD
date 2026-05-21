load("@bazel-orfs//:openroad.bzl", "orfs_flow")
load("@bazel-orfs//:sweep.bzl", "orfs_sweep")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//test/orfs:check_same.bzl", "check_same")
load(":pdk_libs.bzl", "pdk_files")

package(features = ["layering_check"])

# Expose the asap7 orfs_pdk's full source + Liberty set (PdkInfo.files
# + PdkInfo.libs) as a regular file-providing target so a vanilla
# genrule can pass LEF / .lib paths to `openroad`.
pdk_files(
    name = "asap7_pdk_files",
    pdk = "@orfs//flow:asap7",
    tags = ["manual"],
)

# Drive OpenROAD's integrated synthesis tool (sv_elaborate +
# synthesize, introduced by The-OpenROAD-Project/OpenROAD#10473) on
# gcd.v and emit a Verilog netlist that bazel-orfs's flow can pick up
# via SYNTH_NETLIST_FILES. This is a throwaway hook -- once the
# integrated syn tool is stable and bazel-orfs/ORFS grow first-class
# selectors for the synthesis frontend, this should be replaced by an
# `orfs_synth(SYNTH_HDL_FRONTEND="openroad")` (or equivalent).
#
# Filter the full PdkInfo set at runtime: keep only the RVT/TT NLDM
# corner libs (matching ORFS's default PRIMARY_VT=RVT for asap7, see
# flow/platforms/asap7/config.mk) and the asap7 tech / SC LEFs needed
# by `syn::export_to_odb`. Doing the filtering in shell avoids having
# to enumerate every file in Starlark.
genrule(
    name = "gcd_syn_netlist_gen",
    srcs = [
        ":asap7_pdk_files",
        ":gcd.v",
        ":synthesize_gcd.tcl",
    ],
    outs = ["gcd_syn_netlist.v"],
    cmd = """
LIBS=""
LEFS=""
for f in $(execpaths :asap7_pdk_files); do
  case "$$f" in
    */asap7/lib/NLDM/*RVT_TT*nldm*.lib|*/asap7/lib/NLDM/*RVT_TT*nldm*.lib.gz)
      LIBS="$$LIBS $$f"
      ;;
    */asap7/lef/asap7_tech_1x_201209.lef)
      LEFS="$$LEFS $$f"
      ;;
    */asap7/lef/asap7sc7p5t_28_R_1x_220121a.lef)
      LEFS="$$LEFS $$f"
      ;;
  esac
done
export LIB_FILES="$$LIBS"
export LEF_FILES="$$LEFS"
export VERILOG_FILE="$(execpath :gcd.v)"
export OUT_NETLIST="$@"
echo "Using LIB_FILES=$$LIB_FILES" 1>&2
echo "Using LEF_FILES=$$LEF_FILES" 1>&2
$(execpath //:openroad) -no_init -exit $(execpath :synthesize_gcd.tcl)
""",
    tags = ["manual"],
    tools = ["//:openroad"],
)

orfs_sweep(
    name = "gcd",
    # buildifier: disable=unsorted-dict-items
    arguments = {
        # Various
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "DIE_AREA": "0 0 16.2 16.2",
        "PLACE_DENSITY": "0.35",
        "OPENROAD_HIERARCHICAL": "1",
        # Demonstrate clock period specified in arguments, useful
        # for Optuna scripts.
        "ABC_CLOCK_PERIOD_IN_PS": "310.001",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
    },
    stage = "final",
    sweep = {
        "b": {},
        "base": {},
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["gcd.v"],
)

filegroup(
    name = "gcd_floorplan_odb",
    srcs = [":gcd_floorplan"],
    output_group = "2_floorplan.odb",
    tags = ["manual"],
)

sh_test(
    name = "gcd_whittle_test",
    timeout = "long",
    srcs = ["test_whittle.sh"],
    args = [
        "$(rootpath //:openroad)",
        "$(rootpath //etc:whittle.py)",
        "$(rootpath :gcd_floorplan_odb)",
    ],
    data = [
        ":gcd_floorplan_odb",
        "//:openroad",
        "//etc:whittle.py",
        "//etc:whittle_cleanup.tcl",
        "//etc:whittle_cut.tcl",
    ],
    tags = ["orfs"],
)

check_same(
    name = "gcd_idempotent_test",
    flow = "gcd",
    tags = ["orfs"],
    variant_a = "base",
    variant_b = "b",
)

# End-to-end smoke target for OpenROAD's integrated synthesis: feed the
# syn-produced netlist into the rest of orfs_flow(). With RULES_JSON
# set, orfs_flow auto-generates `gcd_syn_test` as the QoR test target.
orfs_flow(
    name = "gcd_syn",
    top = "gcd",
    arguments = {
        # `constraint.sdc` references `$::env(ABC_CLOCK_PERIOD_IN_PS)`,
        # so it must be set explicitly; otherwise ORFS's Makefile would
        # try to grep that literal Tcl expression out of the SDC and
        # then echo it through bash. Match the value used by the
        # existing `gcd` orfs_sweep above.
        "ABC_CLOCK_PERIOD_IN_PS": "310.001",
        "CORE_AREA": "1.08 1.08 15.12 15.12",
        "DIE_AREA": "0 0 16.2 16.2",
        "PLACE_DENSITY": "0.35",
    },
    sources = {
        "RULES_JSON": [":rules-base.json"],
        "SDC_FILE": [":constraint.sdc"],
        "SYNTH_NETLIST_FILES": [":gcd_syn_netlist_gen"],
    },
    tags = ["manual"],
    test_kwargs = {
        "tags": ["orfs"],
    },
    verilog_files = ["gcd.v"],
)
