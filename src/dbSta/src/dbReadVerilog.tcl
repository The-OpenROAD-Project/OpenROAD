############################################################################
##
## Copyright (c) 2019, OpenROAD
## All rights reserved.
##
## BSD 3-Clause License
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
##
############################################################################

# Read Verilog to OpenDB

sta::define_cmd_args "read_verilog" {filename}

proc read_verilog { filename } {
  ord::read_verilog_cmd [file nativename $filename]
}

sta::define_cmd_args "link_design" {[top_cell_name]}

proc link_design { {top_cell_name ""} } {
  variable current_design_name

  if { $top_cell_name == "" } {
    if { $current_design_name == "" } {
      utl::error ORD 1009 "missing top_cell_name argument and no current_design."
      return 0
    } else {
      set top_cell_name $current_design_name
    }
  }
  if { ![ord::db_has_tech] } {
    utl::error ORD 1010 "no technology has been read."
  }
  ord::link_design_db_cmd $top_cell_name
}

sta::define_cmd_args "write_verilog" {[-sort] [-include_pwr_gnd]\
					[-remove_cells cells] filename}

proc write_verilog { args } {
  sta::parse_key_args "write_verilog" args keys {-remove_cells} \
    flags {-sort -include_pwr_gnd}

  set remove_cells {}
  if { [info exists keys(-remove_cells)] } {
    set remove_cells [sta::parse_libcell_arg $keys(-remove_cells)]
  }
  set sort [info exists flags(-sort)]
  set include_pwr_gnd [info exists flags(-include_pwr_gnd)]
  sta::check_argc_eq1 "write_verilog" $args
  set filename [file nativename [lindex $args 0]]
  ord::write_verilog_cmd $filename $sort $include_pwr_gnd $remove_cells
}
