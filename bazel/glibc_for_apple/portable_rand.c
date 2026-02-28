/*
 * Portable rand()/srand() implementation matching glibc's TYPE_3 algorithm.
 *
 * glibc's rand() uses a trinomial feedback shift register (TYPE_3) with
 * 31 words of state (not the simple LCG often described in textbooks).
 * Apple's libc uses a completely different algorithm.
 *
 * This implements glibc's exact algorithm from stdlib/random_r.c to ensure
 * identical rand() sequences across platforms.
 *
 * Reference: glibc/stdlib/random_r.c (LGPL 2.1)
 * TYPE_3: degree 31, separation 3
 */

#define DEG_3 31
#define SEP_3 3

static unsigned int _state[DEG_3];
static int _fptr = SEP_3;
static int _rptr = 0;
static int _initialized = 0;

static void _init_state(unsigned int seed) {
  _state[0] = seed;
  for (int i = 1; i < DEG_3; i++) {
    /* This is the same initialization as glibc's __srandom_r:
     * state[i] = (16807 * state[i-1]) % 2147483647
     * Using 64-bit arithmetic to avoid overflow */
    long long val = (16807LL * (long long)_state[i - 1]) % 2147483647LL;
    if (val < 0)
      val += 2147483647LL;
    _state[i] = (unsigned int)val;
  }
  _fptr = SEP_3;
  _rptr = 0;

  /* Warmup: 10 * DEG_3 = 310 iterations (matches glibc) */
  for (int i = 0; i < 10 * DEG_3; i++) {
    _state[_fptr] = (_state[_fptr] + _state[_rptr]);
    _fptr++;
    if (_fptr >= DEG_3)
      _fptr = 0;
    _rptr++;
    if (_rptr >= DEG_3)
      _rptr = 0;
  }
  _initialized = 1;
}

void srand(unsigned int seed) { _init_state(seed); }

int rand(void) {
  if (!_initialized)
    _init_state(1);

  unsigned int result = _state[_fptr] + _state[_rptr];
  _state[_fptr] = result;

  int ret = (int)((result >> 1) & 0x7fffffffU);

  _fptr++;
  if (_fptr >= DEG_3)
    _fptr = 0;
  _rptr++;
  if (_rptr >= DEG_3)
    _rptr = 0;

  return ret;
}

int rand_r(unsigned int *seedp) {
  /* glibc's rand_r uses a different (simpler) algorithm */
  unsigned int next = *seedp;
  int result;

  next *= 1103515245;
  next += 12345;
  result = (unsigned int)(next / 65536) % 2048;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int)(next / 65536) % 1024;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int)(next / 65536) % 1024;

  *seedp = next;
  return result;
}
