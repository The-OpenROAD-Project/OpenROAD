// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
  if (argc != 4) {
    fprintf(stderr,
            "Usage: %s <variable-name> <input-file> <output_file>\n",
            argv[0]);
    return 1;
  }

  const char* variable_name = argv[1];
  const char* input_file = argv[2];
  const char* output_file = argv[3];

  FILE* in_file = fopen(input_file, "rb");
  if (!in_file) {
    perror("Could not open input.");
    return EXIT_FAILURE;
  }

  FILE* out_file = fopen(output_file, "wb");
  if (!out_file) {
    fclose(in_file);
    perror("Could not open output.");
    return EXIT_FAILURE;
  }

  fprintf(out_file,
          "static inline constexpr unsigned char %s[] = {\n  ",
          variable_name);
  uint8_t buffer[100 * 12];
  size_t nread;
  while ((nread = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
    for (int i = 0; i < nread; ++i) {
      fprintf(out_file, "0x%02x,%s", buffer[i], i % 12 == 11 ? "\n  " : " ");
    }
  }
  const bool any_issue = ferror(in_file) || ferror(out_file);
  if (any_issue) {
    perror("trouble reading file.");
  }
  fprintf(out_file, "};\n");
  fclose(in_file);
  fclose(out_file);

  return any_issue ? EXIT_FAILURE : EXIT_SUCCESS;
}
