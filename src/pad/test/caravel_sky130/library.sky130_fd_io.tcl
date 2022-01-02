Footprint library {
  types {
    sig       top_gpiov2_pad
    xres      top_xres4v2_pad
    vccd_hvc  vccd_hvc_power_pad
    vccd_lvc  vccd_lvc_power_pad
    vssd_hvc  vssd_hvc_ground_pad
    vssd_lvc  vssd_lvc_ground_pad
    vdda_hvc  vdda_hvc_power_pad
    vdda_lvc  vdda_lvc_power_pad
    vssa_hvc  vssa_hvc_ground_pad
    vssa_lvc  vssa_lvc_ground_pad
    vddio_hvc vddio_hvc_power_pad 
    vddio_lvc vddio_lvc_power_pad 
    vssio_hvc vssio_hvc_ground_pad
    vssio_lvc vssio_lvc_ground_pad
    brk_vccd  break_vccd
    brk_vdda  break_vdda
    connect   connect_vcchib_vccd_vswitch_vddio
    corner    sky130_ef_io__corner_pad
    fill      {sky130_ef_io__com_bus_slice_20um sky130_ef_io__com_bus_slice_10um sky130_ef_io__com_bus_slice_5um sky130_ef_io__com_bus_slice_1um}
  }

  connect_by_abutment {
    AMUXBUS_A
    AMUXBUS_B
    VSSA
    VDDA
    VSWITCH
    VDDIO_Q
    VSSIO_Q
    VCCHIB
    VDDIO
    VCCD
    VSSIO
    VSSD
  }

  pad_pin_name PAD
  pad_pin_layer met5

  breakers {
    break_vccd
    break_vdda
  }

  cells {
    break_vccd {
      cell_name sky130_ef_io__disconnect_vccd_slice_5um
      orient {bottom R180 right R270 top R0 left R90}
      physical_only 1
      breaks {
        VCCD {}
      }
    }
    break_vdda {
      cell_name sky130_ef_io__disconnect_vdda_slice_5um
      orient {bottom R180 right R270 top R0 left R90}
      physical_only 1
      breaks {
        VDDA {}
      }
    }
    connect_vcchib_vccd_vswitch_vddio {
      cell_name sky130_ef_io__connect_vcchib_vccd_and_vswitch_vddio_slice_20um
      orient {bottom R180 right R270 top R0 left R90}
      physical_only 1
    }
    top_gpiov2_pad {
      cell_name sky130_ef_io__gpiov2_pad_wrapped
      orient {bottom R180 right R270 top R0 left R90}
      signal_type 1
    }
    top_xres4v2_pad {
      cell_name sky130_fd_io__top_xres4v2
      orient {bottom R180 right R270 top R0 left R90}
      signal_type 1
    }
    vccd_hvc_power_pad {
      cell_name sky130_ef_io__vccd_hvc_pad
      power_pad 1
      pad_pin_name VCCD
      orient {bottom R180 right R270 top R0 left R90}
    }
    vccd_lvc_power_pad {
      cell_name sky130_ef_io__vccd_lvc_pad
      power_pad 1
      pad_pin_name VCCD
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssd_hvc_ground_pad {
      cell_name sky130_ef_io__vssd_hvc_pad
      ground_pad 1
      pad_pin_name VSSD
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssd_lvc_ground_pad {
      cell_name sky130_ef_io__vssd_lvc_pad
      ground_pad 1
      pad_pin_name VSSD
      orient {bottom R180 right R270 top R0 left R90}
    }
    vdda_hvc_power_pad {
      cell_name sky130_ef_io__vdda_hvc_pad
      power_pad 1
      pad_pin_name VDDA
      orient {bottom R180 right R270 top R0 left R90}
    }
    vdda_lvc_power_pad {
      cell_name sky130_ef_io__vdda_lvc_pad
      power_pad 1
      pad_pin_name VDDA
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssa_hvc_ground_pad {
      cell_name sky130_ef_io__vssa_hvc_pad
      ground_pad 1
      pad_pin_name VSSA
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssa_lvc_ground_pad {
      cell_name sky130_ef_io__vssa_lvc_pad
      ground_pad 1
      pad_pin_name VSSA
      orient {bottom R180 right R270 top R0 left R90}
    }
    vddio_hvc_power_pad {
      cell_name sky130_ef_io__vddio_hvc_pad
      power_pad 1
      pad_pin_name VDDIO
      orient {bottom R180 right R270 top R0 left R90}
    }
    vddio_lvc_power_pad {
      cell_name sky130_ef_io__vddio_lvc_pad
      power_pad 1
      pad_pin_name VDDIO
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssio_hvc_ground_pad {
      cell_name sky130_ef_io__vssio_hvc_pad
      ground_pad 1
      pad_pin_name VSSIO
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssio_lvc_ground_pad {
      cell_name sky130_ef_io__vssio_lvc_pad
      ground_pad 1
      pad_pin_name VSSIO
      orient {bottom R180 right R270 top R0 left R90}
    }
    sky130_ef_io__corner_pad {
      physical_only 1
      offset {200.0 204}
      orient {ll R180 lr MX ur R0 ul MY}
    }
    sky130_ef_io__com_bus_slice_1um {
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
      offset {1.0 197.965}
    }
    sky130_ef_io__com_bus_slice_5um {
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
      offset {5.0 197.965}
    }
    sky130_ef_io__com_bus_slice_10um {
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
      offset {10.0 197.965}
    }
    sky130_ef_io__com_bus_slice_20um  {
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
      offset {20.0 197.965}
    }
  }
}

