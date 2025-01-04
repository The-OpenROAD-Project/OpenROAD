# Test for creating a cell usage snapshot.

# Test environment setup.
set snapshot_file_name cell_usage_snapshot-top-test_stage.json
source "helpers.tcl"
make_result_dir

# Populate DB with the test design.
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top

# Create the cell usage snapshot and compare it to the golden file.
create_cell_usage_snapshot -path $result_dir -stage test_stage
diff_files cell_usage_snapshot-top-test_stage.jsonok $result_dir/$snapshot_file_name

