%{
#include <libgen.h>
#include "lefin.h"
#include "lefout.h"
#include "defin.h"
#include "defout.h"
#include "utility/Logger.h"



odb::dbLib*
read_lef(odb::dbDatabase* db, const char* path)
{
  utl::Logger* logger = new utl::Logger(NULL);
  lefin lefParser(db, logger, false);
  const char *libname = basename(const_cast<char*>(path));
  if (!db->getTech()) {
    return lefParser.createTechAndLib(libname, path);
  } else {
    return lefParser.createLib(libname, path);
  }
}

odb::dbChip*
read_def(odb::dbDatabase* db, std::string path)
{
  utl::Logger* logger = new utl::Logger(NULL);
  std::vector<odb::dbLib *> libs;
  for (dbLib *lib : db->getLibs()) {
    libs.push_back(lib);
  }
  defin defParser(db, logger);
  return defParser.createChip(libs, path.c_str());
}

int
write_def(odb::dbBlock* block, const char* path,
	      odb::defout::Version version = odb::defout::Version::DEF_5_8)
{
  utl::Logger* logger = new utl::Logger(NULL);
  defout writer(logger);
  writer.setVersion(version);
  return writer.writeBlock(block, path);
}

int
write_lef(odb::dbLib* lib, const char* path)
{
  lefout writer;
  return writer.writeTechAndLib(lib, path);
}

odb::dbDatabase*
read_db(odb::dbDatabase* db, const char* db_path)
{
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
    fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
    fprintf(stderr, "Errno: %d\n", errno);
    return errno;
  }
  db->write(fp);
  fclose(fp);
  return 1;
}

int *new_int(int ivalue) {
  int *i = (int *) malloc(sizeof(ivalue));
  *i = ivalue;
  return i;
}
int get_int(int *i) {
  return *i;
}

%}

odb::dbLib*
read_lef(odb::dbDatabase* db, const char* path);
odb::dbChip*
read_def(odb::dbDatabase* db, std::string path);

int
write_def(odb::dbBlock* block, const char* path,
	      odb::defout::Version version = odb::defout::Version::DEF_5_8);
int
write_lef(odb::dbLib* lib, const char* path);

odb::dbDatabase* read_db(odb::dbDatabase* db, const char* db_path);
int write_db(odb::dbDatabase* db, const char* db_path);
int *new_int(int ivalue);
int get_int(int *i);
