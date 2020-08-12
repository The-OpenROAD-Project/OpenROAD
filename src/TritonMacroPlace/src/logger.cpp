#include "logger.h"
#include <iostream>
#include <cstdlib>

using std::cout;
using std::endl;
using std::string;

#define VERBOSE_CHECK() if( verbose_ < verbose ) { return; };

namespace MacroPlace {

Logger::Logger(string name, int verbose)
  : name_(name), verbose_(verbose) {}

// Procedure message
void Logger::proc(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] " << input << endl;
}
void Logger::procBegin(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] Begin " << input << " ..." << endl;
}
void Logger::procEnd(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[PROC] End " << input << endl;
}

// Error message
void Logger::error(string input, int code, int verbose) {
  VERBOSE_CHECK()
  cout << "[ERROR] " << input;
  cout << " (" << name_ << "-" << code << ")" << endl;
  exit(code);
}

void Logger::warn(string input, int code, int verbose) {
  VERBOSE_CHECK()
  cout << "[WARN] " << input;
  cout << " (" << name_ << "-" << code << ")" << endl;
}

// Info message
void Logger::infoInt(string input, int val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void Logger::infoIntPair(string input, int val1, int val2, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = (" << val1 << ", " << val2 << ")" << endl;
}

void Logger::infoInt64(string input, int64_t val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}


void Logger::infoFloat(string input, float val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %.6f\n", input.c_str(), val);
  fflush(stdout);
}

void Logger::infoFloatPair(string input, float val1, float val2, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = (%.6f, %.6f)\n", input.c_str(), val1, val2);
  fflush(stdout);
}
// SI format due to WNS/TNS
void Logger::infoFloatSignificant(string input, float val, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = %g\n", input.c_str(), val);
  fflush(stdout);
}

void Logger::infoFloatSignificantPair(string input, float val1, float val2, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %s = (%g, %g)\n", input.c_str(), val1, val2);
  fflush(stdout);
}
void Logger::infoString(string input, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << endl;
}
void Logger::infoString(string input, string val, int verbose) {
  VERBOSE_CHECK()
  cout << "[INFO] " << input << " = " << val << endl;
}
void Logger::infoRuntime(string input, double runtime, int verbose) {
  VERBOSE_CHECK()
  printf("[INFO] %sRuntime = %.4f\n", input.c_str(), runtime);
  fflush(stdout);
}

}
