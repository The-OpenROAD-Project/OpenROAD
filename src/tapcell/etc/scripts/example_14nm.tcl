read_lef input.lef
read_def input.def

tapcell -no_cell_at_top_bottom -add_boundary_cell
        -tap_nwin2_master "TAPCELL_NWIN2_MASTER_NAME"
        -tap_nwin3_master "TAPCELL_NWIN3_MASTER_NAME"
        -tap_nwout2_master "TAPCELL_NWOUT2_MASTER_NAME"
        -tap_nwout3_master "TAPCELL_NWOUT3_MASTER_NAME"
        -tap_nwintie_master "TAPCELL_NWINTIE_MASTER_NAME"
        -tap_nwouttie_master "TAPCELL_NWOUTTIE_MASTER_NAME"
        -cnrcap_nwin_master "CNRCAP_NWIN_MASTER_NAME"
        -cnrcap_nwout_master "CNRCAP_NWOUT_MASTER_NAME"
        -incnrcap_nwin_master "INCNRCAP_NWIN_MASTER_NAME"
        -incnrcap_nwout_master "INCNRCAP_NWOUT_MASTER_NAME"
        -endcap_master "ENDCAP_MASTER_NAME"
        -tapcell_master "TAPCELL_MASTER_NAME"
        -endcap_cpp "12"
        -tbtie_cpp "16"
        -distance "25"
        -halo_width_x "2"
        -halo_width_y "2"

write_def tap.def