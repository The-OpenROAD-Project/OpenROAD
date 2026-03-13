// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Delta computation uses the bsdiff algorithm by Colin Percival.
// Original bsdiff: Copyright 2003-2005 Colin Percival, BSD-2-Clause.
// See https://www.daemonology.net/bsdiff/

#include "utl/OdbDelta.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "utl/ScopedTemporaryFile.h"

namespace utl {

namespace {

// --- Suffix array construction (prefix-doubling) ---
// O(n log^2 n) with O(1) comparisons via rank arrays.

void buildSuffixArray(std::vector<int64_t>& SA,
                      const uint8_t* data,
                      int64_t n)
{
  SA.resize(n + 1);
  std::vector<int64_t> rank(n + 1);
  std::vector<int64_t> tmp(n + 1);

  for (int64_t i = 0; i < n; ++i) {
    SA[i] = i;
    rank[i] = data[i];
  }
  SA[n] = n;
  rank[n] = -1;  // empty suffix sentinel

  for (int64_t k = 1; k <= n; k *= 2) {
    auto cmp = [&rank, n, k](int64_t a, int64_t b) {
      if (rank[a] != rank[b]) {
        return rank[a] < rank[b];
      }
      int64_t ra = (a + k <= n) ? rank[a + k] : -2;
      int64_t rb = (b + k <= n) ? rank[b + k] : -2;
      return ra < rb;
    };
    std::sort(SA.begin(), SA.end(), cmp);

    tmp[SA[0]] = 0;
    for (int64_t i = 1; i <= n; ++i) {
      tmp[SA[i]] = tmp[SA[i - 1]] + (cmp(SA[i - 1], SA[i]) ? 1 : 0);
    }
    rank = tmp;
    if (rank[SA[n]] == n) {
      break;  // all ranks unique, done
    }
  }
}

int64_t matchlen(const uint8_t* old_data,
                 int64_t old_size,
                 const uint8_t* new_data,
                 int64_t new_size)
{
  int64_t i = 0;
  while (i < old_size && i < new_size && old_data[i] == new_data[i]) {
    ++i;
  }
  return i;
}

int64_t search(const std::vector<int64_t>& I,
               const uint8_t* old_data,
               int64_t old_size,
               const uint8_t* new_data,
               int64_t new_size,
               int64_t st,
               int64_t en,
               int64_t* pos)
{
  // Iterative binary search on suffix array.
  while (en - st >= 2) {
    int64_t pivot = st + (en - st) / 2;
    if (std::memcmp(old_data + I[pivot],
                    new_data,
                    std::min(old_size - I[pivot], new_size))
        < 0) {
      st = pivot;
    } else {
      en = pivot;
    }
  }

  int64_t x
      = matchlen(old_data + I[st], old_size - I[st], new_data, new_size);
  int64_t y
      = matchlen(old_data + I[en], old_size - I[en], new_data, new_size);

  if (x > y) {
    *pos = I[st];
    return x;
  }
  *pos = I[en];
  return y;
}

// --- Little-endian I/O helpers ---

void writeLE64(std::vector<uint8_t>& out, int64_t value)
{
  uint64_t uv = static_cast<uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<uint8_t>(uv & 0xFF));
    uv >>= 8;
  }
}

int64_t readLE64(const uint8_t*& ptr, const uint8_t* end)
{
  if (ptr + 8 > end) {
    throw std::runtime_error("OdbDelta: unexpected end of delta data");
  }
  uint64_t uv = 0;
  for (int i = 0; i < 8; ++i) {
    uv |= static_cast<uint64_t>(ptr[i]) << (8 * i);
  }
  ptr += 8;
  return static_cast<int64_t>(uv);
}

void writeLE32(std::vector<uint8_t>& out, uint32_t value)
{
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    value >>= 8;
  }
}

uint32_t readLE32(const uint8_t*& ptr, const uint8_t* end)
{
  if (ptr + 4 > end) {
    throw std::runtime_error("OdbDelta: unexpected end of delta data");
  }
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i) {
    value |= static_cast<uint32_t>(ptr[i]) << (8 * i);
  }
  ptr += 4;
  return value;
}

}  // namespace

// --- bsdiff-based delta computation ---

