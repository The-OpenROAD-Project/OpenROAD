#############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2023, Precision Innovations Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
#############################################################################

sta::define_cmd_args "generate_ram_netlist" {-bytes_per_word bits
                                             -word_count words
                                             [-storage_cell name]
                                             [-tristate_cell name]
                                             [-inv_cell name]
                                             [-read_ports count]}
  
proc generate_ram_netlist { args } {
  sta::parse_key_args "generate_ram_netlist" args \
      keys {-bytes_per_word -word_count -storage_cell -tristate_cell -inv_cell
      -read_ports } flags {}

  if { [info exists keys(-bytes_per_word)] } {
    set bytes_per_word $keys(-bytes_per_word)
  } else {
    utl::error RAM 1 "The -bytes_per_word argument must be specified."
  }

  if { [info exists keys(-word_count)] } {
    set word_count $keys(-word_count)
  } else {
    utl::error RAM 2 "The -word_count argument must be specified."
  }

  set storage_cell ""
  if { [info exists keys(-storage_cell)] } {
    set storage_cell $keys(-storage_cell)
  }

  set tristate_cell ""
  if { [info exists keys(-tristate_cell)] } {
    set tristate_cell $keys(-tristate_cell)
  }

  set inv_cell ""
  if { [info exists keys(-inv_cell)] } {
    set inv_cell $keys(-inv_cell)
  }

  set read_ports 1
  if { [info exists keys(-read_ports)] } {
    set read_ports $keys(-read_ports)
  }
  
  ram::generate_ram_netlist_cmd $bytes_per_word $word_count $storage_cell \
      $tristate_cell $inv_cell $read_ports
}


