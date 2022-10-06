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


## Example scripts

You can find script examples for both 45nm and 14nm in
`tap/etc/scripts`


## Limitations

## FAQs

Check out [GitHub discussion](https://github.com/The-OpenROAD-Project/OpenROAD/discussions/categories/q-a?discussions_q=category%3AQ%26A+tap+in%3Atitle)
about this tool.

## License

BSD 3-Clause License. See [LICENSE](LICENSE) file.
