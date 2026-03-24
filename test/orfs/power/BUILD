load("@rules_shell//shell:sh_test.bzl", "sh_test")
load(":power.bzl", "power_report")

package(features = ["layering_check"])

# Extract individual output files from gcd_final
filegroup(
    name = "gcd_final_verilog",
    srcs = ["//test/orfs/gcd:gcd_final"],
    output_group = "6_final.v",
    tags = ["manual"],
)

filegroup(
    name = "gcd_final_sdc",
    srcs = ["//test/orfs/gcd:gcd_final"],
    output_group = "6_final.sdc",
    tags = ["manual"],
)

filegroup(
    name = "gcd_final_spef",
    srcs = ["//test/orfs/gcd:gcd_final"],
    output_group = "6_final.spef",
    tags = ["manual"],
)

# Run power.tcl with OpenSTA (standalone sta)
power_report(
    name = "gcd_power_sta",
    src = "//test/orfs/gcd:gcd_final",
    out = "gcd_power_sta.txt",
    lib_filter = "RVT_FF",
    script = ":power.tcl",
    sdc = ":gcd_final_sdc",
    spef = ":gcd_final_spef",
    sta = True,
    tags = ["manual"],
    tool = "//src/sta:opensta",
    verilog = ":gcd_final_verilog",
)

sh_test(
    name = "gcd_power_sta_test",
    srcs = ["ok.sh"],
    args = ["$(location :gcd_power_sta)"],
    data = [":gcd_power_sta"],
    tags = ["orfs"],
)

# Run power.tcl with OpenROAD
power_report(
    name = "gcd_power_openroad",
    src = "//test/orfs/gcd:gcd_final",
    out = "gcd_power_openroad.txt",
    lib_filter = "RVT_FF",
    script = ":power.tcl",
    sdc = ":gcd_final_sdc",
    spef = ":gcd_final_spef",
    tags = ["manual"],
    tool = "//:openroad",
    verilog = ":gcd_final_verilog",
)

sh_test(
    name = "gcd_power_openroad_test",
    srcs = ["ok.sh"],
    args = ["$(location :gcd_power_openroad)"],
    data = [":gcd_power_openroad"],
    tags = ["orfs"],
)
