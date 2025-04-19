// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// base64-encode input files and split into string of length
// suitable for representing in a C string (64k).  Store the
// result in a nullptr terminated C array.

#include <getopt.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <string_view>

static constexpr int kDefaultChunkSize = 65536;

// Encode data as base64. Call write_char() function to emit character.
void EncodeBase64(std::string_view input,
                  const std::function<void(char)>& write_char)
{
  static constexpr char b64[]
      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t input_len = input.length();
  std::string_view::iterator it = input.begin();
  for (/**/; input_len >= 3; input_len -= 3) {
    uint8_t b0 = *it++;
    uint8_t b1 = *it++;
    uint8_t b2 = *it++;
    write_char(b64[(b0 >> 2) & 0x3f]);
    write_char(b64[((b0 & 0x03) << 4) | ((int) (b1 & 0xf0) >> 4)]);
    write_char(b64[((b1 & 0x0f) << 2) | ((int) (b2 & 0xc0) >> 6)]);
    write_char(b64[b2 & 0x3f]);
  }
  if (input_len > 0) {
    uint8_t b0 = *it++;
    uint8_t b1 = input_len > 1 ? *it : 0;
    write_char(b64[(b0 >> 2) & 0x3f]);
    write_char(b64[((b0 & 0x03) << 4) | ((int) (b1 & 0xf0) >> 4)]);
    write_char(input_len > 1 ? b64[((b1 & 0x0f) << 2)] : '=');
    write_char('=');
  }
}

int usage(const char* progname, const char* msg)
{
  fprintf(stderr,
          R"(-- %s --
Usage:
%s --output <output> --varname <varname> --namespace <namespace> input [input...]
Options:
  --output <filename>      (-o) : output file c++ snippet             (required)
  --varname <identifier>   (-v) : Name of const char* variable        (required)
  --namespace <identifier> (-n) : Namespace the variable should be in (required)
  --chunk <number>         (-c) : chunk size (default: %d)
)",
          msg,
          progname,
          kDefaultChunkSize);

  return 1;
}

int main(int argc, char* argv[])
{
  static constexpr struct option kLongOpts[]
      = {{"output", required_argument, nullptr, 'o'},
         {"namespace", required_argument, nullptr, 'n'},
         {"varname", required_argument, nullptr, 'v'},
         {"chunk", required_argument, nullptr, 'c'},
         {nullptr, 0, nullptr, 0}};

  const char* output_file = nullptr;
  const char* cpp_ns = nullptr;
  const char* varname = nullptr;
  int chunk_size = kDefaultChunkSize;
  int opt;
  int state = 0;
  while ((opt = getopt_long(argc, argv, "o:n:v:c:", kLongOpts, &state)) != -1) {
    switch (opt) {
      case 'o':
        output_file = optarg;
        break;
      case 'n':
        cpp_ns = optarg;
        break;
      case 'v':
        varname = optarg;
        break;
      case 'c':
        chunk_size = atoi(optarg);
        break;
    }
  }

  if (!varname || !output_file || !cpp_ns) {
    return usage(argv[0], "Required option(s) missing.");
  }
  if (chunk_size < 1) {
    return usage(argv[0], "Chunk size looks funny.");
  }

  if (optind >= argc) {
    return usage(argv[0], "Expect inputs");
  }

  // We accept a list of inputs, but only emit one output. Concat.
  std::string all_input;
  char buf[4096];
  for (int i = optind; i < argc; ++i) {
    FILE* f = fopen(argv[i], "rb");
    if (!f) {
      return usage(argv[0], "Can't read input.");
    }
    while (const size_t r = fread(buf, 1, sizeof(buf), f)) {
      all_input.append(buf, r);
    }
    fclose(f);
  }

  std::ofstream out(output_file, std::ios::binary);

  // Writing output, but chunking strings every kChunksize chars.
  int chars_written = 0;
  auto chunk_writer = [&](char c) {
    if (chars_written == chunk_size) {
      out << "\",\n\"";
      chars_written = 0;
    }
    out.write(&c, 1);
    ++chars_written;
  };

  out << "namespace " << cpp_ns << " {\n";
  out << "const char* " << varname << "[] = {\n\"";
  EncodeBase64(all_input, chunk_writer);
  out << "\",\nnullptr};\n} // end namespace " << cpp_ns << "\n";
}
