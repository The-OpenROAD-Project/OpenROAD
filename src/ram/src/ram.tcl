# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2025, The OpenROAD Authors

sta::define_cmd_args "generate_ram_netlist" {-bytes_per_word bits
                                             -word_count words
                                             [-storage_cell name]
                                             [-tristate_cell name]
                                             [-inv_cell name]
                                             [-read_ports count]}

proc generate_ram_netlist {args} {
    sta::parse_key_args "generate_ram_netlist" args \
        keys {-bytes_per_word -word_count -storage_cell -tristate_cell -inv_cell
      -read_ports } flags {}

    if {[info exists keys(-bytes_per_word)]} {
        set bytes_per_word $keys(-bytes_per_word)
    } else {
        utl::error RAM 1 "The -bytes_per_word argument must be specified."
    }

    if {[info exists keys(-word_count)]} {
        set word_count $keys(-word_count)
    } else {
        utl::error RAM 2 "The -word_count argument must be specified."
    }

    set storage_cell ""
    if {[info exists keys(-storage_cell)]} {
        set storage_cell $keys(-storage_cell)
    }

    set tristate_cell ""
    if {[info exists keys(-tristate_cell)]} {
        set tristate_cell $keys(-tristate_cell)
    }

    set inv_cell ""
    if {[info exists keys(-inv_cell)]} {
        set inv_cell $keys(-inv_cell)
    }

    set read_ports 1
    if {[info exists keys(-read_ports)]} {
        set read_ports $keys(-read_ports)
    }

    ram::generate_ram_netlist_cmd $bytes_per_word $word_count $storage_cell \
        $tristate_cell $inv_cell $read_ports
}
