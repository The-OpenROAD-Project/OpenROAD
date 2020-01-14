read_lef input.lef
read_def input.def

tapcell -endcap_cpp "1" \
        -distance "25" \
        -tapcell_master "TAPCELL_MASTER_NAME" \
        -endcap_master "ENDCAP_MASTER_NAME" \
        -halo_width_x "2"
        -halo_width_y "2"

write_def tap.def
