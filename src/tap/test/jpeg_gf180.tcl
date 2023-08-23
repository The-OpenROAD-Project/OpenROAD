source "helpers.tcl"
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef
read_def gf180/jpeg.def

# Testing endcaps on top/bottom cells eventhough they should not be there
catch {tapcell -distance 100 \
    -tapcell_master "gf180mcu_fd_sc_mcu9t5v0__filltie" \
    -endcap_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwin2_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwin3_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwout2_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwout3_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwintie_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -tap_nwouttie_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -cnrcap_nwin_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -cnrcap_nwout_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -incnrcap_nwin_master "gf180mcu_fd_sc_mcu9t5v0__endcap" \
    -incnrcap_nwout_master "gf180mcu_fd_sc_mcu9t5v0__endcap"} err
puts $err
