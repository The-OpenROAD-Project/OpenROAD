// Stub libxml2 — satisfies ld.lld's dynamic dependency on libxml2.so.2.
// lld only uses libxml2 for Windows manifest merging (lld/COFF), so these
// symbols are never called on Linux.  Providing a stub here means developers
// no longer need the system libxml2-dev package installed.

// Every symbol here is an exported .so entry point — external linkage is
// intentional and required.
// NOLINTBEGIN(misc-use-internal-linkage)

#include <stdlib.h>

void* xmlFree = NULL;

void* xmlAddChild(void* p, void* c)
{
  abort();
}
void* xmlCopyNamespace(void* n)
{
  abort();
}
void xmlDocDumpFormatMemoryEnc(void* d, void* b, void* s, const char* e, int f)
{
  abort();
}
void* xmlDocGetRootElement(void* d)
{
  abort();
}
void* xmlDocSetRootElement(void* d, void* r)
{
  abort();
}
void xmlFreeDoc(void* d)
{
  abort();
}
void xmlFreeNode(void* n)
{
  abort();
}
void xmlFreeNs(void* n)
{
  abort();
}
void* xmlNewDoc(const void* v)
{
  abort();
}
void* xmlNewNs(void* n, const void* h, const void* p)
{
  abort();
}
void* xmlNewProp(void* n, const void* name, const void* v)
{
  abort();
}
void* xmlReadMemory(const char* b, int sz, const char* u, const char* e, int o)
{
  abort();
}
void xmlSetGenericErrorFunc(void* c, void* h)
{
  abort();
}
void* xmlStrdup(const void* s)
{
  abort();
}
void xmlUnlinkNode(void* n)
{
  abort();
}

// NOLINTEND(misc-use-internal-linkage)
