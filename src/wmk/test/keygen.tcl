# Key generation, and the round trip that makes it usable.
#
# A watermark is only as good as the secret key behind it, and the two
# properties that matter are not visible from a single run: that a fresh draw
# is unpredictable, and that the stage keys can be recovered later from the
# secret key and the public parts alone.  Without the second, an owner who kept
# the secret key would still be unable to verify, because the keys embedding
# used would be gone.
#
# The draw itself cannot be tested for randomness here, and pretending
# otherwise would be worse than not trying.  What is checked is that two draws
# differ, that a secret key is never invented when the system's random source
# cannot be read, and that derivation is a function of exactly the inputs the
# scheme says it is.
source "helpers.tcl"

set bundle [generate_watermark_key -design_id jpeg_encoder]
check "the secret key is 32 bytes" \
  { string length [dict get $bundle key_hex] } 64
check "the nonce is 16 bytes" \
  { string length [dict get $bundle nonce_hex] } 32

foreach stage { placement cts routing } {
  check "the $stage key is 32 bytes" \
    { string length [dict get $bundle $stage] } 64
}

# The three stages must not share a key: disclosing one to prove ownership of
# one stage would otherwise disclose the others.
check "the three stage keys differ" {
  expr {
    [dict get $bundle placement] ne [dict get $bundle cts]
    && [dict get $bundle cts] ne [dict get $bundle routing]
    && [dict get $bundle placement] ne [dict get $bundle routing]
  }
} 1

# Two draws from the system random source must not collide.  Equality here
# would mean the source is not being read at all.
set other [generate_watermark_key -design_id jpeg_encoder]
check "a second draw is a different secret key" \
  { expr { [dict get $other key_hex] ne [dict get $bundle key_hex] } } 1
check "and a different nonce" \
  { expr { [dict get $other nonce_hex] ne [dict get $bundle nonce_hex] } } 1

# Verification happens in a later process, so the stage keys have to come back
# from the secret key and the public parts alone.
set key [dict get $bundle key_hex]
set nonce [dict get $bundle nonce_hex]
foreach stage { placement cts routing } {
  check "the $stage key is recoverable" {
    derive_watermark_key -key_hex $key -design_id jpeg_encoder \
      -nonce_hex $nonce -stage $stage
  } [dict get $bundle $stage]
}

# The identifier and the nonce are what separate one watermark instance from
# another, so changing either has to change every derived key.
check "a different design gets a different key" {
  expr { [derive_watermark_key -key_hex $key -design_id gcd \
            -nonce_hex $nonce -stage routing] ne [dict get $bundle routing] }
} 1
check "a different nonce gets a different key" {
  expr { [derive_watermark_key -key_hex $key -design_id jpeg_encoder \
            -nonce_hex [dict get $other nonce_hex] -stage routing] \
          ne [dict get $bundle routing] }
} 1

# Supplying the secret key and nonce reproduces the whole bundle, which is what
# lets a flow re-derive the same watermark without storing the stage keys.
set again [generate_watermark_key -design_id jpeg_encoder -key_hex $key \
  -nonce_hex $nonce]
check "the bundle is reproducible from its inputs" \
  { expr { $again eq $bundle } } 1

# The design identifier and the nonce are public, and are needed again to
# derive the stage keys.  They can be written on their own so that an owner can
# hand them over without handing over the file that also holds the secret key.
set public_path [make_result_file "keygen_public.txt"]
generate_watermark_key -design_id jpeg_encoder -key_hex $key \
  -nonce_hex $nonce -public_file $public_path
set fh [open $public_path r]
set public_text [read $fh]
close $fh
check "the public file names the design and the nonce" {
  expr {
    [string match "*design_id jpeg_encoder*" $public_text]
    && [string match "*nonce_hex $nonce*" $public_text]
  }
} 1
# The point of a second file is that it carries nothing secret.  A key or a
# stage key appearing here would make it as dangerous to pass on as the first.
check "and carries no key material" {
  expr {
    ![string match "*$key*" $public_text]
    && ![string match "*[dict get $bundle placement]*" $public_text]
    && ![string match "*[dict get $bundle cts]*" $public_text]
    && ![string match "*[dict get $bundle routing]*" $public_text]
  }
} 1

exit_summary
