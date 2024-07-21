# BSD 3-Clause License
#
# Copyright (c) 2023, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

sta::define_cmd_args "preview_dft" { [-verbose]}

proc preview_dft { args } {
  sta::parse_key_args "preview_dft" args \
    keys {} \
    flags {-verbose}

  sta::check_argc_eq0 "preview_dft" $args

  if { [ord::get_db_block] == "NULL" } {
    utl::error DFT 1 "No design block found."
  }

  set verbose [info exists flags(-verbose)]

  dft::preview_dft $verbose
}

sta::define_cmd_args "scan_replace" { }
proc scan_replace { args } {
  sta::parse_key_args "scan_replace" args \
    keys {} flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error DFT 8 "No design block found."
  }
  dft::scan_replace
}

sta::define_cmd_args "insert_dft" { }
proc insert_dft { args } {
  sta::parse_key_args "insert_dft" args \
    keys {} flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error DFT 9 "No design block found."
  }
  dft::insert_dft
}

sta::define_cmd_args "set_dft_config" { [-max_length max_length] \
                                        [-max_chains max_chains] \
                                        [-clock_mixing clock_mixing]}
proc set_dft_config { args } {
  sta::parse_key_args "set_dft_config" args \
    keys {-max_length -max_chains -clock_mixing} \
    flags {}

  sta::check_argc_eq0 "set_dft_config" $args

  if {[info exists keys(-max_length)]} {
    set max_length $keys(-max_length)
    sta::check_positive_integer "-max_length" $max_length
    dft::set_dft_config_max_length $max_length
  }

  if {[info exists keys(-max_chains)]} {
    set max_chains $keys(-max_chains)
    sta::check_positive_integer "-max_chains" $max_chains
    dft::set_dft_config_max_chains $max_chains
  }

  if {[info exists keys(-clock_mixing)]} {
    set clock_mixing $keys(-clock_mixing)
    puts $clock_mixing
    dft::set_dft_config_clock_mixing $clock_mixing
  }
}

sta::define_cmd_args "report_dft_config" { }
proc report_dft_config {} {
  sta::parse_key_args "report_dft_config" args keys {} flags {}
  dft::report_dft_config
}
