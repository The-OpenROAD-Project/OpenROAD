# Key files preserve public inputs and reject malformed data without logging keys.
source "helpers.tcl"
set private [make_result_file key_file_format_private.txt]
set public [make_result_file key_file_format_public.txt]
set malformed [make_result_file key_file_format_malformed.txt]
set key [string repeat a 64]
set header {# OpenROAD watermark key file v1}

proc write_key_fixture { path text } {
  set fh [open $path w]
  fconfigure $fh -encoding utf-8 -translation lf
  puts $fh $text
  close $fh
}

proc catch_key_error { path } {
  set failed [catch {
    derive_watermark_key -key_file $path -stage placement
  } message options]
  return [list $failed $message $options]
}

proc expect_key_error { text code } {
  global malformed key
  write_key_fixture $malformed $text
  # Catch inside tee so its redirection is completed even on an error. Check
  # the logger output as well as the exception: utl::error returns only its ID.
  lassign [tee -quiet -variable diagnostic [list catch_key_error $malformed]] failed message options
  check "malformed key file is rejected" { set failed } 1
  check "expected key-file diagnostic" { set message } $code
  check "logger contains the diagnostic" { expr { [string first $code $diagnostic] >= 0 } } 1
  check "logger does not expose the key" { string first $key $diagnostic } -1
  check "exception does not expose the key" {
    string first $key [list $message $options]
  } -1
}

# Legacy parser errors must redact the rejected line, including invalid names.
expect_key_error "placement $key extra-token" WMK-0131
expect_key_error "$key value" WMK-0131
expect_key_error "placement $key\nplacement $key" WMK-0131

# List-parser diagnostics can themselves contain data from the input. An odd
# number of fields, unknown fields and duplicate fields are also invalid.
expect_key_error "$header\nplacement \{$key" WMK-0131
expect_key_error "$header\nplacement \{key\}$key" WMK-0131
expect_key_error "$header\nplacement $key routing" WMK-0131
expect_key_error "$header\n$key value" WMK-0131
expect_key_error "$header\nplacement $key\nplacement $key" WMK-0131
expect_key_error "$header\n" WMK-0131
foreach prefix [list "" "$header\n"] {
  foreach field { key_hex placement cts routing } {
    expect_key_error "${prefix}$field ${key}z" WMK-0131
  }
  expect_key_error "${prefix}placement $key\nnonce_hex 0g" WMK-0131
  expect_key_error "${prefix}placement $key\nnonce_hex 0" WMK-0131
  expect_key_error "${prefix}design_id test" WMK-0132
}

set ::wmk_key_file_executed 0
expect_key_error "$header\nset ::wmk_key_file_executed 1" WMK-0131
check "file contents are not executed" { set ::wmk_key_file_executed } 0

# Test the writer and both reader paths (stored key and derivation from public
# inputs plus the master key). Include characters that Tcl and text channels
# might otherwise interpret or translate.
file delete -force $private $public
foreach design_id [list "" {jpeg NG45} "tabs\tand spaces" "line1\nline2\rline3" \
  "backslash\\\nnewline" "braces{unbalanced" "{literal}" {$var [set ::wmk_key_file_executed 1];} \
  "unicode-\u6c34" "nul-\u0000-end"] {
  foreach nonce_hex { "" 0011aAff } {
    set expected [generate_watermark_key -design_id $design_id -key_hex $key \
      -nonce_hex $nonce_hex]
    set returned [generate_watermark_key -design_id $design_id -key_hex $key \
      -nonce_hex $nonce_hex -file $private -public_file $public]
    set material [wmk::read_key_file $private]
    set parameters [wmk::read_key_file $public]
    check "generation returns only public parameters" {
      lsort [dict keys $returned]
    } {design_id nonce_hex}
    check "public file contains only public parameters" {
      lsort [dict keys $parameters]
    } {design_id nonce_hex}
    foreach name { design_id nonce_hex } {
      check "private file preserves $name" {
        dict get $material $name
      } [dict get $expected $name]
      check "public file preserves $name" {
        dict get $parameters $name
      } [dict get $expected $name]
    }
    set fh [open $public a]
    puts $fh [list key_hex $key]
    close $fh
    foreach stage { placement cts routing } {
      check "stored $stage key is unchanged" {
        derive_watermark_key -key_file $private -stage $stage
      } [dict get $expected $stage]
      check "$stage derivation preserves the public inputs" {
        derive_watermark_key -key_file $public -stage $stage
      } [dict get $expected $stage]
    }
  }
}
check "identifiers are not executed" { set ::wmk_key_file_executed } 0

# Older files did not use Tcl quoting: braces and backslashes remain literal.
foreach design_id [list legacy "{literal}" {back\slash} {"quoted"}] {
  write_key_fixture $malformed "\ndesign_id $design_id\nnonce_hex abcd\nkey_hex $key\n"
  check "legacy public inputs derive the same key" {
    derive_watermark_key -key_file $malformed -stage placement
  } [derive_watermark_key -key_hex $key -design_id $design_id \
    -nonce_hex abcd -stage placement]
}
write_key_fixture $malformed "placement $key"
check "legacy stored stage key is readable" {
  derive_watermark_key -key_file $malformed -stage placement
} $key
exit_summary
