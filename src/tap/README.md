# Tapcell

Tapcell and endcap insertion.

## Commands

```
tapcell [-tapcell_master tapcell_master]
        [-endcap_master endcap_master]
        [-distance dist]
        [-halo_width_x halo_x]
        [-halo_width_y halo_y]
        [-tap_nwin2_master tap_nwin2_master]
        [-tap_nwin3_master tap_nwin3_master]
        [-tap_nwout2_master tap_nwout2_master]
        [-tap_nwout3_master tap_nwout3_master]
        [-tap_nwintie_master tap_nwintie_master]
        [-tap_nwouttie_master tap_nwouttie_master]
        [-cnrcap_nwin_master cnrcap_nwin_master]
        [-cnrcap_nwout_master cnrcap_nwout_master]
        [-incnrcap_nwin_master incnrcap_nwin_master]
        [-incnrcap_nwout_master incnrcap_nwout_master]
        [-tap_prefix tap_prefix]
        [-endcap_prefix endcap_prefix]
```

- `-tapcell_master`. Specify the master used as a tapcell.
- `-endcap_master`. Specify the master used as an endcap.
- `-distance`. Specify the distance (in microns) between each tapcell in the checkerboard.
- `-halo_width_x`. Specify the horizontal halo size (in microns) around macros during cut rows.
- `-halo_width_y`. Specify the vertical halo size (in microns) around macros during cut rows.
- `-tap_nwintie_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation.
- `-tap_nwin2_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation. This master should be
smaller than `tap_nwintie_master`.
- `-tap_nwin3_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation. This master should be
smaller than `tap_nwin2_master`.
- `-tap_nwouttie_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation.
- `-tap_nwout2_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation. This master should be
smaller than `tap_nwouttie_master`.
- `-tap_nwout3_master`. Specify the master cell placed at the top and bottom of
macros and the core area according the row orientation. This master should be
smaller than `tap_nwout2_master`.
- `-incnrcap_nwin_master`. Specify the master cell placed at the corners of macros,
according the row orientation.
- `-incnrcap_nwout_master`. Specify the master cell placed at the corners of macros,
according the row orientation.
- `-cnrcap_nwin_master`. Specify the macro cell placed at the corners the core area
according the row orientation.
- `-cnrcap_nwout_master`. Specify the macro cell placed at the corners the core area
according the row orientation.
- `-tap_prefix`. Specify the name prefix for the tapcell instances. The default prefix is `TAP_`.
- `-endcap_prefix`. Specify the name prefix for the endcaps instances. The default prefix is `PHY_`.

The figures below show two examples of tapcell insertion. When only the 
`-tapcell_master` and `-endcap_master` masters are given, the tapcell placement
is similar to Figure 1. When the remaining masters are give, the tapcell
placement is similar to Figure 2.

| <img src="./doc/image/tapcell_example1.svg" width=450px> | <img src="./doc/image/tapcell_example2.svg" width=450px> |
|:--:|:--:|
| Figure 1: Tapcell insertion representation | Figure 2:  Tapcell insertion around macro representation |

### Only cutting rows

```
cut_rows [-endcap_master endcap_master]
         [-halo_width_x halo_x]
         [-halo_width_y halo_y]
```

- `-endcap_master`. Specify the master used as an endcap.
- `-halo_width_x`. Specify the horizontal halo size (in microns) around macros during cut rows.
- `-halo_width_y`. Specify the vertical halo size (in microns) around macros during cut rows.

### Only adding boundary/endcap cells

Place endcaps into the design, the naming for of the arguments to `place_endcaps` is based on the LEF58 CLASS specifications for endcaps.

```
place_endcaps
  [-corner master]
  [-edge_corner master]
  [-endcap masters]
  [-endcap_horizontal masters]
  [-endcap_vertical master]
  [-prefix prefix]
  [-left_top_corner master]
  [-right_top_corner master]
  [-left_bottom_corner master]
  [-right_bottom_corner master]
  [-left_top_edge master]
  [-right_top_edge master]
  [-left_bottom_edge master]
  [-right_bottom_edge master]
  [-left_edge master]
  [-right_edge master]
  [-top_edge masters]
  [-bottom_edge masters]
```

- `-prefix`. Specifies the prefix to use for the boundary cells. Defaults to "PHY_"
- `-corner`. Specify the master for the corner cells on the outer corners.
- `-edge_corner`. Specify the master for the corner cells on the inner corners.
- `-endcap`. Specify the master used as an endcap.
- `-endcap_horizontal`. Specify the list of masters for the top and bottom row endcaps. (overrides `-endcap`)
- `-endcap_vertical`. Specify the master for the left and right row endcaps. (overrides `-endcap`)
- `-left_top_corner`. Specify the master for the corner cells on the outer top left corner. (overrides `-corner`)
- `-right_top_corner`. Specify the master for the corner cells on the outer top right corner. (overrides `-corner`)
- `-left_bottom_corner`. Specify the master for the corner cells on the outer bottom left corner. (overrides `-corner`)
- `-right_bottom_corner`. Specify the master for the corner cells on the outer bottom right corner. (overrides `-corner`)
- `-left_top_edge`. Specify the master for the corner cells on the inner top left corner. (overrides `-edge_corner`)
- `-right_top_edge`. Specify the master for the corner cells on the inner top right corner. (overrides `-edge_corner`)
- `-left_bottom_edge`. Specify the master for the corner cells on the inner bottom left corner. (overrides `-edge_corner`)
- `-right_bottom_edge`. Specify the master for the corner cells on the inner bottom right corner. (overrides `-edge_corner`)
- `-left_edge`. Specify the master for the left row endcaps. (overrides `-endcap_vertical`)
- `-right_edge`. Specify the master for the right row endcaps. (overrides `-endcap_vertical`)
- `-top_edge`. Specify the list of masters for the top row endcaps. (overrides `-endcap_horizontal`)
- `-bottom_edge`. Specify the list of masters for the bottom row endcaps. (overrides `-endcap_horizontal`)

### Only adding tapcells cells

```
place_tapcells
  -master tapcell_master
  -distance dist
```

- `-master`. Specifies the master to use for the tapcells.
- `-distance`. Distance between tapcells.

## Example scripts

You can find script examples for both 45nm and 14nm in
`tap/etc/scripts`


## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+tap+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
