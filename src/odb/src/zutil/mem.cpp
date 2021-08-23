///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ATH__failMessage(const char* msg)
{
  fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "\nexiting ...\n");
  exit(1);
}
void Ath__hashError(const char* msg, int exitFlag)
{
  fprintf(stderr, "Cannot find %s in hash table\n", msg);
  fprintf(stderr, "\nexiting ...\n");

  if (exitFlag > 0)
    exit(1);
}
void Ath__allocFailure(const char* msg)
{
  fprintf(stderr, "Failed to allocate %s\n", msg);
  perror("");
  fprintf(stderr, "\nexiting ...\n");
}
char* ATH__allocCharWord(int n)
{
  if (n <= 0)
    ATH__failMessage("Cannot zero/negative number of chars");

  char* a = new char[n];
  if (a == NULL) {
    ATH__failMessage("Cannot allocate chars");
  }
  a[0] = '\0';
  return a;
}
void ATH__deallocCharWord(const char* a)
{
  if (a == NULL) {
    ATH__failMessage("Cannot deallocate allocate chars");
  }
  delete[] a;
}
