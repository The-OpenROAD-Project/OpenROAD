# Key output errors must preserve an owner's existing key material.
source "helpers.tcl"
set private [file normalize [make_result_file key_files_private.txt]]
set public [file normalize [make_result_file key_files_public.txt]]
set key [string repeat 0 64]
set args [list -design_id test -key_hex $key -nonce_hex 001122]
proc read_text { path } {
  set fh [open $path r]
  set result [read $fh]
  close $fh
  return $result
}
proc seed_file { path mode } {
  file delete -force $path
  set fh [open $path {WRONLY CREAT EXCL} $mode]
  puts $fh "existing key material"
  close $fh
  file attributes $path -permissions $mode
}
seed_file $private 0644
set before [read_text $private]
check "permissive private destination is refused" {
  catch { generate_watermark_key {*}$args -file $private }
} 1
check "permission error does not truncate the old key" { read_text $private } $before
file attributes $private -permissions 0600
foreach alias [list $private [file join [file dirname $private] . [file tail $private]]] {
  check "identical output paths are refused" {
    catch { generate_watermark_key {*}$args -file $private -public_file $alias }
  } 1
  check "alias error preserves the old key" { read_text $private } $before
}
foreach type { -symbolic -hard } {
  file delete -force $public
  file link $type $public $private
  check "$type aliases are refused" {
    catch { generate_watermark_key {*}$args -file $private -public_file $public }
  } 1
  check "$type alias preserves the old key" { read_text $private } $before
}
file delete -force $public
check "unwritable public destination fails before replacing the private file" {
  catch { generate_watermark_key {*}$args -file $private -public_file $public/missing/file }
} 1
check "failed public write preserves the private key" { read_text $private } $before
check "temporary private file is cleaned after public write fails" {
  glob -nocomplain $private.wmk-*.tmp
} {}
# A failed write to a staged file must also preserve both previous outputs.
seed_file $public 0644
rename puts real_puts
# tclint-disable-next-line redefined-builtin
proc puts { args } {
  if { [llength $args] == 2 && [string match {nonce_hex *} [lindex $args 1]] } {
    error "injected key write failure"
  }
  uplevel 1 [list real_puts {*}$args]
}
set failed [catch { generate_watermark_key {*}$args -file $private -public_file $public }]
rename puts {}
rename real_puts puts
check "write failure is returned" { set failed } 1
check "failed write preserves private output" { read_text $private } $before
check "failed write preserves public output" { read_text $public } $before
check "failed write cleans temporary outputs" {
  glob -nocomplain $private.wmk-*.tmp $public.wmk-*.tmp
} {}
set result [generate_watermark_key {*}$args -file $private -public_file $public]
check "with -file the result carries only the public parameters" {
  lsort [dict keys $result]
} {design_id nonce_hex}
check "successful output retains owner-only permissions" {
  expr {([file attributes $private -permissions] & 0o077) == 0}
} 1
set material [wmk::read_key_file $private]
set public_text [read_text $public]
check "private file retains the secret key" { dict get $material key_hex } $key
foreach stage { key_hex placement cts routing } {
  check "private file retains $stage" { dict exists $material $stage } 1
  check "public file omits $stage" { string match "*$stage *" $public_text } 0
}
foreach stage { placement cts routing } {
  check "persisted $stage key can be rederived" {
    derive_watermark_key -design_id test -key_hex $key -nonce_hex 001122 -stage $stage
  } [dict get $material $stage]
  check "and read back through -key_file" {
    derive_watermark_key -key_file $private -stage $stage
  } [dict get $material $stage]
}
check "a key file without stored stage keys still derives them" {
  set fh [open $public a]
  puts $fh "key_hex $key"
  close $fh
  derive_watermark_key -key_file $public -stage routing
} [dict get $material routing]
exit_summary