std::vector<uint8_t> computeOdbDelta(const std::vector<uint8_t>& base,
                                     const std::vector<uint8_t>& current)
{
  const int64_t old_size = static_cast<int64_t>(base.size());
  const int64_t new_size = static_cast<int64_t>(current.size());
  const uint8_t* old_data = base.data();
  const uint8_t* new_data = current.data();

  // Build suffix array of old data.
  std::vector<int64_t> I;
  buildSuffixArray(I, old_data, old_size);

  // Compute bsdiff control/diff/extra data.
  std::vector<uint8_t> ctrl_buf;
  std::vector<uint8_t> diff_buf;
  std::vector<uint8_t> extra_buf;

  int64_t scan = 0;
  int64_t len = 0;
  int64_t last_scan = 0;
  int64_t last_pos = 0;
  int64_t last_offset = 0;
  int64_t pos = 0;

  while (scan < new_size) {
    int64_t oldscore = 0;
    scan += len;

    int64_t scsc = scan;
    for (; scan < new_size; ++scan) {
      len = search(I,
                   old_data,
                   old_size,
                   new_data + scan,
                   new_size - scan,
                   0,
                   old_size,
                   &pos);

      while (scsc < scan + len) {
        if (scsc + last_offset < old_size && scsc + last_offset >= 0
            && old_data[scsc + last_offset] == new_data[scsc]) {
          ++oldscore;
        }
        ++scsc;
      }

      if ((len == oldscore && len != 0) || (len > oldscore + 8)) {
        break;
      }
      if (scan + last_offset < old_size && scan + last_offset >= 0
          && old_data[scan + last_offset] == new_data[scan]) {
        --oldscore;
      }
    }

    if (len != oldscore || scan == new_size) {
      // Count matching bytes from the front.
      int64_t s = 0;
      int64_t sf = 0;
      int64_t lenf = 0;
      for (int64_t i = 0; i < scan - last_scan && i < old_size - last_pos;
           ++i) {
        if (old_data[last_pos + i] == new_data[last_scan + i]) {
          ++s;
        }
        if (s * 2 - i > sf * 2 - lenf) {
          sf = s;
          lenf = i + 1;
        }
      }

      // Count matching bytes from the back.
      int64_t lenb = 0;
      if (scan < new_size) {
        s = 0;
        int64_t sb = 0;
        for (int64_t i = 1; i < scan - last_scan && i < pos; ++i) {
          if (old_data[pos - i] == new_data[scan - i]) {
            ++s;
          }
          if (s * 2 - i > sb * 2 - lenb) {
            sb = s;
            lenb = i;
          }
        }
      }

      // Handle overlap.
      if (last_scan + lenf > scan - lenb) {
        int64_t overlap = (last_scan + lenf) - (scan - lenb);
        s = 0;
        int64_t ss = 0;
        int64_t lens = 0;
        for (int64_t i = 0; i < overlap; ++i) {
          if (new_data[last_scan + lenf - overlap + i]
              == old_data[last_pos + lenf - overlap + i]) {
            ++s;
          }
          if (new_data[scan - lenb + i] == old_data[pos - lenb + i]) {
            --s;
          }
          if (s > ss) {
            ss = s;
            lens = i + 1;
          }
        }

        lenf += lens - overlap;
        lenb -= lens;
      }

      // Write diff data.
      for (int64_t i = 0; i < lenf; ++i) {
        diff_buf.push_back(static_cast<uint8_t>(new_data[last_scan + i]
                                                - old_data[last_pos + i]));
      }

      // Write extra data.
      for (int64_t i = 0; i < (scan - lenb) - (last_scan + lenf); ++i) {
        extra_buf.push_back(new_data[last_scan + lenf + i]);
      }

      // Write control tuple.
      writeLE64(ctrl_buf, lenf);
      writeLE64(ctrl_buf, (scan - lenb) - (last_scan + lenf));
      writeLE64(ctrl_buf, (pos - lenb) - (last_pos + lenf));

      last_scan = scan - lenb;
      last_pos = pos - lenb;
      last_offset = pos - scan;
    }
  }

  // Build output: header + control + diff + extra.
  std::vector<uint8_t> delta;
  delta.reserve(32 + ctrl_buf.size() + diff_buf.size() + extra_buf.size());

  // Header: magic(4) + version(4) + base_size(8) + current_size(8) +
  //         ctrl_len(4) + diff_len(4)
  writeLE32(delta, OdbDelta::MAGIC);
  writeLE32(delta, OdbDelta::VERSION);
  writeLE64(delta, old_size);
  writeLE64(delta, new_size);
  writeLE32(delta, static_cast<uint32_t>(ctrl_buf.size()));
  writeLE32(delta, static_cast<uint32_t>(diff_buf.size()));

  delta.insert(delta.end(), ctrl_buf.begin(), ctrl_buf.end());
  delta.insert(delta.end(), diff_buf.begin(), diff_buf.end());
  delta.insert(delta.end(), extra_buf.begin(), extra_buf.end());

  return delta;
}

