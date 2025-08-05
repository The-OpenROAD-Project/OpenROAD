//
// Some logging utilities
// 1. "log() << ..." will show a time stamp first
// 2. "print(a, b, c)" is python-like print for any a, b, c that has operator<< overloaded. For example,
//      int a = 10;
//      double b = 3.14;
//      std::string c = "Gengjie";
//      print(a, b, c);
//     This code piece will show "10 3.14 Gengjie".
//

#pragma once

#include <chrono>
#include <iostream>
#include <string>

namespace utils {

// 1. Timer

class timer {
    using clock = std::chrono::high_resolution_clock;

private:
    clock::time_point _start;

public:
    timer();
    void start();
    double elapsed() const;  // seconds
};

std::ostream& operator<<(std::ostream& os, const timer& t);

// 2. Memory

class mem_use {
public:
    static double get_current();  // MB
    static double get_peak();     // MB
};

// 3. Easy print

// print(a, b, c)
inline void print() { std::cout << std::endl; }
template <typename T, typename... TAIL>
void print(const T& t, TAIL... tail) {
    std::cout << t << ' ';
    print(tail...);
}

// "log() << a << b << c" puts a time stamp in beginning
std::ostream& log(std::ostream& os = std::cout);

// "printlog(a, b, c)" puts a time stamp in beginning
template <typename... T>
void printlog(T... t) {
    log();
    print(t...);
}

template <typename... T>
void printflog(T... t) {
    log();
    printf(t...);
}

void logeol(int n = 1);
void loghline();
void logmem();

}  // namespace utils