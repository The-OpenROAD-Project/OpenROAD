# Check that 3D power-grid error locations use the 3D assembly origin.

source "helpers.tcl"

read_3dbx 3dic_test_data/3dic_error_file_assembly_coordinates.3dbx

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
set_psm_interchip_resistance -resistance 0.1

set error_file [make_result_file 3dic_error_file_assembly_coordinates.rpt]

catch { check_power_grid -net VDD -error_file $error_file } err
puts $err

report_file $error_file
