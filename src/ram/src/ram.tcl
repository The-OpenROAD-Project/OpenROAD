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

# Original command for backward compatibility
sta::define_cmd_args "generate_ram_netlist" {
    -bytes_per_word bits
    -word_count words
    [-storage_cell name]
    [-tristate_cell name]
    [-inv_cell name]
    [-read_ports count]
}
  
proc generate_ram_netlist { args } {
  sta::parse_key_args "generate_ram_netlist" args keys {
      -bytes_per_word 
      -word_count 
      -storage_cell 
      -tristate_cell 
      -inv_cell
      -read_ports
  } flags {}
  
  # Required args
  if { ![info exists keys(-bytes_per_word)] } {
    utl::error RAM 1 "The -bytes_per_word argument must be specified."
  }
  if { ![info exists keys(-word_count)] } {
    utl::error RAM 2 "The -word_count argument must be specified."
  }
  
  set bytes_per_word $keys(-bytes_per_word)
  set word_count $keys(-word_count)
  
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
  
  # Call the implementation (original API for compatibility)
  ram::generate_ram_netlist_cmd \
    $bytes_per_word \
    $word_count \
    $storage_cell \
    $tristate_cell \
    $inv_cell \
    $read_ports
}

# New command for DFFRAM-style memory generation
sta::define_cmd_args "generate_dffram" {
    -bytes_per_word bits
    -word_count words
    [-memory_type type]
    [-read_ports count]
    [-optimize_layout true/false]
    [-output_def filename]
}

proc generate_dffram { args } {
  sta::parse_key_args "generate_dffram" args keys {
      -bytes_per_word 
      -word_count 
      -memory_type
      -read_ports
      -optimize_layout
      -output_def
  } flags {}
  
  # Required args
  if { ![info exists keys(-bytes_per_word)] } {
    utl::error RAM 5 "The -bytes_per_word argument must be specified."
  }
  if { ![info exists keys(-word_count)] } {
    utl::error RAM 89 "The -word_count argument must be specified."
  }
  
  # Get parameters
  set bytes_per_word $keys(-bytes_per_word)
  set word_count $keys(-word_count)
  
  # Optional args
  set memory_type "1RW"
  if { [info exists keys(-memory_type)] } {
    set memory_type $keys(-memory_type)
    
    # Validate memory type
    if { $memory_type != "1RW" && $memory_type != "1RW1R" && $memory_type != "2R1W" } {
      utl::error RAM 3 "Invalid memory type '$memory_type'. Must be one of: 1RW, 1RW1R, 2R1W"
    }
  }
  
  set read_ports 1
  if { [info exists keys(-read_ports)] } {
    set read_ports $keys(-read_ports)
  }
  
  set optimize_layout true
  if { [info exists keys(-optimize_layout)] } {
    set optimize_layout $keys(-optimize_layout)
  }
  
  set output_def ""
  if { [info exists keys(-output_def)] } {
    set output_def $keys(-output_def)
  }
  
  # Convert memory_type string to integer
  set mem_type_val 0
  if { $memory_type == "1RW" } {
    set mem_type_val 0
  } elseif { $memory_type == "1RW1R" } {
    set mem_type_val 1
  } elseif { $memory_type == "2R1W" } {
    set mem_type_val 2
  }
  
  # Log configuration
  utl::info RAM 10 "Generating DFFRAM-style memory with configuration:"
  utl::info RAM 11 "  bytes_per_word = $bytes_per_word"
  utl::info RAM 12 "  word_count = $word_count"
  utl::info RAM 13 "  memory_type = $memory_type"
  utl::info RAM 14 "  read_ports = $read_ports"
  utl::info RAM 15 "  optimize_layout = $optimize_layout"
  if { $output_def != "" } {
    utl::info RAM 16 "  output_def = $output_def"
  }
  
  # Call the implementation
  ram::generate_dffram_cmd \
    $bytes_per_word \
    $word_count \
    $read_ports \
    $mem_type_val \
    $output_def \
    $optimize_layout
}

# Command for register file generation
sta::define_cmd_args "generate_register_file" {
    -word_count words
    -word_width bits
    [-output_def filename]
}

proc generate_register_file { args } {
  sta::parse_key_args "generate_register_file" args keys {
      -word_count 
      -word_width
      -output_def
  } flags {}
  
  # Required args
  if { ![info exists keys(-word_count)] } {
    utl::error RAM 30 "The -word_count argument must be specified."
  }
  if { ![info exists keys(-word_width)] } {
    utl::error RAM 31 "The -word_width argument must be specified."
  }
  
  # Get parameters
  set word_count $keys(-word_count)
  set word_width $keys(-word_width)
  
  # Optional args
  set output_def ""
  if { [info exists keys(-output_def)] } {
    set output_def $keys(-output_def)
  }
  
  # Log configuration
  utl::info RAM 40 "Generating register file with configuration:"
  utl::info RAM 41 "  word_count = $word_count"
  utl::info RAM 42 "  word_width = $word_width"
  if { $output_def != "" } {
    utl::info RAM 43 "  output_def = $output_def"
  }
  
  # Call the implementation
  ram::generate_register_file_cmd \
    $word_count \
    $word_width \
    $output_def
}

# Command for generating RAM macro of standard sizes
sta::define_cmd_args "generate_ram_macro" {
    -size name
    [-memory_type type]
    [-output_def filename]
}

proc generate_ram_macro { args } {
  sta::parse_key_args "generate_ram_macro" args keys {
      -size
      -memory_type
      -output_def
  } flags {}
  
  # Required args
  if { ![info exists keys(-size)] } {
    utl::error RAM 50 "The -size argument must be specified."
  }
  
  # Get parameters
  set size $keys(-size)
  
  # Optional args
  set memory_type "1RW"
  if { [info exists keys(-memory_type)] } {
    set memory_type $keys(-memory_type)
  }
  
  set output_def ""
  if { [info exists keys(-output_def)] } {
    set output_def $keys(-output_def)
  }
  
  # Parse the size parameter (e.g., 32x32, 128x32, etc.)
  if {![regexp {(\d+)x(\d+)} $size match word_count word_width]} {
    utl::error RAM 51 "Invalid size format '$size'. Should be in the format <words>x<bits> (e.g., 32x32, 128x32)."
  }
  
  # Calculate bytes_per_word from word_width
  set bytes_per_word [expr {$word_width / 8}]
  if {[expr {$word_width % 8}] != 0} {
    incr bytes_per_word
    utl::warn RAM 52 "Word width $word_width is not a multiple of 8. Using $bytes_per_word bytes per word."
  }
  
  utl::info RAM 53 "Generating $size RAM macro with $memory_type ports"
  
  # Call the DFFRAM generator
  generate_dffram \
    -bytes_per_word $bytes_per_word \
    -word_count $word_count \
    -memory_type $memory_type \
    -optimize_layout true \
    -output_def $output_def
}