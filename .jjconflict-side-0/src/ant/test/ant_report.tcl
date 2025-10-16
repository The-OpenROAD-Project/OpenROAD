source "helpers.tcl"
# A minimal LEF file that has been modified to include particular antenna values for testing
read_lef ant_check.lef
read_def ant_check.def

# set report file
set report_file [make_result_file ant_report.rpt]

check_antennas -report_file $report_file

diff_file ant_report.rptok $report_file
