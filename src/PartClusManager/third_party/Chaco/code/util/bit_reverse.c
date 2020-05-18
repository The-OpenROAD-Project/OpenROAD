/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

/* Reverse the bits of a number. */
int       bit_reverse(val, nbits)
int       val;			/* value to reverse bits of */
int       nbits;		/* number of significant bits */
{
    int       mask_low, mask_high;	/* masks for bits to interchange */
    int       bit_low, bit_high;/* values of the bits in question */
    int       i;		/* loop counter */

    mask_low = 1;
    mask_high = 1 << (nbits - 1);
    for (i = 0; i < nbits / 2; i++) {
	bit_low = (val & mask_low) >> i;
	bit_high = (val & mask_high) >> (nbits - i - 1);
	mask_low <<= 1;
	mask_high >>= 1;
	if (bit_low != bit_high) {
	    val ^= (1 << i) ^ (1 << (nbits - i - 1));
	}
    }
    return (val);
}
