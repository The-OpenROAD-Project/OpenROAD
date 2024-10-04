sta::define_cmd_args "read_db" {[-hier] filename}

proc read_db { args } {
  sta::parse_key_args "read_db" $args keys {} flags {-hier}
  sta::check_argc_eq1or2 "read_db" $args    
  set filename [file nativename [lindex $args 0]]
  if { ![file exists $filename] } {
    utl::error "ORD" 7 "$filename does not exist."
  }
  set hierarchy [info exists flags(-hier)]    
  if { ![file readable $filename] } {
    utl::error "ORD" 8 "$filename is not readable."
  }
  ord::read_db_cmd $filename $hierarchy
}
