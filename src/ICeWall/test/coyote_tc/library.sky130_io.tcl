Footprint library {
  types {
    sig top_gpiov2_pad
    vccd_hvc vccd_hvc_power_pad
    vccd_lvc vccd_lvc_power_pad
    vssd_hvc vssd_hvc_ground_pad
    vssd_lvc vssd_lvc_ground_pad
    vdda_hvc vdda_hvc_power_pad
    vdda_lvc vdda_lvc_power_pad
    vssa_hvc vssa_hvc_ground_pad
    vssa_lvc vssa_lvc_ground_pad
    vddio_hvc vddio_hvc_power_pad 
    vddio_lvc vddio_lvc_power_pad 
    vssio_hvc vssio_hvc_ground_pad
    vssio_lvc vssio_hvc_ground_pad
    corner sky130_fd_io__corner_bus_overlay
    fill {s8iom0s8_com_bus_slice_tied_1um}
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

  cells {
    top_gpiov2_pad {
      cell_name sky130_fd_io__top_gpiov2
      overlay sky130_fd_io__overlay_gpiov2 
      orient {bottom R180 right R270 top R0 left R90}
      flip_pair 1
    }
    vccd_hvc_power_pad {
      cell_name sky130_fd_io__top_power_hvc_wpad
      overlay sky130_fd_io__overlay_vccd_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vccd_lvc_power_pad {
      cell_name sky130_fd_io__top_power_lvc_wpad
      overlay sky130_fd_io__overlay_vccd_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssd_hvc_ground_pad {
      cell_name sky130_fd_io__top_ground_hvc_wpad
      overlay sky130_fd_io__overlay_vssd_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssd_lvc_ground_pad {
      cell_name sky130_fd_io__top_ground_lvc_wpad
      overlay sky130_fd_io__overlay_vssd_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vdda_hvc_power_pad {
      cell_name sky130_fd_io__top_power_hvc_wpad
      overlay sky130_fd_io__overlay_vdda_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vdda_lvc_power_pad {
      cell_name sky130_fd_io__top_power_lvc_wpad
      overlay sky130_fd_io__overlay_vdda_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssa_hvc_ground_pad {
      cell_name sky130_fd_io__top_ground_hvc_wpad
      overlay sky130_fd_io__overlay_vssa_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssa_lvc_ground_pad {
      cell_name sky130_fd_io__top_ground_lvc_wpad
      overlay sky130_fd_io__overlay_vssa_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vddio_hvc_power_pad {
      cell_name sky130_fd_io__top_power_hvc_wpad
      overlay sky130_fd_io__overlay_vddio_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vddio_lvc_power_pad {
      cell_name sky130_fd_io__top_power_lvc_wpad
      overlay sky130_fd_io__overlay_vddio_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssio_hvc_ground_pad {
      cell_name sky130_fd_io__top_ground_hvc_wpad
      overlay sky130_fd_io__overlay_vssio_hvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    vssio_lvc_ground_pad {
      cell_name sky130_fd_io__top_ground_lvc_wpad
      overlay sky130_fd_io__overlay_vssio_lvc
      physical_only 1
      orient {bottom R180 right R270 top R0 left R90}
    }
    sky130_fd_io__corner_bus_overlay {
      physical_only 1
      offset {200.0 203.665}
      orient {ll R180 lr R270 ur R0 ul R90}
    }
    s8iom0s8_com_bus_slice_tied_1um {
      physical_only 1
      orient {bottom R0 right R90 top R180 left R270}
    }
  }
}

