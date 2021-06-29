#ifndef UTIL_H_
#define UTIL_H_

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

bool SortBySecond(const std::pair<int, float>& a,
                  const std::pair<int, float>& b);

bool CharMatch(char c, const std::string& delim);

std::string::const_iterator FindDelim(std::string::const_iterator i,
                                      std::string::const_iterator end,
                                      const std::string& delim);

std::string::const_iterator FindNotDelim(std::string::const_iterator i,
                                         std::string::const_iterator end,
                                         const std::string& delim);

std::vector<std::string> Split(const std::string& str, std::string delim = " ");

std::unordered_map<std::string, std::string> ParseConfigFile(
    const char* file_name);

#endif
