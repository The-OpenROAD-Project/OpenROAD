#include "rmp/util.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>

using namespace std;

bool SortBySecond(const pair<int, float> &a, const pair<int, float> &b) {
    return a.second < b.second;
}

// char_match:  determine if the char is part of deliminators
bool CharMatch(char c, string& delim) {
    string::iterator it = delim.begin();
    while(it != delim.end()) {
        if((*it) == c) {
            return true;
        }
        
        ++it;
    }
    return false;
}
 

// find the next position for deliminator char
string::const_iterator FindDelim(string::const_iterator i, string::const_iterator end, string& delim) {
    while(i != end && !CharMatch(*i, delim)) 
        ++i;
    
    return i;
}


// find the next position for non deliminator char
string::const_iterator FindNotDelim(string::const_iterator i, string::const_iterator end, string& delim) {
    while(i != end && CharMatch(*i, delim)) 
         ++i;
    
    return i;
}


// split the strings based on the deliminators
vector<string> Split(const string& str, string delim) {
    vector<string> ret;
    typedef string::const_iterator iter;
    iter i = str.begin();
    while(i != str.end()) {
        i = FindNotDelim(i, str.end(), delim);
        iter j = FindDelim(i, str.end(), delim);
        if(i != str.end()) {
            ret.push_back(string(i,j));
            i = j;
        }
    }

    return ret;
}



unordered_map<string, string> ParseConfigFile(const char* file_name) {
    unordered_map<string, string> params;
    fstream f;
    string line;
    f.open(file_name, ios::in);
    while(getline(f,line)) {
        vector<string> words = Split(line);
 
        if(!words.empty() && words[1] == "=") {
            params[words[0]] = words[2];
        }
    }
    
    f.close();
 
    return params;
}
























