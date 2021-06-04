#ifndef UTIL_H_
#define UTIL_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>

bool SortBySecond(const std::pair<int, float>& a, const std::pair<int, float>& b);

bool CharMatch(char c, std::string& delim);

std::string::const_iterator FindDelim(std::string::const_iterator i, std::string::const_iterator end, std::string& delim);

std::string::const_iterator FindNotDelim(std::string::const_iterator i, std::string::const_iterator end, std::string& delim);

std::vector<std::string> Split(const std::string& str, std::string delim = " ");

std::unordered_map<std::string, std::string> ParseConfigFile(const char* file_name);


#endif
