include("openroad")

set(TEST_NAMES
    constant1
    dont_touch_attr
    make_port
    network_edit1
    sdc_names1
    sdc_names2
    sdc_get1
    sta1
    sta2
    sta3
    sta4
    sta5
    block_sta1
    find_clks1
    find_clks2
    report_json1
    power1
    read_liberty1
    read_verilog1
    read_verilog2
    read_verilog3
    read_verilog4
    read_verilog5
    read_verilog6
    read_verilog7
    read_verilog8
    read_verilog9
    read_verilog10
    report_cell_usage
    write_verilog1
    write_verilog2
    write_verilog3
    write_verilog4
    write_verilog5
    write_verilog6
    write_verilog7
    write_verilog8
    write_sdc1
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  or_integration_test("dbSta" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
