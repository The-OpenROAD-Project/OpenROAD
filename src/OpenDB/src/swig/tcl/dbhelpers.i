// The functions defined in this file are wrong on so many levels.
// For example:
// They are needlessly complicated by the use of string vector arguments
// where multiple calls in the rare circumstances there are multiple lef
// files would suffice. Why does read_def use a vector of
// def file names and then error if there is more than one?
// Tcl is not polymorphic but there are multiple inline functions definitions
// with the same name. A "NULL" db arg creates a database. What good is that?
// I highly discourage their use. -cherry

%{
#include <libgen.h>
#include "lefin.h"
#include "lefout.h"
#include "defin.h"
#include "defout.h"
#include "utility/Logger.h"


int
write_lef(odb::dbLib* lib, const char* path) {
  lefout writer;
  return writer.writeTechAndLib(lib, path);
}

int
write_tech_lef(odb::dbTech* tech, const char* path) {
  lefout writer;
  return writer.writeTech(tech, path);
}

odb::dbDatabase*
read_db(odb::dbDatabase* db, const char* db_path)
{
  if (db == NULL) {
    db = odb::dbDatabase::create();
  }
  FILE *fp = fopen(db_path, "rb");
  if (!fp) {
    int errnum = errno;
    fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
    fprintf(stderr, "Errno: %d\n", errno);
    return NULL;
  }
  db->read(fp);
  fclose(fp);
  return db;
}

int
write_db(odb::dbDatabase* db, const char* db_path)
{
  FILE *fp = fopen(db_path, "wb");
  if (!fp) {
    int errnum = errno;
    fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    fprintf(stderr, "Errno: %d\n", errno);
    return errno;
  }
  db->write(fp);
  fclose(fp);
  return 1;
}

int 
writeEco(odb::dbBlock* block, const char* filename)
{
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    int errnum = errno;
    fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    fprintf(stderr, "Errno: %d\n", errno);
    return errno;
  }

  odb::dbDatabase::writeEco(block, fp);
  fclose(fp);
  return 1;
}

int 
readEco(odb::dbBlock* block, const char* filename)
{
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    int errnum = errno;
    fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    fprintf(stderr, "Errno: %d\n", errno);
    return errno;
  }

  odb::dbDatabase::readEco(block, fp);
  fclose(fp);
  return 1;
}

%}

int
write_lef(odb::dbLib* lib, const char* path);

int
write_tech_lef(odb::dbTech* tech, const char* path);

odb::dbDatabase*
read_db(odb::dbDatabase* db, const char* db_path);
int
write_db(odb::dbDatabase* db, const char* db_path);

int
readEco(odb::dbBlock* block, const char* filename);

int 
writeEco(odb::dbBlock* block, const char* filename);