// --- bsdiff-based delta application ---

std::vector<uint8_t> applyOdbDelta(const std::vector<uint8_t>& base,
                                   const std::vector<uint8_t>& delta)
{
  const uint8_t* ptr = delta.data();
  const uint8_t* end = delta.data() + delta.size();

  // Read header.
  const uint32_t magic = readLE32(ptr, end);
  if (magic != OdbDelta::MAGIC) {
    throw std::runtime_error(
        "OdbDelta: invalid magic number (not a .delta file)");
  }

  const uint32_t version = readLE32(ptr, end);
  if (version != OdbDelta::VERSION) {
    throw std::runtime_error("OdbDelta: unsupported version "
                             + std::to_string(version));
  }

  const int64_t base_size = readLE64(ptr, end);
  const int64_t new_size = readLE64(ptr, end);
  const uint32_t ctrl_len = readLE32(ptr, end);
  const uint32_t diff_len = readLE32(ptr, end);

  if (static_cast<int64_t>(base.size()) != base_size) {
    throw std::runtime_error(
        "OdbDelta: base size mismatch (expected "
        + std::to_string(base_size) + ", got " + std::to_string(base.size())
        + "). Wrong base .odb file?");
  }

  if (ptr + ctrl_len + diff_len > end) {
    throw std::runtime_error("OdbDelta: truncated delta file");
  }

  const uint8_t* ctrl_ptr = ptr;
  const uint8_t* ctrl_end = ctrl_ptr + ctrl_len;
  const uint8_t* diff_ptr = ctrl_end;
  const uint8_t* diff_end = diff_ptr + diff_len;
  const uint8_t* extra_ptr = diff_end;
  const uint8_t* extra_end = end;

  const uint8_t* old_data = base.data();

  std::vector<uint8_t> result(new_size);
  int64_t old_pos = 0;
  int64_t new_pos = 0;

  while (ctrl_ptr < ctrl_end) {
    // Read control tuple.
    int64_t diff_count = readLE64(ctrl_ptr, ctrl_end);
    int64_t extra_count = readLE64(ctrl_ptr, ctrl_end);
    int64_t old_adjust = readLE64(ctrl_ptr, ctrl_end);

    if (new_pos + diff_count > new_size || diff_ptr + diff_count > diff_end) {
      throw std::runtime_error("OdbDelta: corrupt delta (diff overflow)");
    }

    // Apply diff: copy from old + add diff bytes.
    for (int64_t i = 0; i < diff_count; ++i) {
      uint8_t old_byte
          = (old_pos + i >= 0 && old_pos + i < base_size)
                ? old_data[old_pos + i]
                : 0;
      result[new_pos + i] = static_cast<uint8_t>(old_byte + diff_ptr[i]);
    }
    new_pos += diff_count;
    diff_ptr += diff_count;
    old_pos += diff_count;

    if (new_pos + extra_count > new_size
        || extra_ptr + extra_count > extra_end) {
      throw std::runtime_error("OdbDelta: corrupt delta (extra overflow)");
    }

    // Copy extra bytes directly.
    std::memcpy(result.data() + new_pos, extra_ptr, extra_count);
    new_pos += extra_count;
    extra_ptr += extra_count;

    old_pos += old_adjust;
  }

  return result;
}

std::vector<uint8_t> readFileBytes(const char* filename)
{
  InStreamHandler handler(filename, true);
  std::istream& stream = handler.getStream();

  // InStreamHandler may set exception flags on the stream.
  // Clear them so that EOF doesn't throw.
  stream.exceptions(std::ios::goodbit);

  // Read entire stream into vector.
  std::vector<uint8_t> bytes;
  constexpr size_t chunk = 1024 * 1024;  // 1 MB chunks.
  while (stream) {
    const size_t pos = bytes.size();
    bytes.resize(pos + chunk);
    stream.read(reinterpret_cast<char*>(bytes.data() + pos), chunk);
    bytes.resize(pos + stream.gcount());
  }

  if (bytes.empty()) {
    throw std::runtime_error(std::string("OdbDelta: failed to read file: ")
                             + filename);
  }

  return bytes;
}

}  // namespace utl
