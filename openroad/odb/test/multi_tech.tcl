read_lef -tech_name ng45_tech -tech -lib Nangate45/Nangate45_tech.lef
read_lef -tech_name ng45_tech -lib Nangate45/Nangate45_stdcell.lef

read_lef -tech_name sky_tech -tech -lib sky130hd/sky130hd.tlef
read_lef -tech_name sky_tech -lib sky130hd/sky130hd_std_cell.lef

read_def -tech ng45_tech data/gcd/floorplan.def
