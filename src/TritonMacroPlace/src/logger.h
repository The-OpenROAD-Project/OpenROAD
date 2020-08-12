#ifndef __REPLACE_LOGGER__
#define __REPLACE_LOGGER__

#include <string>

namespace MacroPlace {

class Logger {

public:
  Logger(std::string name, int verbose);

  // Print functions
  void proc(std::string input, int verbose = 0);
  void procBegin(std::string input, int verbose = 0);
  void procEnd(std::string input, int verbose = 0);

  void error(std::string input, int code, int verbose = 0);

  void warn(std::string input, int code, int verbose = 0);

  void infoInt(std::string input, int val, int verbose = 0);
  void infoIntPair(std::string input, int val1, int val2, int verbose = 0);

  void infoInt64(std::string input, int64_t val, int verbose = 0);

  void infoFloat(std::string input, float val, int verbose = 0);
  void infoFloatPair(std::string input, float val1, float val2, int verbose = 0);
  void infoFloatSignificant(std::string input, float val, int verbose = 0);
  void infoFloatSignificantPair(std::string input, float val1, float val2, int verbose = 0);


  void infoString(std::string input, int verbose = 0);
  void infoString(std::string input, std::string val, int verbose = 0);

  void infoRuntime(std::string input, double runtime, int verbose = 0);


private:
  int verbose_;
  std::string name_;

};

}

#endif
