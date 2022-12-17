read_db gcd_fill.odb
read_sdc gcd_fill.sdc

set_propagated_clock [all_clocks]

density_fill -rules fill.json




