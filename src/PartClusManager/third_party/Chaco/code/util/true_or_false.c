/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

char     *true_or_false(flag)
int       flag;
{
    if (flag)
	return ("True");
    else
	return ("False");
}
