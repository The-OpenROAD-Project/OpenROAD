# PDK settings for the buffer_varying_lengths tests and rebuffer_reference/.
# Set buffer_varying_lengths_pdk before sourcing (default nangate45).
if { ![info exists buffer_varying_lengths_pdk] } {
  set buffer_varying_lengths_pdk nangate45
}

switch $buffer_varying_lengths_pdk {
  nangate45 {
    set bvl_libs {Nangate45/Nangate45_typ.lib}
    set bvl_lefs {Nangate45/Nangate45.lef}
    set bvl_rc_script ""
    set bvl_dont_use {}
    # Driver, load, and input driving cell.
    set bvl_buffer BUF_X1
    set bvl_buffer_in A
    set bvl_buffer_out Z
    # Buffer sizes in increasing drive strength.
    set bvl_buffers {BUF_X1 BUF_X2 BUF_X4 BUF_X8 BUF_X16 BUF_X32}
    set bvl_pin_layer metal1
    set bvl_wire_layer metal2
    # Path i is i * bvl_step_um long.
    set bvl_step_um 25
    # Clock period, in library time units.
    set bvl_period 1.0
    # rebuffer_reference/ seeds: driver sizes for the sweep, and the first
    # and following buffers of chains that start at the driver.
    set bvl_ref_drivers {BUF_X1 BUF_X4 BUF_X8 BUF_X16}
    set bvl_ref_first_buffers {BUF_X4 BUF_X8 BUF_X16}
    set bvl_ref_chain_buffers {BUF_X4 BUF_X8}
  }
  asap7 {
    set bvl_libs {asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz}
    set bvl_lefs {
      asap7/asap7_tech_1x_201209.lef
      asap7/asap7sc7p5t_28_R_1x_220121a.lef
    }
    set bvl_rc_script asap7/setRC.tcl
    set bvl_dont_use {*xp*_ASAP7*}
    set bvl_buffer BUFx2_ASAP7_75t_R
    set bvl_buffer_in A
    set bvl_buffer_out Y
    set bvl_buffers {
      BUFx2_ASAP7_75t_R BUFx3_ASAP7_75t_R BUFx4_ASAP7_75t_R
      BUFx4f_ASAP7_75t_R BUFx5_ASAP7_75t_R BUFx6f_ASAP7_75t_R
      BUFx8_ASAP7_75t_R BUFx10_ASAP7_75t_R BUFx12_ASAP7_75t_R
      BUFx12f_ASAP7_75t_R BUFx16f_ASAP7_75t_R BUFx24_ASAP7_75t_R
    }
    set bvl_pin_layer M1
    set bvl_wire_layer M3
    set bvl_step_um 3.75
    set bvl_period 1000.0
    set bvl_ref_drivers {
      BUFx2_ASAP7_75t_R BUFx4f_ASAP7_75t_R BUFx8_ASAP7_75t_R
      BUFx16f_ASAP7_75t_R
    }
    set bvl_ref_first_buffers {
      BUFx4f_ASAP7_75t_R BUFx8_ASAP7_75t_R BUFx16f_ASAP7_75t_R
    }
    set bvl_ref_chain_buffers {BUFx4f_ASAP7_75t_R BUFx6f_ASAP7_75t_R}
  }
  default {
    error "unknown buffer_varying_lengths_pdk $buffer_varying_lengths_pdk"
  }
}

proc bvl_read_pdk { } {
  global bvl_libs bvl_lefs bvl_dont_use
  foreach lib $bvl_libs {
    read_liberty $lib
  }
  foreach lef $bvl_lefs {
    read_lef $lef
  }
  if { $bvl_dont_use != {} } {
    set_dont_use $bvl_dont_use
  }
}

# Wire parasitics; call after the design is created.
proc bvl_set_wire_rc { } {
  global bvl_rc_script bvl_wire_layer
  if { $bvl_rc_script != "" } {
    source $bvl_rc_script
  }
  set_wire_rc -layer $bvl_wire_layer
}
